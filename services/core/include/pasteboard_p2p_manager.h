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

#ifndef PASTEBOARD_P2P_MANAGER_H
#define PASTEBOARD_P2P_MANAGER_H

#include "common/block_object.h"
#include "clip/clip_plugin.h"
#include "concurrent_map.h"
#include "eventcenter/event.h"
#include "ffrt/ffrt_utils.h"
#include "input_manager.h"
#include "paste_data.h"

namespace OHOS {
namespace MiscServices {

class PasteboardService;

struct PasteP2pEstablishInfo {
    std::string networkid;
    std::shared_ptr<BlockObject<int32_t>> pasteBlock;
};

struct PasteboardP2pInfo {
    pid_t callPid;
    bool isSuccess;
};

class PasteboardP2PManager {
public:
    explicit PasteboardP2PManager(PasteboardService& service);
    ~PasteboardP2PManager();
    
    void OpenP2PLink(const std::string& networkId);
    void EstablishP2PLink(const std::string& networkId, const std::string& pasteId);
    std::shared_ptr<BlockObject<int32_t>> CheckAndReuseP2PLink(const std::string& networkId,
        const std::string& pasteId);
    void CloseP2PLink(const std::string& networkId);
    
    void PreEstablishP2PLink(const std::string& networkId, ClipPlugin* clipPlugin);
    void PreEstablishP2PLinkCallback(const std::string& networkId, ClipPlugin* clipPlugin);
    bool OpenP2PLinkForPreEstablish(const std::string& networkId, ClipPlugin* clipPlugin);
    
    void PreSyncRemotePasteboardData();
    void PreSyncSwitchMonitorCallback();
    void RegisterPreSyncMonitor();
    void UnRegisterPreSyncMonitor();
    void DeletePreSyncP2pFromP2pMap(const std::string& networkId);
    void DeletePreSyncP2pMap(const std::string& networkId);
    void AddPreSyncP2pTimeoutTask(const std::string& networkId);
    
    bool IsContainUri(const Event& evt);
    bool IsNeedLink(PasteData& data);
    void ClearP2PEstablishTaskInfo();
    
    std::shared_ptr<BlockObject<int32_t>> EstablishP2PLinkTask(
        const std::string& pasteId, const ClipPlugin::GlobalEvent& event);
    void OnEstablishP2PLinkTask(const std::string& networkId, std::shared_ptr<BlockObject<int32_t>> pasteBlock);
    int32_t OnPasteComplete(const std::string& deviceId, const std::string& pasteId);
    void ClearAllP2PLinks();
    void RemoveP2PLinkByNetworkId(const std::string& networkId);
    void RemoveP2PLinksByPid(pid_t pid, std::vector<std::string>& networkIds);
    
private:
    PasteboardService& service_;
    std::shared_ptr<FFRTTimer> ffrtTimer_;
    std::mutex p2pMapMutex_;
    PasteP2pEstablishInfo p2pEstablishInfo_;
    ConcurrentMap<std::string, ConcurrentMap<std::string, PasteboardP2pInfo>> p2pMap_;
    std::map<std::string, std::shared_ptr<BlockObject<int32_t>>> preSyncP2pMap_;
    int32_t subscribeActiveId_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_P2P_MANAGER_H