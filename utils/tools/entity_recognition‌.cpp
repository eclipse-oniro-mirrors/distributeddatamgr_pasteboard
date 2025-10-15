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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

/* 常量定义区 */
#define CONFIDENCE_THRESHOLD 0.5
#define BASE_CONFIDENCE_INC 0.15
#define BASE_CONFIDENCE_INC_SEVEN_FIVE 0.75
#define BASE_CONFIDENCE_INC_NINE_FIVE 0.95
#define MAX_POS_CTRL_TWO 2
#define MAX_POS_CTRL_TREE 3
#define MAX_POS_CTRL_FOUR 4
#define MAX_POS_CTRL_FIVE 5
#define MAX_POS_CTRL_SIX 6
#define MAX_POS_CTRL_SEVEN 7
#define MAX_POS_CTRL_TEEN 10
#define MAX_POS_CTRL_ELVEN 11
#define MAX_POS_CTRL_EGIHTEEN 18
#define MAX_POS_CTRL_FIFTY 50
#define MAX_POS_CTRL_HUNDRED 100
#define MAX_POS_CTRL_TWO_HUNDRED 200
#define LOCATION_POS_ZERO_EGIHT 0.8
#define CONFIDENCE_ZEORO_ONE 0.1
#define CONFIDENCE_ZEORO_TWO 0.2
#define CONFIDENCE_ZEORO_TREE 0.3
#define CONFIDENCE_ZEORO_FOUR 0.4
#define CONFIDENCE_ZEORO_FIVE 0.5
#define CONFIDENCE_ZEORO_SIX 0.6
#define CONFIDENCE_ZEORO_SEVEN 0.7
#define CONFIDENCE_ZEORO_EGIHT 0.8
#define NAME_MAX_LEN_TWO MAX_POS_CTRL
#define NAME_MAX_LEN_TREE 3

constexpr int32_t COMMON_USERID = 0;
constexpr int32_t INIT_INTERVAL = 10000L;
constexpr uint32_t MAX_IPC_THREAD_NUM = 32;
constexpr const char *PASTEBOARD_SERVICE_SA_NAME = "pasteboard_service";
constexpr const char *PASTEBOARD_SERVICE_NAME = "PasteboardService";
constexpr const char *NLU_SO_PATH = "libai_nlu_innerapi.z.so";
constexpr const char *GET_PASTE_DATA_PROCESSOR = "GetPasteDataProcessor";
constexpr const char *FAIL_TO_GET_TIME_STAMP = "FAIL_TO_GET_TIME_STAMP";
constexpr const char *SECURE_PASTE_PERMISSION = "ohos.permission.SECURE_PASTE";
constexpr const char *READ_PASTEBOARD_PERMISSION = "ohos.permission.READ_PASTEBOARD";
constexpr const char *TRANSMIT_CONTROL_PROP_KEY = "persist.distributed_scene.datafiles_trans_ctrl";
constexpr const char *MANAGE_PASTEBOARD_APP_SHARE_OPTION_PERMISSION =
    "ohos.permission.MANAGE_PASTEBOARD_APP_SHARE_OPTION";
constexpr const char *GET_DATA_APP = "GET_DATA_APP";
constexpr const char *NETWORK_DEV_NUM = "NETWORK_DEV_NUM";
constexpr const char *COVER_DELAY_DATA = "COVER_DELAY_DATA";
constexpr const char *UE_COPY = "DISTRIBUTED_PASTEBOARD_COPY";
constexpr const char *UE_PASTE = "DISTRIBUTED_PASTEBOARD_PASTE";

constexpr int32_t INVALID_VERSION = -1;
constexpr int32_t ADD_PERMISSION_CHECK_SDK_VERSION = 12;
constexpr int32_t CTRLV_EVENT_SIZE = 2;
constexpr int32_t CONTROL_TYPE_ALLOW_SEND_RECEIVE = 1;
constexpr uint32_t EVENT_TIME_OUT = 2000;
constexpr uint32_t MAX_RECOGNITION_LENGTH = 1000;
constexpr int32_t DEVICE_COLLABORATION_UID = 5521;
constexpr uint64_t SYSTEM_APP_MASK = (static_cast<uint64_t>(1) << 32);
constexpr uint32_t MAX_BUNDLE_NAME_LENGTH = 127;
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
constexpr int32_t E_OK_OPERATION = 0;
constexpr int32_t SET_VALUE_SUCCESS = 1;
constexpr uid_t ANCO_SERVICE_BROKER_UID = 5557;
constexpr float RECALCULATE_DATA_SIZE = 0.9;

struct AppInfo {
    std::string bundleName = "com.pasteboard.default";
    int32_t tokenType = -1;
    int32_t userId = ERROR_USERID;
    uint32_t tokenId;
    int32_t appIndex = 0;
};

struct HistoryInfo {
    std::string time;
    std::string bundleName;
    std::string state;
    std::string remote;
    int32_t userId = ERROR_USERID;
};

struct PasteDateTime {
    int32_t syncTime = 0;
    int32_t errorCode = 0;
    std::shared_ptr<PasteData> data;
};

struct PasteDateResult {
    int32_t syncTime = 0;
    int32_t errorCode = 0;
};

struct PasteP2pEstablishInfo {
    std::string networkId;
    std::shared_ptr<BlockObject<int32_t>> pasteBlock;
};

struct FocusedAppInfo {
    int32_t windowId = 0;
    sptr<IRemoteObject> abilityToken = nullptr;
};

// ============================ 鸿蒙剪贴板接口 ============================
namespace OHOS {
namespace MiscServices {
// 剪贴板错误码
enum class ErrorCode {
    SUCCESS = 0,
    ERROR_CLIPBOARD_SERVICE_EXCEPTION = 202,
    ERROR_PARAM_CHECK_FAILED = 401,
    ERROR_PERMISSION_DENIED = 201,
    ERROR_SYSTEM_API_EXCEED = 10000001
};

// ============================ 实体识别引擎 ============================

// 实体类型定义
enum class EntityType {
    PERSON,        // 人名
    LOCATION,      // 地点
    ORGANIZATION,  // 组织机构
    PHONE,         // 电话号码
    EMAIL,         // 邮箱地址
    URL,           // 网址
    DATE,          // 日期
    TIME,          // 时间
    MONEY,         // 金额
    ID_CARD,       // 身份证号
    BANK_CARD,     // 银行卡号
    LICENSE_PLATE, // 车牌号
    POSTAL_CODE,   // 邮政编码
    IP_ADDRESS,    // IP地址
    HASHTAG,       // 话题标签
    MENTION,       // @提及
    PRODUCT,       // 产品名称
    EVENT,         // 事件名称
    UNKNOWN        // 未知类型
};

// 剪贴板数据类型
enum class ContentType { TEXT = 0, URI = 1, PIXELMAP = 2 };

// 剪贴板数据类
class PasteData {
private:
    std::string textContent_;
    ContentType type_;
    std::string mimeType_;
    time_t timestamp_;

public:
    PasteData() : type_(ContentType::TEXT), timestamp_(std::time(nullptr)) { }

    explicit PasteData(const std::&text) : textContent_(text), type_(ContentType::TEXT), timestamp_(std::time(nullptr))
    {
        mimeType_ = "text/plain";
    }

    // API：获取文本内容
    std::string GetText() const
    {
        return textContent_;
    }

    // API：设置文本内容
    void SetText(const std::string &text)
    {
        textContent_ = text;
        type_ = ContentType::TEXT;
        mimeType_ = "text/plain";
        timestamp_ = std::time(nullptr);
    }

    // API：获取数据类型
    ContentType GetContentType() const
    {
        return type_;
    }

    // API：获取MIME类型
    std::string GetMimeType() const
    {
        return mimeType_;
    }

    // API：获取时间戳
    time_t GetTimestamp() const
    {
        return timestamp_;
    }

    // API：是否有文本内容
    bool HasText() const
    {
        return !textContent_.empty();
    }

    // 转换为字符串表示
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "PasteData{"
           << "type=" << static_cast<int>(type_) << ", text='" << textContent_ << "'"
           << ", timestamp=" << timestamp_ << "}";
        return ss.str();
    }
};

// 剪贴板服务类
class ClipboardService {
private:
    static ClipboardService *instance_;
    std::mutex mutex_;
    PasteData currentData_;
    std::vector<std::function<void(const PasteData &)>> subscribers_;
    std::atomic<bool> isServiceAvailable_;

    ClipboardService() : isServiceAvailable_(true)
    {
        std::cout << "[Harmony Clipboard] 剪贴板服务初始化" << std::endl;
    }

public:
    // API：获取剪贴板服务实例
    static ClipboardService &GetInstance()
    {
        static ClipboardService instance;
        return instance;
    }

    // API：获取剪贴板数据
    ErrorCode GetPasteData(PasteData &data)
    {
        if (!isServiceAvailable_) {
            std::cout << "[Harmony Clipboard] 错误：剪贴板服务不可用" << std::endl;
            return ErrorCode::ERROR_CLIPBOARD_SERVICE_EXCEPTION;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        data = currentData_;
        std::cout << "[Harmony Clipboard] 获取剪贴板数据: " << data.ToString() << std::endl;
        return ErrorCode::SUCCESS;
    }

    // API：设置剪贴板数据
    ErrorCode SetPasteData(const PasteData &data)
    {
        if (!isServiceAvailable_) {
            return ErrorCode::ERROR_CLIPBOARD_SERVICE_EXCEPTION;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            currentData_ = data;
        }

        std::cout << "[Harmony Clipboard] 设置剪贴板数据: " << data.ToString() << std::endl;

        // 通知订阅者
        NotifySubscribers(data);
        return ErrorCode::SUCCESS;
    }

    // API：清空剪贴板
    ErrorCode Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        currentData_ = PasteData();
        std::cout << "[Harmony Clipboard] 清空剪贴板" << std::endl;
        return ErrorCode::SUCCESS;
    }

