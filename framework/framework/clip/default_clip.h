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

#ifndef OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICES_FRAMEWORK_CLIPS_DEFAULT_CLIPS_H
#define OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICES_FRAMEWORK_CLIPS_DEFAULT_CLIPS_H

#include "clip/clip_plugin.h"

namespace OHOS::MiscServices {
class DefaultClip : public ClipPlugin {
public:
    int32_t SetPasteData(const GlobalEvent &event, const std::vector<uint8_t> &data, uint32_t version,
        const std::vector<uint8_t> &mimeTypes) override;
    std::pair<int32_t, int32_t> GetPasteData(const GlobalEvent &event, std::vector<uint8_t> &data) override;
    std::vector<GlobalEvent> GetTopEvents(uint32_t topN, int32_t user) override;
    void Clear(int32_t user) override;
    int32_t ApplyAdvancedResource(const std::string &deviceId) override;
    int32_t PublishServiceState(const std::string &networkId, ServiceStatus status) override;
};
} // namespace OHOS::MiscServices
#endif // OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICES_FRAMEWORK_CLIPS_DEFAULT_CLIPS_H
