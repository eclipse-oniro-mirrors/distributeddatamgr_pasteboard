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

#include "tlv_object.h"

#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {

bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, std::monostate value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Write monostate value failed.");
    cursor_ += sizeof(TLVHead);
    return true;
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, void *value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Write value failed.");
    cursor_ += sizeof(TLVHead);
    return true;
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, bool value)
{
    return WriteBasic(buffer, type, (int8_t)(value));
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, int8_t value)
{
    return WriteBasic(buffer, type, value);
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, int16_t value)
{
    return WriteBasic(buffer, type, value);
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, double value)
{
    return WriteBasic(buffer, type, value);
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, int32_t value)
{
    return WriteBasic(buffer, type, value);
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, int64_t value)
{
    return WriteBasic(buffer, type, value);
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, uint32_t value)
{
    return WriteBasic(buffer, type, value);
}
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, const std::string &value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead) + value.size())), false,
        PASTEBOARD_MODULE_CLIENT, "Write string value failed.");
    auto *tlvHead = reinterpret_cast<TLVHead *>(buffer.data() + cursor_);
    tlvHead->tag = HostToNet(type);
    tlvHead->len = HostToNet((uint32_t)value.size());
    if (!value.empty()) {
        auto err = memcpy_s(tlvHead->value, value.size(), value.c_str(), value.size());
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((err == EOK), false, PASTEBOARD_MODULE_CLIENT, "memcpy_s not EOK.");
    }

    cursor_ += sizeof(TLVHead) + value.size();
    return true;
}

bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, const RawMem &value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead) + value.bufferLen)), false,
        PASTEBOARD_MODULE_CLIENT, "Write RawMem value failed.");
    auto *tlvHead = reinterpret_cast<TLVHead *>(buffer.data() + cursor_);
    tlvHead->tag = HostToNet(type);
    cursor_ += sizeof(TLVHead);

    if (value.bufferLen != 0 && value.buffer != 0) {
        auto err = memcpy_s(buffer.data() + cursor_, buffer.size() - cursor_,
            reinterpret_cast<const void *>(value.buffer), value.bufferLen);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((err == EOK), false, PASTEBOARD_MODULE_CLIENT, "memcpy_s not EOK.");
    }
    cursor_ += value.bufferLen;
    tlvHead->len = HostToNet((uint32_t)value.bufferLen);
    return true;
}

bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, const AAFwk::Want &value)
{
    return Write(buffer, type, ParcelUtil::Parcelable2Raw(&value));
}

bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, const Media::PixelMap &value)
{
    std::vector<std::uint8_t> u8Value;
    if (!value.EncodeTlv(u8Value)) {
        return false;
    }
    return Write(buffer, type, u8Value);
}

bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, const Object &value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Write Object value failed.");
    auto tagCursor = cursor_;
    cursor_ += sizeof(TLVHead);
    auto valueCursor = cursor_;
    for (const auto &[key, val] : value.value_) {
        if (!Write(buffer, TAG_MAP_KEY, key)) {
            return false;
        }
        if (!Write(buffer, TAG_MAP_VALUE_TYPE, val)) {
            return false;
        }
    }
    WriteHead(buffer, type, tagCursor, cursor_ - valueCursor);
    return true;
}

bool TLVObject::Write(
    std::vector<std::uint8_t> &buffer, uint16_t type, std::map<std::string, std::vector<uint8_t>> &value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Write vector value failed.");
    auto tagCursor = cursor_;
    cursor_ += sizeof(TLVHead);
    auto valueCursor = cursor_;

    bool ret = true;
    for (auto &item : value) {
        ret = ret && Write(buffer, TAG_MAP_KEY, item.first);
        ret = ret && Write(buffer, TAG_MAP_VALUE, item.second);
    }
    WriteHead(buffer, type, tagCursor, cursor_ - valueCursor);
    return ret;
}

template<typename _InTp>
bool TLVObject::WriteVariant(std::vector<std::uint8_t> &buffer, uint16_t type, uint32_t step, const _InTp &input)
{
    return true;
}

