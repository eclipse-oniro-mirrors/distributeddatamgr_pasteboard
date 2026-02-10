/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "pasteboard_disposable_manager.h"

#include <thread>

#include "ffrt/ffrt_utils.h"
#include "ipc_skeleton.h"
#include "parameters.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_window_manager.h"
#include "permission/permission_utils.h"

namespace OHOS::MiscServices {
DisposableManager &DisposableManager::GetInstance()
{
    static DisposableManager instance;
    return instance;
}

void DisposableManager::ProcessNoMatchInfo(const std::vector<DisposableInfo> &noMatchInfoList)
{
    for (const auto &item : noMatchInfoList) {
        if (item.observer == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer is null, pid=%{public}d", item.pid);
            continue;
        }
        item.observer->OnTextReceived("", IPasteboardDisposableObserver::ERR_TARGET_MISMATCH);
    }
}

void DisposableManager::ProcessMatchedInfo(const std::vector<DisposableInfo> &matchedInfoList, PasteData &pasteData,
    const sptr<IPasteboardDelayGetter> &delayGetter, const sptr<IPasteboardEntryGetter> &entryGetter)
{
    std::string text = GetPlainText(pasteData, delayGetter, entryGetter);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "textLen=%{public}zu", text.length());

    for (const auto &item : matchedInfoList) {
        if (item.observer == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer is null, pid=%{public}d", item.pid);
            continue;
        }
        if (pasteData.GetShareOption() == ShareOption::InApp) {
            item.observer->OnTextReceived("", IPasteboardDisposableObserver::ERR_DATA_IN_APP);
            continue;
        }
        if (item.type != DisposableType::PLAIN_TEXT) {
            item.observer->OnTextReceived("", IPasteboardDisposableObserver::ERR_TYPE_NOT_SUPPORT);
            continue;
        }
        if (!PermissionUtils::IsPermissionGranted(PermissionUtils::PERMISSION_READ_PASTEBOARD, item.tokenId)) {
            item.observer->OnTextReceived("", IPasteboardDisposableObserver::ERR_NO_PERMISSION);
            continue;
        }
        if (text.empty()) {
            item.observer->OnTextReceived("", IPasteboardDisposableObserver::ERR_NO_TEXT);
            continue;
        }
        if (text.length() > item.maxLen) {
            item.observer->OnTextReceived("", IPasteboardDisposableObserver::ERR_LENGTH_MISMATCH);
            continue;
        }
        item.observer->OnTextReceived(text, IPasteboardDisposableObserver::ERR_OK);
    }
}

bool DisposableManager::TryProcessDisposableData(PasteData &pasteData,
    const sptr<IPasteboardDelayGetter> &delayGetter, const sptr<IPasteboardEntryGetter> &entryGetter)
{
    std::vector<DisposableInfo> matchedInfoList;
    std::vector<DisposableInfo> noMatchInfoList;
    int32_t windowId = -1;
    {
        std::lock_guard lock(disposableInfoMutex_);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(!disposableInfoList_.empty(), false, PASTEBOARD_MODULE_SERVICE,
            "no disposable observer");
        windowId = WindowManager::GetFocusWindowId();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "focusWindowId=%{public}d", windowId);
        std::partition_copy(disposableInfoList_.begin(), disposableInfoList_.end(),
            std::back_inserter(matchedInfoList), std::back_inserter(noMatchInfoList),
            [windowId](const DisposableInfo &info) { return info.targetWindowId == windowId; });
        disposableInfoList_.clear();
    }

    ProcessNoMatchInfo(noMatchInfoList);
    noMatchInfoList.clear();

    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!matchedInfoList.empty(), false, PASTEBOARD_MODULE_SERVICE,
        "no matched disposable observer, windowId=%{public}d", windowId);
    std::thread thread([=, data = std::make_shared<PasteData>(pasteData)]() {
        PASTEBOARD_CHECK_AND_RETURN_LOGE(data != nullptr, PASTEBOARD_MODULE_SERVICE, "data is null");
        ProcessMatchedInfo(matchedInfoList, *data, delayGetter, entryGetter);
    });
    pthread_setname_np(thread.native_handle(), "ProcessMatchedInfo");
    thread.detach();
    return true;
}

