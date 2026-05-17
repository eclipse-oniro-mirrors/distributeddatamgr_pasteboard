# Pasteboard Service Restoration Design Spec

## Problem Discovery

Comparing commit 051f0fe8 (multi-foreground-user pasteboard) with current HEAD revealed partial code regression:

**Missing Features (Phase 2 - Need Confirmation):**
1. userId-parameterized focus window checking methods
2. userId-parameterized IME thaw methods  
3. ShouldRegisterPreSyncMonitor check for main screen user
4. GrantPermission tokenId parameter restoration
5. RefreshCriticalState call restoration (8 locations)
6. pasteboard_window_manager userId parameter versions
7. Other resolver usage modifications

**Already Restored (Phase 1):**
1. ✅ UserContextResolver virtual design with dependency injection support
2. ✅ Memory management mechanism (HasActivePasteboardWork/RefreshCriticalState)  
3. ✅ ClearByEventUser method (replaces ClearByResolvedUser)
4. ✅ userId-parameterized IME methods (IsCurrentImeByPid/GetDefaultInputMethod)
5. ✅ pasteboard_service.h declarations restored

## Design Goal

Restore all 051f0fe8 features for complete multi-user clipboard support while maintaining HEAD improvements.

## Phase 2 Design Details

### 1. userId-Parameterized Focus Window Checking

**Current Issue:** HEAD removed userId parameter, only supports caller's focus check

**Design:**
```cpp
// pasteboard_service.h
bool IsFocusedApp(uint32_t tokenId);                      // Keep: caller version
bool IsFocusedApp(uint32_t tokenId, int32_t userId);  // Add: userId version

FocusedAppInfo GetFocusedAppInfo(void) const;          // Keep: caller version  
FocusedAppInfo GetFocusedAppInfo(int32_t userId) const; // Add: userId version

// pasteboard_service.cpp
bool PasteboardService::IsFocusedApp(uint32_t tokenId)
{
    // Simplified version: use current user
    return IsFocusedApp(tokenId, GetCurrentAccountId());
}

bool PasteboardService::IsFocusedApp(uint32_t tokenId, int32_t userId)
{
    // userId version: check specified user's focus window
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

**Key Changes:**
- WindowManager::GetFocusWindowInfo needs userId parameter version
- GetFocusedAppInfo(void) calls userId version with GetCurrentAccountId()
- IsFocusedApp(tokenId) calls userId version with current user

### 2. userId-Parameterized IME Thaw Methods

**Current Issue:** HEAD's IsNeedThaw removed userId parameter

**Design:**
```cpp
// pasteboard_service.h
bool IsNeedThaw(PasteboardEventStatus status);                      // Keep: caller version
bool IsNeedThaw(int32_t userId, PasteboardEventStatus status);  // Add: userId version

// pasteboard_service.cpp
bool PasteboardService::IsNeedThaw(PasteboardEventStatus status)
{
    // Simplified version: use current user
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
    int32_t ret = GetDefaultInputMethod(userId, property);
    if (ret != ErrorCode::NO_ERROR || property == nullptr) {
        return false;
    }
    return true;
}
```

**Key Changes:**
- IsNeedThaw(status) calls userId version with GetCurrentAccountId()
- GetDefaultInputMethod already restored in Phase 1

### 3. Main Screen User Pre-Sync Monitor Check

**Current Issue:** HEAD removed ShouldRegisterPreSyncMonitor, all users register pre-sync

**Design:**
```cpp
// pasteboard_service.h
bool ShouldRegisterPreSyncMonitor(int32_t userId) const;

// pasteboard_service.cpp
bool PasteboardService::ShouldRegisterPreSyncMonitor(int32_t userId) const
{
    return IsMainScreenUser(userId);
}

void PasteboardService::PreSyncRemotePasteboardData()
{
    auto clipPlugin = GetClipPlugin();
    if (!clipPlugin) {
        return;
    }
    if (!clipPlugin->NeedSyncTopEvent()) {
        return;
    }
    // Add check: only for main screen user
    if (!ShouldRegisterPreSyncMonitor(MAIN_SCREEN_USER_ID)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "skip presync, main screen user disabled");
        return;
    }
    clipPlugin->SendPreSyncEvent(MAIN_SCREEN_USER_ID);
}

void PasteboardService::RegisterPreSyncMonitor()
{
    if (!ShouldRegisterPreSyncMonitor(MAIN_SCREEN_USER_ID)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "skip presync monitor, userId=%{public}d", MAIN_SCREEN_USER_ID);
        return;
    }
    // ... register logic
}
```

**Key Changes:**
- ShouldRegisterPreSyncMonitor checks IsMainScreenUser
- PreSyncRemotePasteboardData adds check before sending event
- RegisterPreSyncMonitor adds check before registering

### 4. GrantPermission tokenId Parameter Restoration

**Current Issue:** HEAD changed to bundleName, removed token validation

**Design:**
```cpp
// pasteboard_service.h
int32_t GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId);
int32_t GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId, bool isRemoteData);
std::set<std::pair<int32_t, uint32_t>> readTokens_;  // Restore: readTokens_

