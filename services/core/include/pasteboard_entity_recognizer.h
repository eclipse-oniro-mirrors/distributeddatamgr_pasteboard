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

#ifndef PASTEBOARD_ENTITY_RECOGNIZER_H
#define PASTEBOARD_ENTITY_RECOGNIZER_H

#include "pasteboard_service.h"

namespace OHOS {
namespace MiscServices {

class PasteboardEntityRecognizer {
public:
    explicit PasteboardEntityRecognizer(PasteboardService& service);
    ~PasteboardEntityRecognizer();
    
    void RecognizePasteData(PasteData& pasteData);
    void OnRecognizePasteData(const std::string& primaryText);
    void OnRecognizePasteDataInner(const std::string& primaryText, void* nulGuard);
    
    int32_t GetAllEntryPlainText(uint32_t dataId, uint32_t recordId,
        std::vector<std::shared_ptr<PasteDataEntry>>& entries, std::string& primaryText);
    std::string GetAllPrimaryText(const PasteData& pasteData);
    int32_t ExtractEntity(const std::string& entity, std::string& location);
    
private:
    PasteboardService& service_;
    std::mutex entityRecognizeMutex_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_ENTITY_RECOGNIZER_H