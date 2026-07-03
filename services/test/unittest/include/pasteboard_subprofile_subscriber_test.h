/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_SUBPROFILE_SUBSCRIBER_TEST_H
#define PASTEBOARD_SUBPROFILE_SUBSCRIBER_TEST_H

#include <gmock/gmock.h>
#include "pasteboard_service.h"

namespace OHOS::MiscServices {

class IPasteboardServiceInterface {
public:
    virtual ~IPasteboardServiceInterface() = default;
    virtual int32_t ClearByUser(int32_t userId) = 0;
};

class PasteboardServiceInterfaceMock : public IPasteboardServiceInterface {
public:
    PasteboardServiceInterfaceMock() = default;
    ~PasteboardServiceInterfaceMock() override = default;
    
    static PasteboardServiceInterfaceMock *GetMock()
    {
        return mock_;
    }
    static void SetMock(PasteboardServiceInterfaceMock *mock)
    {
        mock_ = mock;
    }

    MOCK_METHOD(int32_t, ClearByUser, (int32_t userId), (override));
    
    static inline bool mockFlag_ = false;

private:
    static inline PasteboardServiceInterfaceMock *mock_ = nullptr;
};

} // namespace OHOS::MiscServices

#endif // PASTEBOARD_SUBPROFILE_SUBSCRIBER_TEST_H