std::string DisposableManager::GetPlainText(PasteData &pasteData,
    const sptr<IPasteboardDelayGetter> &delayGetter, const sptr<IPasteboardEntryGetter> &entryGetter)
{
    if (pasteData.IsDelayData() && delayGetter != nullptr) {
        delayGetter->GetPasteData("", pasteData);
    }

    std::string allText;
    auto mimeTypes = pasteData.GetMimeTypes();
    auto iter = std::find(mimeTypes.begin(), mimeTypes.end(), MIMETYPE_TEXT_PLAIN);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(iter != mimeTypes.end(), allText, PASTEBOARD_MODULE_SERVICE, "no plain text");

    for (auto record : pasteData.AllRecords()) {
        if (record == nullptr) {
            continue;
        }
        auto entry = record->GetEntryByMimeType(MIMETYPE_TEXT_PLAIN);
        if (entry == nullptr) {
            continue;
        }
        std::string utdId = entry->GetUtdId();
        if (pasteData.IsDelayRecord() && entryGetter != nullptr && !entry->HasContent(utdId)) {
            int32_t ret = entryGetter->GetRecordValueByType(record->GetRecordId(), *entry);
            if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get entry failed, recordId=%{public}u, type=%{public}s",
                    record->GetRecordId(), utdId.c_str());
                continue;
            }
        }
        std::shared_ptr<std::string> text = entry->ConvertToPlainText();
        if (text == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "to text failed, recordId=%{public}u", record->GetRecordId());
            continue;
        }
        allText += (*text);
    }
    return allText;
}

int32_t DisposableManager::AddDisposableInfo(const DisposableInfo &info)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(info.observer != nullptr,
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "param invalid, observer is null");
    int32_t typeInt = static_cast<int32_t>(info.type);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(typeInt >= 0 && typeInt < static_cast<int32_t>(DisposableType::MAX),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "param invalid, type=%{public}d", typeInt);

    bool hasPerm = PermissionUtils::IsPermissionGranted(PermissionUtils::PERMISSION_READ_PASTEBOARD, info.tokenId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasPerm,
        static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR), PASTEBOARD_MODULE_SERVICE,
        "check permission failed, pid=%{public}d, tokenId=%{public}u", info.pid, info.tokenId);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid=%{public}d, windowId=%{public}d, type=%{public}d, "
        "maxLen=%{public}u", info.pid, info.targetWindowId, typeInt, info.maxLen);

    FFRTTask expirationTask = [this, pid = info.pid] {
        std::thread thread([=]() {
            RemoveDisposableInfo(pid, true);
        });
        pthread_setname_np(thread.native_handle(), "RemoveDisposabl");
        thread.detach();
    };
    std::string taskName = "disposable_expiration[pid=" + std::to_string(info.pid) + "]";
    int32_t timeout = system::GetIntParameter("pasteboard.disposable_expiration", DISPOSABLE_EXPIRATION_DEFAULT,
        DISPOSABLE_EXPIRATION_MIN, DISPOSABLE_EXPIRATION_MAX);
    auto timer = FFRTPool::GetTimer("pasteboard_service");
    timer->SetTimer(taskName, expirationTask, timeout);

    std::lock_guard lock(disposableInfoMutex_);
    auto iter = std::find_if(disposableInfoList_.begin(), disposableInfoList_.end(),
        [pid = info.pid](const DisposableInfo &item) { return item.pid == pid; });
    if (iter == disposableInfoList_.end()) {
        disposableInfoList_.push_back(info);
    } else {
        *iter = info;
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void DisposableManager::RemoveDisposableInfo(pid_t pid, bool needNotify)
{
    std::lock_guard lock(disposableInfoMutex_);
    auto iter = std::find_if(disposableInfoList_.begin(), disposableInfoList_.end(),
        [pid](const DisposableInfo &info) { return info.pid == pid; });
    PASTEBOARD_CHECK_AND_RETURN_LOGD(iter != disposableInfoList_.end(), PASTEBOARD_MODULE_SERVICE,
        "disposable info not find, pid=%{public}d", pid);

    if (needNotify) {
        if (iter->observer == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer is null, pid=%{public}d", pid);
        } else {
            iter->observer->OnTextReceived("", IPasteboardDisposableObserver::ERR_TIMEOUT);
        }
    }

    disposableInfoList_.erase(iter);
}

} // namespace OHOS::MiscServices
