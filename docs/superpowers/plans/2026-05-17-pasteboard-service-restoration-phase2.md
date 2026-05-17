# 剪贴板服务恢复第二阶段实施计划

> **对于代理工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 来逐任务执行此计划。步骤使用复选框 (`- [ ]`) 语法进行跟踪。

**目标：** 恢复所有 051f0fe8 功能的第二阶段，实现完整的多用户剪贴板支持

**架构：** 保持第一阶段已恢复的 UserContextResolver 依赖注入架构，新增 userId 参数化方法和内存管理恢复

**技术栈：** C++17, OHOS PasteboardService, InputMethodController, WindowManager, AccessTokenKit

---

## 文件结构

**需要修改的文件：**
- `services/core/include/pasteboard_service.h` - 新增 userId 参数方法声明，恢复 readTokens_
- `services/core/src/pasteboard_service.cpp` - 新增 userId 参数方法实现，恢复 GrantPermission tokenId 参数，恢复 RefreshCriticalState 调用（8处），焦点窗口检查 userId 版本
- `services/test/unittest/src/pasteboard_service_check_test.cpp` - 无需修改（第一阶段已兼容）

**无需修改的文件：**
- `services/core/include/pasteboard_window_manager.h` - 通过 GetInstance(userId) 传入参数
- `services/core/src/pasteboard_window_manager.cpp` - 通过 GetInstance(userId) 传入参数

---

## 任务分解（按优先级）

### 任务 1：RefreshCriticalState 调用恢复（优先级1 - 关键内存管理）

**文件：**
- Modify: `services/core/src/pasteboard_service.cpp`（8处调用）

**目标：** 在 8 个关键方法中恢复 RefreshCriticalState() 调用，确保剪贴板服务在有工作时不会被系统杀掉

- [ ] **步骤 1：在 NotifyDelayGetterDied 中添加调用**

找到 `void PasteboardService::NotifyDelayGetterDied(int32_t userId)` 方法（约第343行），在 `delayGetters_.Erase(userId);` 之后添加：

```cpp
void PasteboardService::NotifyDelayGetterDied(int32_t userId)
{
    if (userId == ERROR_USERID) {
        return;
    }
    delayGetters_.Erase(userId);
    RefreshCriticalState();  // 新增：通知系统内存状态变化
}
```

- [ ] **步骤 2：在 NotifyEntryGetterDied 中添加调用**

找到 `void PasteboardService::NotifyEntryGetterDied(int32_t userId)` 方法，在 `entryGetters_.Erase(userId);` 之后添加：

```cpp
void PasteboardService::NotifyEntryGetterDied(int32_t userId)
{
    if (userId == ERROR_USERID) {
        return;
    }
    entryGetters_.Erase(userId);
    RefreshCriticalState();  // 新增：通知系统内存状态变化
}
```

- [ ] **步骤 3：在 ClearInner 中添加调用和检查**

找到 `int32_t PasteboardService::ClearInner(int32_t userId, const AppInfo &appInfo)` 方法，在 `CleanDistributedData(userId);` 之后添加：

```cpp
int32_t PasteboardService::ClearInner(int32_t userId, const AppInfo &appInfo)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInner: userId=%{public}d, bundleName=%{public}s",
        userId, appInfo.bundleName.c_str());
    RADAR_REPORT(DFX_CLEAR_PASTEBOARD, DFX_MANUAL_CLEAR, DFX_SUCCESS);
    auto [hasData, data] = clips_.Find(userId);
    if (hasData) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInner: found data for userId=%{public}d, erasing", userId);
        clips_.Erase(userId);
        delayDataId_ = 0;
        delayTokenId_ = 0;
    }
    CleanDistributedData(userId);
    RefreshCriticalState();  // 新增：通知系统内存状态变化
    if (hasData) {
        std::string bundleName = GetAppBundleName(appInfo);
        NotifyObservers(bundleName, userId, PasteboardEventStatus::PASTEBOARD_CLEAR);
    }
    if (!HasActivePasteboardWork()) {  // 新增：检查是否还有活跃工作
        CancelCriticalTimer();
    }
    CancelCriticalTimer();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInner leave: clips_.Size=%{public}zu, userId=%{public}d",
        clips_.Size(), userId);
    return ERR_OK;
}
```

- [ ] **步骤 4：在 GetRemoteData 中添加调用**

找到 `int32_t PasteboardService::GetRemoteData(...)` 方法，在 `taskMgr_.ClearRemoteDataTask(event);` 之后添加：