    // API：订阅剪贴板变化
    ErrorCode Subscribe(const std::function<void(const PasteData &)> &callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_.push_back(callback);
        std::cout << "[Harmony Clipboard] 注册剪贴板变化监听器" << std::endl;
        return ErrorCode::SUCCESS;
    }

    // API：取消订阅
    ErrorCode Unsubscribe(const std::function<void(const PasteData &)> &callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        subscribers_.erase(it, subscribers_.end());
        std::cout << "[Harmony Clipboard] 取消剪贴板变化监听器" << std::endl;
        return ErrorCode::SUCCESS;
    }

    // 服务可用性检查
    bool IsServiceAvailable() const
    {
        return isServiceAvailable_;
    }

    // 服务故障（用于测试）
    void SetServiceAvailable(bool available)
    {
        isServiceAvailable_ = available;
        std::cout << "[Harmony Clipboard] 服务可用性设置为: " << available << std::endl;
    }

private:
    void NotifySubscribers(const PasteData &data)
    {
        std::vector<std::function<void(const PasteData &)>> currentSubscribers;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            currentSubscribers = subscribers_;
        }

        for (const auto &subscriber : currentSubscribers) {
            try {
                subscriber(data);
            } catch (const std::exception &e) {
                std::cerr << "[Harmony Clipboard] 订阅者回调异常: " << e.what() << std::endl;
            }
        }
    }
};

// 实体识别结果
struct Entity {
    std::string text;                              // 实体文本
    EntityType type;                               // 实体类型
    int startPos;                                  // 起始位置
    int endPos;                                    // 结束位置
    double confidence;                             // 置信度
    std::string normalized;                        // 标准化形式
    std::map<std::string, std::string> attributes; // 附加属性

    void AddAttribute(const std::string &key, const std::string &value)
    {
        attributes[key] = value;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "Entity{text='" << text << "', type=" << static_cast<int>(type) << ", pos=[" << startPos << "-" << endPos
           << "]"
           << ", confidence=" << std::fixed << std::setprecision(MAX_POS_CTRL_TWO) << confidence << ", normalized='"
           << normalized << "'}";
        return ss.str();
    }
};

// 实体识别配置
struct RecognitionConfig {
    bool enableRegexMatching = true;
    bool enableDictionaryMatching = true;
    bool enableRuleBasedMatching = true;
    bool enableNerModel = false; // NLP模型
    double confidenceThreshold = CONFIDENCE_ZEORO_FIVE;
    int maxTextLength = 10000;
    std::vector<EntityType> enabledTypes;

    void EnableAllTypes()
    {
        enabledTypes = { EntityType::PERSON, EntityType::LOCATION, EntityType::ORGANIZATION, EntityType::PHONE,
            EntityType::EMAIL, EntityType::URL, EntityType::DATE, EntityType::TIME, EntityType::MONEY,
            EntityType::ID_CARD, EntityType::BANK_CARD, EntityType::LICENSE_PLATE, EntityType::POSTAL_CODE,
            EntityType::IP_ADDRESS, EntityType::HASHTAG, EntityType::MENTION, EntityType::PRODUCT, EntityType::EVENT };
    }
};

// 高级实体识别引擎
class AdvancedEntityRecognizer {
private:
    RecognitionConfig config_;
    std::map<EntityType, std::vector<std::regex>> regexPatterns_;
    std::map<EntityType, std::unordered_set<std::string>> dictionaries_;
    std::map<std::string, EntityType> keywordMap_;

    // 中文分词和命名实体识别相关数据
    std::unordered_set<std::string> chineseSurnames_;
    std::unordered_set<std::string> locationSuffixes_;
    std::unordered_set<std::string> organizationSuffixes_;

public:
    AdvancedEntityRecognizer()
    {
        InitializeRegexPatterns();
        InitializeDictionaries();
        InitializeChineseNerData();
        config_.EnableAllTypes();
    }

    // 设置识别配置
    void SetConfig(const RecognitionConfig &config)
    {
        config_ = config;
        std::cout << "[Entity Recognizer] 配置已更新" << std::endl;
    }

    // 主识别函数
    std::vector<Entity> Recognize(const std::string &text)
    {
        if (text.empty() || text.length() > config_.maxTextLength) {
            std::cout << "[Entity Recognizer] 文本长度无效: " << text.length() << std::endl;
            return {};
        }

        std::vector<Entity> entities;
        std::cout << "\n=== 开始实体识别 ===" << std::endl;
        std::cout << "输入文本: "
                  << (text.length() > MAX_POS_CTRL_HUNDRED ? text.substr(0, MAX_POS_CTRL_HUNDRED) + "..." : text)
                  << std::endl;

        // 多阶段识别
        if (config_.enableRegexMatching) {
            RecognizeWithRegex(text, entities);
        }

        if (config_.enableDictionaryMatching) {
            RecognizeWithDictionary(text, entities);
        }

        if (config_.enableRuleBasedMatching) {
            RecognizeWithRules(text, entities);
        }

        // 后处理：去重、置信度过滤、位置调整
        PostProcessEntities(entities, text);

        std::cout << "识别完成，找到 " << entities.size() << " 个实体" << std::endl;
        return entities;
    }

    // 批量识别
    std::map<std::string, std::vector<Entity>> RecognizeBatch(const std::vector<std::string> &texts)
    {
        std::map<std::string, std::vector<Entity>> results;
        for (const auto &text : texts) {
            results[text] = Recognize(text);
        }
        return results;
    }

    // 添加自定义词典
    void AddDictionaryEntries(EntityType type, const std::vector<std::string> &entries)
    {
        for (const auto &entry : entries) {
            dictionaries_[type].insert(entry);
            keywordMap_[entry] = type;
        }
        std::cout << "[Entity Recognizer] 为类型" << static_cast<int>(type) << "添加了 " << entries.size()
                  << " 个词典条目" << std::endl;
    }

private:
    void InitializeRegexPatterns()
    {
        // 电话号码
        regexPatterns_[EntityType::PHONE] = { std::regex(R"(1[3-9]\d{9})"), // 手机号
            std::regex(R"(\d{3,4}-\d{7,8})"),                               // 固话
            std::regex(R"(\(\d{3,4}\)\d{7,8})") };

        // 邮箱
        regexPatterns_[EntityType::EMAIL] = { std::regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})") };

        // URL
        regexPatterns_[EntityType::URL] = { std::regex(R"(https?://[^\s]+)"),
            std::regex(R"(www\.[^\s]+\.[a-zA-Z]{2,})"), std::regex(R"([a-zA-Z0-9-]+\.[a-zA-Z]{2,}(/\S*)?)") };

        // 日期
        regexPatterns_[EntityType::DATE] = { std::regex(R"(\d{4}年\d{1,2}月\d{1,2}日)"),
            std::regex(R"(\d{4}-\d{1,2}-\d{1,2})"), std::regex(R"(\d{1,2}月\d{1,2}日)"),
            std::regex(R"(\d{4}/\d{1,2}/\d{1,2})") };

        // 时间
        regexPatterns_[EntityType::TIME] = { std::regex(R"(\d{1,2}:\d{2}(:\d{2})?)"),
            std::regex(R"((上午|下午)?\d{1,2}点\d{1,2}分?(\d{1,2}秒)?)") };

        // 金额
        regexPatterns_[EntityType::MONEY] = { std::regex(
            R"((¥|￥|\$)?\s*(\d{1,3}(,\d{3})*(\.\d{1,2})?|\d+(\.\d{1,2})?)\s*(元|美元|人民币)?)") };

        // 身份证号
        regexPatterns_[EntityType::ID_CARD] = { std::regex(
            R"([1-9]\d{5}(18|19|20)\d{2}((0[1-9])|(1[0-2]))(([0-2][1-9])|10|20|30|31)\d{3}[0-9Xx])") };

        // 银行卡号
        regexPatterns_[EntityType::BANK_CARD] = { std::regex(R"(\d{16,19})") };

        // 车牌号
        regexPatterns_[EntityType::LICENSE_PLATE] = { std::regex(
            R"([京津沪渝冀豫云辽黑湘皖鲁新苏浙赣鄂桂甘晋蒙陕吉闽贵粤青藏川宁琼][A-Z][A-Z0-9]{5})") };

        // 邮政编码
        regexPatterns_[EntityType::POSTAL_CODE] = { std::regex(R"(\d{6})") };

        // IP地址
        regexPatterns_[EntityType::IP_ADDRESS] = { std::regex(R"(\b(?:[0-9]{1,3}\.){3}[0-9]{1,3}\b)") };

        // 话题标签
        regexPatterns_[EntityType::HASHTAG] = { std::regex(R"(#[\u4e00-\u9fa5a-zA-Z0-9_]+)") };

