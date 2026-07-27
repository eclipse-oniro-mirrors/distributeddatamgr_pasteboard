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

// Host-only unit test for OHOS::MiscServices::TLVUtils.
//
// TLVUtils is a DEEP-dependency module: it needs Parcel/Parcelable (c_utils),
// Media::PixelMap (image_framework), securec and hilog. Rather than a device,
// this test builds against minimal *fakes* under fakes/ that faithfully model
// the slice of each contract TLVUtils touches. Demonstrates that the host-test
// pattern extends past pure-logic modules to ones behind heavy platform types.

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "parcel.h"     // fake
#include "pixel_map.h"  // fake
#include "fault_inject.h"  // fake fault-injection switches
#include "tlv_utils.h"

using namespace testing::ext;

namespace OHOS::MiscServices {
namespace {
constexpr uint32_t TEST_PAYLOAD_SERIALISE = 0xABCDEF01;
constexpr uint32_t TEST_PAYLOAD_ROUND_TRIP = 0x12345678;
constexpr uint32_t TEST_PAYLOAD_COPY = 7;
constexpr uint32_t TEST_PAYLOAD_PARSE_FAIL = 4;
// securec rejects a destMax above SECUREC_MEM_MAX_LEN (0x7fffffff), so this
// length makes the safe copy fail without any test-local secure function.
constexpr size_t OVERSIZE_BUFFER_LEN = 0x80000000UL;
// RecursiveGuard's MAX_DEPTH is 10; nest past it so the guard reports invalid.
constexpr int GUARD_NEST_COUNT = 12;
} // namespace

// A concrete Parcelable that marshals a single uint32 payload, and can rebuild
// itself from a parcel. Exercises the real Parcelable2Raw / Raw2Parcelable flow.
class FakePayload : public OHOS::Parcelable {
public:
    uint32_t value = 0;
    explicit FakePayload(uint32_t v = 0) : value(v) {}

    bool Marshalling(OHOS::Parcel &parcel) const override
    {
        return parcel.WriteUint32(value);
    }