```cpp
int32_t PasteboardService::GetRemoteData(int32_t userId, const Event &event, PasteData &data, int32_t &syncTime)
{
    // ... existing code ...
    if (result.first != nullptr) {
        auto [hasData, existingData] = clips_.Find(userId);
        if (!hasData) {
            clips_.InsertOrAssign(userId, result.first);
        }
        ret = static_cast<int32_t>(PasteboardError::E_OK);
    }
    taskMgr_.ClearRemoteDataTask(event);
    RefreshCriticalState();  // 新增：通知系统内存状态变化
    return ret;
}
```

- [ ] **步骤 5：在 GetRemotePasteData 中添加调用（两处）**

找到 `int32_t PasteboardService::GetRemotePasteData(...)` 方法，在两个位置添加调用：

```cpp
int32_t PasteboardService::GetRemotePasteData(int32_t userId, const Event &event, PasteData &data, int32_t &syncTime)
{
    // ... existing code ...
    if (result.first != nullptr) {
        auto [hasData, existingData] = clips_.Find(userId);
        if (!hasData) {
            clips_.InsertOrAssign(userId, result.first);
            auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
            copyTime_.InsertOrAssign(userId, curTime);
            SetDataExpirationTimer(userId);
            RefreshCriticalState();  // 新增：数据插入后通知系统
        }
        pasteDataTime->syncTime = result.second.syncTime;
        pasteDataTime->data = result.first;
        pasteDataTime->errorCode = result.second.errorCode;
        taskMgr_.Notify(event, pasteDataTime);
    }
    block->SetValue(pasteDataTime);
    taskMgr_.ClearRemoteDataTask(event);
    RefreshCriticalState();  // 新增：任务清理后通知系统
    // ... rest of code ...
}
```

- [ ] **步骤 6：在 SaveData 中添加调用**

找到 `int32_t PasteboardService::SaveData(...)` 方法，在 `SetDataExpirationTimer(appInfo.userId);` 之后添加：

```cpp
int32_t PasteboardService::SaveData(PasteData &pasteData, int64_t dataSize, const AppInfo &appInfo)
{
    // ... existing code ...
    auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    copyTime_.InsertOrAssign(appInfo.userId, curTime);
    SetDataExpirationTimer(appInfo.userId);
    RefreshCriticalState();  // 新增：数据保存后通知系统
    if (!(pasteData.IsDelayData())) {
        SetDistributedData(appInfo.userId, pasteData);
        NotifyObservers(appInfo.bundleName, appInfo.userId, PasteboardEventStatus::PASTEBOARD_WRITE);
    }
    // ... rest of code ...
}
```

- [ ] **步骤 7：在 ClearAgedData 中添加调用**

找到 `void PasteboardService::ClearAgedData(int32_t userId)` 方法，在 `copyTime_.Erase(userId);` 之后添加：

```cpp
void PasteboardService::ClearAgedData(int32_t userId)
{
    // ... existing code ...
    if (curTime - copyTime.second < agedTime_.load()) {
        return;
    }
    clips_.Erase(userId);
    if (data->GetRecordCount() > 0 && data->GetRecord(0)->GetTag() == PasteDataRecord::TAG_URI) {
        delayDataId_ = 0;
        delayTokenId_ = 0;
    }
    copyTime_.Erase(userId);
    RefreshCriticalState();  // 新增：过期数据清理后通知系统
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "data is out of the time");
    RADAR_REPORT(DFX_CLEAR_PASTEBOARD, DFX_AUTO_CLEAR, DFX_SUCCESS);
}
```

- [ ] **步骤 8：在 ClearUriOnUninstall(userId版本) 中添加调用**

找到 `void PasteboardService::ClearUriOnUninstall(int32_t userId, int32_t tokenId)` 方法，在 lambda 内部的返回前添加：

```cpp
void PasteboardService::ClearUriOnUninstall(int32_t userId, int32_t tokenId)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE, "userId is invalid");
    clips_.ComputeIfPresent(userId, [this, tokenId, userId](auto, auto &pasteData) {
        if (pasteData == nullptr) {
            return true;
        }
        if (pasteData->GetTokenId() != static_cast<uint32_t>(tokenId)) {
            return true;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "clear uri, tokenId=%{public}d, userId=%{public}d", tokenId, userId);
        ClearUriOnUninstall(pasteData);
        delayGetters_.ComputeIfPresent(userId, [](auto, auto &delayGetter) {
            if (delayGetter.first != nullptr && delayGetter.second != nullptr) {
                delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
            }
            return false;
        });
        RefreshCriticalState();  // 新增：URI清理后通知系统
        return true;
    });
}
```

- [ ] **步骤 9：验证 RefreshCriticalState 调用**

编译代码验证所有调用点语法正确：

```bash
cd /home/luna/workspaces/foundation/distributeddatamgr/pasteboard
# 如果有编译脚本，运行编译验证语法
# 预期：编译通过，无语法错误
```

