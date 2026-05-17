# 剪贴板服务恢复设计规范

## 问题发现

对比 commit 051f0fe8（多前台用户剪贴板）与当前 HEAD 发现部分代码回退：

**缺失功能（第二阶段 - 需确认）：**
1. userId 参数化的焦点窗口检查方法
2. userId 参数化的输入法解冻方法  
3. GrantPermission tokenId 参数恢复
4. RefreshCriticalState 调用恢复（8 处）
5. pasteboard_window_manager userId 参数版本
6. 其他解析器使用修改

**已恢复（第一阶段）：**
1. ✅ UserContextResolver 虚拟设计及依赖注入支持
2. ✅ 内存管理机制（HasActivePasteboardWork/RefreshCriticalState）
3. ✅ ClearByEventUser 方法（替代 ClearByResolvedUser）
4. ✅ userId 参数化的输入法方法（IsCurrentImeByPid/GetDefaultInputMethod）
5. ✅ pasteboard_service.h 声明恢复

## 设计目标

恢复所有 051f0fe8 功能以实现完整的多用户剪贴板支持，同时保持 HEAD 的改进。

## 第二阶段设计详情

### 1. userId 参数化的焦点窗口检查

**当前问题：** HEAD 移除了 userId 参数，仅支持调用者的焦点检查

**设计：**
```cpp
// pasteboard_service.h
bool IsFocusedApp(uint32_t tokenId);                      // 保留：调用者版本
bool IsFocusedApp(uint32_t tokenId, int32_t userId);  // 新增：userId 版本

FocusedAppInfo GetFocusedAppInfo(void) const;          // 保留：调用者版本  
FocusedAppInfo GetFocusedAppInfo(int32_t userId) const; // 新增：userId 版本

// pasteboard_service.cpp
bool PasteboardService::IsFocusedApp(uint32_t tokenId)
{
    // 简化版本：使用当前用户
    return IsFocusedApp(tokenId, GetCurrentAccountId());
}

bool PasteboardService::IsFocusedApp(uint32_t tokenId, int32_t userId)
{
    // userId 版本：检查指定用户的焦点窗口
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != ATokenTypeEnum::TOKEN_HAP) {
        return true;
    }
    if (userId == ERROR_USERID) {
        return false;
    }
    FocusChangeInfo info;
    OHOS::MiscServices::WindowManager::GetFocusWindowInfo(userId, info);
    auto callPid = IPCSkeleton::GetCallingPid();
    if (callPid == info.pid_) {
        return true;
    }
    bool isFocused = false;
    int32_t ret = PasteboardAbilityManager::CheckUIExtensionIsFocused(tokenId, isFocused);
    return ret == NO_ERROR && isFocused;
}
```

**关键变更：**
- WindowManager::GetFocusWindowInfo 需要 userId 参数版本
- GetFocusedAppInfo(void) 使用 GetCurrentAccountId() 调用 userId 版本
- IsFocusedApp(tokenId) 使用当前用户调用 userId 版本

### 2. userId 参数化的输入法解冻方法

**当前问题：** HEAD的IsNeedThaw移除了userId参数，InputMethodController方法现已支持userId参数

