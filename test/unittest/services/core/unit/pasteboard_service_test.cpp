#include <gtest/gtest.h>
#include "services/core/include/pasteboard_service.h"

using namespace OHOS::Pasteboard;

class PasteboardServiceTest : public testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
    }
};

TEST_F(PasteboardServiceTest, CreateService) {
    auto service = std::make_shared<PasteboardService>();
    ASSERT_NE(service, nullptr);
}

TEST_F(PasteboardServiceTest, Initialize) {
    auto service = std::make_shared<PasteboardService>();
    bool result = service->Initialize();
    ASSERT_TRUE(result);
}

TEST_F(PasteboardServiceTest, GetPasteData) {
    auto service = std::make_shared<PasteboardService>();
    service->Initialize();
    auto data = service->GetPasteData();
    ASSERT_NE(data, nullptr);
}
