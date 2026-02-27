#include <gtest/gtest.h>
#include "clip/default_clip.h"

using namespace OHOS::Pasteboard;

class ClipPluginTest : public testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
    }
};

TEST_F(ClipPluginTest, CreateDefaultClip) {
    auto clip = std::make_shared<DefaultClip>();
    ASSERT_NE(clip, nullptr);
}

TEST_F(ClipPluginTest, SetClipData) {
    auto clip = std::make_shared<DefaultClip>();
    std::string data = "test data";
    bool result = clip->SetData(data);
    ASSERT_TRUE(result);
}

TEST_F(ClipPluginTest, GetClipData) {
    auto clip = std::make_shared<DefaultClip>();
    std::string data = "test data";
    clip->SetData(data);
    std::string result = clip->GetData();
    ASSERT_EQ(result, data);
}
