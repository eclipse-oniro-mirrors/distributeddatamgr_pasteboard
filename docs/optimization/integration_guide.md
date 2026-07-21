# 优化方案集成说明

## 重要提示

⚠️ **本文档中的代码为示例实现，需要根据实际 API 进行适配。**

---

## 已知问题及修复

### 1. PasteDataCompression 集成

**问题：** 示例代码中假设 PasteData 类有以下方法，但实际不存在：
- `SetCompressedData()`
- `GetCompressedData()`
- `HasCompressedData()`
- `ClearCompressedData()`

**修复方案：** 改为直接操作二进制数据

```cpp
// ❌ 错误的用法（示例）
pasteData.SetCompressedData(compressed);

// ✅ 正确的集成方式
std::vector<uint8_t> serialized = SerializePasteData(pasteData);
auto result = PasteDataCompression::CompressPasteData(serialized);

// 存储压缩后的数据
// 方式1：通过 PasteData 的自定义字段
pasteData.SetCustomData("compressed", serialized);

// 方式2：序列化整个 PasteData（包含压缩数据）
SaveCompressedData(serialized);
```

---

### 2. ModuleLoader 平台兼容性

**问题：** dlopen/dlclose 在某些平台可能不可用

**修复方案：** 添加平台检查

```cpp
// module_loader.h

bool LoadModule(Module module)
{
    // ... 
    
#ifdef __linux__
    void* handle = dlopen(info.path.c_str(), RTLD_LAZY);
#else
    // Windows/其他平台使用 LoadLibrary 或跳过
    PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, 
        "Module loading not supported on this platform");
    return false;
#endif
    
    // ...
}
```

---

### 3. MemoryMonitor 异常处理

**问题：** MonitorLoop 中的异常未捕获

**修复方案：** 添加异常处理

```cpp
void MonitorLoop()
{
    while (running_) {
        try {
            MemoryPressureLevel level = CheckMemoryPressure();
            
            if (level != MemoryPressureLevel::NONE) {
                Cleanup(level);
            }
            
            if (stats_.totalChecks % 12 == 0) {
                ReportStats();
            }
            
        } catch (const std::exception& e) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT,
                "Exception in monitor loop: %{public}s", e.what());
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(config_.checkInterval));
    }
}
```

---

## 实际集成步骤

### 步骤 1：验证依赖

确保有以下头文件：
```cpp
#include "paste_data.h"          // PasteData 类
#include "paste_data_record.h"   // PasteDataRecord 类
#include "udmf/unified_data.h"   // UDMF::UnifiedData 类
```

### 步骤 2：适配压缩功能

**方式 A：在 PasteData 中添加压缩支持**

```cpp
// paste_data.h (修改)

class PasteData {
public:
    // 现有方法...
    
    // 新增压缩支持
    void SetCompressed(bool compressed) { compressed_ = compressed; }
    bool IsCompressed() const { return compressed_; }
    
private:
    bool compressed_ = false;
    std::vector<uint8_t> compressedData_;
};
```

**方式 B：使用 TLV 序列化**

```cpp
// 在 PasteData 中已有 TLV 序列化支持
// 直接在现有序列化流程中集成压缩

int32_t PasteboardClient::SetPasteData(PasteData &pasteData)
{
    std::vector<uint8_t> tlvData;
    int fd = -1;
    int64_t tlvSize = 0;
    
    // 使用现有的 TLV 序列化
    int32_t ret = WritePasteData(pasteData, tlvData, fd, tlvSize, ...);
    
    if (ret == SUCCESS && tlvSize > COMPRESSION_THRESHOLD) {
        // 压缩 TLV 数据
        auto compressed = CompressionUtils::Compress(tlvData);
        
        if (compressed.size() < tlvData.size()) {
            tlvData = compressed;
            tlvSize = compressed.size();
        }
    }
    
    // 发送到服务
    // ...
}
```

### 步骤 3：集成缓存

```cpp
// pasteboard_client.cpp

#include "smart_cache.h"
#include "udmf_lazy_loader.h"

// 全局缓存（使用实际的数据类型）
SmartCache<std::string, std::vector<uint8_t>> g_dataCache({
    .maxSize = 100,
    .ttl = 300
});

int32_t PasteboardClient::GetPasteData(PasteData &pasteData)
{
    std::string cacheKey = GenerateCacheKey();
    
    // 查缓存
    auto cached = g_dataCache.Get(cacheKey);
    if (cached) {
        // 使用缓存数据
        return DeserializeFromCache(pasteData, *cached);
    }
    
    // 缓存未命中，从服务加载
    auto proxyService = GetPasteboardService();
    int32_t ret = proxyService->GetPasteData(pasteData);
    
    if (ret == SUCCESS) {
        // 存入缓存
        std::vector<uint8_t> cacheData = SerializeToCache(pasteData);
        g_dataCache.Put(cacheKey, std::make_shared<std::vector<uint8_t>>(cacheData));
    }
    
    return ret;
}
```

### 步骤 4：模块拆分（可选）

如果需要实现模块按需加载：

1. **拆分 BUILD.gn**：

```gn
# core/BUILD.gn
ohos_shared_library("pasteboard_core") {
  sources = [ "basic_copy_paste.cpp" ]
  # ...
}

# unified_data/BUILD.gn
ohos_shared_library("pasteboard_unified") {
  sources = [ "unified_data.cpp" ]
  deps = [ ":pasteboard_core" ]
  # ...
}
```

2. **运行时加载**：

```cpp
bool LoadUnifiedDataModule()
{
    void* handle = dlopen("libpasteboard_unified.z.so", RTLD_LAZY);
    if (!handle) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, 
            "Failed to load UnifiedData module");
        return false;
    }
    
    // 获取符号
    using GetUnifiedDataFunc = int32_t(*)(UDMF::UnifiedData&);
    auto func = (GetUnifiedDataFunc)dlsym(handle, "GetUnifiedDataImpl");
    
    if (!func) {
        dlclose(handle);
        return false;
    }
    
    // 使用
    UDMF::UnifiedData data;
    return func(data);
}
```

---

## 性能测试建议

### 1. 内存占用测试

```cpp
#include <sys/resource.h>

size_t GetProcessMemory()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss; // KB
}

// 测试前
size_t before = GetProcessMemory();

// 测试操作
PasteData data = CreateLargePasteData();
int32_t ret = SetPasteData(data);

// 测试后
size_t after = GetProcessMemory();
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, 
    "Memory delta: %{public}zu KB", after - before);
```

### 2. 缓存命中率测试

```cpp
// 启用统计
SmartCache<std::string, PasteData>::Config config;
config.enableStats = true; // 如果实现

// 运行一段时间后
auto stats = g_dataCache.GetStats();
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT,
    "Cache hit rate: %.1f%%", stats.HitRate() * 100);
```

---

## 注意事项

1. **线程安全**：所有优化类都使用 mutex 保护，无需额外加锁
2. **异常处理**：实际集成时建议添加 try-catch
3. **资源释放**：MemoryMonitor 在析构时自动停止监控线程
4. **配置调整**：根据设备内存大小动态调整缓存和阈值

---

## 后续工作

1. ✅ 提交修复后的代码
2. ⏳ 实际集成到 PasteData 类
3. ⏳ 性能测试和调优
4. ⏳ 添加单元测试

---

## 联系方式

如有集成问题，请在 PR #2071 中评论。