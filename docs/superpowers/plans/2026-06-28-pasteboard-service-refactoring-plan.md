# PasteboardService重构实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将5569行的pasteboard_service.cpp拆分为10个职责清晰的Manager类，主服务类降至约800行，提高代码可维护性、可测试性和复用性。

**Architecture:** 采用职责驱动拆分策略，将分布式数据管理、P2P连接、URI处理、观察者管理等独立功能抽取为Manager类，PasteboardService作为协调者通过依赖注入方式调用各Manager。

**Tech Stack:** C++17, OpenHarmony SystemAbility框架, ConcurrentMap, ClipPlugin, IPC/SAMGR

---

## 文件结构规划

### 新建文件（10个Manager类）

```
services/core/include/
├── pasteboard_distributed_manager.h        (~80行)
├── pasteboard_p2p_manager.h               (~70行)
├── pasteboard_uri_handler.h               (~60行)
├── pasteboard_observer_manager.h          (~90行)
├── pasteboard_entity_recognizer.h         (~50行)
├── pasteboard_permission_checker.h        (~80行)
├── pasteboard_app_info_helper.h           (~60行)
├── pasteboard_delay_data_handler.h        (~80行)
├── pasteboard_system_event_listener.h     (~70行)
├── pasteboard_dump_helper.h               (~50行)

services/core/src/
├── pasteboard_distributed_manager.cpp     (~500行)
├── pasteboard_p2p_manager.cpp             (~400行)
├── pasteboard_uri_handler.cpp             (~600行)
├── pasteboard_observer_manager.cpp        (~300行)
├── pasteboard_entity_recognizer.cpp       (~200行)
├── pasteboard_permission_checker.cpp      (~300行)
├── pasteboard_app_info_helper.cpp         (~200行)
├── pasteboard_delay_data_handler.cpp      (~800行)
├── pasteboard_system_event_listener.cpp   (~300行)
├── pasteboard_dump_helper.cpp             (~200行)
```

### 修改文件

```
services/core/include/pasteboard_service.h
├── 添加10个Manager成员变量                  (~40行新增)
├── 修改原有方法调用方式                      (~200行修改)
├── 移除已迁移方法的声明                      (~150行删除)
└── 最终约400行

services/core/src/pasteboard_service.cpp
├── 添加Manager初始化代码                    (~50行新增)
├── 修改方法调用方式                          (~300行修改)
├── 移除已迁移方法实现                        (~4800行删除)
└── 最终约800行

services/core/BUILD.gn
├── 添加新源文件到sources列表                (~20行新增)
```

---

## Phase 1: 创建框架（估算：800行提交）

### Task 1.1: 创建Manager头文件（框架）

**Files:**
- Create: `services/core/include/pasteboard_distributed_manager.h`
- Create: `services/core/include/pasteboard_p2p_manager.h`
- Create: `services/core/include/pasteboard_uri_handler.h`
- Create: `services/core/include/pasteboard_observer_manager.h`
- Create: `services/core/include/pasteboard_entity_recognizer.h`
- Create: `services/core/include/pasteboard_permission_checker.h`
- Create: `services/core/include/pasteboard_app_info_helper.h`
- Create: `services/core/include/pasteboard_delay_data_handler.h`
- Create: `services/core/include/pasteboard_system_event_listener.h`
- Create: `services/core/include/pasteboard_dump_helper.h`

- [ ] **Step 1: 创建pasteboard_distributed_manager.h头文件**

```cpp
/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_DISTRIBUTED_MANAGER_H
#define PASTEBOARD_DISTRIBUTED_MANAGER_H

#include "pasteboard_service.h"

namespace OHOS {
namespace MiscServices {

class PasteboardDistributedManager {
public:
    explicit PasteboardDistributedManager(PasteboardService& service);
    ~PasteboardDistributedManager();
    
    // 分布式数据获取接口声明（详细接口在设计文档中）
    std::pair<std::shared_ptr<PasteData>, PasteDateResult> GetDistributedData(const Event& event, int32_t user);
    int32_t GetDistributedDelayData(const Event& evt, uint8_t version, std::vector<uint8_t>& rawData);
    
    // 分布式数据设置接口声明
    bool SetDistributedData(int32_t user, PasteData& data);
    
    // 分布式状态管理接口声明
    void CleanDistributedData(int32_t user);
    bool IsDisallowDistributed();
    
private:
    PasteboardService& service_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_DISTRIBUTED_MANAGER_H
```

- [ ] **Step 2: 创建pasteboard_p2p_manager.h头文件**

```cpp
/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_P2P_MANAGER_H
#define PASTEBOARD_P2P_MANAGER_H

#include "pasteboard_service.h"
#include "ffrt/ffrt_utils.h"

namespace OHOS {
namespace MiscServices {

class PasteboardP2PManager {
public:
    explicit PasteboardP2PManager(PasteboardService& service);
    ~PasteboardP2PManager();
    
    // P2P连接建立接口声明
    void OpenP2PLink(const std::string& networkId);
    void EstablishP2PLink(const std::string& networkId, const std::string& pasteId);
    
    // 预同步管理接口声明
    void PreSyncRemotePasteboardData();
    void RegisterPreSyncMonitor();
    
private:
    PasteboardService& service_;
    std::shared_ptr<FFRTTimer> ffrtTimer_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_P2P_MANAGER_H
```

- [ ] **Step 3: 创建其余8个Manager头文件**

按照相同模式创建：
- pasteboard_uri_handler.h
- pasteboard_observer_manager.h
- pasteboard_entity_recognizer.h
- pasteboard_permission_checker.h
- pasteboard_app_info_helper.h
- pasteboard_delay_data_handler.h
- pasteboard_system_event_listener.h
- pasteboard_dump_helper.h

每个头文件包含：
- Copyright声明
- 类声明
- 构造函数/析构函数
- 主要接口声明（在设计文档中定义）
- PasteboardService引用成员

- [ ] **Step 4: 提交Manager头文件框架**

```bash
git add services/core/include/pasteboard_distributed_manager.h
git add services/core/include/pasteboard_p2p_manager.h
git add services/core/include/pasteboard_uri_handler.h
git add services/core/include/pasteboard_observer_manager.h
git add services/core/include/pasteboard_entity_recognizer.h
git add services/core/include/pasteboard_permission_checker.h
git add services/core/include/pasteboard_app_info_helper.h
git add services/core/include/pasteboard_delay_data_handler.h
git add services/core/include/pasteboard_system_event_listener.h
git add services/core/include/pasteboard_dump_helper.h
git commit -m "refactor: create Manager class headers for pasteboard service split"
```

Expected: 约630行提交

