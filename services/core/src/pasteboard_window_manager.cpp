/*
 * Copyright (C) 2025-2026 Huawei Device Co., Ltd.
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

#include "pasteboard_window_manager.h"

#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif // SCENE_BOARD_ENABLE

namespace OHOS::MiscServices {
int32_t WindowManager::GetFocusWindowId()
{
    return GetFocusWindowId(-1);
}

int32_t WindowManager::GetFocusWindowId(int32_t userId)
{
    Rosen::FocusChangeInfo info;
    GetFocusWindowInfo(userId, info);
    return info.windowId_;
}

void WindowManager::GetFocusWindowInfo(int32_t userId, Rosen::FocusChangeInfo &info)
{
#ifdef SCENE_BOARD_ENABLE
    Rosen::WindowManagerLite::GetInstance(userId).GetFocusWindowInfo(info);
#else
    Rosen::WindowManager::GetInstance(userId).GetFocusWindowInfo(info);
#endif
}

Rosen::WMError WindowManager::GetVisibilityWindowInfo(
    int32_t userId, std::vector<sptr<Rosen::WindowVisibilityInfo>> &infos)
{
#ifdef SCENE_BOARD_ENABLE
    return Rosen::WindowManagerLite::GetInstance(userId).GetVisibilityWindowInfo(infos);
#else
    return Rosen::WindowManager::GetInstance(userId).GetVisibilityWindowInfo(infos);
#endif
}
} // namespace OHOS::MiscServices