- [ ] **步骤 10：提交 RefreshCriticalState 恢复**

```bash
git add services/core/src/pasteboard_service.cpp
git commit -m "feat: restore RefreshCriticalState calls in 8 locations for memory management"
```

---

### 任务 2：GrantPermission tokenId 参数恢复（优先级1 - 关键内存管理）

**文件：**
- Modify: `services/core/include/pasteboard_service.h:370-375`（参数声明）
- Modify: `services/core/src/pasteboard_service.cpp`（方法实现和7处调用）

**目标：** 恢复 GrantPermission 和 GrantUriPermission 的 tokenId 参数，添加 HapTokenInfo 验证逻辑

- [ ] **步骤 1：修改 pasteboard_service.h 声明**

找到 GrantPermission 和 GrantUriPermission 方法声明（约第370-375行），修改参数：

```cpp
// pasteboard_service.h
// 修改前：
int32_t GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    const std::string &targetBundleName, int32_t appIndex);
int32_t GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    const std::string &targetBundleName, bool isRemoteData, int32_t appIndex);

// 修改后：
int32_t GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId);
int32_t GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId, bool isRemoteData);
```

同时修改 readBundles_ 成员变量（约第481行）：

```cpp
// 修改前：
std::set<std::pair<std::string, int32_t>> readBundles_;

// 修改后：
std::set<std::pair<int32_t, uint32_t>> readTokens_;
```

- [ ] **步骤 2：实现 GrantPermission tokenId 参数版本**

找到 GrantPermission 实现（约第2100行），完整替换：

```cpp
int32_t PasteboardService::GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetTokenId != 0, 
        static_cast<int32_t>(PasteboardError::INVALID_TOKEN_ID),
        PASTEBOARD_MODULE_SERVICE, "target token invalid");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetUserId != ERROR_USERID,
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR), PASTEBOARD_MODULE_SERVICE,
        "target user invalid");
    
    // 新增：HapTokenInfo验证
    HapTokenInfo targetInfo;
    int32_t tokenRet = AccessTokenKit::GetHapTokenInfo(targetTokenId, targetInfo);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(tokenRet == 0, 
        static_cast<int32_t>(PasteboardError::INVALID_TOKEN_ID),
        PASTEBOARD_MODULE_SERVICE, "get target hap token info failed, ret=%{public}d", tokenRet);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetInfo.userID == targetUserId,
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR), PASTEBOARD_MODULE_SERVICE,
        "target token user mismatch, tokenUser=%{public}d, targetUser=%{public}d", targetInfo.userID, targetUserId);
    
    size_t offset = 0;
    size_t length = grantUris.size();
    size_t count = PasteData::URI_BATCH_SIZE;
    bool hasGranted = false;
    int32_t permissionCode = 0;
    int32_t ret = 0;
    
    while (length > offset) {
        if (length - offset < PasteData::URI_BATCH_SIZE) {
            count = length - offset;
        }
        auto sendValues = std::vector<Uri>(grantUris.begin() + offset, grantUris.begin() + offset + count);
        if (isRemoteData) {
            permissionCode = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermissionPrivileged(
                sendValues, permFlag, targetInfo.bundleName, targetInfo.instIndex);
        } else {
            permissionCode = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermission(
                sendValues, permFlag, targetInfo.bundleName, targetInfo.instIndex, srcTokenId);
        }
        hasGranted = hasGranted || (permissionCode == 0);
        ret = permissionCode == 0 ? ret : permissionCode;
        offset += count;
    }
    
    if (hasGranted) {
        std::lock_guard<std::mutex> lock(readBundleMutex_);
        // 恢复：readTokens_
        if (readTokens_.count({ targetUserId, targetTokenId }) == 0) {
            readTokens_.insert({ targetUserId, targetTokenId });
        }
    }
    return ret;
}
```

- [ ] **步骤 3：实现 GrantUriPermission tokenId 参数版本**

找到 GrantUriPermission 实现，完整替换：

```cpp
int32_t PasteboardService::GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId, bool isRemoteData)
{
    std::vector<Uri> readUris = grantUris[PasteDataRecord::READ_PERMISSION];
    std::vector<Uri> writeUris = grantUris[PasteDataRecord::READ_WRITE_PERMISSION];
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!readUris.empty() || !writeUris.empty(), 
        static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no uri");
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "readUris=%{public}zu, writeUris=%{public}zu, targetToken=%{public}u, targetUser=%{public}d",
        readUris.size(), writeUris.size(), targetTokenId, targetUserId);
    
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(callingUid != ANCO_SERVICE_BROKER_UID, 
        static_cast<int32_t>(PasteboardError::E_OK), PASTEBOARD_MODULE_SERVICE, "callingUid = ANCO_SERVICE_BROKER_UID");
    
    if (isRemoteData) {
        RemoveInvalidRemoteUri(readUris);
        RemoveInvalidRemoteUri(writeUris);
    }
    
    auto permFlag = PasteDataRecord::READ_PERMISSION;
    int32_t ret = GrantPermission(readUris, permFlag, isRemoteData, targetTokenId, targetUserId, srcTokenId);
    if (!isRemoteData) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "NeedPersistance, permFlag is %{public}d", permFlag);
        permFlag = PasteDataRecord::READ_WRITE_PERMISSION;
    }
    auto result = GrantPermission(writeUris, permFlag, isRemoteData, targetTokenId, targetUserId, srcTokenId);
    ret = result == 0 ? ret : result;
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "leave, ret=%{public}d", ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
}
```