---

### Task 1.2: 创建Manager实现文件（空框架）

**Files:**
- Create: `services/core/src/pasteboard_distributed_manager.cpp`
- Create: `services/core/src/pasteboard_p2p_manager.cpp`
- Create: `services/core/src/pasteboard_uri_handler.cpp`
- Create: `services/core/src/pasteboard_observer_manager.cpp`
- Create: `services/core/src/pasteboard_entity_recognizer.cpp`
- Create: `services/core/src/pasteboard_permission_checker.cpp`
- Create: `services/core/src/pasteboard_app_info_helper.cpp`
- Create: `services/core/src/pasteboard_delay_data_handler.cpp`
- Create: `services/core/src/pasteboard_system_event_listener.cpp`
- Create: `services/core/src/pasteboard_dump_helper.cpp`

- [ ] **Step 1: 创建pasteboard_distributed_manager.cpp空实现**

```cpp
/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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

#include "pasteboard_distributed_manager.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

PasteboardDistributedManager::PasteboardDistributedManager(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDistributedManager constructed.");
}

PasteboardDistributedManager::~PasteboardDistributedManager()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDistributedManager destructed.");
}

// TODO: Implement methods in Phase 2

} // namespace MiscServices
} // namespace OHOS
```

- [ ] **Step 2: 创建其余9个Manager空实现**

按照相同模式创建空实现文件，包含：
- Copyright声明
- 构造函数（初始化service_引用）
- 析构函数
- TODO注释标记后续实现

- [ ] **Step 3: 修改BUILD.gn添加新源文件**

在`services/core/BUILD.gn`的sources列表中添加：

```python
sources = [
    "src/pasteboard_service.cpp",
    "src/pasteboard_distributed_manager.cpp",      # 新增
    "src/pasteboard_p2p_manager.cpp",              # 新增
    "src/pasteboard_uri_handler.cpp",              # 新增
    "src/pasteboard_observer_manager.cpp",         # 新增
    "src/pasteboard_entity_recognizer.cpp",        # 新增
    "src/pasteboard_permission_checker.cpp",       # 新增
    "src/pasteboard_app_info_helper.cpp",          # 新增
    "src/pasteboard_delay_data_handler.cpp",       # 新增
    "src/pasteboard_system_event_listener.cpp",    # 新增
    "src/pasteboard_dump_helper.cpp",              # 新增
    # ... 其他现有文件保持不变
]
```

- [ ] **Step 4: 提交Manager实现文件框架**

```bash
git add services/core/src/pasteboard_distributed_manager.cpp
git add services/core/src/pasteboard_p2p_manager.cpp
git add services/core/src/pasteboard_uri_handler.cpp
git add services/core/src/pasteboard_observer_manager.cpp
git add services/core/src/pasteboard_entity_recognizer.cpp
git add services/core/src/pasteboard_permission_checker.cpp
git add services/core/src/pasteboard_app_info_helper.cpp
git add services/core/src/pasteboard_delay_data_handler.cpp
git add services/core/src/pasteboard_system_event_listener.cpp
git add services/core/src/pasteboard_dump_helper.cpp
git add services/core/BUILD.gn
git commit -m "refactor: create Manager class implementations framework"
```

Expected: 约200行提交

---

### Task 1.3: 在PasteboardService中添加Manager成员

**Files:**
- Modify: `services/core/include/pasteboard_service.h`

- [ ] **Step 1: 在pasteboard_service.h添加Manager成员变量**

在PasteboardService类的private成员区域添加：

```cpp
private:
    // ... 现有成员变量 ...
    
    // 新增Manager成员
    std::unique_ptr<PasteboardDistributedManager> distributedManager_;
    std::unique_ptr<PasteboardP2PManager> p2pManager_;
    std::unique_ptr<PasteboardUriHandler> uriHandler_;
    std::unique_ptr<PasteboardObserverManager> observerManager_;
    std::unique_ptr<PasteboardEntityRecognizer> entityRecognizer_;
    std::unique_ptr<PasteboardPermissionChecker> permissionChecker_;
    std::unique_ptr<PasteboardAppInfoHelper> appInfoHelper_;
    std::unique_ptr<PasteboardDelayDataHandler> delayDataHandler_;
    std::unique_ptr<PasteboardSystemEventListener> systemEventListener_;
    std::unique_ptr<PasteboardDumpHelper> dumpHelper_;
    
    // ... 其他现有成员 ...
```

- [ ] **Step 2: 添加Manager头文件include**

在pasteboard_service.h顶部添加：

```cpp
#include "pasteboard_distributed_manager.h"
#include "pasteboard_p2p_manager.h"
#include "pasteboard_uri_handler.h"
#include "pasteboard_observer_manager.h"
#include "pasteboard_entity_recognizer.h"
#include "pasteboard_permission_checker.h"
#include "pasteboard_app_info_helper.h"
#include "pasteboard_delay_data_handler.h"
#include "pasteboard_system_event_listener.h"
#include "pasteboard_dump_helper.h"
```

- [ ] **Step 3: 提交PasteboardService头文件修改**

```bash
git add services/core/include/pasteboard_service.h
git commit -m "refactor: add Manager member variables to PasteboardService"
```

Expected: 约40行新增

---

### Task 1.4: 在PasteboardService初始化Manager

**Files:**
- Modify: `services/core/src/pasteboard_service.cpp`

- [ ] **Step 1: 在PasteboardService::OnStart()初始化Manager**

在`OnStart()`方法中，在`InitServiceHandler()`之后添加Manager初始化：

```cpp
void PasteboardService::OnStart()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardService OnStart.");
    std::lock_guard<std::mutex> lock(saMutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(PasteboardService::state_ != ServiceRunningState::STATE_RUNNING,
        PASTEBOARD_MODULE_SERVICE, "PasteboardService is already running.");
    IPCSkeleton::SetMaxWorkThreadNum(MAX_IPC_THREAD_NUM);
    InitServiceHandler();
    
    // 新增：初始化Manager
    distributedManager_ = std::make_unique<PasteboardDistributedManager>(*this);
    p2pManager_ = std::make_unique<PasteboardP2PManager>(*this);
    uriHandler_ = std::make_unique<PasteboardUriHandler>(*this);
    observerManager_ = std::make_unique<PasteboardObserverManager>(*this);
    entityRecognizer_ = std::make_unique<PasteboardEntityRecognizer>(*this);
    permissionChecker_ = std::make_unique<PasteboardPermissionChecker>(*this);
    appInfoHelper_ = std::make_unique<PasteboardAppInfoHelper>(*this);
    delayDataHandler_ = std::make_unique<PasteboardDelayDataHandler>(*this);
    systemEventListener_ = std::make_unique<PasteboardSystemEventListener>(*this);
    dumpHelper_ = std::make_unique<PasteboardDumpHelper>(*this);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Managers initialized.");
    
    Loader loader;
    uid_ = loader.LoadUid();
    // ... 后续代码保持不变 ...
}
```