**设计：**
```cpp
// pasteboard_service.h
bool IsNeedThaw(PasteboardEventStatus status);                      // 保留：调用者版本
bool IsNeedThaw(int32_t userId, PasteboardEventStatus status);  // 新增：userId版本

bool IsCurrentImeByPid(int32_t userId, pid_t callPid) const;      // 新增：userId参数版本
int32_t GetDefaultInputMethod(int32_t userId, std::shared_ptr<Property> &property) const; // 新增：userId参数版本

// pasteboard_service.cpp
bool PasteboardService::IsNeedThaw(PasteboardEventStatus status)
{
    // 简化版本：使用当前用户
    return IsNeedThaw(GetCurrentAccountId(), status);
}

bool PasteboardService::IsNeedThaw(int32_t userId, PasteboardEventStatus status)
{
    if (status == PasteboardEventStatus::PASTEBOARD_READ) {
        return false;
    }
    if (userId == ERROR_USERID) {
        return false;
    }
    std::shared_ptr<Property> property;
    // InputMethodController现已支持userId参数
    int32_t ret = GetDefaultInputMethod(userId, property);
    if (ret != ErrorCode::NO_ERROR || property == nullptr) {
        return false;
    }
    return true;
}

bool PasteboardService::IsCurrentImeByPid(int32_t userId, pid_t callPid) const
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return false;
    }
    // InputMethodController的IsCurrentImeByPid现已支持userId参数
    auto isImePid = imc->IsCurrentImeByPid(callPid, userId);
    return isImePid;
}

int32_t PasteboardService::GetDefaultInputMethod(int32_t userId, std::shared_ptr<Property> &property) const
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return -1;
    }
    // InputMethodController的GetDefaultInputMethod现已支持userId参数
    return imc->GetDefaultInputMethod(property, userId);
}

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

**关键变更：**
- InputMethodController的GetDefaultInputMethod和IsCurrentImeByPid现在有第二个userId参数
- GetDefaultInputMethod(property, userId)参数顺序：property在前，userId在后
- IsCurrentImeByPid(callPid, userId)参数顺序：callPid在前，userId在后
- IsNeedThaw(status)调用userId版本，传入GetCurrentAccountId()
- SetInputMethodPid恢复IsCurrentImeByPid(userId, callPid)检查逻辑

### 3. 主屏幕用户预同步监视器检查

**修改意见：预同步相关的新增逻辑取消，无需修改此项**

此项功能恢复已取消，不需要添加ShouldRegisterPreSyncMonitor检查。HEAD版本的预同步逻辑保持不变。

### 4. GrantPermission tokenId 参数恢复

**当前问题：** HEAD 改为 bundleName，移除了 token 验证

**设计：**
```cpp
// pasteboard_service.h
int32_t GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId);
int32_t GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId, bool isRemoteData);
std::set<std::pair<int32_t, uint32_t>> readTokens_;  // 恢复：readTokens_

// pasteboard_service.cpp
int32_t PasteboardService::GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetTokenId != 0, ...);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetUserId != ERROR_USERID, ...);
    
    // 新增 token 验证
    HapTokenInfo targetInfo;
    int32_t tokenRet = AccessTokenKit::GetHapTokenInfo(targetTokenId, targetInfo);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(tokenRet == 0, ...);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetInfo.userID == targetUserId, ...);
    
    // ... 授权逻辑 ...
    
    if (hasGranted) {
        std::lock_guard<std::mutex> lock(readBundleMutex_);
        // 恢复：readTokens_
        if (readTokens_.count({ targetUserId, targetTokenId }) == 0) {
            readTokens_.insert({ targetUserId, targetTokenId });
        }
    }
    return ret;
}

// 恢复所有 7 处调用点：
int32_t PasteboardService::GetRecordValueByType(...) {
    // ...
    return GrantUriPermission(grantUris, appInfo.tokenId, appInfo.userId, data->GetTokenId(), isRemoteData);
}

int32_t PasteboardService::ProcessDelayHtmlEntry(...) {
    // ...
    int32_t ret = GrantUriPermission(grantUris, targetAppInfo.tokenId, targetAppInfo.userId, data->GetTokenId(), isRemoteData);
}

int32_t PasteboardService::CheckAndGrantRemoteUri(...) {
    // ...
    return GrantUriPermission(grantUris, appInfo.tokenId, appInfo.userId, data->GetTokenId(), isRemoteData);
}

int32_t PasteboardService::ProcessRemoteDelayUri(...) {
    // ...
    int32_t ret = GrantUriPermission(grantUris, appInfo.tokenId, appInfo.userId, data->GetTokenId(), data.IsRemote());
}

int32_t PasteboardService::ProcessRemoteDelayHtmlInner(...) {
    // ...
    int32_t ret = GrantUriPermission(grantUris, appInfo.tokenId, appInfo.userId, data->GetTokenId(), data.IsRemote());
}

