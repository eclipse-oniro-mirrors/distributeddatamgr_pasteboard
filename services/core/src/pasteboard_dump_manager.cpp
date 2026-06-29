/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pasteboard_dump_manager.h"
#include "pasteboard_hilog.h"
#include "pasteboard_error.h"
#include "pasteboard_dump_helper.h"
#include "time.h"
#include <sys/time.h>

namespace OHOS {
namespace MiscServices {

std::mutex PasteboardDumpManager::historyMutex_;
std::vector<std::string> PasteboardDumpManager::dataHistory_;
std::shared_ptr<Command> PasteboardDumpManager::copyHistory;
std::shared_ptr<Command> PasteboardDumpManager::copyData;

namespace {
constexpr const char* FAIL_TO_GET_TIME_STAMP = "fail to get timestamp";
constexpr const size_t DATA_HISTORY_SIZE = 10;
constexpr int USEC_TO_MSEC = 1000;
}

PasteboardDumpManager::PasteboardDumpManager(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDumpManager constructed.");
}

PasteboardDumpManager::~PasteboardDumpManager()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDumpManager destructed.");
}

void PasteboardDumpHelper::InitializeDumpCommands()
{
    copyHistory = std::make_shared<Command>(std::vector<std::string>{ "--copy-history" },
        "Dump access history last ten times.",
        [this](const std::vector<std::string>& input, std::string& output) -> bool {
            output = DumpHistory();
            return true;
        });
    copyData = std::make_shared<Command>(std::vector<std::string>{ "--data" }, "Show copy data details.",
        [this](const std::vector<std::string>& input, std::string& output) -> bool {
            output = DumpData();
            return true;
        });
    PasteboardDumpHelper::GetInstance().RegisterCommand(copyHistory);
    PasteboardDumpHelper::GetInstance().RegisterCommand(copyData);
}

bool PasteboardDumpHelper::SetPasteboardHistory(HistoryInfo& info)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(info.userId != ERROR_USERID, false,
        PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::string history = std::move(info.time) + " " + std::move(info.bundleName) + " " + std::move(info.state) + " " +
                          " " + std::move(info.remote) + " userId:" + std::to_string(info.userId);
    std::lock_guard<decltype(historyMutex_)> lg(historyMutex_);
    if (dataHistory_.size() == DATA_HISTORY_SIZE) {
        dataHistory_.erase(dataHistory_.begin());
    }
    dataHistory_.push_back(std::move(history));
    return true;
}

std::string PasteboardDumpHelper::GetTime()
{
    time_t timeSeconds = time(0);
    if (timeSeconds == -1) {
        return FAIL_TO_GET_TIME_STAMP;
    }
    struct tm nowTime;
    localtime_r(&timeSeconds, &nowTime);

    struct timeval timeVal = { 0, 0 };
    gettimeofday(&timeVal, nullptr);

    std::string targetTime = std::to_string(nowTime.tm_year + 1900) + "-" + std::to_string(nowTime.tm_mon + 1) + "-" +
                             std::to_string(nowTime.tm_mday) + " " + std::to_string(nowTime.tm_hour) + ":" +
                             std::to_string(nowTime.tm_min) + ":" + std::to_string(nowTime.tm_sec) + "." +
                             std::to_string(timeVal.tv_usec / USEC_TO_MSEC);
    return targetTime;
}

std::string PasteboardDumpHelper::DumpUserHistory(int32_t userId) const
{
    std::lock_guard<decltype(historyMutex_)> lg(historyMutex_);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID, "Access history fail! invalid userId.",
        PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::string result;
    if (!dataHistory_.empty()) {
        result.append("Access history last ten times: ").append("\n");
        for (auto iter = dataHistory_.rbegin(); iter != dataHistory_.rend(); ++iter) {
            std::string userIdPrefix = " userId:" + std::to_string(userId);
            size_t userIdPos = (*iter).find(userIdPrefix);
            if (userIdPos != std::string::npos) {
                std::string historyWithoutUserId = (*iter).substr(0, userIdPos);
                result.append("          ").append(historyWithoutUserId).append("\n");
            }
        }
    } else {
        result.append("Access history fail! dataHistory_ no data.").append("\n");
    }
    return result;
}

std::string PasteboardDumpHelper::DumpHistory() const
{
    auto foregroundUsers = service_.ResolveForegroundUsers();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!foregroundUsers.empty(), "Access history fail! no foreground user.",
        PASTEBOARD_MODULE_SERVICE, "no foreground user");
    std::string result;
    for (const auto& ctx : foregroundUsers) {
        if (ctx.userId == ERROR_USERID) {
            continue;
        }
        result += "UserId: " + std::to_string(ctx.userId) + "\n";
        result += DumpUserHistory(ctx.userId);
    }
    return result;
}

std::string PasteboardDumpHelper::DumpUserData(int32_t userId)
{
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = service_.clips_.Find(service_.GetCompositeKey(userId));
#else
    auto it = service_.clips_.Find(service_.GetCompositeKey(userId));
#endif
    if (!it.first || it.second == nullptr) {
        return "No copy data.\n";
    }
    size_t recordCounts = it.second->GetRecordCount();
    auto property = it.second->GetProperty();
    std::string shareOption;
    PasteData::ShareOptionToString(property.shareOption, shareOption);
    std::string sourceDevice = property.isRemote ? "remote" : "local";
    std::string result;
    result.append("|Owner       :  ").append(property.bundleName).append("\n")
        .append("|Timestamp   :  ").append(property.setTime).append("\n")
        .append("|Share Option:  ").append(shareOption).append("\n")
        .append("|Record Count:  ").append(std::to_string(recordCounts)).append("\n")
        .append("|Mime types  :  {");
    if (!property.mimeTypes.empty()) {
        for (size_t i = 0; i < property.mimeTypes.size(); ++i) {
            result.append(property.mimeTypes[i]).append(",");
        }
    }
    result.append("}").append("\n").append("|source device:  ").append(sourceDevice);
    return result;
}

std::string PasteboardDumpHelper::DumpData()
{
    auto foregroundUsers = service_.ResolveForegroundUsers();
    if (foregroundUsers.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query foreground users failed.");
        return "";
    }
    std::string result;
    for (const auto& ctx : foregroundUsers) {
        if (ctx.userId == ERROR_USERID) {
            continue;
        }
        result += "UserId: " + std::to_string(ctx.userId) + "\n";
        result += DumpUserData(ctx.userId);
    }
    return result;
}

size_t PasteboardDumpHelper::GetDataSize(PasteData& data) const
{
    return data.GetTextSize();
}

} // namespace MiscServices
} // namespace OHOS