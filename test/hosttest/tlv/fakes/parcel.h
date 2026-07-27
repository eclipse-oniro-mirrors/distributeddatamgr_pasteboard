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

// HOST-TEST FAKE for c_utils parcel.h.
//
// Models only the slice of the Parcel/Parcelable contract that tlv_utils.cpp
// exercises, but models it FAITHFULLY (it buffers real bytes) so the tests are
// meaningful rather than tautological:
//   Parcelable::Marshalling(Parcel&)         - pure virtual, impl by test types
//   ParcelableType::Unmarshalling(Parcel&)   - static, impl by test types
//   Parcel(Allocator*)                       - ctor (allocator ignored)
//   Parcel::GetData() -> uintptr_t           - pointer to buffered bytes
//   Parcel::GetDataSize() -> size_t          - buffered byte count
//   Parcel::ParseFrom(uintptr_t, size_t)     - adopt an external byte buffer
//   Parcel write/read helpers                - enough for test Parcelables
//
// The real Parcel derives Parcelable from RefBase; the fake does not need that.

#ifndef PASTEBOARD_HOSTTEST_FAKE_PARCEL_H
#define PASTEBOARD_HOSTTEST_FAKE_PARCEL_H

#include <cstdint>
#include <cstdlib>
#include <vector>

#include "fault_inject.h"

namespace OHOS {

class Allocator {
public:
    virtual ~Allocator() = default;
    virtual void *Alloc(size_t size) = 0;
    virtual void Dealloc(void *data) = 0;
};

class Parcel {
public:
    explicit Parcel(Allocator *allocator = nullptr) : allocator_(allocator) {}
    ~Parcel()
    {
        if (owned_ && external_ != nullptr) {
            free(external_);
        }
    }

    // --- write side (used by test Parcelables' Marshalling) ---
    bool WriteUint32(uint32_t v)
    {
        const auto *p = reinterpret_cast<const uint8_t *>(&v);
        buffer_.insert(buffer_.end(), p, p + sizeof(v));
        return true;
    }
    bool WriteBuffer(const void *data, size_t len)
    {
        const auto *p = reinterpret_cast<const uint8_t *>(data);
        buffer_.insert(buffer_.end(), p, p + len);
        return true;
    }

    // --- read side (used by test Parcelables' Unmarshalling) ---
    uint32_t ReadUint32()
    {
        uint32_t v = 0;
        if (readCursor_ + sizeof(v) <= Size()) {
            const uint8_t *from = Data() + readCursor_;
            auto *to = reinterpret_cast<uint8_t *>(&v);
            for (size_t i = 0; i < sizeof(v); ++i) {
                to[i] = from[i];
            }
            readCursor_ += sizeof(v);
        }
        return v;
    }

    // --- contract used directly by TLVUtils ---
    uintptr_t GetData() const
    {
        return reinterpret_cast<uintptr_t>(Data());
    }
    size_t GetDataSize() const
    {
        return Size();
    }
    // Adopt an externally malloc'd buffer (Parcel takes ownership, as real one).
    bool ParseFrom(uintptr_t data, size_t size)
    {
        if (hosttest_fault::g_forceParseFromFail) {
            return false; // test-injected failure (covers the ParseFrom branch)
        }
        if (data == 0 || size == 0) {
            return false;
        }
        if (owned_ && external_ != nullptr) {
            free(external_);
        }
        external_ = reinterpret_cast<uint8_t *>(data);
        externalSize_ = size;
        owned_ = true;
        readCursor_ = 0;
        return true;
    }

private:
    const uint8_t *Data() const
    {
        return external_ != nullptr ? external_ : buffer_.data();
    }
    size_t Size() const
    {
        return external_ != nullptr ? externalSize_ : buffer_.size();
    }

    Allocator *allocator_ = nullptr;
    std::vector<uint8_t> buffer_;   // bytes written via WriteXxx
    uint8_t *external_ = nullptr;   // adopted via ParseFrom
    size_t externalSize_ = 0;
    bool owned_ = false;
    size_t readCursor_ = 0;
};

class Parcelable {
public:
    virtual ~Parcelable() = default;
    virtual bool Marshalling(Parcel &parcel) const = 0;
    // Concrete types provide: static T *Unmarshalling(Parcel &parcel);
};

} // namespace OHOS
#endif // PASTEBOARD_HOSTTEST_FAKE_PARCEL_H