        // @提及
        regexPatterns_[EntityType::MENTION] = { std::regex(R"(@[\u4e00-\u9fa5a-zA-Z0-9_]+)") };
    }

    void InitializeDictionaries()
    {
        // 初始化一些基础词典
        InitializePersonDictionary();
        InitializeLocationDictionary();
        InitializeOrganizationDictionary();
        InitializeProductDictionary();
        InitializeEventDictionary();
    }

    void InitializePersonDictionary()
    {
        std::vector<std::string> persons = { "张三", "李四", "王五", "赵六", "孙悟空", "猪八戒", "唐僧", "诸葛亮",
            "刘备", "关羽", "张飞", "曹操", "孙权" };
        AddDictionaryEntries(EntityType::PERSON, persons);
    }

    void InitializeLocationDictionary()
    {
        std::vector<std::string> locations = { "北京", "上海", "广州", "深圳", "杭州", "南京", "武汉", "成都", "重庆",
            "西安", "天安门", "故宫", "长城", "西湖" };
        AddDictionaryEntries(EntityType::LOCATION, locations);
    }

    void InitializeOrganizationDictionary()
    {
        std::vector<std::string> organizations = { "XX技术有限公司", "XX科技", "XX大学", "北京大学", "人民医院",
            "XX银行" };
        AddDictionaryEntries(EntityType::ORGANIZATION, organizations);
    }

    void InitializeProductDictionary()
    {
        std::vector<std::string> products = { "phoneA", "phoneB", "phoneC" };
        AddDictionaryEntries(EntityType::PRODUCT, products);
    }

    void InitializeEventDictionary()
    {
        std::vector<std::string> events = { "春节", "中秋节", "国庆节", "双十一", "奥运会", "世界杯", "期末考试",
            "公司年会" };
        AddDictionaryEntries(EntityType::EVENT, events);
    }

    void InitializeChineseNerData()
    {
        // 中文姓氏
        chineseSurnames_ = { "李", "王", "张", "刘", "陈", "杨", "赵", "黄", "周", "吴", "徐", "孙", "胡", "朱", "高",
            "林", "何", "郭", "马", "罗" };

        // 地点后缀
        locationSuffixes_ = { "省", "市", "区", "县", "镇", "乡", "村", "路", "街", "巷", "广场", "大厦", "小区",
            "公园", "机场", "车站" };

        // 组织机构后缀
        organizationSuffixes_ = { "公司", "集团", "医院", "学校", "大学", "学院", "银行", "酒店", "商场", "工厂",
            "机构", "中心", "局", "所" };
    }

    void RecognizeWithRegex(const std::string &text, std::vector<Entity> &entities)
    {
        for (const auto &type : config_.enabledTypes) {
            if (regexPatterns_.find(type) == regexPatterns_.end()) {
                continue;
            }

            for (const auto &pattern : regexPatterns_[type]) {
                std::sregex_iterator it(text.begin(), text.end(), pattern);
                std::sregex_iterator end;

                while (it != end) {
                    std::smatch match = *it;
                    std::string matchStr = match.str();
                    int startPos = match.position();
                    int endPos = startPos + matchStr.length();
                    ++it;
                }
            }
        }
    }

    void RecognizeWithDictionary(const std::string &text, std::vector<Entity> &entities)
    {
        for (const auto &dictEntry : dictionaries_) {
            EntityType type = dictEntry.first;
            if (!IsTypeEnabled(type)) {
                continue;
            }

            for (const auto &word : dictEntry.second) {
                size_t pos = 0;
                while ((pos = text.find(word, pos)) != std::string::npos) {
                    entities.emplace_back(word, type, pos, pos + word.length(), BASE_CONFIDENCE_INC_NINE_FIVE);
                    pos += word.length();
                }
            }
        }
    }

    void RecognizeWithRules(const std::string &text, std::vector<Entity> &entities)
    {
        RecognizeChineseNames(text, entities);
        RecognizeChineseLocations(text, entities);
        RecognizeChineseOrganizations(text, entities);
        RecognizeContextualEntities(text, entities);
    }

    void RecognizeChineseNames(const std::string &text, std::vector<Entity> &entities)
    {
        if (!IsTypeEnabled(EntityType::PERSON)) {
            return;
        }

        for (const auto &surname : chineseSurnames_) {
            size_t pos = 0;
            while ((pos = text.find(surname, pos)) == std::string::npos) {
                // 检查可能的姓名长度（2-3个字符）
                pos += surname.length();
                continue;
            }
            for (int length = MAX_POS_CTRL_TWO; length <= MAX_POS_CTRL_TREE; length++) {
                if (pos + length >= text.length()) {
                    continue;
                }
                std::string potentialName = text.substr(pos, length);
                if (!IsLikelyChineseName(potentialName)) {
                    continue;
                }
                double confidence = CalculateNameConfidence(potentialName);
                if (confidence >= config_.confidenceThreshold) {
                    entities.emplace_back(potentialName, EntityType::PERSON, pos, pos + length, confidence);
                }
            }
        }
    }

    void RecognizeChineseLocations(const std::string &text, std::vector<Entity> &entities)
    {
        if (!IsTypeEnabled(EntityType::LOCATION)) {
            return;
        }

        // 基于后缀识别地点
        for (const auto &suffix : locationSuffixes_) {
            size_t pos = 0;
            while ((pos = text.find(suffix, pos)) != std::string::npos) {
                if (pos >= MAX_POS_CTRL_TWO) { // 至少有两个字符的前缀
                    int start = std::max(0, static_cast<int>(pos) - MAX_POS_CTRL_FOUR); // 最多4个字符的前缀
                    std::string potentialLocation = text.substr(start, pos - start + suffix.length());
                    entities.emplace_back(
                        potentialLocation, EntityType::LOCATION, start, pos + suffix.length(), LOCATION_POS_ZERO_EGIHT);
                }
                pos += suffix.length();
            }
        }
    }

    void RecognizeChineseOrganizations(const std::string &text, std::vector<Entity> &entities)
    {
        if (!IsTypeEnabled(EntityType::ORGANIZATION)) {
            return;
        }

        for (const auto &suffix : organizationSuffixes_) {
            size_t pos = 0;
            while ((pos = text.find(suffix, pos)) != std::string::npos) {
                if (pos >= MAX_POS_CTRL_TWO) {
                    int start = std::max(0, static_cast<int>(pos) - MAX_POS_CTRL_SIX); // 最多6个字符的前缀
                    std::string potentialOrg = text.substr(start, pos - start + suffix.length());
                    entities.emplace_back(potentialOrg, EntityType::ORGANIZATION, start, pos + suffix.length(),
                        BASE_CONFIDENCE_INC_SEVEN_FIVE);
                }
                pos += suffix.length();
            }
        }
    }

    void RecognizeContextualEntities(const std::string &text, std::vector<Entity> &entities)
    {
        // 基于上下文关键词识别
        std::map<std::string, EntityType> contextualKeywords = { { "电话", EntityType::PHONE },
            { "手机", EntityType::PHONE }, { "邮箱", EntityType::EMAIL }, { "邮件", EntityType::EMAIL },
            { "网址", EntityType::URL }, { "网站", EntityType::URL }, { "日期", EntityType::DATE },
            { "时间", EntityType::TIME }, { "金额", EntityType::MONEY }, { "价格", EntityType::MONEY } };

        for (const auto &keyword : contextualKeywords) {
            size_t pos = 0;
            while ((pos = text.find(keyword.first, pos)) != std::string::npos) {
                // 在关键词后面寻找可能的实体
                size_t entityStart = pos + keyword.first.length();
                if (entityStart < text.length()) {
                    // 简化的上下文识别逻辑
                    entities.emplace_back(
                        keyword.first, keyword.second, pos, pos + keyword.first.length(), CONFIDENCE_ZEORO_SEVEN);
                }
                pos += keyword.first.length();
            }
        }
    }

    void PostProcessEntities(std::vector<Entity> &entities, const std::string &text)
    {
        // 去重（基于位置）
        RemoveDuplicateEntities(entities);

        // 过滤低置信度实体
        FilterLowConfidenceEntities(entities);

        // 调整边界
        AdjustEntityBoundaries(entities, text);

        // 标准化实体文本
        NormalizeEntities(entities);
    }

    void RemoveDuplicateEntities(std::vector<Entity> &entities)
    {
        return;
    }

    void FilterLowConfidenceEntities(std::vector<Entity> &entities)
    {
        entities.erase(std::remove_if(entities.begin(), entities.end(),
                                    this](const Entity &entity) {
            return entity.confidence < config_.confidenceThreshold;
                                    }),
                    entities.end());
    }

    void AdjustEntityBoundaries(std::vector<Entity> &entities, const std::string &text)
    {
        for (auto &entity : entities) {
            // 去除实体边界的空白字符
            while (entity.startPos > 0 && std::isspace(text[entity.startPos - 1])) {
                entity.startPos--;
            }
            while (entity.endPos < text.length() && std::isspace(text[entity.endPos])) {
                entity.endPos++;
            }
            entity.text = text.substr(entity.startPos, entity.endPos - entity.startPos);
        }
    }

    void NormalizeEntities(std::vector<Entity> &entities)
    {
        for (auto &entity : entities) {
            switch (entity.type) {
                case EntityType::PHONE:
                    entity.normalized = NormalizePhoneNumber(entity.text);
                    break;
                case EntityType::DATE:
                    entity.normalized = NormalizeDate(entity.text);
                    break;
                case EntityType::MONEY:
                    entity.normalized = NormalizeMoney(entity.text);
                    break;
                default:
                    entity.normalized = entity.text;
            }
        }
    }

    std::string NormalizePhoneNumber(const std::string &phone)
    {
        std::string normalized = phone;
        // 去除分隔符
        std::erase_if(normalized, [](char c) {
            return c == '-' || c == '(' || c == ')' || c == ' ';
        });
        return normalized;
    }

    std::string NormalizeDate(const std::string &date)
    {
        // 简化的日期标准化
        std::string normalized = date;
        std::replace(normalized.begin(), normalized.end(), '年', '-');
        std::replace(normalized.begin(), normalized.end(), '月', '-');
        std::replace(normalized.begin(), normalized.end(), '日', ' ');
        return normalized;
    }

    std::string NormalizeMoney(const std::string &money)
    {
        std::string normalized = money;
        // 去除货币符号和空格
        std::erase_if(normalized, [](char c) {
            return c == '¥' || c == '￥' || c == '$' || c == ' ' || c == ',';
        });
        return normalized + "元"; // 统一后缀
    }

    bool IsLikelyChineseName(const std::string &name)
    {
        if (name.length() < NAME_MAX_LEN_TWO || name.length() > NAME_MAX_LEN_TREE) {
            return false;
        }

        // 检查是否包含常见姓氏
        std::string firstChar = name.substr(0, 1);
        return chineseSurnames_.find(firstChar) != chineseSurnames_.end();
    }

    double CalculateNameConfidence(const std::string &name)
    {
        double confidence = CONFIDENCE_ZEORO_FIVE;
        if (name.length() == NAME_MAX_LEN) {
            confidence += CONFIDENCE_ZEORO_TWO;
        }
        if (name.length() == NAME_MAX_LEN_TREE) {
            confidence += CONFIDENCE_ZEORO_TREE;
        }

        // 检查是否在词典中
        if (dictionaries_[EntityType::PERSON].find(name) != dictionaries_[EntityType::PERSON].end()) {
            confidence += CONFIDENCE_ZEORO_TREE;
        }

        return std::min(confidence, 1.0);
    }

    double CalculateRegexConfidence(const std::string &match, EntityType type)
    {
        double baseConfidence = ;

        // 根据匹配质量和类型调整置信度
        switch (type) {
            case EntityType::PHONE:
                if (match.length() == MAX_POS_CTRL_ELVEN) {
                    baseConfidence += CONFIDENCE_ZEORO_ONE; // 手机号
                }
                break;
            case EntityType::EMAIL:
                if (match.find('.') != std::string::npos) {
                    baseConfidence += CONFIDENCE_ZEORO_ONE;
                }
                break;
            case EntityType::ID_CARD:
                if (match.length() == MAX_POS_CTRL_EGIHTEEN) {
                    baseConfidence += BASE_CONFIDENCE_INC;
                }
                break;
            default:
                break;
        }

        return std::min(baseConfidence, 1.0);
    }

    bool IsTypeEnabled(EntityType type)
    {
        return std::find(config_.enabledTypes.begin(), config_.enabledTypes.end(), type) != config_.enabledTypes.end();
    }
};

} // namespace MiscServices
} // namespace OHOS