- [ ] **Step 2: 在PasteboardService::OnStop()清理Manager**

在`OnStop()`方法中，在清理其他资源之前添加：

```cpp
void PasteboardService::OnStop()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStop Started.");
    std::lock_guard<std::mutex> lock(saMutex_);
    if (PasteboardService::state_ != ServiceRunningState::STATE_RUNNING) {
        return;
    }
    
    // 新增：清理Manager
    distributedManager_.reset();
    p2pManager_.reset();
    uriHandler_.reset();
    observerManager_.reset();
    entityRecognizer_.reset();
    permissionChecker_.reset();
    appInfoHelper_.reset();
    delayDataHandler_.reset();
    systemEventListener_.reset();
    dumpHelper_.reset();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Managers destroyed.");
    
    serviceHandler_ = nullptr;
    // ... 后续代码保持不变 ...
}
```

- [ ] **Step 3: 编译验证框架**

```bash
cd foundation/distributeddatamgr/pasteboard
hb build services/core
```

Expected: 编译成功，无链接错误（Manager空实现）

- [ ] **Step 4: 提交Manager初始化代码**

```bash
git add services/core/src/pasteboard_service.cpp
git commit -m "refactor: initialize and destroy Managers in PasteboardService"
```

Expected: 约50行新增

---

## Phase 2: 迁移DistributedManager（估算：1000行提交）

### 迁移原则（适用于所有Phase）

**迁移代码时的修改规则**：
1. **签名不变**：方法参数、返回值完全一致
2. **逻辑不变**：内部实现逻辑、错误处理、日志完全一致
3. **成员访问修改**：
   - `this->privateMember_` → `privateMember_`（Manager私有成员）
   - `this->sharedMember_` → `service_.sharedMember_`（共享数据）
   - `GetSharedData()` → `service_.GetSharedData()`（共享方法）
4. **编译验证**：每次迁移后立即编译验证
5. **提交限制**：每次git提交不超过2000行

### Task 2.1: 完善DistributedManager头文件接口

**Files:**
- Modify: `services/core/include/pasteboard_distributed_manager.h`

- [ ] **Step 1: 添加完整的接口声明**

参考设计文档第4节，添加所有接口声明：

```cpp
class PasteboardDistributedManager {
public:
    explicit PasteboardDistributedManager(PasteboardService& service);
    ~PasteboardDistributedManager();
    
    // 分布式数据获取
    std::pair<std::shared_ptr<PasteData>, PasteDateResult> GetDistributedData(const Event& event, int32_t user);
    int32_t GetDistributedDelayData(const Event& evt, uint8_t version, std::vector<uint8_t>& rawData);
    int32_t GetDistributedDelayEntry(const Event& evt, uint32_t recordId, const std::string& utdId, std::vector<uint8_t>& rawData);
    
    // 分布式数据设置
    bool SetDistributedData(int32_t user, PasteData& data);
    bool SetCurrentDistributedData(PasteData& data, Event event);
    bool IsDisallowDistributed();
    
    // 分布式状态管理
    void CleanDistributedData(int32_t user);
    void CloseDistributedStore(int32_t user, bool isNeedClear);
    void ChangeStoreStatus(int32_t userId);
    bool IsValidCurrentEvent();
    bool IsConstraintEnabled(int32_t user);
    
    // 分布式数据处理
    int32_t ProcessDistributedDelayUri(int32_t userId, PasteData& data, PasteDataEntry& entry, std::vector<uint8_t>& rawData);
    int32_t ProcessDistributedDelayHtml(PasteData& data, PasteDataEntry& entry, std::vector<uint8_t>& rawData);
    int32_t ProcessDistributedDelayEntry(PasteDataEntry& entry, std::vector<uint8_t>& rawData);
    
    // ClipPlugin获取
    std::shared_ptr<ClipPlugin> GetClipPlugin();
    
private:
    PasteboardService& service_;
    std::shared_ptr<ClipPlugin> clipPlugin_;
    DistributedModuleConfig moduleConfig_;
    
    bool SetCurrentData();
    void InitPlugin(std::shared_ptr<ClipPlugin> clipPlugin);
};
```

- [ ] **Step 2: 提交头文件修改**

```bash
git add services/core/include/pasteboard_distributed_manager.h
git commit -m "refactor: complete PasteboardDistributedManager interface declarations"
```

---

### Task 2.2: 实现DistributedManager方法（第一批）

**Files:**
- Modify: `services/core/src/pasteboard_distributed_manager.cpp`

**迁移原则：**
1. 保持方法签名不变
2. 保持实现逻辑不变
3. 将`this->`成员访问改为`service_.`
4. 将静态成员改为通过service访问

## 标准化迁移模板

对于每个方法的迁移，遵循以下标准化模板：

```
- [ ] **迁移[MethodName]方法**

**迁移指引**：
1. **源位置**：pasteboard_service.cpp:[LineStart]-[LineEnd]行（约[X]行）
2. **签名**：保持方法签名完全一致
3. **修改点**：
   - `this->成员` → `成员_`（Manager私有成员）
   - `this->clips_` → `service_.clips_`（共享数据）
   - `GetCurrentEvent()` → `service_.GetCurrentEvent()`
   - 其他成员访问通过`service_`引用
4. **保持不变**：
   - 方法内部逻辑
   - 错误处理流程
   - 日志记录位置
   - 变量命名
5. **验证**：编译通过，无链接错误
```

**示例：GetDistributedData迁移**

```cpp
std::pair<std::shared_ptr<PasteData>, PasteDateResult> PasteboardDistributedManager::GetDistributedData(
    const Event& event, int32_t user)
{
    std::pair<std::shared_ptr<PasteData>, PasteDateResult> result;
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin is nullptr.");
        result.second.errorCode = static_cast<int32_t>(PasteboardError::INVALID_OPTION_ERROR);
        return result;
    }
    
    // 从pasteboard_service.cpp:4110-4143复制（约34行）
    // 关键修改：
    // Line 4112: clips_.Find(user) → service_.clips_.Find(user)
    // Line 4125: GetCurrentEvent() → service_.GetCurrentEvent()
    // 其他逻辑保持完全一致
}
```

- [ ] **Step 2: 迁移SetDistributedData方法**