- [ ] **步骤 4：修改 GetRecordValueByType 调用点**

找到 `int32_t PasteboardService::GetRecordValueByType(...)` 方法（约第899行），修改调用：

```cpp
// 修改前：
return GrantUriPermission(grantUris, appInfo.bundleName, isRemoteData, appInfo.appIndex);

// 修改后：
uint32_t targetTokenId = IPCSkeleton::GetCallingTokenID();  // 从IPC获取
return GrantUriPermission(grantUris, targetTokenId, appInfo.userId, data->GetTokenId(), isRemoteData);
```

- [ ] **步骤 5：修改 ProcessDelayHtmlEntry 调用点**

找到 `int32_t PasteboardService::ProcessDelayHtmlEntry(...)` 方法，修改调用：

```cpp
// 修改前：
int32_t ret = GrantUriPermission(grantUris, targetBundle, isRemoteData, appIndex);

// 修改后：
uint32_t targetTokenId = IPCSkeleton::GetCallingTokenID();  // 从IPC获取
int32_t ret = GrantUriPermission(grantUris, targetTokenId, targetAppInfo.userId, data.GetTokenId(), isRemoteData);
```

- [ ] **步骤 6：修改 CheckAndGrantRemoteUri 调用点**

找到 `int32_t PasteboardService::CheckAndGrantRemoteUri(...)` 方法，修改调用：

```cpp
// 修改前：
return GrantUriPermission(grantUris, appInfo.bundleName, isRemoteData, appInfo.appIndex);

// 修改后：
uint32_t targetTokenId = IPCSkeleton::GetCallingTokenID();  // 从IPC获取
return GrantUriPermission(grantUris, targetTokenId, appInfo.userId, data->GetTokenId(), isRemoteData);
```

- [ ] **步骤 7：修改 ProcessRemoteDelayUri 调用点**

找到 `int32_t PasteboardService::ProcessRemoteDelayUri(...)` 方法，修改调用：

```cpp
// 修改前：
int32_t ret = GrantUriPermission(grantUris, appInfo.bundleName, data.IsRemote(), appInfo.appIndex);

// 修改后：
uint32_t targetTokenId = IPCSkeleton::GetCallingTokenID();  // 从IPC获取
int32_t ret = GrantUriPermission(grantUris, targetTokenId, appInfo.userId, data->GetTokenId(), data.IsRemote());
```

- [ ] **步骤 8：修改 ProcessRemoteDelayHtmlInner 调用点**

找到 `int32_t PasteboardService::ProcessRemoteDelayHtmlInner(...)` 方法，修改调用：

```cpp
// 修改前：
int32_t ret = GrantUriPermission(grantUris, appInfo.bundleName, data.IsRemote(), appInfo.appIndex);

// 修改后：
uint32_t targetTokenId = IPCSkeleton::GetCallingTokenID();  // 从IPC获取
int32_t ret = GrantUriPermission(grantUris, targetTokenId, appInfo.userId, data->GetTokenId(), data.IsRemote());
```

- [ ] **步骤 9：修改 CheckAndGrantUriPermission 调用点（如有）**

如果存在 `CheckAndGrantUriPermission` 方法，同样修改调用：

```cpp
// 修改后：
uint32_t targetTokenId = IPCSkeleton::GetCallingTokenID();  // 从IPC获取
return GrantUriPermission(grantUris, targetTokenId, appInfo.userId, data->GetTokenId(), isRemoteData);
```

- [ ] **步骤 10：验证 GrantPermission tokenId 恢复**

编译验证所有调用点语法正确：

```bash
cd /home/luna/workspaces/foundation/distributeddatamgr/pasteboard
# 预期：编译通过，无语法错误
```

- [ ] **步骤 11：提交 GrantPermission tokenId 恢复**

```bash
git add services/core/include/pasteboard_service.h services/core/src/pasteboard_service.cpp
git commit -m "feat: restore GrantPermission tokenId parameters and add HapTokenInfo validation"
```

---

