/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef PASTE_BOARD_SERVICE_H
#define PASTE_BOARD_SERVICE_H

#include <atomic>
#include <condition_variable>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stack>
#include <sys/time.h>
#include <system_ability_definition.h>
#include <thread>

#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "clip/clip_plugin.h"
#include "event_handler.h"
#include "i_pasteboard_observer.h"
#include "iremote_object.h"
#include "pasteboard_common_event_subscriber.h"
#include "paste_data.h"
#include "pasteboard_dump_helper.h"
#include "pasteboard_service_stub.h"
#include "system_ability.h"

namespace OHOS {
namespace MiscServices {
const std::int32_t ERROR_USERID = -1;
const std::int32_t RESULT_OK = 0;
enum class ServiceRunningState { STATE_NOT_START, STATE_RUNNING };
struct AppInfo {
    std::string bundleName = "com.pasteboard.default";
    int32_t tokenType = -1;
    int32_t userId = ERROR_USERID;
    uint32_t tokenId;
};

struct HistoryInfo {
    std::string time;
    std::string bundleName;
    std::string state;
    std::string remote;
};

class PasteboardService final : public SystemAbility, public PasteboardServiceStub {
    DECLARE_SYSTEM_ABILITY(PasteboardService)

public:
    API_EXPORT PasteboardService();
    API_EXPORT ~PasteboardService();
    virtual void Clear() override;
    virtual int32_t GetPasteData(PasteData &data) override;
    virtual bool HasPasteData() override;
    virtual int32_t SetPasteData(PasteData &pasteData) override;
    virtual bool IsRemoteData() override;
    virtual bool HasDataType(const std::string &mimeType) override;
    virtual int32_t GetDataSource(std::string &bundleNme) override;
    virtual void AddPasteboardChangedObserver(const sptr<IPasteboardChangedObserver> &observer) override;
    virtual void RemovePasteboardChangedObserver(const sptr<IPasteboardChangedObserver> &observer) override;
    virtual void RemoveAllChangedObserver() override;
    virtual void AddPasteboardEventObserver(const sptr<IPasteboardChangedObserver> &observer) override;
    virtual void RemovePasteboardEventObserver(const sptr<IPasteboardChangedObserver> &observer) override;
    virtual void RemoveAllEventObserver() override;
    virtual void OnStart() override;
    virtual void OnStop() override;
    static int32_t currentUserId;
    size_t GetDataSize(PasteData &data) const;
    bool SetPasteboardHistory(HistoryInfo &info);
    int Dump(int fd, const std::vector<std::u16string> &args) override;

private:
    using Event = ClipPlugin::GlobalEvent;
    using ServiceListenerFunc = void (PasteboardService::*)();
    static constexpr const int32_t LISTENING_SERVICE[] = { DISTRIBUTED_HARDWARE_DEVICEMANAGER_SA_ID,
        DISTRIBUTED_DEVICE_PROFILE_SA_ID, WINDOW_MANAGER_SERVICE_ID };
    static constexpr const char *PLUGIN_NAME = "distributed_clip";
    static constexpr uint32_t PLAIN_INDEX = 0;
    static constexpr uint32_t HTML_INDEX = 1;
    static constexpr uint32_t URI_INDEX = 2;
    static constexpr uint32_t WANT_INDEX = 3;
    static constexpr uint32_t PIXELMAP_INDEX = 4;
    static constexpr uint32_t MAX_INDEX_LENGTH = 8;
    static constexpr const pid_t EDM_UID = 3057;
    static constexpr const pid_t ROOT_UID = 0;
    static constexpr uint32_t EXPIRATION_INTERVAL = 2;
    static constexpr uint32_t OPEN_P2P_SLEEP_TIME = 5;
    static constexpr int TRANMISSION_BASELINE = 30 * 1024 * 1024;
    static constexpr int MIN_TRANMISSION_TIME = 600;
    static constexpr int64_t ONE_HOUR_MILLISECONDS = 60 * 60 * 1000;
    struct classcomp {
        bool operator()(const sptr<IPasteboardChangedObserver> &l, const sptr<IPasteboardChangedObserver> &r) const
        {
            return l->AsObject() < r->AsObject();
        }
    };
    using ObserverMap = std::map<int32_t, std::shared_ptr<std::set<sptr<IPasteboardChangedObserver>, classcomp>>>;
    void AddSysAbilityListener();
    int32_t Init();
    static int32_t GetCurrentAccountId();
    std::string DumpHistory() const;
    std::string DumpData();
    void NotifyObservers(std::string bundleName, PasteboardEventStatus status);
    void InitServiceHandler();
    bool IsCopyable(uint32_t tokenId) const;