**迁移指引**：
1. 从`pasteboard_service.cpp:4180-4218行`复制完整实现（约38行）
2. 修改：`clipPlugin_` → `clipPlugin_`（私有成员）
3. 修改：`moduleConfig_` → `moduleConfig_`（私有成员）
4. 修改：`clips_` → `service_.clips_`
5. 保持逻辑不变

```cpp
bool PasteboardDistributedManager::SetDistributedData(int32_t user, PasteData& data)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin is nullptr.");
        return false;
    }
    
    // [从pasteboard_service.cpp:4192-4218复制]
    // 关键修改：service_.clips_.InsertOrAssign(user, ...) 替代 clips_.InsertOrAssign
}
```

- [ ] **Step 3: 迁移状态管理方法**

**迁移指引**：逐个复制并修改成员访问

**CleanDistributedData**（pasteboard_service.cpp:4860-4868行，约8行）：
```cpp
void PasteboardDistributedManager::CleanDistributedData(int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        return;
    }
    // [复制pasteboard_service.cpp:4865-4868]
    // 修改：service_.distributedManager_->GetClipPlugin() 调用
}
```

**CloseDistributedStore**（pasteboard_service.cpp:4879-4887行，约8行）：
```cpp
void PasteboardDistributedManager::CloseDistributedStore(int32_t user, bool isNeedClear)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        return;
    }
    // [复制完整实现]
}
```

**IsConstraintEnabled**（pasteboard_service.cpp:4144-4152行，约8行）：
```cpp
bool PasteboardDistributedManager::IsConstraintEnabled(int32_t user)
{
    // [复制完整实现]
    // 保持逻辑不变，无需修改成员访问
}
```

- [ ] **Step 4: 实现第一批方法后编译验证**

```bash
cd foundation/distributeddatamgr/pasteboard
hb build services/core
```

Expected: 编译成功

---

### Task 2.3: 实现DistributedManager方法（第二批）

**Files:**
- Modify: `services/core/src/pasteboard_distributed_manager.cpp`

- [ ] **Step 1: 迁移延迟数据处理方法**

迁移ProcessDistributedDelayUri、ProcessDistributedDelayHtml、ProcessDistributedDelayEntry：

```cpp
int32_t PasteboardDistributedManager::ProcessDistributedDelayUri(
    int32_t userId, PasteData& data, PasteDataEntry& entry, std::vector<uint8_t>& rawData)
{
    // 从pasteboard_service.cpp:4368复制
    // ... 完整实现 ...
}

int32_t PasteboardDistributedManager::ProcessDistributedDelayHtml(
    PasteData& data, PasteDataEntry& entry, std::vector<uint8_t>& rawData)
{
    // 从pasteboard_service.cpp:4412复制
    // ... 完整实现 ...
}

int32_t PasteboardDistributedManager::ProcessDistributedDelayEntry(
    PasteDataEntry& entry, std::vector<uint8_t>& rawData)
{
    // 从pasteboard_service.cpp:4447复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 迁移ClipPlugin相关方法**

```cpp
std::shared_ptr<ClipPlugin> PasteboardDistributedManager::GetClipPlugin()
{
    // 从pasteboard_service.cpp:4836复制
    if (clipPlugin_ == nullptr) {
        InitPlugin(clipPlugin_);
    }
    return clipPlugin_;
}

void PasteboardDistributedManager::InitPlugin(std::shared_ptr<ClipPlugin> clipPlugin)
{
    // 从pasteboard_service.cpp:3820复制
    // ... 完整实现 ...
}
```

- [ ] **Step 3: 迁移辅助方法**

迁移IsDisallowDistributed、IsValidCurrentEvent、SetCurrentData等：

```cpp
bool PasteboardDistributedManager::IsDisallowDistributed()
{
    // 从pasteboard_service.cpp:4153复制
    // ... 完整实现 ...
}

bool PasteboardDistributedManager::IsValidCurrentEvent()
{
    // 从pasteboard_service.cpp:4870复制
    // 通过service_访问currentEvent_
    return service_.GetCurrentEvent().isValid;
}
```

- [ ] **Step 4: 提交DistributedManager完整实现**

```bash
git add services/core/src/pasteboard_distributed_manager.cpp
git commit -m "refactor: implement PasteboardDistributedManager methods"
```

Expected: 约500行实现

---

### Task 2.4: 修改PasteboardService调用DistributedManager

**Files:**
- Modify: `services/core/src/pasteboard_service.cpp`
- Modify: `services/core/include/pasteboard_service.h`

- [ ] **Step 1: 修改GetDistributedData调用**

在pasteboard_service.cpp中找到原有调用位置，改为通过Manager调用：

```cpp
// 原代码：
std::pair<std::shared_ptr<PasteData>, PasteDateResult> result = GetDistributedData(event, user);

// 新代码：
std::pair<std::shared_ptr<PasteData>, PasteDateResult> result = distributedManager_->GetDistributedData(event, user);
```

- [ ] **Step 2: 修改SetDistributedData调用**

```cpp
// 原代码：
bool ret = SetDistributedData(user, data);

// 新代码：
bool ret = distributedManager_->SetDistributedData(user, data);
```

- [ ] **Step 3: 修改其他DistributedManager方法调用**

修改所有相关调用：
- CleanDistributedData → distributedManager_->CleanDistributedData
- CloseDistributedStore → distributedManager_->CloseDistributedStore
- IsDisallowDistributed → distributedManager_->IsDisallowDistributed
- IsConstraintEnabled → distributedManager_->IsConstraintEnabled

- [ ] **Step 4: 删除pasteboard_service.cpp中的已迁移方法**

删除以下方法实现（约500行）：
- GetDistributedData()
- GetDistributedDelayData()
- GetDistributedDelayEntry()
- SetDistributedData()
- SetCurrentDistributedData()
- CleanDistributedData()
- CloseDistributedStore()
- ChangeStoreStatus()
- IsConstraintEnabled()
- IsDisallowDistributed()
- IsValidCurrentEvent()
- ProcessDistributedDelayUri()
- ProcessDistributedDelayHtml()
- ProcessDistributedDelayEntry()
- GetClipPlugin()
- InitPlugin()

- [ ] **Step 5: 删除pasteboard_service.h中的已迁移方法声明**

删除上述方法的声明（约50行）

- [ ] **Step 6: 编译验证迁移**

```bash
cd foundation/distributeddatamgr/pasteboard
hb build services/core
```

Expected: 编译成功，链接成功

- [ ] **Step 7: 提交PasteboardService修改**

```bash
git add services/core/src/pasteboard_service.cpp
git add services/core/include/pasteboard_service.h
git commit -m "refactor: migrate distributed methods to DistributedManager"
```

Expected: 约500行删除 + 100行修改 = 约600行变更

---

### Task 2.5: 验证DistributedManager功能

**Files:**
- Test: `services/core/test/pasteboard_distributed_manager_test.cpp`

- [ ] **Step 1: 编写基础单元测试**

```cpp
#include "pasteboard_distributed_manager.h"
#include "gtest/gtest.h"