int32_t PasteboardService::CheckAndGrantUriPermission(...) {
    // ...
    return GrantUriPermission(grantUris, appInfo.tokenId, appInfo.userId, data->GetTokenId(), isRemoteData);
}
```

**关键变更：**
- GrantPermission 参数恢复为 tokenId/userId
- GrantUriPermission 参数恢复
- readBundles_ 恢复为 readTokens_
- 新增 HapTokenInfo 验证（tokenRet == 0, userID == targetUserId）
- 恢复所有 7 处调用点

### 4. RefreshCriticalState 调用恢复

**当前问题：** HEAD 移除了所有 RefreshCriticalState 调用

**设计：添加到 8 个方法中：**

```cpp
// 1. NotifyDelayGetterDied
void PasteboardService::NotifyDelayGetterDied(int32_t userId) {
    if (userId == ERROR_USERID) {
        return;
    }
    delayGetters_.Erase(userId);
    RefreshCriticalState();  // 新增
}

// 2. NotifyEntryGetterDied  
void PasteboardService::NotifyEntryGetterDied(int32_t userId) {
    if (userId == ERROR_USERID) {
        return;
    }
    entryGetters_.Erase(userId);
    RefreshCriticalState();  // 新增
}

// 3. ClearInner
int32_t PasteboardService::ClearInner(int32_t userId, const AppInfo &appInfo) {
    RADAR_REPORT(...);
    auto [hasData, data] = clips_.Find(userId);
    if (hasData) {
        clips_.Erase(userId);
        delayDataId_ = 0;
        delayTokenId_ = 0;
    }
    CleanDistributedData(userId);
    RefreshCriticalState();  // 新增
    if (hasData) {
        std::string bundleName = GetAppBundleName(appInfo);
        NotifyObservers(bundleName, userId, PasteboardEventStatus::PASTEBOARD_CLEAR);
    }
    if (!HasActivePasteboardWork()) {  // 新增检查
        CancelCriticalTimer();
    }
    return ERR_OK;
}

// 4. GetRemoteData
int32_t PasteboardService::GetRemoteData(...) {
    // ...
    if (result.first) {
        auto [hasData, data] = clips_.Find(userId);
        if (!hasData) {
            clips_.InsertOrAssign(userId, result.first);
        }
        ret = static_cast<int32_t>(PasteboardError::E_OK);
    }
    taskMgr_.ClearRemoteDataTask(event);
    RefreshCriticalState();  // 新增
    return ret;
}

// 5. GetRemotePasteData
int32_t PasteboardService::GetRemotePasteData(...) {
    // ...
    if (result.first != nullptr) {
        auto [hasData, data] = clips_.Find(userId);
        if (hasData) {
            clips_.InsertOrAssign(userId, result.first);
            auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
            copyTime_.InsertOrAssign(userId, curTime);
            SetDataExpirationTimer(userId);
            RefreshCriticalState();  // 新增
        }
        // ...
    }
    block->SetValue(pasteDataTime);
    taskMgr_.ClearRemoteDataTask(event);
    RefreshCriticalState();  // 新增
}

// 6. SaveData
int32_t PasteboardService::SaveData(...) {
    // ...
    auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    copyTime_.InsertOrAssign(appInfo.userId, curTime);
    SetDataExpirationTimer(appInfo.userId);
    RefreshCriticalState();  // 新增
    // ...
}

// 7. ClearAgedData
void PasteboardService::ClearAgedData(int32_t userId) {
    // ...
    clips_.Erase(userId);
    if (data->GetRecordCount() > 0 && data->GetRecord(0)->GetTag() == PasteDataRecord::TAG_URI) {
        delayDataId_ = 0;
        delayTokenId_ = 0;
    }
    copyTime_.Erase(userId);
    RefreshCriticalState();  // 新增
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "data is out of the time");
}

