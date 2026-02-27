#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "services/zidl/include/ipasteboard_delay_getter.h"

using namespace OHOS::Pasteboard;

class MockPasteboardDelayGetterCallback : public IPasteboardDelayGetterCallback {
public:
    MOCK_METHOD(void, OnDataReady, (const std::shared_ptr<PasteData>&), (override));
};

TEST(PasteboardObserverCallbackMockTest, MockCallback) {
    MockPasteboardDelayGetterCallback callback;
    EXPECT_CALL(callback, OnDataReady(testing::_)).Times(1);
    
    auto data = std::make_shared<PasteData>();
    callback.OnDataReady(data);
}
