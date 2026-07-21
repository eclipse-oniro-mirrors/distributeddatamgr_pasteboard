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

#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include <map>
#include <mutex>
#include <string>
#include <dlfcn.h>
#include <memory>
#include <functional>
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

class ModuleLoader {
public:
    enum class Module {
        CORE,           // 基础剪贴板（必须加载）
        UNIFIED_DATA,   // 统一数据处理
        PROGRESS,       // 进度报告
        ENTITY,         // 实体识别
        REMOTE,         // 远程同步
        DLP,            // DLP 支持
        PATTERN         // 模式检测
    };
    
    struct ModuleInfo {
        std::string name;
        std::string path;
        bool required;  // 是否必须加载
        size_t estimatedSize; // 预估内存占用（KB）
    };
    
    static ModuleLoader& GetInstance()
    {
        static ModuleLoader instance;
        return instance;
    }
    
    bool LoadModule(Module module)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (loadedModules_.count(module)) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, 
                "Module already loaded: %{public}s", GetModuleInfo(module).name.c_str());
            return true;
        }
        
        ModuleInfo info = GetModuleInfo(module);
        
        // 检查内存压力
        if (!info.required && IsMemoryPressure(info.estimatedSize)) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT,
                "Memory pressure, skip loading module: %{public}s", info.name.c_str());
            return false;
        }
        
        // 加载模块
        void* handle = dlopen(info.path.c_str(), RTLD_LAZY);
        if (!handle) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT,
                "Failed to load module %{public}s: %{public}s", 
                info.name.c_str(), dlerror());
            
            if (info.required) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT,
                    "Required module failed to load: %{public}s", info.name.c_str());
            }
            return false;
        }
        
        loadedModules_[module] = handle;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT,
            "Module loaded: %{public}s (estimated %{public}zu KB)",
            info.name.c_str(), info.estimatedSize);
        
        return true;
    }
    
    void UnloadModule(Module module)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = loadedModules_.find(module);
        if (it == loadedModules_.end()) {
            return;
        }
        
        ModuleInfo info = GetModuleInfo(module);
        
        // 不允许卸载必须的模块
        if (info.required) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT,
                "Cannot unload required module: %{public}s", info.name.c_str());
            return;
        }
        
        dlclose(it->second);
        loadedModules_.erase(it);
        
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT,
            "Module unloaded: %{public}s", info.name.c_str());
    }
    
    bool IsModuleLoaded(Module module) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return loadedModules_.count(module) > 0;
    }
    
    size_t GetLoadedModuleCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return loadedModules_.size();
    }
    
    size_t EstimateTotalMemoryUsage() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t total = 0;
        for (const auto& pair : loadedModules_) {
            total += GetModuleInfo(pair.first).estimatedSize;
        }
        
        return total;
    }
    
    std::vector<Module> GetLoadedModules() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<Module> modules;
        for (const auto& pair : loadedModules_) {
            modules.push_back(pair.first);
        }
        
        return modules;
    }
    
    void UnloadAllNonRequired()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<Module> toUnload;
        for (const auto& pair : loadedModules_) {
            ModuleInfo info = GetModuleInfo(pair.first);
            if (!info.required) {
                toUnload.push_back(pair.first);
            }
        }
        
        for (Module module : toUnload) {
            ModuleInfo info = GetModuleInfo(module);
            dlclose(loadedModules_[module]);
            loadedModules_.erase(module);
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT,
                "Module unloaded: %{public}s", info.name.c_str());
        }
    }
    
private:
    ModuleLoader() = default;
    ~ModuleLoader()
    {
        for (auto& pair : loadedModules_) {
            if (pair.second) {
                dlclose(pair.second);
            }
        }
    }
    
    ModuleInfo GetModuleInfo(Module module) const
    {
        switch (module) {
            case Module::CORE:
                return {"Core", "libpasteboard_core.z.so", true, 500};
            case Module::UNIFIED_DATA:
                return {"UnifiedData", "libpasteboard_unified.z.so", false, 300};
            case Module::PROGRESS:
                return {"Progress", "libpasteboard_progress.z.so", false, 200};
            case Module::ENTITY:
                return {"Entity", "libpasteboard_entity.z.so", false, 400};
            case Module::REMOTE:
                return {"Remote", "libpasteboard_remote.z.so", false, 600};
            case Module::DLP:
                return {"DLP", "libpasteboard_dlp.z.so", false, 300};
            case Module::PATTERN:
                return {"Pattern", "libpasteboard_pattern.z.so", false, 250};
            default:
                return {"Unknown", "", false, 0};
        }
    }
    
    bool IsMemoryPressure(size_t requiredSize) const
    {
        // 获取系统可用内存（简化实现）
        size_t availableMemory = GetAvailableMemory();
        size_t threshold = 50 * 1024; // 50 MB
        
        if (availableMemory < threshold) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT,
                "Memory pressure detected: available=%{public}zu KB, threshold=%{public}zu KB",
                availableMemory, threshold);
            return true;
        }
        
        return false;
    }
    
    size_t GetAvailableMemory() const
    {
        FILE* fp = fopen("/proc/meminfo", "r");
        if (!fp) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, 
                "Failed to open /proc/meminfo, using fallback value");
            return 100 * 1024;
        }
        
        char line[256];
        size_t memAvailable = 0;
        size_t value = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "MemAvailable: %zu kB", &value) == 1) {
                memAvailable = value;
                break;
            }
        }
        
        fclose(fp);
        
        if (memAvailable == 0) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT,
                "Failed to parse MemAvailable, using fallback value");
            return 100 * 1024;
        }
        
        return memAvailable;
    }
    
    std::map<Module, void*> loadedModules_;
    mutable std::mutex mutex_;
};

// 模块加载辅助类
class ModuleGuard {
public:
    explicit ModuleGuard(ModuleLoader::Module module) : module_(module), loaded_(false)
    {
        loaded_ = ModuleLoader::GetInstance().LoadModule(module_);
    }
    
    ~ModuleGuard()
    {
        // 可选：自动卸载非必须模块
        // ModuleLoader::GetInstance().UnloadModule(module_);
    }
    
    bool IsLoaded() const { return loaded_; }
    
private:
    ModuleLoader::Module module_;
    bool loaded_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // MODULE_LOADER_H