    virtual int32_t SavePasteData(std::shared_ptr<PasteData> &pasteData) override;
    void SetPasteDataDot(PasteData &pasteData);
    void GetPasteDataDot(PasteData &pasteData, const std::string &bundleName);
    bool GetPasteData(AppInfo &appInfo, PasteData &data, bool isFocusedApp);
    bool CheckPasteData(AppInfo &appInfo, PasteData &data, bool isFocusedApp);
    bool GetRemoteData(AppInfo &appInfo, PasteData &data, bool isFocusedApp);
    void CheckUriPermission(PasteData &data, std::vector<Uri> &grantUris, const std::string &targetBundleName);
    void GrantUriPermission(PasteData &data, const std::string &targetBundleName);
    void RevokeUriPermission(PasteData &lastData);
    void GenerateDistributedUri(PasteData &data);
    bool isBundleOwnUriPermission(const std::string &bundleName, Uri &uri);
    void CheckAppUriPermission(PasteData &data);
    std::string GetAppLabel(uint32_t tokenId);
    sptr<OHOS::AppExecFwk::IBundleMgr> GetAppBundleManager();
    void EstablishP2PLink(int fileSize);
    uint8_t GenerateDataType(PasteData &data);
    bool HasDistributedDataType(const std::string &mimeType);

    std::shared_ptr<PasteData> GetDistributedData(int32_t user);
    bool SetDistributedData(int32_t user, PasteData &data);
    bool HasDistributedData(int32_t user);
    bool GetDistributedEvent(std::shared_ptr<ClipPlugin> plugin, int32_t user, Event &event);
    bool CleanDistributedData(int32_t user);
    void OnConfigChange(bool isOn);
    std::shared_ptr<ClipPlugin> GetClipPlugin();

    static std::string GetTime();
    bool IsDataAged();
    bool HasPastePermission(uint32_t tokenId, bool isFocusedApp, const std::shared_ptr<PasteData> &pasteData);
    static AppInfo GetAppInfo(uint32_t tokenId);
    static std::string GetAppBundleName(const AppInfo &appInfo);
    static bool IsDefaultIME(const AppInfo &appInfo);
    static bool IsFocusedApp(uint32_t tokenId);
    static void SetLocalPasteFlag(bool isCrossPaste, uint32_t tokenId, PasteData &pasteData);
    void ShowHintToast(bool isValid, uint32_t tokenId, const std::shared_ptr<PasteData> &pasteData);
    void SetWebViewPasteData(PasteData &pasteData, const std::string &bundleName);
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void DevManagerInit();
    void DevProfileInit();
    ServiceRunningState state_;
    std::shared_ptr<AppExecFwk::EventHandler> serviceHandler_;
    std::recursive_mutex clipMutex_;
    std::mutex hintMutex_;
    std::mutex observerMutex_;
    ObserverMap observerChangedMap_;
    ObserverMap observerEventMap_;
    ClipPlugin::GlobalEvent currentEvent_;
    const std::string filePath_ = "";
    std::map<int32_t, std::shared_ptr<PasteData>> clips_;
    std::map<int32_t, std::vector<int32_t>> hints_;
    std::map<int32_t, int64_t> copyTime_;
    std::set<std::string> readBundles_;
    std::shared_ptr<PasteBoardCommonEventSubscriber> commonEventSubscriber_ = nullptr;

    std::recursive_mutex mutex;
    std::shared_ptr<ClipPlugin> clipPlugin_ = nullptr;
    std::atomic<uint32_t> sequenceId_ = 0;
    static std::mutex historyMutex_;
    static std::vector<std::string> dataHistory_;
    static std::shared_ptr<Command> copyHistory;
    static std::shared_ptr<Command> copyData;
    static std::atomic<bool> isP2pOpen_;
    std::atomic<bool> setting_ = false;
    std::mutex remoteMutex_;
    std::map<int32_t, ServiceListenerFunc> ServiceListenerFunc_;
    std::map<std::string, int> typeMap_ = {
        { MIMETYPE_TEXT_PLAIN, PLAIN_INDEX },
        { MIMETYPE_TEXT_HTML, HTML_INDEX },
        { MIMETYPE_TEXT_URI, URI_INDEX },
        { MIMETYPE_TEXT_WANT, WANT_INDEX },
        { MIMETYPE_PIXELMAP, PIXELMAP_INDEX }
    };

    void AddObserver(const sptr<IPasteboardChangedObserver> &observer, ObserverMap &observerMap);
    void RemoveSingleObserver(const sptr<IPasteboardChangedObserver> &observer, ObserverMap &observerMap);
    void RemoveAllObserver(ObserverMap &observerMap);
    inline bool IsCallerUidValid();
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_SERVICE_H