### 任务 3：userId 参数化的输入法解冻方法（优先级2 - 功能完整性）

**文件：**
- Modify: `services/core/include/pasteboard_service.h`（新增 userId 参数版本声明）
- Modify: `services/core/src/pasteboard_service.cpp`（实现 userId 参数版本）

**目标：** 新增 userId 参数版本的 IsNeedThaw、IsCurrentImeByPid、GetDefaultInputMethod 方法

- [ ] **步骤 1：在 pasteboard_service.h 添加 userId 参数版本声明**

找到 IsNeedThaw 方法声明（约第303行），添加 userId 版本：

```cpp
// pasteboard_service.h
// 新增userId参数版本：
bool IsNeedThaw(PasteboardEventStatus status);                      // 保留：简化版本
bool IsNeedThaw(int32_t userId, PasteboardEventStatus status);  // 新增：userId版本

bool IsCurrentImeByPid(int32_t userId, pid_t callPid) const;      // 已在Phase 1恢复
int32_t GetDefaultInputMethod(int32_t userId, std::shared_ptr<Property> &property) const; // 已在Phase 1恢复
```

注：IsCurrentImeByPid 和 GetDefaultInputMethod 已在 Phase 1 添加声明，无需再次添加。

- [ ] **步骤 2：实现 IsNeedThaw 简化版本**

找到 `bool PasteboardService::IsNeedThaw(PasteboardEventStatus status)` 方法，修改为调用 userId 版本：

```cpp
bool PasteboardService::IsNeedThaw(PasteboardEventStatus status)
{
    // 简化版本：使用当前用户
    return IsNeedThaw(GetCurrentAccountId(), status);
}
```

- [ ] **步骤 3：实现 IsNeedThaw userId 版本**

在 IsNeedThaw 简化版本之后添加 userId 版本实现：

```cpp
bool PasteboardService::IsNeedThaw(int32_t userId, PasteboardEventStatus status)
{
    if (status == PasteboardEventStatus::PASTEBOARD_READ) {
        return false;
    }
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return false;
    }
    std::shared_ptr<Property> property;
    int32_t ret = GetDefaultInputMethod(userId, property);
    if (ret != ErrorCode::NO_ERROR || property == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "default input method is nullptr!");
        return false;
    }
    return true;
}
```

- [ ] **步骤 4：确认 IsCurrentImeByPid userId 版本实现**

Phase 1 已实现 IsCurrentImeByPid(userId, callPid)，确认存在：

```cpp
bool PasteboardService::IsCurrentImeByPid(int32_t userId, pid_t callPid) const
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return false;
    }
    (void)userId;
    auto isImePid = imc->IsCurrentImeByPid(callPid);
    return isImePid;
}
```

如果 userId 参数未传入 InputMethodController，修改为：

```cpp
bool PasteboardService::IsCurrentImeByPid(int32_t userId, pid_t callPid) const
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return false;
    }
    // InputMethodController的IsCurrentImeByPid现已支持userId参数
    auto isImePid = imc->IsCurrentImeByPid(callPid, userId);  // 传入userId
    return isImePid;
}
```

- [ ] **步骤 5：确认 GetDefaultInputMethod userId 版本实现**

Phase 1 已实现 GetDefaultInputMethod(userId, property)，确认存在：

```cpp
int32_t PasteboardService::GetDefaultInputMethod(int32_t userId, std::shared_ptr<Property> &property) const
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return -1;
    }
    (void)userId;
    return imc->GetDefaultInputMethod(property);
}
```

如果 userId 参数未传入 InputMethodController，修改为：

```cpp
int32_t PasteboardService::GetDefaultInputMethod(int32_t userId, std::shared_ptr<Property> &property) const
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return -1;
    }
    // InputMethodController的GetDefaultInputMethod现已支持userId参数
    return imc->GetDefaultInputMethod(property, userId);  // 传入userId，参数顺序：property在前
}
```

- [ ] **步骤 6：修改 SetInputMethodPid 使用 userId 版本**

找到 `void PasteboardService::SetInputMethodPid(int32_t userId, pid_t callPid)` 方法，确认使用 IsCurrentImeByPid(userId, callPid)：

```cpp
void PasteboardService::SetInputMethodPid(int32_t userId, pid_t callPid)
{
    auto isImePid = IsCurrentImeByPid(userId, callPid);  // 使用userId版本
    if (isImePid) {
        imeMap_.InsertOrAssign(userId, callPid);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "set inputMethod userId = %{public}d, pid = %{public}d",
            userId, callPid);
    }
}
```

如果未使用 userId 版本，修改上述代码。

- [ ] **步骤 7：修改 NotifyObservers 使用 userId 版本**

找到 `void PasteboardService::NotifyObservers(...)` 方法，确认调用 IsNeedThaw(userId, status)：