template<typename _InTp, typename _First, typename... _Rest>
bool TLVObject::WriteVariant(std::vector<std::uint8_t> &buffer, uint16_t type, uint32_t step, const _InTp &input)
{
    if (step == input.index()) {
        auto val = std::get<_First>(input);
        return Write(buffer, type, val);
    }
    return WriteVariant<_InTp, _Rest...>(buffer, type, step + 1, input);
}

template<typename... _Types>
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, const std::variant<_Types...> &input)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Write variant value failed.");
    auto tagCursor = cursor_;
    cursor_ += sizeof(TLVHead);
    auto valueCursor = cursor_;

    uint32_t index = static_cast<uint32_t>(input.index());
    if (!Write(buffer, TAG_VARIANT_INDEX, index)) {
        return false;
    }
    WriteVariant<decltype(input), _Types...>(buffer, TAG_VARIANT_VALUE, 0, input);
    WriteHead(buffer, type, tagCursor, cursor_ - valueCursor);
    return true;
}

template<>
bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, const EntryValue &input)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Write input value failed.");
    auto tagCursor = cursor_;
    cursor_ += sizeof(TLVHead);
    auto valueCursor = cursor_;

    uint32_t index = static_cast<uint32_t>(input.index());
    if (!Write(buffer, TAG_VARIANT_INDEX, index)) {
        return false;
    }
    WriteVariant<decltype(input), std::monostate, int32_t, int64_t, double, bool, std::string, std::vector<uint8_t>,
        std::shared_ptr<OHOS::AAFwk::Want>, std::shared_ptr<OHOS::Media::PixelMap>, std::shared_ptr<Object>, nullptr_t>(
        buffer, TAG_VARIANT_VALUE, 0, input);
    WriteHead(buffer, type, tagCursor, cursor_ - valueCursor);
    return true;
}

bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, const Details &value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Write Details value failed.");
    auto tagCursor = cursor_;
    cursor_ += sizeof(TLVHead);
    auto valueCursor = cursor_;
    for (const auto &[key, val] : value) {
        if (!Write(buffer, TAG_MAP_KEY, key)) {
            return false;
        }
        if (!Write(buffer, TAG_MAP_VALUE_TYPE, val)) {
            return false;
        }
    }
    WriteHead(buffer, type, tagCursor, cursor_ - valueCursor);
    return true;
}

bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, TLVObject &value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Write TLVObject value failed.");
    auto tagCursor = cursor_;
    cursor_ += sizeof(TLVHead);
    auto valueCursor = cursor_;
    bool ret = value.Encode(buffer, cursor_, buffer.size());
    WriteHead(buffer, type, tagCursor, cursor_ - valueCursor);
    return ret;
}

bool TLVObject::Write(std::vector<std::uint8_t> &buffer, uint16_t type, std::vector<uint8_t> &value)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead) + value.size())), false,
        PASTEBOARD_MODULE_CLIENT, "Write vector value failed.");
    WriteHead(buffer, type, cursor_, value.size());
    cursor_ += sizeof(TLVHead);

    if (!value.empty()) {
        auto err = memcpy_s(buffer.data() + cursor_, buffer.size() - cursor_, value.data(), value.size());
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((err == EOK), false, PASTEBOARD_MODULE_CLIENT, "memcpy_s not EOK.");
    }
    cursor_ += value.size();
    return true;
}
bool TLVObject::ReadHead(const std::vector<std::uint8_t> &buffer, TLVHead &head)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, sizeof(TLVHead))), false,
        PASTEBOARD_MODULE_CLIENT, "Read Head value failed.");
    const auto *pHead = reinterpret_cast<const TLVHead *>(buffer.data() + cursor_);
    if (!HasExpectBuffer(buffer, NetToHost(pHead->len)) &&
        !HasExpectBuffer(buffer, NetToHost(pHead->len) + sizeof(TLVHead))) {
        return false;
    }
    head.tag = NetToHost(pHead->tag);
    head.len = NetToHost(pHead->len);
    cursor_ += sizeof(TLVHead);
    return true;
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, std::monostate &value, const TLVHead &head)
{
    return true;
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, void *value, const TLVHead &head)
{
    (void)buffer;
    (void)value;
    (void)head;
    return true;
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, bool &value, const TLVHead &head)
{
    return ReadBasicValue(buffer, value, head);
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, int8_t &value, const TLVHead &head)
{
    return ReadBasicValue(buffer, value, head);
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, int16_t &value, const TLVHead &head)
{
    return ReadBasicValue(buffer, value, head);
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, int32_t &value, const TLVHead &head)
{
    return ReadBasicValue(buffer, value, head);
}

bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, int64_t &value, const TLVHead &head)
{
    return ReadBasicValue(buffer, value, head);
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, double &value, const TLVHead &head)
{
    return ReadBasicValue(buffer, value, head);
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, uint32_t &value, const TLVHead &head)
{
    return ReadBasicValue(buffer, value, head);
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, std::string &value, const TLVHead &head)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, head.len)), false,
        PASTEBOARD_MODULE_CLIENT, "Read string value failed.");
    value.append(reinterpret_cast<const char *>(buffer.data() + cursor_), head.len);
    cursor_ += head.len;
    return true;
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, RawMem &rawMem, const TLVHead &head)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, head.len)), false,
        PASTEBOARD_MODULE_CLIENT, "Read RawMem value failed.");
    rawMem.buffer = (uintptr_t)(buffer.data() + cursor_);
    rawMem.bufferLen = head.len;
    cursor_ += head.len;
    return true;
}

bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, TLVObject &value, const TLVHead &head)
{
    return value.Decode(buffer, cursor_, cursor_ + head.len);
}
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, std::vector<uint8_t> &value, const TLVHead &head)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, head.len)), false,
        PASTEBOARD_MODULE_CLIENT, "Read vector value failed.");
    std::vector<uint8_t> buff(buffer.data() + cursor_, buffer.data() + cursor_ + head.len);
    value = std::move(buff);
    cursor_ += head.len;
    return true;
}
bool TLVObject::ReadValue(
    const std::vector<std::uint8_t> &buffer, std::map<std::string, std::vector<uint8_t>> &value, const TLVHead &head)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((HasExpectBuffer(buffer, head.len)), false,
        PASTEBOARD_MODULE_CLIENT, "Read map value failed.");
    auto mapEnd = cursor_ + head.len;
    for (; cursor_ < mapEnd;) {
        // item key
        TLVHead keyHead{};
        bool ret = ReadHead(buffer, keyHead);
        std::string itemKey;
        ret = ret && ReadValue(buffer, itemKey, keyHead);

        // item value
        TLVHead valueHead{};
        ret = ret && ReadHead(buffer, valueHead);
        std::vector<uint8_t> itemValue(0);
        ret = ret && ReadValue(buffer, itemValue, valueHead);
        if (!ret) {
            return false;
        }
        value.emplace(itemKey, itemValue);
    }
    return true;
}

template<typename _OutTp>
bool TLVObject::ReadVariant(
    const std::vector<std::uint8_t> &buffer, uint32_t step, uint32_t index, _OutTp &output, const TLVHead &head)
{
    return true;
}

template<typename _OutTp, typename _First, typename... _Rest>
bool TLVObject::ReadVariant(
    const std::vector<std::uint8_t> &buffer, uint32_t step, uint32_t index, _OutTp &value, const TLVHead &head)
{
    if (step == index) {
        TLVHead valueHead{};
        ReadHead(buffer, valueHead);
        _First output{};
        auto success = ReadValue(buffer, output, valueHead);
        value = output;
        return success;
    }
    return ReadVariant<_OutTp, _Rest...>(buffer, step + 1, index, value, head);
}

template<typename... _Types>
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, std::variant<_Types...> &value, const TLVHead &head)
{
    TLVHead valueHead{};
    ReadHead(buffer, valueHead);
    uint32_t index = 0;
    if (!ReadValue(buffer, index, valueHead)) {
        return false;
    }
    return ReadVariant<decltype(value), _Types...>(buffer, 0, index, value, valueHead);
}

