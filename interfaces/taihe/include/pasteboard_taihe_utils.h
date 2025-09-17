/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_TAIHE_UTILS_H
#define PASTEBOARD_TAIHE_UTILS_H

#include "ohos.pasteboard.pasteboard.proj.hpp"
#include "paste_data.h"

namespace pasteboardTaihe = ohos::pasteboard::pasteboard;

namespace OHOS {
namespace MiscServices {
class ValueTypeStrategy {
public:
    virtual void AddRecord(
        const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> pasteData) = 0;
    virtual void CreateData(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> &pasteData) = 0;
    virtual pasteboardTaihe::ValueType ConvertToValueType(
        const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry) = 0;
    virtual EntryValue ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value) = 0;
    virtual ~ValueTypeStrategy() = default;
};

class ArrayBufferStrategy : public ValueTypeStrategy {
public:
    void AddRecord(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> pasteData) override;
    void CreateData(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> &pasteData) override;
    pasteboardTaihe::ValueType ConvertToValueType(
        const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry) override;
    EntryValue ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value) override;
};

class PixelMapStrategy : public ValueTypeStrategy {
public:
    void AddRecord(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> pasteData) override;
    void CreateData(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> &pasteData) override;
    pasteboardTaihe::ValueType ConvertToValueType(
        const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry) override;
    EntryValue ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value) override;
};

class WantStrategy : public ValueTypeStrategy {
public:
    void AddRecord(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> pasteData) override;
    void CreateData(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> &pasteData) override;
    pasteboardTaihe::ValueType ConvertToValueType(
        const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry) override;
    EntryValue ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value) override;
};

class HtmlStrategy : public ValueTypeStrategy {
public:
    void AddRecord(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> pasteData) override;
    void CreateData(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> &pasteData) override;
    pasteboardTaihe::ValueType ConvertToValueType(
        const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry) override;
    EntryValue ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value) override;
};

class TextStrategy : public ValueTypeStrategy {
public:
    void AddRecord(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> pasteData) override;
    void CreateData(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> &pasteData) override;
    pasteboardTaihe::ValueType ConvertToValueType(
        const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry) override;
    EntryValue ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value) override;
};

class UriStrategy : public ValueTypeStrategy {
public:
    void AddRecord(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> pasteData) override;
    void CreateData(const std::string &mimeType, const pasteboardTaihe::ValueType &value,
        std::shared_ptr<PasteData> &pasteData) override;
    pasteboardTaihe::ValueType ConvertToValueType(
        const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry) override;
    EntryValue ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value) override;
};
class StrategyFactory {
public:
    static std::unique_ptr<ValueTypeStrategy> CreateStrategyForRecord(
        const pasteboardTaihe::ValueType &value, const std::string &mimeType);
    static std::unique_ptr<ValueTypeStrategy> CreateStrategyForData(const std::string &mimeType);
    static std::unique_ptr<ValueTypeStrategy> CreateStrategyForEntry(const std::string &mimeType);
};

class ShareOptionAdapter {
public:
    static ShareOption FromTaihe(pasteboardTaihe::ShareOption value);
    static pasteboardTaihe::ShareOption ToTaihe(ShareOption value);
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_TAIHE_UTILS_H