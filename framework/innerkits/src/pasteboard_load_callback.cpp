/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "pasteboard_load_callback.h"
#include "pasteboard_client.h"
#include "pasteboard_hilog.h"
#include "pasteboard_service_loader.h"

namespace OHOS {
namespace MiscServices {

void PasteboardLoadCallback::OnLoadSystemAbilitySuccess(
    int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject)
{
    if (PasteboardServiceLoader::IsStaticDestroyed()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "static already destroyed, skip load sa success.");
        return;
    }
    PasteboardServiceLoader::GetInstance().LoadSystemAbilitySuccess(remoteObject);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Load system ability success!");
}

void PasteboardLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    if (PasteboardServiceLoader::IsStaticDestroyed()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "static already destroyed, skip load sa fail.");
        return;
    }
    PasteboardServiceLoader::GetInstance().LoadSystemAbilityFail();
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Load system ability failed!");
}
} // namespace MiscServices
}