// ============================ 实体识别管理器 ============================

class ClipboardEntityRecognitionManager {
private:
    OHOS::MiscServices::ClipboardService &clipboardService_;
    OHOS::MiscServices::AdvancedEntityRecognizer recognizer_;
    std::atomic<bool> isMonitoring_;
    std::thread monitoringThread_;
    std::mutex callbackMutex_;
    std::vector<std::function<void(const std::vector<OHOS::MiscServices::Entity> &)>> callbacks_;

public:
    // 开始监控剪贴板
    void StartMonitoring()
    {
        if (isMonitoring_) {
            std::cout << "[Clipboard NER] 已经在监控中" << std::endl;
            return;
        }

        isMonitoring_ = true;
        std::cout << "[Clipboard NER] 启动剪贴板实体识别监控" << std::endl;

        // 订阅剪贴板变化
        clipboardService_.Subscribe([this](const OHOS::MiscServices::PasteData &data) {
            this->OnClipboardChanged(data);
        });

        // 启动监控线程
        monitoringThread_ = std::thread([this]() {
            this->MonitoringLoop();
        });
    }

    // 停止监控
    void StopMonitoring()
    {
        if (!isMonitoring_) {
            return;
        }

        isMonitoring_ = false;
        if (monitoringThread_.joinable()) {
            monitoringThread_.join();
        }
        std::cout << "[Clipboard NER] 停止剪贴板监控" << std::endl;
    }

    // 注册实体识别回调
    void RegisterEntityCallback(const std::function<void(const std::vector<OHOS::MiscServices::Entity> &)> &callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_.push_back(callback);
        std::cout << "[Clipboard NER] 注册实体识别回调" << std::endl;
    }

    // 手动识别当前剪贴板内容
    std::vector<OHOS::MiscServices::Entity> RecognizeCurrentClipboard()
    {
        OHOS::MiscServices::PasteData data;
        auto errorCode = clipboardService_.GetPasteData(data);
        if (errorCode != OHOS::MiscServices::ErrorCode::SUCCESS) {
            std::cout << "[Clipboard NER] 获取剪贴板数据失败: " << static_cast<int>(errorCode) << std::endl;
            return {};
        }

        if (!data.HasText()) {
            std::cout << "[Clipboard NER] 剪贴板无文本内容" << std::endl;
            return {};
        }

        return recognizer_.Recognize(data.GetText());
    }

    // 设置识别配置
    void SetRecognitionConfig(const OHOS::MiscServices::RecognitionConfig &config)
    {
        recognizer_.SetConfig(config);
    }

private:
    void OnClipboardChanged(const OHOS::MiscServices::PasteData &data)
    {
        if (!data.HasText()) {
            return;
        }

        std::cout << "\n*** 剪贴板内容变化检测 ***" << std::endl;
        std::string content = data.GetText();
        std::cout << "新内容: "
                  << (content.length() > MAX_POS_CTRL_FIFTY ? content.substr(0, MAX_POS_CTRL_FIFTY) + "..." : content)
                  << std::endl;

        // 进行实体识别
        auto entities = recognizer_.Recognize(content);

        // 通知回调
        NotifyCallbacks(entities);

        // 显示识别结果
        DisplayRecognitionResult(entities, content);
    }

    void MonitoringLoop()
    {
        while (isMonitoring_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // 这里可以添加定期检查或其他监控逻辑
            if (!clipboardService_.IsServiceAvailable()) {
                std::cout << "[Clipboard NER] 警告：剪贴板服务不可用" << std::endl;
            }
        }
    }

    void NotifyCallbacks(const std::vector<OHOS::MiscServices::Entity> &entities)
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        for (const auto &callback : callbacks_) {
            try {
                callback(entities);
            } catch (const std::exception &e) {
                std::cerr << "[Clipboard NER] 回调执行异常: " << e.what() << std::endl;
            }
        }
    }

    void DisplayRecognitionResult(
        const std::vector<OHOS::MiscServices::Entity> &entities, const std::string &originalText)
    {
        if (entities.empty()) {
            std::cout << "未识别到任何实体" << std::endl;
            return;
        }

        std::cout << "\n=== 实体识别结果 ===" << std::endl;
        std::cout << "原始文本: " << originalText << std::endl;
        std::cout << "识别到 " << entities.size() << " 个实体:" << std::endl;

        for (const auto &entity : entities) {
            std::cout << "  " << entity.ToString() << std::endl;
        }

        // 高亮显示文本中的实体
        HighlightEntitiesInText(entities, originalText);
    }

    void HighlightEntitiesInText(const std::vector<OHOS::MiscServices::Entity> &entities, const std::string &text)
    {
        if (text.length() > MAX_POS_CTRL_TWO_HUNDRED) {
            std::cout << "文本过长，不进行高亮显示" << std::endl;
            return;
        }

        std::string highlighted = text;

        // 从后往前替换，避免位置偏移
        std::vector<OHOS::MiscServices::Entity> sortedEntities = entities;
        std::sort(sortedEntities.begin(), sortedEntities.end(),
            [](const OHOS::MiscServices::Entity &a, const OHOS::MiscServices::Entity &b) {
                return a.startPos > b.startPos;
            });

        for (const auto &entity : sortedEntities) {
            if (entity.startPos < highlighted.length() && entity.endPos <= highlighted.length()) {
                std::string entityText = highlighted.substr(entity.startPos, entity.endPos - entity.startPos);
                std::string highlightedEntity = "[" + entityText + "]";
                highlighted.replace(entity.startPos, entity.endPos - entity.startPos, highlightedEntity);
            }
        }

        std::cout << "高亮文本: " << highlighted << std::endl;
    }
};

// ============================ 演示和测试代码 ============================

