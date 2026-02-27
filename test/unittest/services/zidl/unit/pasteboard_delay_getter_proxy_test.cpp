#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "services/zidl/include/pasteboard_delay_getter_proxy.h"

using namespace OHOS::Pasteboard;

class MockPasteboardDelayGetterCallback : public IPasteboardDelayGetterCallback {
public:
    MOCK_METHOD(void, OnDataReady, (const std::shared_ptr<PasteData>&), (override));
};

class PasteboardDelayGetterProxyTest : public testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
    }
};

TEST_F(PasteboardDelayGetterProxyTest, CreateProxy) {
    auto proxy = std::make_shared<PasteboardDelayGetterProxy>();
    ASSERT_NE(proxy, nullptr);
}

TEST_F(PasteboardDelayGetterProxyTest, RequestData) {
    auto proxy = std::make_shared<PasteboardDelayGetterProxy>();
    MockPasteboardDelayGetterCallback callback;
    int32_t result = proxy->RequestData(callback);
    ASSERT_GE(result, 0);
}