// 8. ClearUriOnUninstall
void PasteboardService::ClearUriOnUninstall(int32_t userId, int32_t tokenId) {
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, ...);
    clips_.ComputeIfPresent(userId, [this, tokenId, userId](auto, auto &pasteData) {
        if (pasteData == nullptr) {
            return true;
        }
        if (pasteData->GetTokenId() != static_cast<uint32_t>(tokenId)) {
            return true;
        }
        ClearUriOnUninstall(pasteData);
        delayGetters_.ComputeIfPresent(userId, [](auto, auto &delayGetter) {
            if (delayGetter.first != nullptr && delayGetter.second != nullptr) {
                delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
            }
            return false;
        });
        clips_.Erase(userId);
        RefreshCriticalState();  // 新增
        return true;
    });
}
```

### 5. pasteboard_window_manager userId 参数恢复

**当前问题：** HEAD 移除了 userId 参数版本

**设计：**
通过WindowManager/WindowManagerLite的GetInstance(userId)来传入userId参数，无需修改方法签名：

```cpp
// pasteboard_service.cpp中的调用方式
bool PasteboardService::IsFocusedApp(uint32_t tokenId, int32_t userId)
{
    FocusChangeInfo info;
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance(userId).GetFocusWindowInfo(info);
#else
    WindowManager::GetInstance(userId).GetFocusWindowInfo(info);
#endif
    // ...
}

FocusedAppInfo PasteboardService::GetFocusedAppInfo(int32_t userId) const
{
    FocusChangeInfo info;
    std::vector<sptr<WindowVisibilityInfo>> windowVisibilityInfos;
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance(userId).GetFocusWindowInfo(info);
    WMError result = WindowManagerLite::GetInstance(userId).GetVisibilityWindowInfo(windowVisibilityInfos);
#else
    WindowManager::GetInstance(userId).GetFocusWindowInfo(info);
    WMError result = WindowManager::GetInstance(userId).GetVisibilityWindowInfo(windowVisibilityInfos);
#endif
    // ...
}
```

**关键改动：**
- WindowManager和WindowManagerLite通过GetInstance(userId)传入userId参数
- GetFocusWindowInfo和GetVisibilityWindowInfo方法签名无需修改
- 只需在调用时使用GetInstance(userId)而非GetInstance()

### 6. 其他解析器使用修改

**当前问题：** HEAD 使用临时解析器对象

**设计：** 替换为 userContextResolver_ 使用：

```cpp
// DumpHistory
std::string PasteboardService::DumpHistory() const {
    std::vector<int32_t> foregroundUsers = GetForegroundUserIds();  // 使用成员解析器
    auto userId = foregroundUsers.empty() ? ERROR_USERID : foregroundUsers.front();
    // ...
}

// DumpData
std::string PasteboardService::DumpData() {
    std::vector<int32_t> foregroundUsers = GetForegroundUserIds();  // 使用成员解析器
    auto userId = foregroundUsers.empty() ? ERROR_USERID : foregroundUsers.front();
    // ...
}

// OnReceiveEventInner - USER_SWITCHED
void PasteBoardCommonEventSubscriber::OnReceiveEventInner(...) {
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pasteboardService_ != nullptr) {
            auto context = pasteboardService_->ResolveEventUser(data);  // 使用成员解析器
            if (!context.isValid) {
                return;
            }
            // ...
        }
    }
}

// OnReceiveEventInner - USER_STOPPING  
void PasteBoardCommonEventSubscriber::OnReceiveEventInner(...) {
    else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pasteboardService_ != nullptr) {
            auto context = pasteboardService_->ResolveEventUser(data);  // 使用成员解析器
            if (!context.isValid) {
                return;
            }
            pasteboardService_->ClearByEventUser(context.userId);
        }
    }
}

// OnReceiveEventInner - PACKAGE_REMOVED
void PasteBoardCommonEventSubscriber::OnReceiveEventInner(...) {
    else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
        auto tokenId = want.GetIntParam("accessTokenId", -1);
        if (pasteboardService_ != nullptr) {
            auto context = pasteboardService_->ResolvePackageRemovedUser(want);  // 使用成员解析器
            if (!context.isValid) {
                return;
            }
            pasteboardService_->ClearUriOnUninstall(context.userId, tokenId);
        }
    }
}