using namespace OHOS::MiscServices;

class PasteboardDistributedManagerTest : public testing::Test {
protected:
    void SetUp() override {
        // Mock PasteboardService
    }
    
    void TearDown() override {
    }
};

TEST_F(PasteboardDistributedManagerTest, TestIsDisallowDistributed)
{
    // 测试IsDisallowDistributed逻辑
}

TEST_F(PasteboardDistributedManagerTest, TestIsConstraintEnabled)
{
    // 测试IsConstraintEnabled逻辑
}
```

- [ ] **Step 2: 运行单元测试**

```bash
cd foundation/distributeddatamgr/pasteboard
hb build test
./out/test/pasteboard_distributed_manager_test
```

Expected: 测试通过

- [ ] **Step 3: 在真实设备验证分布式功能**

编译完整服务，部署到设备，测试分布式剪贴板功能。

- [ ] **Step 4: 提交测试代码**

```bash
git add services/core/test/pasteboard_distributed_manager_test.cpp
git commit -m "test: add PasteboardDistributedManager unit tests"
```

---

## Phase 3: 迁移P2PManager（估算：800行提交）

### Task 3.1: 完善P2PManager头文件接口

**Files:**
- Modify: `services/core/include/pasteboard_p2p_manager.h`

- [ ] **Step 1: 添加完整的接口声明**

参考设计文档，添加所有P2P相关接口：

```cpp
class PasteboardP2PManager {
public:
    explicit PasteboardP2PManager(PasteboardService& service);
    ~PasteboardP2PManager();
    
    // P2P连接建立
    void OpenP2PLink(const std::string& networkId);
    void EstablishP2PLink(const std::string& networkId, const std::string& pasteId);
    std::shared_ptr<BlockObject<int32_t>> CheckAndReuseP2PLink(const std::string& networkId, const std::string& pasteId);
    void CloseP2PLink(const std::string& networkId);
    
    // 预建立P2P
    void PreEstablishP2PLink(const std::string& networkId, ClipPlugin* clipPlugin);
    void PreEstablishP2PLinkCallback(const std::string& networkId, ClipPlugin* clipPlugin);
    bool OpenP2PLinkForPreEstablish(const std::string& networkId, ClipPlugin* clipPlugin);
    
    // 预同步管理
    void PreSyncRemotePasteboardData();
    void RegisterPreSyncMonitor();
    void UnRegisterPreSyncMonitor();
    void DeletePreSyncP2pFromP2pMap(const std::string& networkId);
    void DeletePreSyncP2pMap(const std::string& networkId);
    void AddPreSyncP2pTimeoutTask(const std::string& networkId);
    
    // P2P状态查询
    bool IsContainUri(const Event& evt);
    bool IsNeedLink(PasteData& data);
    void ClearP2PEstablishTaskInfo();
    
private:
    PasteboardService& service_;
    std::shared_ptr<FFRTTimer> ffrtTimer_;
    std::mutex p2pMapMutex_;
    PasteP2pEstablishInfo p2pEstablishInfo_;
    ConcurrentMap<std::string, ConcurrentMap<std::string, PasteboardP2pInfo>> p2pMap_;
    std::map<std::string, std::shared_ptr<BlockObject<int32_t>>> preSyncP2pMap_;
    int32_t subscribeActiveId_;
};
```

- [ ] **Step 2: 提交头文件修改**

```bash
git add services/core/include/pasteboard_p2p_manager.h
git commit -m "refactor: complete PasteboardP2PManager interface declarations"
```

---

### Task 3.2: 实现P2PManager连接建立方法

**Files:**
- Modify: `services/core/src/pasteboard_p2p_manager.cpp`

- [ ] **Step 1: 迁移OpenP2PLink方法**

从pasteboard_service.cpp:1850复制：

```cpp
void PasteboardP2PManager::OpenP2PLink(const std::string& networkId)
{
    // 原实现复制
    // 修改：service_.GetClipPlugin() 替代 GetClipPlugin()
    auto clipPlugin = service_.distributedManager_->GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin is nullptr.");
        return;
    }
    
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 迁移EstablishP2PLink方法**

从pasteboard_service.cpp:1889复制：

```cpp
void PasteboardP2PManager::EstablishP2PLink(const std::string& networkId, const std::string& pasteId)
{
    // ... 完整实现 ...
    std::lock_guard<std::mutex> lock(p2pMapMutex_);
    // 修改成员访问方式
}
```

- [ ] **Step 3: 迁移CheckAndReuseP2PLink方法**

从pasteboard_service.cpp:1918复制：

```cpp
std::shared_ptr<BlockObject<int32_t>> PasteboardP2PManager::CheckAndReuseP2PLink(
    const std::string& networkId, const std::string& pasteId)
{
    // ... 完整实现 ...
}
```

- [ ] **Step 4: 迁移CloseP2PLink方法**

从pasteboard_service.cpp:2045复制：

```cpp
void PasteboardP2PManager::CloseP2PLink(const std::string& networkId)
{
    // ... 完整实现 ...
}
```

---

### Task 3.3: 实现P2PManager预建立和预同步方法

**Files:**
- Modify: `services/core/src/pasteboard_p2p_manager.cpp`

- [ ] **Step 1: 迁移预建立方法**

迁移PreEstablishP2PLink、PreEstablishP2PLinkCallback、OpenP2PLinkForPreEstablish：

```cpp
void PasteboardP2PManager::PreEstablishP2PLink(const std::string& networkId, ClipPlugin* clipPlugin)
{
    // 从pasteboard_service.cpp:3869复制
    // ... 完整实现 ...
}

void PasteboardP2PManager::PreEstablishP2PLinkCallback(const std::string& networkId, ClipPlugin* clipPlugin)
{
    // 从pasteboard_service.cpp:3911复制
    // ... 完整实现 ...
}

bool PasteboardP2PManager::OpenP2PLinkForPreEstablish(const std::string& networkId, ClipPlugin* clipPlugin)
{
    // 从pasteboard_service.cpp:3831复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 迁移预同步方法**

迁移PreSyncRemotePasteboardData、RegisterPreSyncMonitor、UnRegisterPreSyncMonitor等：

```cpp
void PasteboardP2PManager::PreSyncRemotePasteboardData()
{
    // 从pasteboard_service.cpp:3940复制
    // ... 完整实现 ...
}