void DemonstrateClipboardEntityRecognition()
{
    using namespace OHOS::MiscServices;

    std::cout << "=== 鸿蒙剪贴板实体识别系统演示 ===" << std::endl;

    // 创建管理器
    ClipboardEntityRecognitionManager manager;

    // 设置识别配置
    RecognitionConfig config;
    config.confidenceThreshold = CONFIDENCE_ZEORO_SIX;
    config.EnableAllTypes();
    manager.SetRecognitionConfig(config);

    // 注册回调
    manager.RegisterEntityCallback([](const std::vector<Entity> &entities) {
        std::cout << "\n[回调] 检测到 " << entities.size() << " 个实体" << std::endl;
        for (const auto &entity : entities) {
            std::cout << "  - " << entity.text << " (" << static_cast<int>(entity.type)
                      << ") 置信度: " << entity.confidence << std::endl;
        }
    });

    // 测试数据
    std::vector<std::string> testTexts = { "此处调用剪贴板GetData()接口获取数据" };

    // 单次识别演示
    std::cout << "\n1. 单次识别演示:" << std::endl;
    for (const auto &text : testTexts) {
        PasteData data(text);
        ClipboardService::GetInstance().SetPasteData(data);

        auto entities = manager.RecognizeCurrentClipboard();
        std::cout << "文本: " << text << std::endl;
        std::cout << "识别到 " << entities.size() << " 个实体" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 监控模式演示
    std::cout << "\n2. 监控模式演示（10秒）:" << std::endl;
    manager.StartMonitoring();

    // 剪贴板变化
    std::thread([&manager]() {
        std::vector<std::string> dynamicTexts = {
            "紧急联系孙悟空，事件#西游记取经",
            "医院地址：人民医院，时间明天上午9点，费用300元",
        };

        for (const auto &text : dynamicTexts) {
            std::this_thread::sleep_for(std::chrono::seconds(MAX_POS_CTRL_TREE));
            PasteData data(text);
            ClipboardService::GetInstance().SetPasteData(data);
        }
    }).detach();

    std::this_thread::sleep_for(std::chrono::seconds(MAX_POS_CTRL_TEEN));
    manager.StopMonitoring();

    std::cout << "\n演示完成" << std::endl;
}

// ============================ 性能测试和统计 ============================

class PerformanceAnalyzer {
private:
    std::map<std::string, std::chrono::microseconds> timings_;
    std::map<OHOS::MiscServices::EntityType, int> entityCounts_;
    int totalRecognitions_;

public:
    PerformanceAnalyzer() : totalRecognitions_(0) { }

    void RecordRecognition(
        const std::vector<OHOS::MiscServices::Entity> &entities, const std::chrono::microseconds &duration)
    {
        totalRecognitions_++;

        for (const auto &entity : entities) {
            entityCounts_[entity.type]++;
        }
    }

    void PrintStatistics()
    {
        std::cout << "\n=== 性能统计 ===" << std::endl;
        std::cout << "总识别次数: " << totalRecognitions_ << std::endl;
        std::cout << "实体类型分布:" << std::endl;

        for (const auto &count : entityCounts_) {
            std::cout << "  类型" << static_cast<int>(count.first) << ": " << count.second << " 个" << std::endl;
        }
    }
};

int main()
{
    std::cout << "鸿蒙剪贴板实体识别系统启动" << std::endl;

    try {
        DemonstrateClipboardEntityRecognition();

        // 性能测试
        PerformanceAnalyzer analyzer;

        std::cout << "\n3. 性能测试:" << std::endl;
        OHOS::MiscServices::AdvancedEntityRecognizer recognizer;

        auto start = std::chrono::high_resolution_clock::now();
        auto entities = recognizer.Recognize("测试文本包含张三和李四li@company.com在2024-01-15会议");
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        analyzer.RecordRecognition(entities, duration);

        std::cout << "识别耗时: " << duration.count() << " 微秒" << std::endl;
        analyzer.PrintStatistics();
    } catch (const std::exception &e) {
        std::cerr << "系统错误: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n系统运行完成，按Enter键退出..." << std::endl;
    std::cin.get();

    return 0;
}

std::shared_ptr<PasteData> ConvertUtils::Convert(const UnifiedData &unifiedData)
{
    auto pasteData = std::make_shared<PasteData>(Convert(unifiedData.GetRecords()));
    pasteData->SetProperty(ConvertProperty(unifiedData.GetProperties(), unifiedData));
    return pasteData;
}

std::shared_ptr<UnifiedData> ConvertUtils::Convert(const PasteData &pasteData)
{
    auto unifiedData = std::make_shared<UnifiedData>();
    unifiedData->SetRecords(Convert(pasteData.AllRecords()));
    unifiedData->SetProperties(ConvertProperty(pasteData.GetProperty()));
    unifiedData->SetDataId(pasteData.GetDataId());
    return unifiedData;
}

std::vector<std::shared_ptr<UnifiedRecord>> ConvertUtils::Convert(
    const std::vector<std::shared_ptr<PasteDataRecord>> &records)
{
    std::vector<std::shared_ptr<UnifiedRecord>> unifiedRecords;
    for (auto const &record : records) {
        unifiedRecords.emplace_back(Convert(record));
    }
    return unifiedRecords;
}

std::vector<std::shared_ptr<PasteDataRecord>> ConvertUtils::Convert(
    const std::vector<std::shared_ptr<UnifiedRecord>> &records)
{
    std::vector<std::shared_ptr<PasteDataRecord>> pasteboardRecords;
    for (auto const &record : records) {
        pasteboardRecords.emplace_back(Convert(record));
    }
    return pasteboardRecords;
}

std::shared_ptr<UnifiedRecord> ConvertUtils::Convert(std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr, PASTEBOARD_MODULE_CLIENT,
        "paste record is nullptr");
    std::shared_ptr<UnifiedRecord> udmfRecord = std::make_shared<UnifiedRecord>();
    auto entries = Convert(record->GetEntries(), record);
    for (auto &udmfEntry : *entries) {
        udmfRecord->AddEntry(udmfEntry.first, std::move(udmfEntry.second));
    }
    udmfRecord->SetChannelName(CHANNEL_NAME);
    udmfRecord->SetDataId(record->GetDataId());
    udmfRecord->SetRecordId(record->GetRecordId());
    return udmfRecord;
}

std::shared_ptr<PasteDataRecord> ConvertUtils::Convert(std::shared_ptr<UnifiedRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr, PASTEBOARD_MODULE_CLIENT,
        "udmfRecord is nullptr");
    std::shared_ptr<PasteDataRecord> pbRecord = std::make_shared<PasteDataRecord>();
    auto utdId = record->GetUtdId();
    pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, record->GetOriginValue()));
    for (auto const &entry : Convert(record->GetEntries())) {
        if (entry == nullptr) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "entry is empty");
            continue;
        }
        if (utdId == entry->GetUtdId()) {
            continue;
        }
        pbRecord->AddEntry(entry->GetUtdId(), entry);
    }
    pbRecord->SetDataId(record->GetDataId());
    pbRecord->SetRecordId(record->GetRecordId());
    if (record->GetEntryGetter() != nullptr) {
        pbRecord->SetDelayRecordFlag(true);
    }
    return pbRecord;
}

std::vector<std::shared_ptr<PasteDataEntry>> ConvertUtils::Convert(
    const std::shared_ptr<std::map<std::string, UDMF::ValueType>> &entries)
{
    std::vector<std::shared_ptr<PasteDataEntry>> pbEntries;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entries != nullptr, pbEntries, PASTEBOARD_MODULE_CLIENT,
        "pbEntries is empty");
    for (auto const &[utdId, value] : *entries) {
        pbEntries.emplace_back(std::make_shared<PasteDataEntry>(utdId, value));
    }
    return pbEntries;
}

UDMF::ValueType ConvertUtils::Convert(const std::shared_ptr<PasteDataEntry>& entry,
    std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, nullptr, PASTEBOARD_MODULE_CLIENT,
        "entry is null, convert failed.");
    auto utdId = entry->GetUtdId();
    auto value = entry->GetValue();
    if (std::holds_alternative<std::monostate>(value) || std::holds_alternative<std::shared_ptr<Object>>(value)) {
        if (std::holds_alternative<std::shared_ptr<Object>>(value) && CommonUtils::IsFileUri(utdId) &&
            record->GetUriV0() != nullptr) {
            auto object = std::get<std::shared_ptr<Object>>(value);
            object->value_[UDMF::FILE_URI_PARAM] = record->GetUriV0()->ToString();
        }
        return value;
    }
    auto mimeType = entry->GetMimeType();
    auto object = std::make_shared<UDMF::Object>();
    if (mimeType == MIMETYPE_TEXT_PLAIN) {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
        if (std::holds_alternative<std::string>(value)) {
            object->value_[UDMF::CONTENT] = std::get<std::string>(value);
        }
    } else if (mimeType == MIMETYPE_TEXT_HTML) {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
        if (std::holds_alternative<std::string>(value)) {
            object->value_[UDMF::HTML_CONTENT] = std::get<std::string>(value);
        }
    } else if (mimeType == MIMETYPE_TEXT_URI) {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
        if (std::holds_alternative<std::string>(value) && record->GetUriV0() != nullptr) {
            object->value_[UDMF::FILE_URI_PARAM] = record->GetUriV0()->ToString();
        }
    } else if (mimeType == MIMETYPE_PIXELMAP) {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
        if (std::holds_alternative<std::shared_ptr<OHOS::Media::PixelMap>>(value)) {
            object->value_[UDMF::PIXEL_MAP] = std::get<std::shared_ptr<OHOS::Media::PixelMap>>(value);
        }
    } else if (mimeType == MIMETYPE_TEXT_WANT) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "mimeType is want, udmf not support");
    } else {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
        if (std::holds_alternative<std::vector<uint8_t>>(value)) {
            auto arrayBuffer = std::get<std::vector<uint8_t>>(value);
            object->value_[UDMF::ARRAY_BUFFER] = arrayBuffer;
            object->value_[UDMF::ARRAY_BUFFER_LENGTH] = static_cast<int64_t>(arrayBuffer.size());
        }
    }
    return object;
}