template<>
bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, EntryValue &value, const TLVHead &head)
{
    TLVHead valueHead{};
    ReadHead(buffer, valueHead);
    uint32_t index = 0;
    if (!ReadValue(buffer, index, valueHead)) {
        return false;
    }
    return ReadVariant<decltype(value), std::monostate, int32_t, int64_t, double, bool,
        std::string, std::vector<uint8_t>, std::shared_ptr<OHOS::AAFwk::Want>,
        std::shared_ptr<OHOS::Media::PixelMap>, std::shared_ptr<Object>, nullptr_t>(
        buffer, 0, index, value, valueHead);
}

bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, Details &value, const TLVHead &head)
{
    auto mapEnd = cursor_ + head.len;
    while (cursor_ < mapEnd) {
        TLVHead keyHead{};
        if (!ReadHead(buffer, keyHead)) {
            return false;
        }
        std::string itemKey = "";
        if (!ReadValue(buffer, itemKey, keyHead)) {
            return false;
        }
        TLVHead variantHead{};
        if (!ReadHead(buffer, variantHead)) {
            return false;
        }
        ValueType itemValue;
        if (!ReadValue(buffer, itemValue, variantHead)) {
            return false;
        }
        value.emplace(itemKey, itemValue);
    }
    return true;
}

bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, Object &value, const TLVHead &head)
{
    auto mapEnd = cursor_ + head.len;
    while (cursor_ < mapEnd) {
        TLVHead keyHead{};
        if (!ReadHead(buffer, keyHead)) {
            return false;
        }
        std::string itemKey = "";
        if (!ReadValue(buffer, itemKey, keyHead)) {
            return false;
        }
        TLVHead valueHead{};
        if (!ReadHead(buffer, valueHead)) {
            return false;
        }
        EntryValue itemValue;
        if (!ReadValue(buffer, itemValue, head)) {
            return false;
        }
        value.value_.emplace(itemKey, itemValue);
    }
    return true;
}

bool TLVObject::ReadValue(const std::vector<std::uint8_t> &buffer, AAFwk::Want &value, const TLVHead &head)
{
    RawMem rawMem{};
    if (!ReadValue(buffer, rawMem, head)) {
        return false;
    }
    auto ret = ParcelUtil::Raw2Parcelable<AAFwk::Want>(rawMem);
    if (!ret) {
        return false;
    }
    value = *(ret);
    return true;
}

bool TLVObject::ReadValue(
    const std::vector<std::uint8_t> &buffer, std::shared_ptr<Media::PixelMap> &value, const TLVHead &head)
{
    std::vector<std::uint8_t> u8Value;
    if (!ReadValue(buffer, u8Value, head)) {
        return false;
    }
    value = Vector2PixelMap(u8Value);
    return true;
}

bool TLVObject::Encode(std::vector<std::uint8_t> &buffer, size_t &cursor, size_t total)
{
    cursor_ = cursor;
    total_ = total;
    bool ret = Encode(buffer);
    cursor = cursor_;
    return ret;
}
bool TLVObject::Decode(const std::vector<std::uint8_t> &buffer, size_t &cursor, size_t total)
{
    cursor_ = cursor;
    total_ = total;
    bool ret = Decode(buffer);
    cursor = cursor_;
    return ret;
}

std::shared_ptr<Media::PixelMap> TLVObject::Vector2PixelMap(std::vector<std::uint8_t> &value)
{
    if (value.size() == 0) {
        return nullptr;
    }
    return std::shared_ptr<Media::PixelMap>(Media::PixelMap::DecodeTlv(value));
}

std::vector<std::uint8_t> TLVObject::PixelMap2Vector(std::shared_ptr<Media::PixelMap> pixelMap)
{
    if (pixelMap == nullptr) {
        return {};
    }
    std::vector<std::uint8_t> value;
    if (!pixelMap->EncodeTlv(value)) {
        return {};
    }
    return value;
}
} // namespace OHOS::MiscServices