// pasteboard_service.cpp
int32_t PasteboardService::GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId, int32_t targetUserId, uint32_t srcTokenId)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetTokenId != 0, ...);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetUserId != ERROR_USERID, ...);
    
    // Add token validation
    HapTokenInfo targetInfo;
    int32_t tokenRet = AccessTokenKit::GetHapTokenInfo(targetTokenId, targetInfo);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(tokenRet == 0, ...);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(targetInfo.userID == targetUserId, ...);
    
    // ... grant logic ...
    
    if (hasGranted) {
        std::lock_guard<std::mutex> lock(readBundleMutex_);
        // Restore: readTokens_
        if (readTokens_.count({ targetUserId, targetTokenId }) == 0) {
            readTokens_.insert({ targetUserId, targetTokenId });
        }
    }
    return ret;
}

// Restore all 7 call sites:
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

**Key Changes:**
- GrantPermission parameters restored to tokenId/userId
- GrantUriPermission parameters restored
- readBundles_ restored to readTokens_
- HapTokenInfo validation added (tokenRet == 0, userID == targetUserId)
- All 7 call sites restored

### 5. RefreshCriticalState Call Restoration

**Current Issue:** HEAD removed all RefreshCriticalState calls

**Design: Add to 8 methods:**

```cpp
// 1. NotifyDelayGetterDied
void PasteboardService::NotifyDelayGetterDied(int32_t userId) {
    if (userId == ERROR_USERID) {
        return;
    }
    delayGetters_.Erase(userId);
    RefreshCriticalState();  // Add
}

// 2. NotifyEntryGetterDied  
void PasteboardService::NotifyEntryGetterDied(int32_t userId) {
    if (userId == ERROR_USERID) {
        return;
    }
    entryGetters_.Erase(userId);
    RefreshCriticalState();  // Add
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
    RefreshCriticalState();  // Add
    if (hasData) {
        std::string bundleName = GetAppBundleName(appInfo);
        NotifyObservers(bundleName, userId, PasteboardEventStatus::PASTEBOARD_CLEAR);
    }
    if (!HasActivePasteboardWork()) {  // Add check
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
    RefreshCriticalState();  // Add
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
            RefreshCriticalState();  // Add
        }
        // ...
    }
    block->SetValue(pasteDataTime);
    taskMgr_.ClearRemoteDataTask(event);
    RefreshCriticalState();  // Add
}

// 6. SaveData
int32_t PasteboardService::SaveData(...) {
    // ...
    auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    copyTime_.InsertOrAssign(appInfo.userId, curTime);
    SetDataExpirationTimer(appInfo.userId);
    RefreshCriticalState();  // Add
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
    RefreshCriticalState();  // Add
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
        RefreshCriticalState();  // Add
        return true;
    });
}
```

### 6. pasteboard_window_manager userId Parameter Restoration

**Current Issue:** HEAD removed userId parameter versions

**Design:**
```cpp
// pasteboard_window_manager.h
void GetFocusWindowInfo(int32_t userId, FocusChangeInfo &info);              // Add userId version
WMError GetVisibilityWindowInfo(int32_t userId, std::vector<sptr<WindowVisibilityInfo>> &infos); // Add userId version

// pasteboard_window_manager.cpp  
void WindowManager::GetFocusWindowInfo(int32_t userId, FocusChangeInfo &info)
{
    // userId version implementation
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance().GetFocusWindowInfo(info);
#else
    WindowManager::GetInstance().GetFocusWindowInfo(info);
#endif
    // Filter by userId if needed
}

WMError WindowManager::GetVisibilityWindowInfo(int32_t userId, std::vector<sptr<WindowVisibilityInfo>> &infos)
{
    // userId version implementation
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance().GetVisibilityWindowInfo(infos);
#else
    WindowManager::GetInstance().GetVisibilityWindowInfo(infos);
#endif
    // Filter by userId if needed  
}
```

### 7. Other Resolver Usage Modifications

**Current Issue:** HEAD uses temporary resolver objects

**Design:** Replace with userContextResolver_ usage:

```cpp
// DumpHistory
std::string PasteboardService::DumpHistory() const {
    std::vector<int32_t> foregroundUsers = GetForegroundUserIds();  // Use member resolver
    auto userId = foregroundUsers.empty() ? ERROR_USERID : foregroundUsers.front();
    // ...
}

// DumpData
std::string PasteboardService::DumpData() {
    std::vector<int32_t> foregroundUsers = GetForegroundUserIds();  // Use member resolver
    auto userId = foregroundUsers.empty() ? ERROR_USERID : foregroundUsers.front();
    // ...
}

// OnReceiveEventInner - USER_SWITCHED
void PasteBoardCommonEventSubscriber::OnReceiveEventInner(...) {
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pasteboardService_ != nullptr) {
            auto context = pasteboardService_->ResolveEventUser(data);  // Use member resolver
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
            auto context = pasteboardService_->ResolveEventUser(data);  // Use member resolver
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
            auto context = pasteboardService_->ResolvePackageRemovedUser(want);  // Use member resolver
            if (!context.isValid) {
                return;
            }
            pasteboardService_->ClearUriOnUninstall(context.userId, tokenId);
        }
    }
}

// ClearUriOnUninstall (tokenId-only version)
void PasteboardService::ClearUriOnUninstall(int32_t tokenId) {
    auto context = ResolveCallerContext(IPCSkeleton::GetCallingTokenID());  // Use member resolver
    if (!context.isValid) {
        return;
    }
    ClearUriOnUninstall(context.userId, tokenId);
}
```

## File Impact Analysis

| File | Estimated Lines | Changes |
|------|----------------|--------|
| pasteboard_service.h | ~100 lines | Add userId parameter versions, Restore readTokens_, Add ShouldRegisterPreSyncMonitor |
| pasteboard_service.cpp | ~1000 lines | Add userId parameter methods, Restore GrantPermission tokenId params, Add RefreshCriticalState calls |
| pasteboard_window_manager.h | ~50 lines | Add userId parameter versions |
| pasteboard_window_manager.cpp | ~100 lines | Implement userId parameter versions |
| pasteboard_service_check_test.cpp | 0 lines | No changes needed (Phase 1 compatible) |

## Complexity Assessment

**High Complexity Areas:**
- GrantPermission tokenId validation logic (~200 lines)
- userId parameter additions across multiple methods (~500 lines)  
- RefreshCriticalState call restoration in 8 different locations

**Low Risk Areas:**
- Test file compatibility (Phase 1 already verified)
- Member resolver usage (simple replacements)
- Backward compatibility maintained (caller versions preserved)

## Implementation Priority

### Priority 1 (Critical - Memory Management):
1. RefreshCriticalState call restoration (8 locations)
2. GrantPermission tokenId parameter restoration  

### Priority 2 (Feature Completeness):
3. userId-parameterized focus window checking
4. userId-parameterized IME thaw methods  
5. ShouldRegisterPreSyncMonitor check

### Priority 3 (Lower - Compatibility):
6. pasteboard_window_manager userId parameter versions
7. Other resolver usage modifications  

## Test Strategy

**Unit Tests:**
- PasteboardServiceCheckTest: Verify SetUserContextResolver dependency injection
- PasteboardServiceMockTest: Verify userId parameter methods
- Window manager tests: Verify userId parameter versions

**Integration Tests:**
- Multi-user clipboard operations
- Pre-sync monitor registration  
- URI permission granting

## Success Criteria

**Functionality Restoration:**
✅ All 051f0fe8 features restored
✅ UserContextResolver dependency injection complete
✅ userId parameter methods complete  
✅ Memory management mechanism complete

**Architecture Consistency:**
✅ Uses userContextResolver_ member consistently
✅ Uses GetForegroundUserIds/ResolveEventUser encapsulation
✅ userId parameters backward compatible (caller versions kept)

**Test Pass Rate:**
✅ pasteboard_service_check_test compilation passes
✅ UserContextResolver methods available
✅ FakeUserContextResolver mock works

## Risk Assessment

**Low Risk:**
- Test file compatibility - Phase 1 verified
- userId parameter methods - backward compatible design

**Medium Risk:**
- GrantPermission tokenId logic - needs careful HapTokenInfo validation
- RefreshCriticalState timing - needs verification of call timing

## Debt Cleanup

This restoration will clean technical debt:
1. ✅ Remove temporary resolver objects - Phase 1 cleaned
2. ✅ Restore userId parameter methods - Phase 2 cleans
3. ✅ Restore token validation logic - Phase 2 cleans
4. ✅ Restore RefreshCriticalState calls - Phase 2 cleans

## Long-term Improvement Suggestions

**Architectural:**
- Consider adding CI checks to prevent similar regression
- Add architecture tests for critical feature coverage  
- Document multi-user clipboard service requirements

**Process:**
- Regular commit comparison against base commit (051f0fe8)
- Add pre-merge hooks to verify feature completeness
- Create feature tracking for multi-user scenarios

## Conclusion

Phase 2 restoration completes the multi-user clipboard feature set, ensuring:
- Memory management works correctly
- userId parameters available for multi-user scenarios  
- Pre-sync only happens for main screen user  
- URI permissions properly tracked by token
- All resolver usage consistent with dependency injection

**Total Lines Estimate:** ~1500 lines across 4 files  
**Estimated Effort:** 4-6 hours of careful implementation
**Test Validation:** Full unit test coverage + integration test scenarios