std::shared_ptr<std::vector<std::pair<std::string, UDMF::ValueType>>> ConvertUtils::Convert(
    const std::vector<std::shared_ptr<PasteDataEntry>> &entries, std::shared_ptr<PasteDataRecord> record)
{
    std::map<std::string, UDMF::ValueType> udmfEntryMap;
    std::vector<std::pair<std::string, UDMF::ValueType>> udmfEntries;
    std::vector<std::string> entryUtdIds;
    for (auto const &entry : entries) {
        if (entry == nullptr) {
            continue;
        }
        if (udmfEntryMap.find(entry->GetUtdId()) == udmfEntryMap.end()) {
            entryUtdIds.emplace_back(entry->GetUtdId());
        }
        auto udmfEntry = Convert(entry, record);
        if (std::holds_alternative<nullptr_t>(udmfEntry)) {
            continue;
        }
        udmfEntryMap.insert_or_assign(entry->GetUtdId(), udmfEntry);
    }
    for (auto const &utdId : entryUtdIds) {
        auto item = udmfEntryMap.find(utdId);
        if (item != udmfEntryMap.end()) {
            udmfEntries.emplace_back(std::pair<std::string, UDMF::ValueType>(item->first, item->second));
        }
    }
    return std::make_shared<std::vector<std::pair<std::string, UDMF::ValueType>>>(udmfEntries);
}

ShareOption ConvertUtils::UdmfOptions2PbOption(ShareOptions udmfOptions)
{
    ShareOption pbOption = CrossDevice;
    switch (udmfOptions) {
        case UDMF::IN_APP:
            pbOption = InApp;
            break;
        case UDMF::CROSS_APP:
            pbOption = LocalDevice;
            break;
        case UDMF::CROSS_DEVICE:
            pbOption = CrossDevice;
            break;
        default:
            break;
    }
    return pbOption;
}

ShareOptions ConvertUtils::PbOption2UdmfOptions(ShareOption pbOption)
{
    ShareOptions udmfOptions = UDMF::CROSS_DEVICE;
    switch (pbOption) {
        case InApp:
            udmfOptions = UDMF::IN_APP;
            break;
        case LocalDevice:
            udmfOptions = UDMF::CROSS_APP;
            break;
        case CrossDevice:
            udmfOptions = UDMF::CROSS_DEVICE;
            break;
        default:
            break;
    }
    return udmfOptions;
}

PasteDataProperty ConvertUtils::ConvertProperty(
    const std::shared_ptr<UnifiedDataProperties> &properties, const UnifiedData &unifiedData)
{
    if (!properties) {
        return {};
    }
    PasteDataProperty pasteDataProperty;
    pasteDataProperty.shareOption = UdmfOptions2PbOption(properties->shareOptions);
    pasteDataProperty.additions = properties->extras;
    pasteDataProperty.timestamp = properties->timestamp;
    pasteDataProperty.tag = properties->tag;
    auto utdIds = unifiedData.GetTypesLabels();
    pasteDataProperty.mimeTypes = Convert(utdIds);
    pasteDataProperty.isRemote = properties->isRemote;
    return PasteDataProperty(pasteDataProperty);
}

std::shared_ptr<UnifiedDataProperties> ConvertUtils::ConvertProperty(const PasteDataProperty &properties)
{
    auto unifiedDataProperties = std::make_shared<UnifiedDataProperties>();
    unifiedDataProperties->shareOptions = PbOption2UdmfOptions(properties.shareOption);
    unifiedDataProperties->extras = properties.additions;
    unifiedDataProperties->timestamp = properties.timestamp;
    unifiedDataProperties->tag = properties.tag;
    unifiedDataProperties->isRemote = properties.isRemote;
    return unifiedDataProperties;
}

std::vector<std::string> ConvertUtils::Convert(const std::vector<std::string> &utdIds)
{
    std::vector<std::string> types;
    for (const auto &utdId : utdIds) {
        types.push_back(CommonUtils::Convert2MimeType(utdId));
    }
    return types;
}
std::shared_ptr<PasteData> PasteboardUtils::Convert(const UnifiedData &unifiedData)
{
    auto unifiedRecords = unifiedData.GetRecords();
    auto pasteData = std::make_shared<PasteData>(Convert(unifiedRecords));
    auto unifiedDataProperties = unifiedData.GetProperties();
    auto properties = Convert(*unifiedDataProperties);
    auto recordTypes = unifiedData.GetTypesLabels();
    properties.mimeTypes = Convert(recordTypes);
    pasteData->SetProperty(properties);
    return pasteData;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::ConvertToUnifiedRecord(
    const std::shared_ptr<PasteDataRecord>& record)
{
    auto type = Convert(record->GetUDType(), record->GetMimeType());
    switch (type) {
        case UDMF::TEXT:
            return PasteRecord2Text(record);
        case UDMF::PLAIN_TEXT:
            return PasteRecord2PlaintText(record);
        case UDMF::OPENHARMONY_WANT:
            return PasteRecord2Want(record);
        case UDMF::HTML:
            return PasteRecord2Html(record);
        case UDMF::HYPERLINK:
            return PasteRecord2Link(record);
        case UDMF::FILE:
            return PasteRecord2File(record);
        case UDMF::IMAGE:
            return PasteRecord2Image(record);
        case UDMF::VIDEO:
            return PasteRecord2Video(record);
        case UDMF::AUDIO:
            return PasteRecord2Audio(record);
        case UDMF::FOLDER:
            return PasteRecord2Folder(record);
        case UDMF::SYSTEM_DEFINED_PIXEL_MAP:
            return PasteRecord2PixelMap(record);
        case UDMF::SYSTEM_DEFINED_RECORD:
            return PasteRecord2SystemDefined(record);
        case UDMF::SYSTEM_DEFINED_FORM:
            return PasteRecord2Form(record);
        case UDMF::SYSTEM_DEFINED_APP_ITEM:
            return PasteRecord2AppItem(record);
        default:
            return nullptr;
    }
}

std::shared_ptr<UnifiedData> PasteboardUtils::Convert(const PasteData &pasteData)
{
    auto unifiedData = std::make_shared<UnifiedData>();
    for (std::size_t i = 0; i < pasteData.GetRecordCount(); ++i) {
        auto pasteboardRecord = pasteData.GetRecordAt(i);
        if (pasteboardRecord == nullptr) {
            continue;
        }
        std::shared_ptr<UnifiedRecord> convertedRecord = ConvertToUnifiedRecord(pasteboardRecord);
        if (convertedRecord) {
            unifiedData->AddRecord(convertedRecord);
        } else {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "not find type, go to customData");
            unifiedData->AddRecords(Custom2AppDefined(pasteboardRecord));
        }
    }
    auto pastedProp = pasteData.GetProperty();
    auto unifiedProp = Convert(pastedProp);
    unifiedData->SetProperties(unifiedProp);
    return unifiedData;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::ConvertToPasteDataRecord(
    const std::shared_ptr<UnifiedRecord> &record)
{
    auto type = record->GetType();
    switch (type) {
        case UDMF::TEXT:
            return Text2PasteRecord(record);
        case UDMF::PLAIN_TEXT:
            return PlainText2PasteRecord(record);
        case UDMF::OPENHARMONY_WANT:
            return Want2PasteRecord(record);
        case UDMF::HTML:
            return Html2PasteRecord(record);
        case UDMF::HYPERLINK:
            return Link2PasteRecord(record);
        case UDMF::FILE:
            return File2PasteRecord(record);
        case UDMF::IMAGE:
            return Image2PasteRecord(record);
        case UDMF::VIDEO:
            return Video2PasteRecord(record);
        case UDMF::AUDIO:
            return Audio2PasteRecord(record);
        case UDMF::FOLDER:
            return Folder2PasteRecord(record);
        case UDMF::SYSTEM_DEFINED_PIXEL_MAP:
            return PixelMap2PasteRecord(record);
        case UDMF::SYSTEM_DEFINED_RECORD:
            return SystemDefined2PasteRecord(record);
        case UDMF::SYSTEM_DEFINED_FORM:
            return Form2PasteRecord(record);
        case UDMF::SYSTEM_DEFINED_APP_ITEM:
            return AppItem2PasteRecord(record);
        case UDMF::APPLICATION_DEFINED_RECORD:
            return AppDefined2PasteRecord(record);
        default:
            return nullptr;
    }
}

std::vector<std::shared_ptr<PasteDataRecord>> PasteboardUtils::Convert(
    const std::vector<std::shared_ptr<UnifiedRecord>> &records)
{
    std::vector<std::shared_ptr<PasteDataRecord>> pasteboardRecords;
    for (const auto &record : records) {
        if (record == nullptr) {
            continue;
        }
        std::shared_ptr<PasteDataRecord> pasteboardRecord = ConvertToPasteDataRecord(record);
        
        if (pasteboardRecord) {
            pasteboardRecords.push_back(pasteboardRecord);
        } else {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "not find type, convert to AppDefinedRecord");
            pasteboardRecords.push_back(AppDefined2PasteRecord(record));
        }
    }
    return pasteboardRecords;
}