```cpp
void PasteboardService::NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status)
{
    auto [hasPid, pid] = imeMap_.Find(userId);
    if (hasPid && IsNeedThaw(userId, status)) {  // 使用userId版本
        ThawInputMethod(pid);
    }
    // ... rest of code
}
```

如果未使用 userId 版本，修改上述代码。

- [ ] **步骤 8：验证 userId 参数化的输入法方法**

编译验证：

```bash
cd /home/luna/workspaces/foundation/distributeddatamgr/pasteboard
# 预期：编译通过
```

- [ ] **步骤 9：提交 userId 参数化的输入法方法**

```bash
git add services/core/include/pasteboard_service.h services/core/src/pasteboard_service.cpp
git commit -m "feat: add userId-parameterized IME thaw methods (IsNeedThaw, IsCurrentImeByPid, GetDefaultInputMethod)"
```

---

### 任务 4：pasteboard_window_manager userId 参数版本（含焦点窗口检查）（优先级2 - 功能完整性）

**文件：**
- Modify: `services/core/include/pasteboard_service.h`（新增 userId 参数版本声明）
- Modify: `services/core/src/pasteboard_service.cpp`（实现 userId 参数版本焦点窗口检查）

**目标：** 通过 WindowManager::GetInstance(userId) 传入 userId 参数，实现 userId 参数版本的焦点窗口检查方法

- [ ] **步骤 1：在 pasteboard_service.h 添加 userId 参数版本声明**

找到 IsFocusedApp 和 GetFocusedAppInfo 方法声明（约第229-238行），添加 userId 版本：

```cpp
// pasteboard_service.h
bool IsFocusedApp(uint32_t tokenId);                      // 保留：简化版本
bool IsFocusedApp(uint32_t tokenId, int32_t userId);  // 新增：userId版本

FocusedAppInfo GetFocusedAppInfo(void) const;          // 保留：简化版本  
FocusedAppInfo GetFocusedAppInfo(int32_t userId) const; // 新增：userId版本
```

- [ ] **步骤 2：实现 IsFocusedApp 简化版本**

找到 `bool PasteboardService::IsFocusedApp(uint32_t tokenId)` 方法，修改为调用 userId 版本：

```cpp
bool PasteboardService::IsFocusedApp(uint32_t tokenId)
{
    // 简化版本：使用当前用户
    return IsFocusedApp(tokenId, GetCurrentAccountId());
}
```

- [ ] **步骤 3：实现 IsFocusedApp userId 版本**

在 IsFocusedApp 简化版本之后添加 userId 版本实现：

```cpp
bool PasteboardService::IsFocusedApp(uint32_t tokenId, int32_t userId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != ATokenTypeEnum::TOKEN_HAP) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "caller is not application");
        return true;
    }
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return false;
    }
    FocusChangeInfo info;
    // 通过GetInstance(userId)传入userId
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance(userId).GetFocusWindowInfo(info);
#else
    WindowManager::GetInstance(userId).GetFocusWindowInfo(info);
#endif
    auto callPid = IPCSkeleton::GetCallingPid();
    if (callPid == info.pid_) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "pid is same, it is focused app");
        return true;
    }
    bool isFocused = false;
    int32_t ret = PasteboardAbilityManager::CheckUIExtensionIsFocused(tokenId, isFocused);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "check result:%{public}d, isFocused:%{public}d", ret, isFocused);
    return ret == NO_ERROR && isFocused;
}
```

- [ ] **步骤 4：实现 GetFocusedAppInfo 简化版本**

找到 `FocusedAppInfo PasteboardService::GetFocusedAppInfo(void) const` 方法，修改为调用 userId 版本：

```cpp
FocusedAppInfo PasteboardService::GetFocusedAppInfo(void) const
{
    // 简化版本：使用当前用户
    return GetFocusedAppInfo(GetCurrentAccountId());
}
```

- [ ] **步骤 5：实现 GetFocusedAppInfo userId 版本**

在 GetFocusedAppInfo 简化版本之后添加 userId 版本实现：