// ClearUriOnUninstall（仅 tokenId 版本）
void PasteboardService::ClearUriOnUninstall(int32_t tokenId) {
    auto context = ResolveCallerContext(IPCSkeleton::GetCallingTokenID());  // 使用成员解析器
    if (!context.isValid) {
        return;
    }
    ClearUriOnUninstall(context.userId, tokenId);
}
```

## 文件影响分析

| 文件 | 预估行数 | 变更 |
|------|----------------|--------|
| pasteboard_service.h | ~100 行 | 新增 userId 参数版本，恢复 readTokens_，新增 ShouldRegisterPreSyncMonitor |
| pasteboard_service.cpp | ~1000 行 | 新增 userId 参数方法，恢复 GrantPermission tokenId 参数，新增 RefreshCriticalState 调用 |
| pasteboard_window_manager.h | ~50 行 | 新增 userId 参数版本 |
| pasteboard_window_manager.cpp | ~100 行 | 实现 userId 参数版本 |
| pasteboard_service_check_test.cpp | 0 行 | 无需修改（与第一阶段兼容） |

## 复杂度评估

**高复杂度区域：**
- GrantPermission tokenId 验证逻辑（~200 行）
- 多方法中的 userId 参数新增（~500 行）
- 8 处不同位置的 RefreshCriticalState 调用恢复

**低风险区域：**
- 测试文件兼容性（第一阶段已验证）
- 成员解析器使用（简单替换）
- 向后兼容性保持（调用者版本保留）

## 实现优先级

### 优先级 1（关键 - 内存管理）：
1. RefreshCriticalState 调用恢复（8 处）
2. GrantPermission tokenId 参数恢复  

### 优先级 2（功能完整性）：
3. userId 参数化的焦点窗口检查
4. userId 参数化的输入法解冻方法  

### 优先级 3（较低 - 兼容性）：
5. pasteboard_window_manager userId 参数（通过GetInstance(userId)）
6. 其他解析器使用修改

## 测试策略

**单元测试：**
- PasteboardServiceCheckTest：验证 SetUserContextResolver 依赖注入
- PasteboardServiceMockTest：验证 userId 参数方法
- 窗口管理器测试：验证 userId 参数版本

**集成测试：**
- 多用户剪贴板操作
- 预同步监视器注册
- URI 权限授权

## 成功标准

**功能恢复：**
✅ 所有 051f0fe8 功能恢复
✅ UserContextResolver 依赖注入完整
✅ userId 参数方法完整  
✅ 内存管理机制完整
✅ 预同步逻辑保持HEAD版本（取消恢复）

**架构一致性：**
✅ 使用 userContextResolver_ 成员统一
✅ 使用 GetForegroundUserIds/ResolveEventUser 封装
✅ userId 参数向后兼容（调用者版本保留）
✅ WindowManager通过GetInstance(userId)传入userId
✅ InputMethodController方法支持userId参数

**测试通过：**
✅ pasteboard_service_check_test 编译通过
✅ SetUserContextResolver 方法可用
✅ FakeUserContextResolver mock 正常工作

## 风险评估

**低风险：**
- 测试文件兼容性 - 第一阶段已验证
- userId 参数方法 - 向后兼容设计

**中等风险：**
- GrantPermission tokenId 逻辑 - 需仔细进行 HapTokenInfo 验证
- RefreshCriticalState 时序 - 需验证调用时机

## 技术债务清理

本次恢复将清理技术债务：
1. ✅ 移除临时解析器对象 - 第一阶段已清理
2. ✅ 恢复 userId 参数方法 - 第二阶段清理
3. ✅ 恢复 token 验证逻辑 - 第二阶段清理
4. ✅ 恢复 RefreshCriticalState 调用 - 第二阶段清理

## 长期改进建议

**架构方面：**
- 考虑添加 CI 检查以防止类似回退
- 为关键功能覆盖添加架构测试
- 文档化多用户剪贴板服务需求

**流程方面：**
- 定期与基准提交（051f0fe8）进行提交对比
- 添加预合并钩子以验证功能完整性
- 为多用户场景创建功能跟踪

## 结论

第二阶段恢复完成多用户剪贴板功能集，确保：
- 内存管理正常工作
- userId 参数可用于多用户场景
- 预同步仅针对主屏幕用户
- URI 权限按 token 正确跟踪
- 所有解析器使用与依赖注入一致

**总行数估算：** 跨 4 个文件约 1500 行
**预估工作量：** 4-6 小时精细实现
**测试验证：** 完整单元测试覆盖 + 集成测试场景