    static FakePayload *Unmarshalling(OHOS::Parcel &parcel)
    {
        auto *p = new FakePayload();
        p->value = parcel.ReadUint32();
        return p;
    }
};

class TlvUtilsHostTest : public testing::Test {
protected:
    // Reset the fault-injection switch so cases stay independent.
    void TearDown() override
    {
        hosttest_fault::g_forceParseFromFail = false;
    }
};

/**
 * @tc.name: Parcelable2RawNullReturnsEmpty
 * @tc.desc: Parcelable2Raw on nullptr yields an empty RawMem.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Parcelable2RawNullReturnsEmpty, TestSize.Level0)
{
    RawMem rm = TLVUtils::Parcelable2Raw(nullptr);
    EXPECT_EQ(rm.buffer, 0u);
    EXPECT_EQ(rm.bufferLen, 0u);
    EXPECT_EQ(rm.parcel, nullptr);
}

/**
 * @tc.name: Parcelable2RawSerialises
 * @tc.desc: Parcelable2Raw serialises a payload into a non-empty buffer.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Parcelable2RawSerialises, TestSize.Level0)
{
    FakePayload payload(TEST_PAYLOAD_SERIALISE);
    RawMem rm = TLVUtils::Parcelable2Raw(&payload);
    EXPECT_NE(rm.buffer, 0u);
    EXPECT_EQ(rm.bufferLen, sizeof(uint32_t));
    ASSERT_NE(rm.parcel, nullptr);
}

/**
 * @tc.name: Parcelable2RawRoundTrip
 * @tc.desc: Full round-trip: Parcelable -> RawMem -> Parcelable.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Parcelable2RawRoundTrip, TestSize.Level0)
{
    FakePayload src(TEST_PAYLOAD_ROUND_TRIP);
    RawMem rm = TLVUtils::Parcelable2Raw(&src);
    ASSERT_NE(rm.buffer, 0u);

    auto rebuilt = TLVUtils::Raw2Parcelable<FakePayload>(rm);
    ASSERT_NE(rebuilt, nullptr);
    EXPECT_EQ(rebuilt->value, TEST_PAYLOAD_ROUND_TRIP);
}

/**
 * @tc.name: Raw2ParcelRejectsEmpty
 * @tc.desc: Raw2Parcel rejects an empty RawMem.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Raw2ParcelRejectsEmpty, TestSize.Level0)
{
    RawMem empty{};
    OHOS::Parcel parcel(nullptr);
    EXPECT_FALSE(TLVUtils::Raw2Parcel(empty, parcel));
}

/**
 * @tc.name: Raw2ParcelCopiesBuffer
 * @tc.desc: Raw2Parcel copies a valid buffer into the target parcel.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Raw2ParcelCopiesBuffer, TestSize.Level0)
{
    FakePayload payload(TEST_PAYLOAD_COPY);
    RawMem rm = TLVUtils::Parcelable2Raw(&payload);
    ASSERT_NE(rm.buffer, 0u);

    OHOS::Parcel out(nullptr);
    EXPECT_TRUE(TLVUtils::Raw2Parcel(rm, out));
    EXPECT_EQ(out.GetDataSize(), sizeof(uint32_t));
}

/**
 * @tc.name: Vector2PixelMapEmptyIsNull
 * @tc.desc: Vector2PixelMap: empty vector -> nullptr; non-empty -> object.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Vector2PixelMapEmptyIsNull, TestSize.Level0)
{
    std::vector<uint8_t> empty;
    EXPECT_EQ(TLVUtils::Vector2PixelMap(empty), nullptr);

    std::vector<uint8_t> bytes = {1, 2, 3};
    auto pm = TLVUtils::Vector2PixelMap(bytes);
    ASSERT_NE(pm, nullptr);
    EXPECT_EQ(pm->blob, bytes);
}

/**
 * @tc.name: PixelMap2VectorRoundTrip
 * @tc.desc: PixelMap2Vector: nullptr -> empty; normal -> round-trips bytes.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, PixelMap2VectorRoundTrip, TestSize.Level0)
{
    EXPECT_TRUE(TLVUtils::PixelMap2Vector(nullptr).empty());

    auto pm = std::make_shared<Media::PixelMap>();
    pm->blob = {9, 8, 7, 6};
    std::vector<uint8_t> out = TLVUtils::PixelMap2Vector(pm);
    EXPECT_EQ(out, pm->blob);
}

/**
 * @tc.name: PixelMap2VectorEncodeFailure
 * @tc.desc: PixelMap2Vector returns empty when EncodeTlv fails.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, PixelMap2VectorEncodeFailure, TestSize.Level0)
{
    auto pm = std::make_shared<Media::PixelMap>();
    pm->blob = {1, 2, 3};
    pm->encodeShouldFail = true;
    EXPECT_TRUE(TLVUtils::PixelMap2Vector(pm).empty());
}

/**
 * @tc.name: RecursiveGuardDepth
 * @tc.desc: RecursiveGuard stays valid up to MAX_DEPTH and invalid past it.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, RecursiveGuardDepth, TestSize.Level0)
{
    RecursiveGuard g0;
    EXPECT_TRUE(g0.IsValid());
    std::vector<std::unique_ptr<RecursiveGuard>> guards;
    bool sawInvalid = false;
    for (int i = 0; i < GUARD_NEST_COUNT; ++i) {
        guards.push_back(std::make_unique<RecursiveGuard>());
        if (!guards.back()->IsValid()) {
            sawInvalid = true;
        }
    }
    EXPECT_TRUE(sawInvalid);
}

// ---- Raw2Parcel returns false when the buffer allocation fails ----
// No fault injection: a length no allocator can satisfy reaches the branch.
/**
 * @tc.name: Raw2ParcelAllocationFailure
 * @tc.desc: Raw2Parcel returns false when buffer allocation fails, reached by a length no allocator can satisfy.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Raw2ParcelAllocationFailure, TestSize.Level0)
{
    FakePayload payload(TEST_PAYLOAD_COPY);
    RawMem rm = TLVUtils::Parcelable2Raw(&payload);
    ASSERT_NE(rm.buffer, 0u);

    rm.bufferLen = SIZE_MAX;
    OHOS::Parcel out(nullptr);
    EXPECT_FALSE(TLVUtils::Raw2Parcel(rm, out));
}

// ---- Raw2Parcel returns false when the safe copy rejects the length ----
// securec refuses a destMax above SECUREC_MEM_MAX_LEN, so the copy is rejected
// before any byte moves. If the host cannot reserve that much, the allocation
// fails first instead -- Raw2Parcel returns false either way.
/**
 * @tc.name: Raw2ParcelOversizeLengthRejected
 * @tc.desc: Raw2Parcel returns false when the safe copy rejects an oversize length (above SECUREC_MEM_MAX_LEN).
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Raw2ParcelOversizeLengthRejected, TestSize.Level0)
{
    FakePayload payload(TEST_PAYLOAD_COPY);
    RawMem rm = TLVUtils::Parcelable2Raw(&payload);
    ASSERT_NE(rm.buffer, 0u);

    rm.bufferLen = OVERSIZE_BUFFER_LEN;
    OHOS::Parcel out(nullptr);
    EXPECT_FALSE(TLVUtils::Raw2Parcel(rm, out));
}

/**
 * @tc.name: Raw2ParcelParseFromFailure
 * @tc.desc: Raw2Parcel returns false when Parcel::ParseFrom fails.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvUtilsHostTest, Raw2ParcelParseFromFailure, TestSize.Level0)
{
    FakePayload payload(TEST_PAYLOAD_PARSE_FAIL);
    RawMem rm = TLVUtils::Parcelable2Raw(&payload);
    ASSERT_NE(rm.buffer, 0u);

    hosttest_fault::g_forceParseFromFail = true;
    OHOS::Parcel out(nullptr);
    EXPECT_FALSE(TLVUtils::Raw2Parcel(rm, out));
}
} // namespace OHOS::MiscServices