```cpp
FocusedAppInfo PasteboardService::GetFocusedAppInfo(int32_t userId) const
{
    FocusedAppInfo appInfo = { 0 };
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return appInfo;
    }
    FocusChangeInfo info;
    std::vector<sptr<WindowVisibilityInfo>> windowVisibilityInfos;
    // 通过GetInstance(userId)传入userId
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance(userId).GetFocusWindowInfo(info);
    WMError result = WindowManagerLite::GetInstance(userId).GetVisibilityWindowInfo(windowVisibilityInfos);
#else
    WindowManager::GetInstance(userId).GetFocusWindowInfo(info);
    WMError result = WindowManager::GetInstance(userId).GetVisibilityWindowInfo(windowVisibilityInfos);
#endif
    if (result == WMError::WM_OK) {
        for (const auto& windowInfo : windowVisibilityInfos) {
            if (windowInfo == nullptr) {
                continue;
            }
            if (windowInfo->windowId_ == static_cast<uint32_t>(info.windowId_)) {
                appInfo.left = windowInfo->rect_.posX_;
                appInfo.top = windowInfo->rect_.posY_;
                appInfo.width = windowInfo->rect_.width_;
                appInfo.height = windowInfo->rect_.height_;
                appInfo.abilityToken = windowInfo->abilityToken_;
                break;
            }
        }
    }
    return appInfo;
}
```

- [ ] **步骤 6：修改 ShowProgress 使用 userId 版本**

找到 `int32_t PasteboardService::ShowProgress(...)` 方法，确认调用 GetFocusedAppInfo(void) 或 userId 版本：

```cpp
FocusedAppInfo appInfo = GetFocusedAppInfo();  // 简化版本，已调用userId版本
message.left = appInfo.left;
message.top = appInfo.top;
message.width = static_cast<int32_t>(appInfo.width);
message.height = static_cast<int32_t>(appInfo.height);
message.callerToken = appInfo.abilityToken;
```

无需修改，简化版本已调用 userId 版本。

- [ ] **步骤 7：修改 VerifyPermission 使用 userId 版本**

找到 `bool PasteboardService::VerifyPermission(uint32_t tokenId)` 方法，确认调用 IsFocusedApp(tokenId, userId) 或简化版本：

```cpp
bool PasteboardService::VerifyPermission(uint32_t tokenId)
{
    // ... existing code ...
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (inputEventCallback_ != nullptr) {
            isCtrlVAction = inputEventCallback_->IsCtrlVProcess(callPid, IsFocusedApp(tokenId));  // 简化版本
            inputEventCallback_->Clear();
        }
    }
    // ...
}
```

无需修改，简化版本已调用 userId 版本。

- [ ] **步骤 8：验证 userId 参数版本的焦点窗口检查**

编译验证：

```bash
cd /home/luna/workspaces/foundation/distributeddatamgr/pasteboard
# 预期：编译通过
```

- [ ] **步骤 9：提交 userId 参数版本的焦点窗口检查**

```bash
git add services/core/include/pasteboard_service.h services/core/src/pasteboard_service.cpp
git commit -m "feat: add userId-parameterized focus window checking via WindowManager::GetInstance(userId)"
```

---

### 任务 5：其他解析器使用修改（优先级3 - 兼容性）

**文件：**
- Modify: `services/core/src/pasteboard_service.cpp`（多处使用临时 resolver）

**目标：** 将所有临时创建的 UserContextResolver 替换为使用 userContextResolver_ 成员和封装方法

- [ ] **步骤 1：修改 DumpHistory 使用 GetForegroundUserIds**

找到 `std::string PasteboardService::DumpHistory() const` 方法，确认使用 GetForegroundUserIds() 封装方法：

```cpp
std::string PasteboardService::DumpHistory() const
{
    std::vector<int32_t> foregroundUsers = GetForegroundUserIds();  // 使用封装方法
    auto userId = foregroundUsers.empty() ? ERROR_USERID : foregroundUsers.front();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID, ...);
    // ... rest of code
}
```

如果使用了临时 resolver，替换为上述代码。

- [ ] **步骤 2：修改 DumpData 使用 GetForegroundUserIds**

找到 `std::string PasteboardService::DumpData()` 方法，确认使用 GetForegroundUserIds() 封装方法：

```cpp
std::string PasteboardService::DumpData()
{
    std::vector<int32_t> foregroundUsers = GetForegroundUserIds();  // 使用封装方法
    auto userId = foregroundUsers.empty() ? ERROR_USERID : foregroundUsers.front();
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query foreground user failed.");
        return "";
    }
    // ... rest of code
}
```

如果使用了临时 resolver，替换为上述代码。

- [ ] **步骤 3：修改 OnReceiveEventInner USER_SWITCHED 使用 ResolveEventUser**

找到 `void PasteBoardCommonEventSubscriber::OnReceiveEventInner(...)` 方法中的 USER_SWITCHED 处理，确认使用 ResolveEventUser() 封装方法：

```cpp
if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pasteboardService_ != nullptr) {
        auto context = pasteboardService_->ResolveEventUser(data);  // 使用封装方法
        if (!context.isValid) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "user switched userId invalid.");
            return;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id switched: %{public}d", context.userId);
        pasteboardService_->ChangeStoreStatus(context.userId);
        pasteboardService_->switch_.DeInit();
        pasteboardService_->switch_.Init(context.userId);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetSwitch end, userId=%{public}d", context.userId);
    }
}
```

