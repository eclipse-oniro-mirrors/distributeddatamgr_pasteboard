/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include "default_clip.h"
#include "pasteboard_event_dfx.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
std::map<std::string, ClipPlugin::Factory *> ClipPlugin::factories_;
DefaultClip g_defaultClip;
bool ClipPlugin::RegCreator(const std::string &name, Factory *factory)
{
    if (factory == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "factory is null, name=%{public}s", name.c_str());
        return false;
    }
    auto it = factories_.find(name);
    if (it != factories_.end()) {
        return false;
    }
    factories_[name] = factory;
    return true;
}

ClipPlugin *ClipPlugin::CreatePlugin(const std::string &name)
{
    auto it = factories_.find(name);
    if (it == factories_.end() || it->second == nullptr) {
        return &g_defaultClip;
    }
    RADAR_REPORT(
        RadarReporter::DFX_PLUGIN_CREATE_DESTROY, RadarReporter::DFX_PLUGIN_CREATE, RadarReporter::DFX_SUCCESS);
    return it->second->Create();
}

bool ClipPlugin::DestroyPlugin(const std::string &name, ClipPlugin *plugin)
{
    if (plugin == nullptr) {
        return false;
    }

    if (plugin == &g_defaultClip) {
        return true;
    }

    auto it = factories_.find(name);
    if (it == factories_.end() || it->second == nullptr) {
        return false;
    }
    RADAR_REPORT(
        RadarReporter::DFX_PLUGIN_CREATE_DESTROY, RadarReporter::DFX_PLUGIN_DESTROY, RadarReporter::DFX_SUCCESS);
    return it->second->Destroy(plugin);
}

std::vector<ClipPlugin::GlobalEvent> ClipPlugin::GetTopEvents(uint32_t topN)
{
    (void)topN;
    return std::vector<GlobalEvent>();
}

std::vector<ClipPlugin::GlobalEvent> ClipPlugin::GetTopEvents(uint32_t topN, int32_t user)
{
    (void)user;
    (void)topN;
    return std::vector<GlobalEvent>();
}

void ClipPlugin::Clear() {}

int32_t ClipPlugin::ApplyAdvancedResource(const std::string &deviceId)
{
    return 0;
}

int32_t ClipPlugin::PublishServiceState(const std::string &networkId, ServiceStatus status)
{
    return 0;
}

void ClipPlugin::Clear(int32_t user)
{
    (void)user;
}

int32_t ClipPlugin::Close(int32_t user)
{
    (void)user;
    return 0;
}

void ClipPlugin::RegisterDelayCallback(const DelayDataCallback &dataCallback, const DelayEntryCallback &entryCallback)
{
    (void)dataCallback;
    (void)entryCallback;
}

int32_t ClipPlugin::GetPasteDataEntry(const GlobalEvent &event, uint32_t recordId, const std::string &utdId,
    std::vector<uint8_t> &rawData)
{
    (void)event;
    (void)recordId;
    (void)utdId;
    (void)rawData;
    return 0;
}

bool ClipPlugin::NeedSyncTopEvent()
{
    return false;
}

void ClipPlugin::RegisterPreSyncCallback(const PreSyncCallback &callback)
{
    (void)callback;
}

void ClipPlugin::RegisterPreSyncMonitorCallback(const PreSyncMonitorCallback &callback)
{
    (void)callback;
}

void ClipPlugin::SendPreSyncEvent(int32_t userId)
{
    (void)userId;
}

void ClipPlugin::SetMaxLocalCapacity(int64_t maxLocalCapacity)
{
    (void)maxLocalCapacity;
}

bool ClipPlugin::GlobalEvent::Marshal(Serializable::json &node) const
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, version, GET_NAME(version)),
        false, PASTEBOARD_MODULE_SERVICE, "Set version fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, frameNum, GET_NAME(frameNum)),
        false, PASTEBOARD_MODULE_SERVICE, "Set frameNum fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, user, GET_NAME(user)),
        false, PASTEBOARD_MODULE_SERVICE, "Set user fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, seqId, GET_NAME(seqId)),
        false,  PASTEBOARD_MODULE_SERVICE, "Set seqId fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, expiration, GET_NAME(expiration)),
        false,  PASTEBOARD_MODULE_SERVICE, "Set expiration fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, status, GET_NAME(status)),
        false,  PASTEBOARD_MODULE_SERVICE, "Set status fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, deviceId, GET_NAME(deviceId)),
        false,  PASTEBOARD_MODULE_SERVICE, "Set deviceId fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, account, GET_NAME(account)),
        false,  PASTEBOARD_MODULE_SERVICE, "Set account fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, dataType, GET_NAME(dataType)),
        false,  PASTEBOARD_MODULE_SERVICE, "Set dataType fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(SetValue(node, syncTime, GET_NAME(syncTime)),
        false,  PASTEBOARD_MODULE_SERVICE, "Set syncTime fail");
    return true;
}

bool ClipPlugin::GlobalEvent::Unmarshal(const Serializable::json &node)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(version), version),
        false,  PASTEBOARD_MODULE_SERVICE, "Get version fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(frameNum), frameNum),
        false,  PASTEBOARD_MODULE_SERVICE, "Get frameNum fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(user), user),
        false,  PASTEBOARD_MODULE_SERVICE, "Get user fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(seqId), seqId),
        false,  PASTEBOARD_MODULE_SERVICE, "Get seqId fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(expiration), expiration),
        false,  PASTEBOARD_MODULE_SERVICE, "Get expiration fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(status), status),
        false,  PASTEBOARD_MODULE_SERVICE, "Get status fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(deviceId), deviceId),
        false,  PASTEBOARD_MODULE_SERVICE, "Get deviceId fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(account), account),
        false,  PASTEBOARD_MODULE_SERVICE, "Get account fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(dataType), dataType),
        false,  PASTEBOARD_MODULE_SERVICE, "Get dataType fail");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(GetValue(node, GET_NAME(syncTime), syncTime),
        false,  PASTEBOARD_MODULE_SERVICE, "Get syncTime fail");
    return true;
}

void ClipPlugin::ChangeStoreStatus(int32_t userId)
{
    (void)userId;
}
} // namespace OHOS::MiscServices
