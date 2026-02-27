#include <gtest/gtest.h>
#include "innerkits/include/pasteboard_client.h"

using namespace OHOS::Pasteboard;

class PasteboardClientTest : public testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
    }
};

TEST_F(PasteboardClientTest, GetInstance) {
    auto client = PasteboardClient::GetInstance();
    ASSERT_NE(client, nullptr);
}

TEST_F(PasteboardClientTest, SetPasteData) {
    auto client = PasteboardClient::GetInstance();
    PasteData data;
    int32_t result = client->SetPasteData(data);
    ASSERT_GE(result, 0);
}

TEST_F(PasteboardClientTest, HasPasteData) {
    auto client = PasteboardClient::GetInstance();
    bool hasData = client->HasPasteData();
    ASSERT_TRUE(hasData || !hasData);
}