如果使用了临时 resolver，替换为上述代码。

- [ ] **步骤 4：修改 OnReceiveEventInner USER_STOPPING 使用 ResolveEventUser**

找到 USER_STOPPING 处理，确认使用 ResolveEventUser() 封装方法：

```cpp
else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pasteboardService_ != nullptr) {
        auto context = pasteboardService_->ResolveEventUser(data);  // 使用封装方法
        if (!context.isValid) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "user stopping userId invalid.");
            return;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id is stopping: %{public}d", context.userId);
        pasteboardService_->ClearByEventUser(context.userId);  // 使用ClearByEventUser而非ClearByResolvedUser
    }
}
```

如果使用了临时 resolver，替换为上述代码。

- [ ] **步骤 5：修改 OnReceiveEventInner PACKAGE_REMOVED 使用 ResolvePackageRemovedUser**

找到 PACKAGE_REMOVED 处理，确认使用 ResolvePackageRemovedUser() 封装方法：

```cpp
else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
    auto tokenId = want.GetIntParam("accessTokenId", -1);
    if (pasteboardService_ != nullptr) {
        auto context = pasteboardService_->ResolvePackageRemovedUser(want);  // 使用封装方法
        if (!context.isValid) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "package removed userId invalid.");
            return;
        }
        pasteboardService_->ClearUriOnUninstall(context.userId, tokenId);
    }
}
```

如果使用了临时 resolver，替换为上述代码。

- [ ] **步骤 6：修改 ClearUriOnUninstall(tokenId版本) 使用 ResolveCallerContext**

找到 `void PasteboardService::ClearUriOnUninstall(int32_t tokenId)` 方法，确认使用 ResolveCallerContext() 封装方法：

```cpp
void PasteboardService::ClearUriOnUninstall(int32_t tokenId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearUriOnUninstall: tokenId=%{public}d", tokenId);
    auto context = ResolveCallerContext(IPCSkeleton::GetCallingTokenID());  // 使用封装方法
    if (!context.isValid) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clear uri uninstall failed, caller user invalid");
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "ClearUriOnUninstall: resolved userId=%{public}d for tokenId=%{public}d", context.userId, tokenId);
    ClearUriOnUninstall(context.userId, tokenId);
}
```

如果使用了临时 resolver，替换为上述代码。

- [ ] **步骤 7：验证其他解析器使用修改**

编译验证：

```bash
cd /home/luna/workspaces/foundation/distributeddatamgr/pasteboard
# 预期：编译通过
```

- [ ] **步骤 8：提交其他解析器使用修改**

```bash
git add services/core/src/pasteboard_service.cpp
git commit -m "refactor: replace temporary UserContextResolver with userContextResolver_ member and encapsulation methods"
```

---

## 完成验证

- [ ] **步骤 1：编译完整验证**

```bash
cd /home/luna/workspaces/foundation/distributeddatamgr/pasteboard
# 运行完整编译
# 预期：编译成功，无错误
```

- [ ] **步骤 2：运行单元测试验证**

```bash
# 运行pasteboard_service_check_test
# 预期：测试通过，SetUserContextResolver可用
```

- [ ] **步骤 3：提交第二阶段完成**

```bash
git log --oneline -10  # 查看提交历史
git status             # 确认无未提交文件
```

---

## 自查清单

**覆盖完整性：**
- ✅ RefreshCriticalState 8处调用恢复
- ✅ GrantPermission tokenId参数恢复（含7处调用）
- ✅ userId参数化的IME方法（IsNeedThaw/IsCurrentImeByPid/GetDefaultInputMethod）
- ✅ userId参数化的焦点窗口检查（IsFocusedApp/GetFocusedAppInfo）
- ✅ 其他解析器使用修改（DumpHistory/DumpData/OnReceiveEventInner/ClearUriOnUninstall）

**placeholder扫描：**
- ✅ 无TBD/TODO/implement later
- ✅ 所有代码完整，无引用未定义函数

**类型一致性：**
- ✅ GetDefaultInputMethod(property, userId) - property在前
- ✅ IsCurrentImeByPid(callPid, userId) - callPid在前
- ✅ GrantPermission参数顺序一致
- ✅ readTokens_类型为pair<int32_t, uint32_t>

---

## 实施完成标记

所有任务完成后，在 spec 文档中标记：

```markdown
**第二阶段状态：✅ 完成**
- ✅ RefreshCriticalState 调用恢复（8处）
- ✅ GrantPermission tokenId 参数恢复
- ✅ userId 参数化的输入法解冻方法
- ✅ pasteboard_window_manager userId 参数（含焦点窗口检查）
- ✅ 其他解析器使用修改
```