ShareOption PasteboardUtils::UdmfOptions2PbOption(ShareOptions udmfOptions)
{
    ShareOption pbOption = CrossDevice;
    switch (udmfOptions) {
        case UDMF::IN_APP:
            pbOption = InApp;
            break;
        case UDMF::CROSS_APP:
            pbOption = LocalDevice;
            break;
        case UDMF::CROSS_DEVICE:
            pbOption = CrossDevice;
            break;
        default:
            break;
    }
    return pbOption;
}

ShareOptions PasteboardUtils::PbOption2UdmfOptions(ShareOption pbOption)
{
    ShareOptions udmfOptions = UDMF::CROSS_DEVICE;
    switch (pbOption) {
        case InApp:
            udmfOptions = UDMF::IN_APP;
            break;
        case LocalDevice:
            udmfOptions = UDMF::CROSS_APP;
            break;
        case CrossDevice:
            udmfOptions = UDMF::CROSS_DEVICE;
            break;
        default:
            break;
    }
    return udmfOptions;
}

PasteDataProperty PasteboardUtils::Convert(const UnifiedDataProperties &properties)
{
    PasteDataProperty pasteDataProperty;
    pasteDataProperty.shareOption = UdmfOptions2PbOption(properties.shareOptions);
    pasteDataProperty.additions = properties.extras;
    pasteDataProperty.timestamp = properties.timestamp;
    pasteDataProperty.tag = properties.tag;
    pasteDataProperty.isRemote = properties.isRemote;
    return PasteDataProperty(pasteDataProperty);
}

std::shared_ptr<UnifiedDataProperties> PasteboardUtils::Convert(const PasteDataProperty &properties)
{
    auto unifiedDataProperties = std::make_shared<UnifiedDataProperties>();
    unifiedDataProperties->shareOptions = PbOption2UdmfOptions(properties.shareOption);
    unifiedDataProperties->extras = properties.additions;
    unifiedDataProperties->timestamp = properties.timestamp;
    unifiedDataProperties->tag = properties.tag;
    unifiedDataProperties->isRemote = properties.isRemote;
    return unifiedDataProperties;
}

std::vector<std::string> PasteboardUtils::Convert(const std::vector<std::string> &utdIds)
{
    std::vector<std::string> types;
    for (const auto &utdId : utdIds) {
        types.push_back(CommonUtils::Convert2MimeType(utdId));
    }
    return types;
}

std::string PasteboardUtils::Convert(UDType uDType)
{
    switch (uDType) {
        case UDType::PLAIN_TEXT:
            // fall-through
            return MIMETYPE_TEXT_PLAIN;
            // fall-through
        case UDType::HTML:
            return MIMETYPE_TEXT_HTML;
            // fall-through
        case UDType::FILE:
            // fall-through
        case UDType::IMAGE:
            // fall-through
        case UDType::VIDEO:
            // fall-through
        case UDType::AUDIO:
            // fall-through
        case UDType::FOLDER:
            // fall-through
        case UDType::FILE_URI:
            return MIMETYPE_TEXT_URI;
            // fall-through
        case UDType::SYSTEM_DEFINED_PIXEL_MAP:
            return MIMETYPE_PIXELMAP;
            // fall-through
        case UDType::OPENHARMONY_WANT:
            return MIMETYPE_TEXT_WANT;
        default:
            return UDMF::UtdUtils::GetUtdIdFromUtdEnum(uDType);
    }
}

