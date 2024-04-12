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

#ifndef PASTE_BOARD_SERVICE_PROXY_H
#define PASTE_BOARD_SERVICE_PROXY_H

#include "i_pasteboard_observer.h"
#include "i_pasteboard_service.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace MiscServices {
class PasteboardServiceProxy : public IRemoteProxy<IPasteboardService> {
public:
    explicit PasteboardServiceProxy(const sptr<IRemoteObject> &object);
    ~PasteboardServiceProxy() = default;
    DISALLOW_COPY_AND_MOVE(PasteboardServiceProxy);
    virtual void Clear() override;
    virtual int32_t GetPasteData(PasteData &data) override;
    virtual bool HasPasteData() override;
    virtual int32_t SetPasteData(PasteData &pasteData) override;
    virtual bool IsRemoteData() override;
    virtual int32_t GetDataSource(std::string &bundleName) override;
    virtual bool HasDataType(const std::string &mimeType) override;
    virtual void AddPasteboardChangedObserver(const sptr<IPasteboardChangedObserver> &observer) override;
    virtual void RemovePasteboardChangedObserver(const sptr<IPasteboardChangedObserver> &observer) override;
    virtual void RemoveAllChangedObserver() override;
    virtual void AddPasteboardEventObserver(const sptr<IPasteboardChangedObserver> &observer) override;
    virtual void RemovePasteboardEventObserver(const sptr<IPasteboardChangedObserver> &observer) override;
    virtual void RemoveAllEventObserver() override;
    virtual int32_t SetGlobalShareOption(const std::map<uint32_t, ShareOption> &globalShareOptions) override;
    virtual int32_t RemoveGlobalShareOption(const std::vector<uint32_t> &tokenIds) override;
    virtual std::map<uint32_t, ShareOption> GetGlobalShareOption(const std::vector<uint32_t> &tokenIds) override;

private:
    static inline BrokerDelegator<PasteboardServiceProxy> delegator_;
    void ProcessObserver(uint32_t code, const sptr<IPasteboardChangedObserver> &observer);
    static constexpr int32_t MAX_GET_GLOBAL_SHARE_OPTION_SIZE = 500;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_SERVICE_PROXY_H