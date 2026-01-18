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

namespace OHOS::MiscServices {
int32_t DefaultClip::SetPasteData(const GlobalEvent &event, const std::vector<uint8_t> &data, uint32_t version,
    const std::vector<uint8_t> &mimeTypes)
{
    return 0;
}

std::pair<int32_t, int32_t> DefaultClip::GetPasteData(const GlobalEvent &event, std::vector<uint8_t> &data)
{
    return std::make_pair(0, 0);
}

std::vector<DefaultClip::GlobalEvent> DefaultClip::GetTopEvents(uint32_t topN, int32_t user)
{
    return std::vector<GlobalEvent>();
}

void DefaultClip::Clear(int32_t user) {}

int32_t DefaultClip::ApplyAdvancedResource(const std::string &deviceId)
{
    return 0;
}

int32_t DefaultClip::PublishServiceState(const std::string &networkId, ServiceStatus status)
{
    return 0;
}
} // namespace OHOS::MiscServices