UDType PasteboardUtils::Convert(int32_t uDType, const std::string &mimeType)
{
    if (uDType != UDMF::UD_BUTT) {
        return static_cast<UDType>(uDType);
    }
    if (mimeType == MIMETYPE_TEXT_URI) {
        return UDMF::FILE;
    }
    if (mimeType == MIMETYPE_TEXT_PLAIN) {
        return UDMF::PLAIN_TEXT;
    }
    if (mimeType == MIMETYPE_TEXT_HTML) {
        return UDMF::HTML;
    }
    if (mimeType == MIMETYPE_TEXT_WANT) {
        return UDMF::OPENHARMONY_WANT;
    }
    if (mimeType == MIMETYPE_PIXELMAP) {
        return UDMF::SYSTEM_DEFINED_PIXEL_MAP;
    }
    auto type = UDMF::UtdUtils::GetUtdEnumFromUtdId(mimeType);
    if (type != UDMF::UD_BUTT) {
        return static_cast<UDType>(type);
    }
    return UDMF::UD_BUTT;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::PlainText2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto plainText = static_cast<UDMF::PlainText *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(plainText != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get PLAIN_TEXT record failed.");
    auto pbRecord = std::make_shared<PasteDataRecord>();
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDType::PLAIN_TEXT);
    auto value = record->GetOriginValue();
    if (std::holds_alternative<std::shared_ptr<Object>>(value)) {
        pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, value));
        return pbRecord;
    }
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::CONTENT] = plainText->GetContent();
    object->value_[UDMF::ABSTRACT] = plainText->GetAbstract();
    pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
    pbRecord->SetDetails(plainText->GetDetails());
    pbRecord->SetUDType(UDMF::PLAIN_TEXT);
    return pbRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2PlaintText(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "plain text record is null.");
    auto entry = record->GetEntryByMimeType(MIMETYPE_TEXT_PLAIN);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "entry is null.");
    auto plainText = std::make_shared<UDMF::PlainText>(UDMF::PLAIN_TEXT, entry->GetValue());
    if (record->GetDetails()) {
        plainText->SetDetails(*record->GetDetails());
    }
    return plainText;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Want2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto want = static_cast<UDMF::UnifiedRecord *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(want != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get want record failed.");
    auto recordValue = want->GetValue();
    auto wantValue = std::get_if<std::shared_ptr<OHOS::AAFwk::Want>>(&recordValue);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(wantValue != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get want from unified record failed.");
    auto wantRecord = PasteDataRecord::NewWantRecord(*(wantValue));
    wantRecord->SetUDType(UDMF::OPENHARMONY_WANT);
    return wantRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Want(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 want record is null.");
    auto wantRecord = record->GetWant();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(wantRecord != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get want from paste record failed.");
    return std::make_shared<UDMF::UnifiedRecord>(UDMF::OPENHARMONY_WANT, wantRecord);
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Html2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto html = static_cast<UDMF::Html *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get HTML record failed.");
    auto pbRecord = std::make_shared<PasteDataRecord>();
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDType::HTML);
    auto value = record->GetOriginValue();
    if (std::holds_alternative<std::shared_ptr<Object>>(value)) {
        pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, value));
        return pbRecord;
    }
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::HTML_CONTENT] = html->GetHtmlContent();
    object->value_[UDMF::PLAIN_CONTENT] = html->GetPlainContent();
    pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
    pbRecord->SetDetails(html->GetDetails());
    pbRecord->SetUDType(UDMF::HTML);
    return pbRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Html(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 html record is null.");
    auto entry = record->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "entry is null.");
    auto html = std::make_shared<UDMF::Html>(UDMF::HTML, entry->GetValue());
    if (record->GetDetails()) {
        html->SetDetails(*record->GetDetails());
    }
    return html;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Link2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto link = static_cast<UDMF::Link *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(link != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get Link record failed.");
    auto pbRecord = std::make_shared<PasteDataRecord>();
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDType::HYPERLINK);
    auto value = record->GetOriginValue();
    if (std::holds_alternative<std::shared_ptr<Object>>(value)) {
        pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, value));
        return pbRecord;
    }
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::URL] = link->GetUrl();
    object->value_[UDMF::DESCRIPTION] = link->GetDescription();
    pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
    pbRecord->SetDetails(link->GetDetails());
    pbRecord->SetUDType(UDMF::HYPERLINK);
    return pbRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Link(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 link record is null.");
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDType::HYPERLINK);
    auto entry = record->GetEntry(utdId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "entry is null.");
    auto link = std::make_shared<UDMF::Link>(UDMF::HYPERLINK, entry->GetValue());
    if (record->GetDetails()) {
        link->SetDetails(*record->GetDetails());
    }
    return link;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::File2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto file = static_cast<UDMF::File *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(file != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get file record failed.");
    auto uriRecord = PasteDataRecord::NewUriRecord(OHOS::Uri(file->GetUri()));
    uriRecord->SetDetails(file->GetDetails());
    uriRecord->SetUDType(UDMF::FILE);
    return uriRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2File(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 file record is null.");
    auto uri = record->GetUriV0();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get uri from paste record to file failed.");
    auto unifiedRecord = std::make_shared<UDMF::File>(uri->ToString());
    auto details = record->GetDetails();
    if (details != nullptr) {
        unifiedRecord->SetDetails(*details);
    }
    return unifiedRecord;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Image2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto image = static_cast<UDMF::Image *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(image != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get image record failed.");
    auto uriRecord = PasteDataRecord::NewUriRecord(OHOS::Uri(image->GetUri()));
    uriRecord->SetDetails(image->GetDetails());
    uriRecord->SetUDType(UDMF::IMAGE);
    return uriRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Image(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 iamge record is null.");
    auto uri = record->GetUriV0();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get uri from paste record to image failed.");
    auto unifiedRecord = std::make_shared<UDMF::Image>(uri->ToString());
    auto details = record->GetDetails();
    if (details != nullptr) {
        unifiedRecord->SetDetails(*details);
    }
    return unifiedRecord;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Video2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto video = static_cast<UDMF::Video *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(video != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get video record failed.");
    auto uriRecord = PasteDataRecord::NewUriRecord(OHOS::Uri(video->GetUri()));
    uriRecord->SetDetails(video->GetDetails());
    uriRecord->SetUDType(UDMF::VIDEO);
    return uriRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Video(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 video record is null.");
    auto uri = record->GetUriV0();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get uri from paste record to video failed.");
    auto unifiedRecord = std::make_shared<UDMF::Video>(uri->ToString());
    auto details = record->GetDetails();
    if (details != nullptr) {
        unifiedRecord->SetDetails(*details);
    }
    return unifiedRecord;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Audio2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto audio = static_cast<UDMF::Audio *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(audio != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get audio record failed.");
    auto uriRecord = PasteDataRecord::NewUriRecord(OHOS::Uri(audio->GetUri()));
    uriRecord->SetDetails(audio->GetDetails());
    uriRecord->SetUDType(UDMF::AUDIO);
    return uriRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Audio(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 audio record is null.");
    auto uri = record->GetUriV0();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get uri from paste record to audio failed.");
    auto unifiedRecord = std::make_shared<UDMF::Audio>(uri->ToString());
    auto details = record->GetDetails();
    if (details != nullptr) {
        unifiedRecord->SetDetails(*details);
    }
    return unifiedRecord;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Folder2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto folder = static_cast<UDMF::Folder *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(folder != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get folder record failed.");
    auto uriRecord = PasteDataRecord::NewUriRecord(OHOS::Uri(folder->GetUri()));
    uriRecord->SetDetails(folder->GetDetails());
    uriRecord->SetUDType(UDMF::FOLDER);
    return uriRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Folder(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 folder record is null.");
    auto uri = record->GetUriV0();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get uri from paste record to folder failed.");
    auto unifiedRecord = std::make_shared<UDMF::Folder>(uri->ToString());
    auto details = record->GetDetails();
    if (details != nullptr) {
        unifiedRecord->SetDetails(*details);
    }
    return unifiedRecord;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::PixelMap2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto pixelMap = static_cast<UDMF::SystemDefinedPixelMap *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(pixelMap != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get pixelMap record failed.");
    auto recordValue = pixelMap->GetValue();
    auto pixelMapValue = std::get_if<std::shared_ptr<Media::PixelMap>>(&recordValue);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(pixelMapValue != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get pixelMap from unified record failed.");
    auto pixelMapRecord = PasteDataRecord::NewPixelMapRecord(*(pixelMapValue));
    pixelMapRecord->SetUDType(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    return pixelMapRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2PixelMap(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "pixel map record is null.");
    auto pixelMapRecord = record->GetPixelMapV0();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(pixelMapRecord != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get pixelMap from paste record failed.");
    return std::make_shared<UDMF::UnifiedRecord>(UDMF::SYSTEM_DEFINED_PIXEL_MAP, pixelMapRecord);
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::AppItem2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto appItem = static_cast<UDMF::SystemDefinedAppItem *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(appItem != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get SystemDefined record failed.");
    auto pbRecord = std::make_shared<PasteDataRecord>();
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDType::SYSTEM_DEFINED_APP_ITEM);
    auto value = record->GetOriginValue();
    if (std::holds_alternative<std::shared_ptr<Object>>(value)) {
        pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, value));
        return pbRecord;
    }
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::APP_ID] = appItem->GetAppId();
    object->value_[UDMF::APP_NAME] = appItem->GetAppName();
    object->value_[UDMF::APP_ICON_ID] = appItem->GetAppIconId();
    object->value_[UDMF::APP_LABEL_ID] = appItem->GetAppLabelId();
    object->value_[UDMF::BUNDLE_NAME] = appItem->GetBundleName();
    object->value_[UDMF::ABILITY_NAME] = appItem->GetAbilityName();
    pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
    pbRecord->SetDetails(appItem->GetDetails());
    pbRecord->SetUDType(UDMF::SYSTEM_DEFINED_APP_ITEM);
    return pbRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2AppItem(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "app Item record is null.");
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDType::SYSTEM_DEFINED_APP_ITEM);
    auto entry = record->GetEntry(utdId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "entry is null.");
    auto unifiedRecord = std::make_shared<UDMF::SystemDefinedAppItem>(UDMF::SYSTEM_DEFINED_APP_ITEM, entry->GetValue());
    if (record->GetDetails() != nullptr) {
        unifiedRecord->SetDetails(*record->GetDetails());
    }
    return unifiedRecord;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Form2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto form = static_cast<UDMF::SystemDefinedForm *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(form != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "Form2 Paste Record is null.");
    std::vector<uint8_t> arrayBuffer;
    auto kvRecord = PasteDataRecord::NewKvRecord(Convert(UDType::SYSTEM_DEFINED_FORM), arrayBuffer);
    kvRecord->SetDetails(form->GetDetails());
    kvRecord->SetSystemDefinedContent(form->GetItems());
    kvRecord->SetUDType(UDType::SYSTEM_DEFINED_FORM);
    return kvRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Form(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "record2 from record is null.");
    auto unifiedRecord = std::make_shared<UDMF::SystemDefinedForm>();
    if (record->GetSystemDefinedContent() != nullptr) {
        unifiedRecord->SetItems(*record->GetSystemDefinedContent());
    }
    if (record->GetDetails() != nullptr) {
        unifiedRecord->SetDetails(*record->GetDetails());
    }
    return unifiedRecord;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::SystemDefined2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto systemDefined = static_cast<UDMF::SystemDefinedRecord *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(systemDefined != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get systemRecord record failed.");
    std::vector<uint8_t> arrayBuffer;
    auto kvRecord = PasteDataRecord::NewKvRecord(Convert(UDType::SYSTEM_DEFINED_RECORD), arrayBuffer);
    kvRecord->SetDetails(systemDefined->GetDetails());
    kvRecord->SetUDType(UDType::SYSTEM_DEFINED_RECORD);
    return kvRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2SystemDefined(const std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "system fined record is null.");
    auto unifiedRecord = std::make_shared<UDMF::SystemDefinedRecord>();
    if (record->GetDetails() != nullptr) {
        unifiedRecord->SetDetails(*record->GetDetails());
    }
    return unifiedRecord;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::Text2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto text = static_cast<UDMF::Text *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(text != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get TEXT record failed.");
    std::vector<uint8_t> arrayBuffer;
    std::string type = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::TEXT);
    auto kvRecord = PasteDataRecord::NewKvRecord(type, arrayBuffer);
    kvRecord->SetUDType(UDMF::TEXT);
    kvRecord->SetDetails(text->GetDetails());
    return kvRecord;
}

std::shared_ptr<UnifiedRecord> PasteboardUtils::PasteRecord2Text(std::shared_ptr<PasteDataRecord> record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "cord2 text record is null.");
    auto unifiedRecord = std::make_shared<UDMF::Text>();
    if (record->GetDetails() != nullptr) {
        unifiedRecord->SetDetails(*record->GetDetails());
    }
    return unifiedRecord;
}

std::vector<std::shared_ptr<UnifiedRecord>> PasteboardUtils::Custom2AppDefined(
    const std::shared_ptr<PasteDataRecord> record)
{
    std::vector<std::shared_ptr<UnifiedRecord>> unifiedRecords;
    if (record == nullptr) {
        return unifiedRecords;
    }
    if (record->GetCustomData() == nullptr) {
        return unifiedRecords;
    }
    auto customData = record->GetCustomData();
    if (customData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "customData is null");
        return unifiedRecords;
    }
    for (auto &[type, rawData] : customData->GetItemData()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "app defied type:%{public}s.", type.c_str());
        unifiedRecords.push_back(std::make_shared<UDMF::ApplicationDefinedRecord>(type, rawData));
    }
    return unifiedRecords;
}

std::shared_ptr<PasteDataRecord> PasteboardUtils::AppDefined2PasteRecord(const std::shared_ptr<UnifiedRecord> record)
{
    auto appRecord = static_cast<UDMF::ApplicationDefinedRecord *>(record.get());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(appRecord != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "get ApplicationDefinedRecord record failed.");
    auto utdId = appRecord->GetApplicationDefinedType();
    auto pbRecord = std::make_shared<PasteDataRecord>();
    auto value = record->GetOriginValue();
    if (std::holds_alternative<std::shared_ptr<Object>>(value)) {
        pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, value));
        return pbRecord;
    }
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::ARRAY_BUFFER] = appRecord->GetRawData();
    object->value_[UDMF::ARRAY_BUFFER_LENGTH] = static_cast<int64_t>(appRecord->GetRawData().size());
    pbRecord->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
    pbRecord->SetUDType(UDMF::APPLICATION_DEFINED_RECORD);
    return pbRecord;
}

std::vector<std::string> PasteboardUtils::DeduplicateVector(const std::vector<std::string> &vec)
{
    std::unordered_set<std::string> tmp(vec.begin(), vec.end());
    std::vector<std::string> result(tmp.begin(), tmp.end());
    return result;
}

PasteboardUtils::PasteboardUtils()
{
}

PasteboardUtils &PasteboardUtils::GetInstance()
{
    static PasteboardUtils instance;
    return instance;
}