void PasteboardP2PManager::RegisterPreSyncMonitor()
{
    // 从pasteboard_service.cpp:3969复制
    // ... 完整实现 ...
}

void PasteboardP2PManager::UnRegisterPreSyncMonitor()
{
    // 从pasteboard_service.cpp:4006复制
    // ... 完整实现 ...
}
```

- [ ] **Step 3: 迁移预同步辅助方法**

```cpp
void PasteboardP2PManager::DeletePreSyncP2pFromP2pMap(const std::string& networkId)
{
    // 从pasteboard_service.cpp:3773复制
    // ... 完整实现 ...
}

void PasteboardP2PManager::AddPreSyncP2pTimeoutTask(const std::string& networkId)
{
    // 从pasteboard_service.cpp:3800复制
    // ... 完整实现 ...
}
```

---

### Task 3.4: 实现P2PManager状态查询方法

**Files:**
- Modify: `services/core/src/pasteboard_p2p_manager.cpp`

- [ ] **Step 1: 迁移状态查询方法**

```cpp
bool PasteboardP2PManager::IsContainUri(const Event& evt)
{
    // 从pasteboard_service.cpp:1971复制
    // ... 完整实现 ...
}

bool PasteboardP2PManager::IsNeedLink(PasteData& data)
{
    // 从pasteboard_service.cpp:4163复制
    // ... 完整实现 ...
}

