/*
 * Copyright (C) 2021-2026 Huawei Device Co., Ltd.
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
#include "pasteboard_service.h"

#include "ipc_skeleton.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
namespace {
constexpr const char *FAIL_TO_GET_TIME_STAMP = "FAIL_TO_GET_TIME_STAMP";
constexpr int32_t MAX_DUMP_UID = 10000;
constexpr int32_t TM_YEAR_BASE = 1900;
} // namespace

void PasteboardService::InitializeDumpCommands()
{
    copyHistory = std::make_shared<Command>(std::vector<std::string>{ "--copy-history" },
        "Dump access history last ten times.",
        [this](const std::vector<std::string> &input, std::string &output) -> bool {
            output = DumpHistory();
            return true;
        });
    copyData = std::make_shared<Command>(std::vector<std::string>{ "--data" }, "Show copy data details.",
        [this](const std::vector<std::string> &input, std::string &output) -> bool {
            output = DumpData();
            return true;
        });
    PasteboardDumpHelper::GetInstance().RegisterCommand(copyHistory);
    PasteboardDumpHelper::GetInstance().RegisterCommand(copyData);
    CommonEventSubscriber();
    AccountStateSubscriber();
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    SubProfileSubscriber();
#endif // PB_COCKPIT_PLATFORM_ENABLE
    PasteboardEventSubscriber();
}

bool PasteboardService::SetPasteboardHistory(HistoryInfo &info)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(info.userId != ERROR_USERID, false,
        PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::string history = std::move(info.time) + " " + std::move(info.bundleName) + " " + std::move(info.state) + " " +
                          " " + std::move(info.remote) + " userId:" + std::to_string(info.userId);
    constexpr const size_t DATA_HISTORY_SIZE = 10;
    std::lock_guard<decltype(historyMutex_)> lg(historyMutex_);
    if (dataHistory_.size() == DATA_HISTORY_SIZE) {
        dataHistory_.erase(dataHistory_.begin());
    }
    dataHistory_.push_back(std::move(history));
    return true;
}

int PasteboardService::Dump(int fd, const std::vector<std::u16string> &args)
{
    if (fd < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid fd: %{public}d", fd);
        return ERR_OK;
    }
    int uid = static_cast<int>(IPCSkeleton::GetCallingUid());
    if (uid > MAX_DUMP_UID) {
        return ERR_OK;
    }

    std::vector<std::string> argsStr;
    for (auto item : args) {
        argsStr.emplace_back(Str16ToStr8(item));
    }

    if (PasteboardDumpHelper::GetInstance().Dump(fd, argsStr)) {
        return ERR_OK;
    }
    return ERR_OK;
}

std::string PasteboardService::GetTime()
{
    constexpr int USEC_TO_MSEC = 1000;
    time_t timeSeconds = time(0);
    if (timeSeconds == -1) {
        return FAIL_TO_GET_TIME_STAMP;
    }
    struct tm nowTime;
    localtime_r(&timeSeconds, &nowTime);

    struct timeval timeVal = { 0, 0 };
    gettimeofday(&timeVal, nullptr);

    std::string targetTime = std::to_string(nowTime.tm_year + TM_YEAR_BASE) + "-" + std::to_string(nowTime.tm_mon + 1) + "-" +
                             std::to_string(nowTime.tm_mday) + " " + std::to_string(nowTime.tm_hour) + ":" +
                             std::to_string(nowTime.tm_min) + ":" + std::to_string(nowTime.tm_sec) + "." +
                             std::to_string(timeVal.tv_usec / USEC_TO_MSEC);
    return targetTime;
}

std::string PasteboardService::DumpUserHistory(int32_t userId) const
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

std::string PasteboardService::DumpHistory() const
{
    auto foregroundUsers = ResolveForegroundUsers();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!foregroundUsers.empty(), "Access history fail! no foreground user.",
        PASTEBOARD_MODULE_SERVICE, "no foreground user");
    std::string result;
    for (const auto &ctx : foregroundUsers) {
        if (ctx.userId == ERROR_USERID) {
            continue;
        }
        result += "UserId: " + std::to_string(ctx.userId) + "\n";
        result += DumpUserHistory(ctx.userId);
    }
    return result;
}

std::string PasteboardService::DumpUserData(int32_t userId)
{
    auto it = clips_.Find(userId);
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

std::string PasteboardService::DumpData()
{
    auto foregroundUsers = ResolveForegroundUsers();
    if (foregroundUsers.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query foreground users failed.");
        return "";
    }
    std::string result;
    for (const auto &ctx : foregroundUsers) {
        if (ctx.userId == ERROR_USERID) {
            continue;
        }
        result += "UserId: " + std::to_string(ctx.userId) + "\n";
        result += DumpUserData(ctx.userId);
    }
    return result;
}
} // namespace MiscServices
} // namespace OHOS