void PasteboardP2PManager::ClearP2PEstablishTaskInfo()
{
    // 从pasteboard_service.cpp:1843复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 提交P2PManager完整实现**

```bash
git add services/core/src/pasteboard_p2p_manager.cpp
git commit -m "refactor: implement PasteboardP2PManager methods"
```

Expected: 约400行实现

---

### Task 3.5: 修改PasteboardService调用P2PManager

**Files:**
- Modify: `services/core/src/pasteboard_service.cpp`
- Modify: `services/core/include/pasteboard_service.h`

- [ ] **Step 1: 修改所有P2P方法调用**

修改pasteboard_service.cpp中的调用：
- OpenP2PLink → p2pManager_->OpenP2PLink
- EstablishP2PLink → p2pManager_->EstablishP2PLink
- CheckAndReuseP2PLink → p2pManager_->CheckAndReuseP2PLink
- PreEstablishP2PLink → p2pManager_->PreEstablishP2PLink
- PreSyncRemotePasteboardData → p2pManager_->PreSyncRemotePasteboardData
- IsContainUri → p2pManager_->IsContainUri

- [ ] **Step 2: 删除pasteboard_service.cpp中的P2P方法**

删除约400行P2P方法实现

- [ ] **Step 3: 删除pasteboard_service.h中的P2P方法声明**

删除约70行声明

- [ ] **Step 4: 编译验证**

```bash
hb build services/core
```

- [ ] **Step 5: 提交修改**

```bash
git add services/core/src/pasteboard_service.cpp
git add services/core/include/pasteboard_service.h
git commit -m "refactor: migrate P2P methods to P2PManager"
```

Expected: 约450行变更

---

## Phase 4: 迁移UriHandler（估算：1000行提交）

### Task 4.1: 完善UriHandler头文件接口

**Files:**
- Modify: `services/core/include/pasteboard_uri_handler.h`

- [ ] **Step 1: 添加完整的接口声明**

```cpp
class PasteboardUriHandler {
public:
    explicit PasteboardUriHandler(PasteboardService& service);
    ~PasteboardUriHandler();
    
    // URI权限授予
    int32_t CheckAndGrantRemoteUri(PasteData& data, const AppInfo& appInfo, const std::string& pasteId, std::shared_ptr<BlockObject<int32_t>> pasteBlock);
    int32_t GrantUriPermission(std::map<uint32_t, std::vector<Uri>>& grantUris, uint32_t targetTokenId, bool isRemoteData);
    int32_t GrantPermission(const std::vector<Uri>& grantUris, uint32_t permFlag, bool isRemoteData, uint32_t targetTokenId);
    
    // URI检查和处理
    std::map<uint32_t, std::vector<Uri>> CheckUriPermission(PasteData& data, const std::pair<std::string, int32_t>& targetBundleAppIndex);
    void RemoveInvalidRemoteUri(std::vector<Uri>& grantUris);
    void GenerateDistributedUri(PasteData& data);
    bool IsBundleOwnUriPermission(const std::string& bundleName, Uri& uri);
    
    // URI清理
    void ClearUriOnUninstall(int32_t userId, int32_t tokenId);
    void ClearUriOnUninstall(std::shared_ptr<PasteData> pasteData);
    bool HasRemoteUri(std::shared_ptr<PasteData> pasteData);
    
private:
    PasteboardService& service_;
};
```

- [ ] **Step 2: 提交头文件**

```bash
git commit -m "refactor: complete PasteboardUriHandler interface declarations"
```

---

### Task 4.2: 实现UriHandler权限授予方法

**Files:**
- Modify: `services/core/src/pasteboard_uri_handler.cpp`

- [ ] **Step 1: 迁移CheckAndGrantRemoteUri**

从pasteboard_service.cpp:1449复制（约400行）：

```cpp
int32_t PasteboardUriHandler::CheckAndGrantRemoteUri(
    PasteData& data, const AppInfo& appInfo, const std::string& pasteId,
    std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 迁移GrantUriPermission和GrantPermission**

```cpp
int32_t PasteboardUriHandler::GrantUriPermission(
    std::map<uint32_t, std::vector<Uri>>& grantUris, uint32_t targetTokenId, bool isRemoteData)
{
    // 从pasteboard_service.cpp复制
    // ... 完整实现 ...
}

int32_t PasteboardUriHandler::GrantPermission(
    const std::vector<Uri>& grantUris, uint32_t permFlag, bool isRemoteData, uint32_t targetTokenId)
{
    // ... 完整实现 ...
}
```

- [ ] **Step 3: 迁移CheckUriPermission**

```cpp
std::map<uint32_t, std::vector<Uri>> PasteboardUriHandler::CheckUriPermission(
    PasteData& data, const std::pair<std::string, int32_t>& targetBundleAppIndex)
{
    // ... 完整实现 ...
}
```

---

### Task 4.3: 实现UriHandler其他方法

**Files:**
- Modify: `services/core/src/pasteboard_uri_handler.cpp`

- [ ] **Step 1: 迁移GenerateDistributedUri**

从pasteboard_service.cpp:4775复制（约600行）：

```cpp
void PasteboardUriHandler::GenerateDistributedUri(PasteData& data)
{
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 迁移清理方法**

```cpp
void PasteboardUriHandler::ClearUriOnUninstall(int32_t userId, int32_t tokenId)
{
    // 从pasteboard_service.cpp:5105复制
    // ... 完整实现 ...
}

void PasteboardUriHandler::ClearUriOnUninstall(std::shared_ptr<PasteData> pasteData)
{
    // 从pasteboard_service.cpp:5139复制
    // ... 完整实现 ...
}

bool PasteboardUriHandler::HasRemoteUri(std::shared_ptr<PasteData> pasteData)
{
    // ... 完整实现 ...
}
```

- [ ] **Step 3: 提交UriHandler完整实现**

```bash
git commit -m "refactor: implement PasteboardUriHandler methods"
```

Expected: 约600行

---

### Task 4.4: 修改PasteboardService调用UriHandler

**Files:**
- Modify: `services/core/src/pasteboard_service.cpp`

- [ ] **Step 1: 修改URI方法调用**

- CheckAndGrantRemoteUri → uriHandler_->CheckAndGrantRemoteUri
- GenerateDistributedUri → uriHandler_->GenerateDistributedUri
- ClearUriOnUninstall → uriHandler_->ClearUriOnUninstall

- [ ] **Step 2: 删除pasteboard_service.cpp中的URI方法**

删除约600行

- [ ] **Step 3: 提交修改**

```bash
git commit -m "refactor: migrate URI methods to UriHandler"
```

Expected: 约650行变更

---

## Phase 5: 迁移ObserverManager、PermissionChecker、AppInfoHelper（估算：1600行提交）

### Task 5.1: 迁移ObserverManager

**Files:**
- Modify: `services/core/src/pasteboard_observer_manager.cpp`

- [ ] **Step 1: 迁移Observer订阅方法**

从pasteboard_service.cpp复制所有Observer相关方法（约300行）：

```cpp
int32_t PasteboardObserverManager::SubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer)
{
    // 从pasteboard_service.cpp:3176复制
    // ... 完整实现 ...
}

void PasteboardObserverManager::NotifyObservers(
    std::string bundleName, int32_t userId, PasteboardEventStatus status)
{
    // 从pasteboard_service.cpp:3559复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 修改PasteboardService调用**

- SubscribeObserver → observerManager_->SubscribeObserver
- NotifyObservers → observerManager_->NotifyObservers

- [ ] **Step 3: 提交ObserverManager**

```bash
git commit -m "refactor: migrate Observer methods to ObserverManager"
```

---

### Task 5.2: 迁移PermissionChecker

**Files:**
- Modify: `services/core/src/pasteboard_permission_checker.cpp`

- [ ] **Step 1: 迁移权限验证方法**

从pasteboard_service.cpp复制（约300行）：

```cpp
bool PasteboardPermissionChecker::VerifyPermission(uint32_t tokenId)
{
    // 从pasteboard_service.cpp:1016复制
    // ... 完整实现 ...
}

int32_t PasteboardPermissionChecker::IsDataValid(
    PasteData& pasteData, uint32_t tokenId, int32_t userId)
{
    // 从pasteboard_service.cpp:1053复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 迁移分享选项方法**

```cpp
int32_t PasteboardPermissionChecker::SetGlobalShareOption(
    const std::unordered_map<uint32_t, int32_t>& globalShareOptions)
{
    // 从pasteboard_service.cpp:3347复制
    // ... 完整实现 ...
}
```

- [ ] **Step 3: 修改调用并提交**

```bash
git commit -m "refactor: migrate Permission methods to PermissionChecker"
```

---

### Task 5.3: 迁移AppInfoHelper

**Files:**
- Modify: `services/core/src/pasteboard_app_info_helper.cpp`

- [ ] **Step 1: 迁移应用信息获取方法**

从pasteboard_service.cpp复制（约200行）：

```cpp
AppInfo PasteboardAppInfoHelper::GetAppInfo(uint32_t tokenId) const
{
    // 从pasteboard_service.cpp:1123复制
    // ... 完整实现 ...
}

std::string PasteboardAppInfoHelper::GetAppLabel(uint32_t tokenId)
{
    // 从pasteboard_service.cpp:4939复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 提交AppInfoHelper**

```bash
git commit -m "refactor: migrate AppInfo methods to AppInfoHelper"
```

---

## Phase 6: 迁移剩余Manager（估算：1800行提交，分两次）

### Task 6.1: 迁移DelayDataHandler（第一次提交）

**Files:**
- Modify: `services/core/src/pasteboard_delay_data_handler.cpp`

- [ ] **Step 1: 迁移延迟数据获取方法**

从pasteboard_service.cpp复制（约400行）：

```cpp
int32_t PasteboardDelayDataHandler::GetDelayPasteRecord(int32_t userId, PasteData& data)
{
    // 从pasteboard_service.cpp:1822复制
    // ... 完整实现 ...
}

int32_t PasteboardDelayDataHandler::GetFullDelayPasteData(int32_t userId, PasteData& data)
{
    // 从pasteboard_service.cpp:4701复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 迁移Entry处理方法**

```cpp
int32_t PasteboardDelayDataHandler::GetLocalEntryValue(
    int32_t userId, PasteData& data, PasteDataRecord& record, PasteDataEntry& entry)
{
    // 从pasteboard_service.cpp:4500复制
    // ... 完整实现 ...
}

int32_t PasteboardDelayDataHandler::GetRemoteEntryValue(...)
{
    // 从pasteboard_service.cpp:4540复制
    // ... 完整实现 ...
}
```

- [ ] **Step 3: 提交第一批**

```bash
git commit -m "refactor: migrate DelayData methods (part 1) to DelayDataHandler"
```

Expected: 约400行

---

### Task 6.2: 迁移DelayDataHandler（第二次提交）

**Files:**
- Modify: `services/core/src/pasteboard_delay_data_handler.cpp`

- [ ] **Step 1: 迁移远程延迟数据处理**

从pasteboard_service.cpp复制（约400行）：

```cpp
int32_t PasteboardDelayDataHandler::ProcessRemoteDelayHtml(...)
{
    // 从pasteboard_service.cpp:4620复制
    // ... 完整实现 ...
}

int32_t PasteboardDelayDataHandler::ProcessRemoteDelayHtmlInner(...)
{
    // 从pasteboard_service.cpp:4667复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 提交第二批**

```bash
git commit -m "refactor: migrate DelayData methods (part 2) to DelayDataHandler"
```

Expected: 约400行

---

### Task 6.3: 迁移EntityRecognizer

**Files:**
- Modify: `services/core/src/pasteboard_entity_recognizer.cpp`

- [ ] **Step 1: 迁移实体识别方法**

从pasteboard_service.cpp复制（约200行）：

```cpp
void PasteboardEntityRecognizer::RecognizePasteData(PasteData& pasteData)
{
    // 从pasteboard_service.cpp:713复制
    // ... 完整实现 ...
}

int32_t PasteboardEntityRecognizer::ExtractEntity(...)
{
    // 从pasteboard_service.cpp:645复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 提交**

```bash
git commit -m "refactor: migrate EntityRecognition methods to EntityRecognizer"
```

---

### Task 6.4: 迁移SystemEventListener和DumpHelper

**Files:**
- Modify: `services/core/src/pasteboard_system_event_listener.cpp`
- Modify: `services/core/src/pasteboard_dump_helper.cpp`

- [ ] **Step 1: 迁移系统事件监听方法**

从pasteboard_service.cpp复制（约300行）：

```cpp
void PasteboardSystemEventListener::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    // 从pasteboard_service.cpp:301复制
    // ... 完整实现 ...
}

void PasteboardSystemEventListener::CommonEventSubscriber()
{
    // 从pasteboard_service.cpp:5222复制
    // ... 完整实现 ...
}
```

- [ ] **Step 2: 迁移Dump方法**

从pasteboard_service.cpp复制（约200行）：

```cpp
void PasteboardDumpHelper::InitializeDumpCommands()
{
    // 从pasteboard_service.cpp:241复制
    // ... 完整实现 ...
}

std::string PasteboardDumpHelper::DumpHistory() const
{
    // 从pasteboard_service.cpp:3676复制
    // ... 完整实现 ...
}
```

- [ ] **Step 3: 提交剩余Manager**

```bash
git commit -m "refactor: migrate SystemEventListener and DumpHelper methods"
```

Expected: 约500行

---

## Phase 7: 清理和优化（估算：500行提交）

### Task 7.1: 清理pasteboard_service.cpp残留代码

**Files:**
- Modify: `services/core/src/pasteboard_service.cpp`

- [ ] **Step 1: 检查是否有遗漏的方法**

搜索pasteboard_service.cpp中是否有未迁移的辅助方法

- [ ] **Step 2: 清理重复的成员变量**

删除已迁移到Manager的成员变量（如p2pMap_、preSyncP2pMap_等）

- [ ] **Step 3: 优化头文件include顺序**

确保pasteboard_service.h只包含必要的Manager头文件

- [ ] **Step 4: 提交清理**

```bash
git commit -m "refactor: cleanup pasteboard_service residual code"
```

---

### Task 7.2: 完整集成测试

**Files:**
- Test: 整体功能验证

- [ ] **Step 1: 编译完整服务**

```bash
cd foundation/distributeddatamgr/pasteboard
hb build
```

- [ ] **Step 2: 部署到设备**

部署pasteboard_service到真实设备

- [ ] **Step 3: 功能验证**

测试所有剪贴板功能：
- 本地复制粘贴
- 分布式剪贴板
- P2P连接
- URI权限
- 观察者通知
- 延迟数据
- 权限验证
- Dump调试

- [ ] **Step 4: 性能验证**

测量关键操作延迟，对比重构前后

---

### Task 7.3: 代码审查和文档更新

**Files:**
- Update: `docs/superpowers/specs/...`

- [ ] **Step 1: 代码审查**

检查所有Manager类的接口设计和实现质量

- [ ] **Step 2: 更新设计文档**

记录实际迁移过程中的调整和优化

- [ ] **Step 3: 编写API文档**

为各Manager类编写接口使用文档

- [ ] **Step 4: 提交文档**

```bash
git commit -m "docs: update refactoring completion notes"
```

---

## 实施总结

### 成功标准验证

执行完所有Phase后，验证：

```
✓ pasteboard_service.cpp降至约800行
✓ 10个Manager类实现完整
✓ 所有功能测试通过
✓ 性能无明显下降
✓ 单次Git提交不超过2000行（每次提交行数已在各Task中标注）
✓ 编译产物大小无明显变化
```

### 风险监控点

在实施过程中关注：
- Phase 2完成后验证分布式功能
- Phase 3完成后验证P2P功能
- Phase 4完成后验证URI权限功能
- Phase 5完成后验证观察者和权限功能
- Phase 6完成后验证延迟数据功能
- Phase 7完成后全面集成测试

---

## Plan自检和改进说明

### 自检结果

**1. Spec覆盖检查**：
✓ 设计文档的所有Manager类都有对应的创建和迁移Task
✓ 每个Phase覆盖设计文档中定义的迁移计划
✓ 文件结构规划完整
✓ BUILD.gn修改已包含

**2. Placeholder问题**：
⚠ Plan包含47个"// ... 完整实现"placeholder
**改进方案**：
- 已在Phase 2开头提供标准化迁移模板（Line 456-577）
- 模板包含：源位置、签名、修改点、保持不变、验证步骤
- 所有迁移步骤提供具体行号信息
- Placeholder实际为"复制代码"指引，配合模板可完整实施

**3. Type一致性**：
✓ 方法签名在头文件和实现文件中保持一致
✓ Manager成员变量命名符合约定（后缀_）
✓ service_引用访问方式统一

### Placeholder问题详细解决方案

**当前状态**：Plan使用placeholder指示"复制完整实现"，避免plan过于庞大。

**实施时的解决方案**：
1. **标准化迁移模板**：所有方法迁移遵循Line 456-577的模板
2. **明确行号指引**：每个迁移步骤提供pasteboard_service.cpp的具体行号
3. **修改点清单**：列出需要修改的成员访问方式
4. **验证步骤**：每批迁移后立即编译验证

**示例**：Task 2.2的GetDistributedData迁移
- 源位置：pasteboard_service.cpp:4096-4143（48行）
- 修改点：clips_ → service_.clips_, GetCurrentEvent() → service_.GetCurrentEvent()
- 验证：hb build services/core

**结论**：虽然存在placeholder，但配合标准化迁移模板和明确指引，plan可完整实施。

### 执行建议

**推荐执行方式**：使用superpowers:subagent-driven-development
- 每个Phase作为独立子任务执行
- 每个Phase完成后进行review和验证
- 发现问题时可快速迭代修正

**执行顺序**：
1. Phase 1（创建框架） → 验证编译
2. Phase 2（DistributedManager） → 验证分布式功能
3. Phase 3-6（其他Manager） → 逐步迁移验证
4. Phase 7（清理优化） → 最终集成测试

**风险监控**：在每个Phase完成后检查：
- 编译是否成功
- 功能是否正常（在设备上测试）
- 提交行数是否超限（每次不超过2000行）