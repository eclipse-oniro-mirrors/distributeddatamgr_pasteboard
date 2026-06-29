# PasteboardService重构设计方案

## 背景

### 问题陈述
`pasteboard_service.cpp`文件当前有5569行代码，包含245个成员函数，文件过大导致以下问题：
- 代码可维护性差：单文件复杂度高，理解和修改困难
- 编译依赖重：修改任何部分都需要重新编译整个文件
- 单元测试困难：难以对特定功能进行独立测试
- 代码复用性低：功能耦合紧密，难以复用独立模块

### 目标
本次重构旨在：
1. **提高代码可维护性**：降低单文件复杂度，使代码更易理解和修改
2. **便于单元测试**：拆分后的模块更易进行独立测试
3. **提高代码复用性**：抽取独立功能模块，便于其他模块复用
4. **降低编译依赖**：减少头文件依赖，加快编译速度

### 约束条件
- 单次Git提交不能超过2000行
- 保持现有接口不变（对外API兼容）
- 保持功能逻辑不变（仅重构结构）
- 采用细粒度拆分策略

## 方案选择

### 可选方案对比

#### 方案A：职责驱动拆分（已选择）
将代码按职责拆分成10-12个独立的功能类，每个类负责一个明确的功能领域。

优点：
- 每个类职责清晰，易于理解和维护
- 便于独立测试每个功能模块
- 降低编译依赖，修改某模块只需重新编译该模块
- 符合单一职责原则

缺点：
- 需要设计良好的接口层
- 类数量较多，可能增加调用复杂度

#### 方案B：分层拆分（已拒绝）
将代码按架构层次拆分成3-4个大模块。

优点：
- 结构清晰，层次分明
- 易于理解整体架构

缺点：
- 单个模块仍然较大（约2000行）
- 测试和复用难度相对较高
- 不符合细粒度拆分要求

#### 方案C：渐进式拆分（已拒绝）
先拆分最明显独立的功能模块，保留核心在主类中。

优点：
- 逐步降低风险
- 可根据实际效果调整后续拆分策略

缺点：
- 主文件仍然较大
- 需要多轮迭代才能完全解决问题

### 决策
选择方案A（职责驱动拆分），因为它能最大程度达成所有重构目标，并且符合用户选择的细粒度拆分策略。

## 架构设计

### 整体架构层次

```
┌─────────────────────────────────────────────────────────┐
│                  PasteboardService                      │  主服务类
│  (SystemAbility入口，IPC接口实现，协调各Manager)        │
│  约800行                                                │
└─────────────────────────────────────────────────────────┘
                            ↓ 调用
┌─────────────────────────────────────────────────────────┐
│              功能管理层 (10个Manager类)                 │
│  PasteboardDistributedManager                           │  分布式数据管理
│  PasteboardP2PManager                                   │  P2P连接管理
│  PasteboardUriHandler                                   │  URI权限处理
│  PasteboardObserverManager                              │  观察者管理
│  PasteboardEntityRecognizer                             │  实体识别
│  PasteboardPermissionChecker                            │  权限检查
│  PasteboardAppInfoHelper                                │  应用信息辅助
│  PasteboardDelayDataHandler                             │  延迟数据处理
│  PasteboardSystemEventListener                          │  系统事件监听
│  PasteboardDumpHelper                                   │  Dump调试
└─────────────────────────────────────────────────────────┘
                            ↓ 使用
┌─────────────────────────────────────────────────────────┐
│                  共享数据层                              │
│  clips_ (剪贴板数据存储)                                │
│  currentEvent_ (当前事件)                               │
│  各种并发Map和状态变量                                  │
└─────────────────────────────────────────────────────────┘
```

### 文件组织结构

```
services/core/src/
├── pasteboard_service.cpp                     (主服务，约800行)
├── pasteboard_distributed_manager.cpp        (分布式数据管理，约500行)
├── pasteboard_p2p_manager.cpp                (P2P连接管理，约400行)
├── pasteboard_uri_handler.cpp                (URI权限处理，约600行)
├── pasteboard_observer_manager.cpp           (观察者管理，约300行)
├── pasteboard_entity_recognizer.cpp          (实体识别，约200行)
├── pasteboard_permission_checker.cpp         (权限检查，约300行)
├── pasteboard_app_info_helper.cpp            (应用信息辅助，约200行)
├── pasteboard_delay_data_handler.cpp         (延迟数据处理，约800行)
├── pasteboard_system_event_listener.cpp      (系统事件监听，约300行)
├── pasteboard_dump_helper.cpp                (Dump调试，约200行)

services/core/include/
├── pasteboard_service.h                      (主服务头文件)
├── pasteboard_distributed_manager.h
├── pasteboard_p2p_manager.h
├── pasteboard_uri_handler.h
├── pasteboard_observer_manager.h
├── pasteboard_entity_recognizer.h
├── pasteboard_permission_checker.h
├── pasteboard_app_info_helper.h
├── pasteboard_delay_data_handler.h
├── pasteboard_system_event_listener.h
├── pasteboard_dump_helper.h
```

### 接口设计原则

1. **Manager类接口**：每个Manager提供明确的公共接口方法，PasteboardService通过这些接口调用功能
2. **数据共享**：Manager类通过引用或指针访问PasteboardService的共享数据成员（如clips_、currentEvent_等）
3. **回调机制**：Manager通过回调函数向PasteboardService报告状态变化和事件
4. **依赖注入**：PasteboardService在初始化时创建各Manager实例，并注入必要的依赖

## Manager类详细设计

### 1. PasteboardDistributedManager（分布式数据管理）

**职责**：管理分布式剪贴板数据的获取、设置和状态管理

**估算行数**：约500行

**主要接口**：
```cpp
class PasteboardDistributedManager {
public:
    PasteboardDistributedManager(PasteboardService& service);
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
    
private:
    PasteboardService& service_;
    std::shared_ptr<ClipPlugin> clipPlugin_;
    DistributedModuleConfig moduleConfig_;
};
```

**迁移方法**：
- GetDistributedData()
- GetDistributedDelayData()
- GetDistributedDelayEntry()
- SetDistributedData()
- SetCurrentDistributedData()
- IsDisallowDistributed()
- CleanDistributedData()
- CloseDistributedStore()
- ChangeStoreStatus()
- IsValidCurrentEvent()
- IsConstraintEnabled()
- ProcessDistributedDelayUri()
- ProcessDistributedDelayHtml()
- ProcessDistributedDelayEntry()
- GetClipPlugin()

### 2. PasteboardP2PManager（P2P连接管理）

**职责**：管理P2P连接的建立、预建立、清理和状态维护

**估算行数**：约400行

**主要接口**：
```cpp
class PasteboardP2PManager {
public:
    PasteboardP2PManager(PasteboardService& service);
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
};
```

**迁移方法**：
- OpenP2PLink()
- EstablishP2PLink()
- CheckAndReuseP2PLink()
- CloseP2PLink()
- PreEstablishP2PLink()
- PreEstablishP2PLinkCallback()
- OpenP2PLinkForPreEstablish()
- PreSyncRemotePasteboardData()
- RegisterPreSyncMonitor()
- UnRegisterPreSyncMonitor()
- DeletePreSyncP2pFromP2pMap()
- DeletePreSyncP2pMap()
- AddPreSyncP2pTimeoutTask()
- IsContainUri()
- IsNeedLink()
- ClearP2PEstablishTaskInfo()
- OnEstablishP2PLinkTask()
- EstablishP2PLinkTask()

### 3. PasteboardUriHandler（URI权限处理）

**职责**：处理URI权限授予、检查、清理和分布式URI生成

**估算行数**：约600行

**主要接口**：
```cpp
class PasteboardUriHandler {
public:
    PasteboardUriHandler(PasteboardService& service);
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

**迁移方法**：
- CheckAndGrantRemoteUri()
- GrantUriPermission()
- GrantPermission()
- CheckUriPermission()
- RemoveInvalidRemoteUri()
- GenerateDistributedUri()
- IsBundleOwnUriPermission()
- ClearUriOnUninstall()
- HasRemoteUri()

### 4. PasteboardObserverManager（观察者管理）

**职责**：管理剪贴板观察者的订阅、取消订阅和通知

**估算行数**：约300行

**主要接口**：
```cpp
class PasteboardObserverManager {
public:
    PasteboardObserverManager(PasteboardService& service);
    ~PasteboardObserverManager();
    
    // 观察者订阅
    int32_t SubscribeObserver(PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer);
    int32_t ResubscribeObserver(PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer);
    int32_t UnsubscribeObserver(PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer);
    int32_t UnsubscribeAllObserver(PasteboardObserverType type);
    
    // 实体识别观察者
    int32_t SubscribeEntityObserver(EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver>& observer);
    int32_t UnsubscribeEntityObserver(EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver>& observer);
    void UnsubscribeAllEntityObserver();
    void NotifyEntityObservers(std::string& entity, EntityType entityType, uint32_t dataLength);
    
    // Disposable观察者
    int32_t SubscribeDisposableObserver(const sptr<IPasteboardDisposableObserver>& observer, int32_t targetWindowId, DisposableType type, uint32_t maxLength);
    
    // 观察者通知
    void NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status);
    void RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap& observerMap);
    
private:
    PasteboardService& service_;
    std::mutex observerMutex_;
    ObserverMap observerLocalChangedMap_;
    ObserverMap observerRemoteChangedMap_;
    ObserverMap observerEventMap_;
    ConcurrentMap<pid_t, std::vector<EntityObserverInfo>> entityObserverMap_;
};
```

**迁移方法**：
- SubscribeObserver()
- ResubscribeObserver()
- UnsubscribeObserver()
- UnsubscribeAllObserver()
- SubscribeEntityObserver()
- UnsubscribeEntityObserver()
- UnsubscribeAllEntityObserver()
- NotifyEntityObservers()
- SubscribeDisposableObserver()
- NotifyObservers()
- RemoveObserverByPid()
- AddObserver()
- RemoveSingleObserver()
- RemoveAllObserver()
- GetObserversSize()
- GetAllObserversSize()

### 5. PasteboardEntityRecognizer（实体识别）

**职责**：识别剪贴板数据中的实体（如地址、电话等），提供智能提取功能

**估算行数**：约200行

**主要接口**：
```cpp
class PasteboardEntityRecognizer {
public:
    PasteboardEntityRecognizer(PasteboardService& service);
    ~PasteboardEntityRecognizer();
    
    // 实体识别
    void RecognizePasteData(PasteData& pasteData);
    void OnRecognizePasteData(const std::string& primaryText);
    void OnRecognizePasteDataInner(const std::string& primaryText, void* nulGuard);
    
    // 文本提取
    int32_t GetAllEntryPlainText(uint32_t dataId, uint32_t recordId, std::vector<std::shared_ptr<PasteDataEntry>>& entries, std::string& primaryText);
    std::string GetAllPrimaryText(const PasteData& pasteData);
    int32_t ExtractEntity(const std::string& entity, std::string& location);
    
private:
    PasteboardService& service_;
    std::mutex entityRecognizeMutex_;
};
```

**迁移方法**：
- RecognizePasteData()
- OnRecognizePasteData()
- OnRecognizePasteDataInner()
- GetAllEntryPlainText()
- GetAllPrimaryText()
- ExtractEntity()

### 6. PasteboardPermissionChecker（权限检查）

**职责**：验证权限、检查SDK版本、管理分享选项

**估算行数**：约300行

**主要接口**：
```cpp
class PasteboardPermissionChecker {
public:
    PasteboardPermissionChecker(PasteboardService& service);
    ~PasteboardPermissionChecker();
    
    // 权限验证
    bool VerifyPermission(uint32_t tokenId);
    int32_t IsDataValid(PasteData& pasteData, uint32_t tokenId, int32_t userId);
    bool IsPermissionGranted(const std::string& perm, uint32_t tokenId);
    bool IsSystemAppByFullTokenID(uint64_t tokenId);
    bool IsCopyable(uint32_t tokenId) const;
    
    // SDK版本检查
    int32_t GetSdkVersion(uint32_t tokenId);
    bool IsCallerUidValid();
    
    // 分享选项检查
    bool CheckMdmShareOption(PasteData& pasteData);
    void UpdateShareOption(PasteData& pasteData);
    
    // 全局分享选项管理
    int32_t SetGlobalShareOption(const std::unordered_map<uint32_t, int32_t>& globalShareOptions);
    int32_t RemoveGlobalShareOption(const std::vector<uint32_t>& tokenIds);
    int32_t GetGlobalShareOption(const std::vector<uint32_t>& tokenIds, std::unordered_map<uint32_t, int32_t>& funcResult);
    
    // 应用分享选项
    int32_t SetAppShareOptions(int32_t shareOptions);
    int32_t RemoveAppShareOptions();
    
private:
    PasteboardService& service_;
    ConcurrentMap<uint32_t, GlobalShareOption> globalShareOptions_;
};
```

**迁移方法**：
- VerifyPermission()
- IsDataValid()
- IsPermissionGranted()
- IsSystemAppByFullTokenID()
- IsCopyable()
- GetSdkVersion()
- IsCallerUidValid()
- CheckMdmShareOption()
- UpdateShareOption()
- SetGlobalShareOption()
- RemoveGlobalShareOption()
- GetGlobalShareOption()
- SetAppShareOptions()
- RemoveAppShareOptions()

### 7. PasteboardAppInfoHelper（应用信息辅助）

**职责**：获取应用信息、焦点应用状态、本地标记设置

**估算行数**：约200行

**主要接口**：
```cpp
class PasteboardAppInfoHelper {
public:
    PasteboardAppInfoHelper(PasteboardService& service);
    ~PasteboardAppInfoHelper();
    
    // 应用信息获取
    AppInfo GetAppInfo(uint32_t tokenId) const;
    void FillHapAppInfo(uint32_t tokenId, AppInfo& info) const;
    void FillNativeAppInfo(uint32_t tokenId, AppInfo& info) const;
    static std::string GetAppBundleName(const AppInfo& appInfo);
    std::string GetAppLabel(uint32_t tokenId);
    
    // BundleManager获取
    sptr<OHOS::AppExecFwk::IBundleMgr> GetAppBundleManager();
    
    // 焦点应用
    bool IsFocusedApp(uint32_t tokenId);
    FocusedAppInfo GetFocusedAppInfo() const;
    
    // 本地标记设置
    static void SetLocalPasteFlag(bool isCrossPaste, uint32_t tokenId, PasteData& pasteData);
    
private:
    PasteboardService& service_;
    std::mutex bundleMutex_;
};
```

**迁移方法**：
- GetAppInfo()
- FillHapAppInfo()
- FillNativeAppInfo()
- GetAppBundleName()
- GetAppLabel()
- GetAppBundleManager()
- IsFocusedApp()
- GetFocusedAppInfo()
- SetLocalPasteFlag()

### 8. PasteboardDelayDataHandler（延迟数据处理）

**职责**：处理延迟数据的获取、同步、远程延迟数据转换

**估算行数**：约800行

**主要接口**：
```cpp
class PasteboardDelayDataHandler {
public:
    PasteboardDelayDataHandler(PasteboardService& service);
    ~PasteboardDelayDataHandler();
    
    // 延迟数据获取
    int32_t GetDelayPasteRecord(int32_t userId, PasteData& data);
    void GetDelayPasteData(int32_t userId, PasteData& data);
    int32_t GetFullDelayPasteData(int32_t userId, PasteData& data);
    int32_t SyncDelayedData();
    
    // 延迟数据处理
    int32_t ProcessDelayHtmlEntry(PasteData& data, const AppInfo& targetAppInfo, PasteDataEntry& entry);
    int32_t PostProcessDelayHtmlEntry(PasteData& data, const AppInfo& targetAppInfo, PasteDataEntry& entry);
    
    // 远程延迟数据
    int32_t GetLocalEntryValue(int32_t userId, PasteData& data, PasteDataRecord& record, PasteDataEntry& entry);
    int32_t GetRemoteEntryValue(const AppInfo& appInfo, PasteData& data, PasteDataRecord& record, PasteDataEntry& entry);
    int32_t ProcessRemoteDelayUri(const std::string& deviceId, const AppInfo& appInfo, PasteData& data, PasteDataRecord& record, PasteDataEntry& entry);
    int32_t ProcessRemoteDelayHtml(const std::string& remoteDeviceId, const AppInfo& appInfo, const std::vector<uint8_t>& rawData, PasteData& data, PasteDataRecord& record, PasteDataEntry& entry);
    int32_t ProcessRemoteDelayHtmlInner(const std::string& remoteDeviceId, const AppInfo& appInfo, PasteData& tmpData, PasteData& data, PasteDataEntry& entry);
    
    // Getter管理
    void NotifyDelayGetterDied(int32_t userId);
    void NotifyEntryGetterDied(int32_t userId);
    
private:
    PasteboardService& service_;
    ConcurrentMap<int32_t, std::pair<sptr<IPasteboardDelayGetter>, sptr<DelayGetterDeathRecipient>>> delayGetters_;
    ConcurrentMap<int32_t, std::pair<sptr<IPasteboardEntryGetter>, sptr<EntryGetterDeathRecipient>>> entryGetters_;
};
```

**迁移方法**：
- GetDelayPasteRecord()
- GetDelayPasteData()
- GetFullDelayPasteData()
- SyncDelayedData()
- ProcessDelayHtmlEntry()
- PostProcessDelayHtmlEntry()
- GetLocalEntryValue()
- GetRemoteEntryValue()
- ProcessRemoteDelayUri()
- ProcessRemoteDelayHtml()
- ProcessRemoteDelayHtmlInner()
- NotifyDelayGetterDied()
- NotifyEntryGetterDied()
- DelayGetterDeathRecipient::OnRemoteDied()
- EntryGetterDeathRecipient::OnRemoteDied()

### 9. PasteboardSystemEventListener（系统事件监听）

**职责**：监听系统能力变化、公共事件、账户状态变化、配置变化

**估算行数**：约300行

**主要接口**：
```cpp
class PasteboardSystemEventListener {
public:
    PasteboardSystemEventListener(PasteboardService& service);
    ~PasteboardSystemEventListener();
    
    // 系统能力监听
    void AddSysAbilityListener();
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId);
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId);
    
    // 具体服务监听回调
    void OnAddDeviceManager();
    void OnAddMemoryManager();
    void OnAddDeviceProfile();
    void OnRemoveDeviceProfile();
    
    // 公共事件订阅
    void CommonEventSubscriber();
    void AccountStateSubscriber();
    void PasteboardEventSubscriber();
    
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    void DistributedAccountSubscriber();
#endif
    
    // 屏幕状态
    void InitScreenStatus();
    ScreenEvent GetScreenStatus(int32_t userId);
    
    // 配置变化
    void OnConfigChange(bool isOn);
    void OnConfigChangeInner(bool isOn);
    
    // Wifi状态
    void HandleWifiOffAndClearDistributedEvent(int32_t userId);
    
private:
    PasteboardService& service_;
    std::shared_ptr<PasteBoardCommonEventSubscriber> commonEventSubscriber_;
    std::shared_ptr<PasteBoardAccountStateSubscriber> accountStateSubscriber_;
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    std::shared_ptr<PasteboardDistributedAccountSubscriber> distributedAccountSubscriber_;
#endif
};
```

**迁移方法**：
- AddSysAbilityListener()
- OnAddSystemAbility()
- OnRemoveSystemAbility()
- OnAddDeviceManager()
- OnAddMemoryManager()
- OnAddDeviceProfile()
- OnRemoveDeviceProfile()
- CommonEventSubscriber()
- AccountStateSubscriber()
- PasteboardEventSubscriber()
- DistributedAccountSubscriber()
- InitScreenStatus()
- GetScreenStatus()
- OnConfigChange()
- OnConfigChangeInner()
- HandleWifiOffAndClearDistributedEvent()

### 10. PasteboardDumpHelper（Dump调试）

**职责**：提供Dump命令、历史记录管理、数据统计

**估算行数**：约200行

**主要接口**：
```cpp
class PasteboardDumpHelper {
public:
    PasteboardDumpHelper(PasteboardService& service);
    ~PasteboardDumpHelper();
    
    // Dump命令初始化
    void InitializeDumpCommands();
    
    // 历史记录
    bool SetPasteboardHistory(HistoryInfo& info);
    static std::string GetTime();
    std::string DumpHistory() const;
    std::string DumpUserHistory(int32_t userId) const;
    
    // 数据Dump
    std::string DumpData();
    std::string DumpUserData(int32_t userId);
    
    // 统计信息
    size_t GetDataSize(PasteData& data) const;
    
private:
    PasteboardService& service_;
    static std::mutex historyMutex_;
    static std::vector<std::string> dataHistory_;
    static std::shared_ptr<Command> copyHistory;
    static std::shared_ptr<Command> copyData;
};
```

**迁移方法**：
- InitializeDumpCommands()
- SetPasteboardHistory()
- GetTime()
- DumpHistory()
- DumpUserHistory()
- DumpData()
- DumpUserData()
- GetDataSize()

## 数据流设计

### 主要业务流程

#### 1. 设置剪贴板数据流程

```
Client → PasteboardService::SetPasteData()
    ↓
    ├─→ PasteboardPermissionChecker::IsDataValid()  // 权限验证
    ├─→ PasteboardAppInfoHelper::GetAppInfo()       // 获取应用信息
    ├─→ PasteboardService::SaveData()               // 保存数据到clips_
    ├─→ PasteboardDistributedManager::SetDistributedData() // 分布式同步
    ├─→ PasteboardEntityRecognizer::RecognizePasteData()   // 实体识别
    ├─→ PasteboardObserverManager::NotifyObservers()       // 通知观察者
    └─→ PasteboardDumpHelper::SetPasteboardHistory()       // 记录历史
```

#### 2. 获取剪贴板数据流程

```
Client → PasteboardService::GetPasteData()
    ↓
    ├─→ PasteboardPermissionChecker::VerifyPermission()  // 权限验证
    ├─→ PasteboardAppInfoHelper::GetAppInfo()            // 获取应用信息
    ├─→ 判断是否有远程数据
    │   ├─ 有远程数据：
    │   │   ├─→ PasteboardP2PManager::CheckAndReuseP2PLink() // 检查P2P连接
    │   │   ├─→ PasteboardDistributedManager::GetDistributedData() // 获取分布式数据
    │   │   ├─→ PasteboardUriHandler::CheckAndGrantRemoteUri() // 授予URI权限
    │   │   └─→ PasteboardDelayDataHandler::GetRemoteEntryValue() // 处理延迟数据
    │   └─ 无远程数据：
    │       ├─→ PasteboardService::GetLocalData() // 获取本地数据
    │       └─→ PasteboardDelayDataHandler::GetLocalEntryValue() // 处理延迟数据
    ├─→ PasteboardObserverManager::NotifyObservers()  // 通知观察者
    └─→ PasteboardDumpHelper::SetPasteboardHistory()  // 记录历史
```

#### 3. P2P预同步流程

```
PasteboardService::PreSyncRemotePasteboardData()
    ↓
    ├─→ PasteboardP2PManager::RegisterPreSyncMonitor()
    ├─→ 监听网络设备变化
    ├─→ 发现新设备时：
    │   ├─→ PasteboardP2PManager::PreEstablishP2PLink()
    │   └─→ PasteboardP2PManager::OpenP2PLinkForPreEstablish()
    └─→ 定时检查和清理：PasteboardP2PManager::DeletePreSyncP2pMap()
```

### Manager之间的依赖关系

```
PasteboardService (协调者)
    ├─ 所有Manager都依赖PasteboardService（注入引用）
    └─ Manager之间无直接依赖，通过PasteboardService协调

主要协作场景：
1. 分布式数据获取需要：DistributedManager + P2PManager + UriHandler + DelayDataHandler
2. 观察者通知需要：ObserverManager + EntityRecognizer
3. 权限检查需要：PermissionChecker + AppInfoHelper
4. 系统事件处理需要：SystemEventListener + 多个Manager（根据事件类型）
```

### 共享数据访问策略

```cpp
// PasteboardService保持的共享数据成员
class PasteboardService {
private:
    // 所有Manager可通过service_引用访问这些共享数据
    ConcurrentMap<int32_t, std::shared_ptr<PasteData>> clips_;          // 剪贴板数据
    ClipPlugin::GlobalEvent currentEvent_;                              // 当前事件
    ConcurrentMap<int32_t, uint32_t> clipChangeCount_;                 // 变化计数
    ConcurrentMap<int32_t, uint64_t> copyTime_;                        // 复制时间
    std::atomic<uint32_t> dataId_;                                     // 数据ID
    std::atomic<uint32_t> delayTokenId_;                               // 延迟Token ID
    
    // Manager类内部维护自己的私有数据
    // 例如：ObserverManager维护observerLocalChangedMap_
    //      P2PManager维护p2pMap_和preSyncP2pMap_
};
```

## 错误处理设计

### 错误码规范

保持现有错误码体系，各Manager返回统一的PasteboardError错误码：

```cpp
// 错误处理原则
- 各Manager方法返回int32_t错误码
- 错误码定义在pasteboard_error.h中，保持不变
- Manager内部错误通过PASTEBOARD_HILOGE记录日志
- Manager通过回调向PasteboardService报告关键错误
```

### 异常场景处理

#### 1. 分布式数据获取失败
```cpp
- DistributedManager::GetDistributedData返回错误码
- PasteboardService回退到本地数据获取
- 记录UE事件上报
```

#### 2. P2P连接失败
```cpp
- P2PManager::EstablishP2PLink超时或失败
- 清理P2P状态，通知上层获取失败
- 可选重试机制（通过配置控制）
```

#### 3. URI权限授予失败
```cpp
- UriHandler::GrantPermission失败时返回错误
- PasteboardService可选择继续处理其他URI或中止
- 不影响本地数据访问
```

#### 4. Manager初始化失败
```cpp
- 各Manager在构造函数中初始化必要资源
- 失败时记录日志，PasteboardService可继续运行（降级模式）
- 通过IsValid()接口检查Manager状态
```

## 测试策略

### 单元测试设计

为每个Manager类编写独立单元测试：

```
services/core/test/
├── pasteboard_distributed_manager_test.cpp
├── pasteboard_p2p_manager_test.cpp
├── pasteboard_uri_handler_test.cpp
├── pasteboard_observer_manager_test.cpp
├── pasteboard_entity_recognizer_test.cpp
├── pasteboard_permission_checker_test.cpp
├── pasteboard_app_info_helper_test.cpp
├── pasteboard_delay_data_handler_test.cpp
├── pasteboard_system_event_listener_test.cpp
└── pasteboard_dump_helper_test.cpp
```

### Mock策略

#### 需要Mock的外部依赖
```cpp
- ClipPlugin: 分布式插件接口
- IBundleMgr: 应用包管理器
- AccessTokenKit: 权限管理
- WindowManager: 窗口管理
- ScreenLockManager: 屏幕锁定管理
- DistributedFileDaemonManager: 分布式文件管理
```

#### Mock PasteboardService用于Manager测试
```cpp
class MockPasteboardService {
public:
    // 提供共享数据的Mock实现
    ConcurrentMap<int32_t, std::shared_ptr<PasteData>>& GetClips();
    ClipPlugin::GlobalEvent& GetCurrentEvent();
    // ... 其他必要接口
};
```

### 测试覆盖目标

```
- 各Manager类单元测试覆盖率: >80%
- 关键业务流程集成测试覆盖
- 边界条件和异常场景测试
- 性能测试（分布式数据获取延迟）
```

## 迁移实施计划

### 分步迁移计划

#### 第1步：创建框架（1-2天）
```
任务：
- 创建10个Manager类的头文件和空实现
- 在PasteboardService中添加Manager成员变量
- 修改BUILD.gn添加新源文件

文件变更：
- 新建10个.h文件
- 新建10个.cpp文件（空实现）
- 修改pasteboard_service.h
- 修改BUILD.gn

提交：约800行
```

#### 第2步：迁移DistributedManager（2-3天）
```
任务：
- 从pasteboard_service.cpp迁移分布式相关方法
- 实现DistributedManager完整功能
- 编写单元测试
- 验证分布式功能正常

文件变更：
- pasteboard_distributed_manager.cpp: 约500行
- pasteboard_distributed_manager.h: 已在第1步创建
- pasteboard_service.cpp: 删除约15个方法（约500行）
- pasteboard_service.h: 修改调用方式（约50行）

提交：约1000行
```

#### 第3步：迁移P2PManager和UriHandler（3-4天）
```
任务：
- 同时迁移P2P和URI处理（两者关联性强）
- 实现完整功能并测试
- 验证P2P连接和URI权限授予

文件变更：
- pasteboard_p2p_manager.cpp: 约400行
- pasteboard_uri_handler.cpp: 约600行
- pasteboard_service.cpp: 删除约20个方法（约800行）
- pasteboard_service.h: 修改调用方式（约80行）

提交1：P2P迁移约800行
提交2：URI迁移约1000行
```

#### 第4步：迁移Observer和Permission相关（2-3天）
```
任务：
- 迁移ObserverManager、PermissionChecker、AppInfoHelper
- 实现观察者管理和权限检查
- 测试观察者订阅通知流程

文件变更：
- pasteboard_observer_manager.cpp: 约300行
- pasteboard_permission_checker.cpp: 约300行
- pasteboard_app_info_helper.cpp: 约200行
- pasteboard_service.cpp: 删除约25个方法（约700行）
- pasteboard_service.h: 修改调用方式（约100行）

提交：约1600行（可拆分为2次提交）
```

#### 第5步：迁移剩余Manager（3-4天）
```
任务：
- 迁移EntityRecognizer、DelayDataHandler、SystemEventListener、DumpHelper
- 实现各Manager完整功能
- 补充完整测试

文件变更：
- pasteboard_entity_recognizer.cpp: 约200行
- pasteboard_delay_data_handler.cpp: 约800行
- pasteboard_system_event_listener.cpp: 约300行
- pasteboard_dump_helper.cpp: 约200行
- pasteboard_service.cpp: 删除约30个方法（约1500行）
- pasteboard_service.h: 修改调用方式（约120行）

提交1：DelayDataHandler约1000行
提交2：其他Manager约800行
```

#### 第6步：清理和优化（1-2天）
```
任务：
- 清理pasteboard_service.cpp中已迁移的代码
- 优化接口和依赖关系
- 完整集成测试
- 代码审查

文件变更：
- pasteboard_service.cpp: 最终约800行
- pasteboard_service.h: 最终约400行
- BUILD.gn调整

提交：约500行（主要是删除残留代码和优化）
```

### 迁移时间估算

**总计：约12-18天**

### 代码迁移原则

```cpp
// 迁移原则
1. 保持方法签名不变（参数、返回值）
2. 保持方法内部实现逻辑不变
3. 将service_成员访问改为通过注入的service_引用
4. 将静态成员变量移到相应Manager类
5. 保持错误处理和日志记录方式不变
```

### BUILD.gn修改

```python
# services/core/BUILD.gn
ohos_shared_library("pasteboard_service_core") {
  sources = [
    "src/pasteboard_service.cpp",
    "src/pasteboard_distributed_manager.cpp",
    "src/pasteboard_p2p_manager.cpp",
    "src/pasteboard_uri_handler.cpp",
    "src/pasteboard_observer_manager.cpp",
    "src/pasteboard_entity_recognizer.cpp",
    "src/pasteboard_permission_checker.cpp",
    "src/pasteboard_app_info_helper.cpp",
    "src/pasteboard_delay_data_handler.cpp",
    "src/pasteboard_system_event_listener.cpp",
    "src/pasteboard_dump_helper.cpp",
    "src/pasteboard_user_context.cpp",
    "src/pasteboard_window_manager.cpp",
    "src/pasteboard_disposable_manager.cpp",
    "src/pasteboard_dialog.cpp",
    "src/pasteboard_pattern.cpp",
    "src/pasteboard_ability_manager.cpp",
    "src/pasteboard_delay_manager.cpp",
    "src/pasteboard_distributed_account_subscriber.cpp",
  ]
  
  configs = [ ":pasteboard_service_config" ]
  
  deps = [
    "//foundation/distributeddatamgr/pasteboard/frameworks/innerkits:pasteboard_inner_sdk",
  ]
  
  external_deps = [
    "access_token:libaccesstoken_sdk",
    "bundle_framework:appexecfwk_core",
    "common:libcommon",
    "data_share:datashare_common",
    "data_share:datashare_native",
    "distributed_file:libdistributed_file_daemon_manager",
    "eventhandler:libeventhandler",
    "hilog:libhilog",
    "hisysevent:libhisysevent",
    "hitrace:hitrace_meter",
    "input:libinput_method_client",
    "ipc:ipc_core",
    "os_account:os_account_innerkits",
    "safwk:system_ability_fwk",
    "samgr:samgr_proxy",
    "window_manager:libwm",
    "window_manager:libwm_lite",
  ]
  
  cflags = []
  if (product_name != "ohos_sdk") {
    cflags += [ "-fstack-protector-strong" ]
  }
  
  defines = []
  if (pasteboard_screenlock_mgr_enable) {
    defines += [ "PB_SCREENLOCK_MGR_ENABLE" ]
  }
  
  if (pasteboard_cockpit_platform_enable) {
    defines += [ "PB_COCKPIT_PLATFORM_ENABLE" ]
  }
  
  if (product_name == "ohos_sdk") {
    defines += [ "WITH_DLP" ]
  }
  
  if (use_musl) {
    if (using_cxx_exception) {
      cflags_cc = [ "-fexceptions" ]
    } else {
      cflags_cc = [ "-fno-exceptions" ]
    }
  }
  
  install_enable = true
  subsystem_name = "distributeddatamgr"
  part_name = "pasteboard"
}
```

### Git提交策略

每次迁移一个Manager类时：
```
1. 添加Manager头文件和实现
2. 修改PasteboardService.h添加Manager成员
3. 从pasteboard_service.cpp删除已迁移的方法
4. 修改BUILD.gn
5. 单次提交控制在2000行以内（符合要求）
6. 提交消息格式：
   "refactor: migrate [ManagerName] from pasteboard_service"
   "Split pasteboard_service.cpp for better maintainability"
```

## 风险和缓解措施

### 识别的风险

#### 1. 功能回归风险
**风险描述**：重构过程中可能引入功能bug，导致现有功能失效

**缓解措施**：
- 逐步迁移，每次迁移后进行功能验证
- 保持现有单元测试，确保基础功能不回归
- 在真实设备上进行集成测试
- 保留回退方案，可快速恢复到旧版本

#### 2. 性能下降风险
**风险描述**：引入额外的Manager调用层可能影响性能

**缓解措施**：
- Manager通过引用注入，避免指针开销
- 关键路径上的Manager调用设计为内联或轻量级
- 性能测试验证关键业务流程延迟
- 如发现性能下降，优化热点路径

#### 3. 编译依赖风险
**风险描述**：新增Manager类可能引入新的头文件依赖

**缓解措施**：
- Manager头文件只包含必要的接口声明
- 实现文件中包含具体依赖
- 使用前向声明减少头文件依赖
- 编译测试验证依赖关系正确

#### 4. 单次提交超限风险
**风险描述**：某次迁移可能超过2000行提交限制

**缓解措施**：
- 提前估算每次迁移的行数变化
- 将大型迁移拆分为多次提交
- 优先迁移行数较少的Manager
- 监控每次提交的行数统计

### 风险监控

```
- 每次迁移后运行现有单元测试
- 每次迁移后在真实设备验证关键功能
- 定期进行性能基准测试
- 监控编译时间和编译产物大小
- 代码审查检查接口设计和依赖关系
```

## 成功标准

### 功能验证标准

```
✓ 所有现有单元测试通过
✓ 分布式剪贴板功能正常（跨设备复制粘贴）
✓ 本地剪贴板功能正常（同设备复制粘贴）
✓ P2P预同步功能正常
✓ URI权限授予功能正常
✓ 观察者订阅和通知功能正常
✓ 延迟数据处理功能正常
✓ 权限验证功能正常
✓ Dump调试功能正常
```

### 性能验证标准

```
✓ 本地剪贴板操作延迟无明显增加（<5%）
✓ 分布式剪贴板操作延迟无明显增加（<10%）
✓ 服务启动时间无明显增加（<5%）
✓ 内存占用无明显增加（<10%）
```

### 架构验证标准

```
✓ pasteboard_service.cpp行数降至约800行
✓ 各Manager类行数符合估算（误差<20%）
✓ 单元测试覆盖率达标（>80%）
✓ 单次Git提交不超过2000行
✓ 编译产物大小无明显变化
✓ 头文件依赖关系清晰合理
```

### 可维护性验证标准

```
✓ 每个Manager类职责清晰单一
✓ Manager类之间无循环依赖
✓ PasteboardService作为协调者，职责明确
✓ 共享数据访问策略合理
✓ 错误处理方式统一
✓ 日志记录位置合理
```

## 后续优化方向

### 短期优化（重构完成后1-3个月）

```
1. 补充完整的单元测试覆盖
2. 性能热点分析和优化
3. 代码审查和接口优化
4. 文档完善（API文档、架构文档）
```

### 中期优化（3-6个月）

```
1. 探索Manager类的进一步拆分（如DelayDataHandler仍较大）
2. 引入依赖注入框架，优化Manager创建流程
3. 探索异步处理机制，提高并发性能
4. 增加更完善的性能监控和统计
```

### 长期优化（6-12个月）

```
1. 探索插件化架构，支持动态加载Manager
2. 引入更完善的测试框架（如Mock框架）
3. 探索分布式剪贴板的性能优化（如预加载策略）
4. 完善错误处理和降级机制
```

## 总结

本设计方案通过职责驱动拆分策略，将5569行的pasteboard_service.cpp拆分为10个职责清晰的Manager类，主服务类降至约800行。该方案能有效达成提高可维护性、便于单元测试、提高代码复用性、降低编译依赖的目标，并通过分步迁移策略确保单次提交不超过2000行约束。

关键设计决策：
- 采用职责驱动拆分而非分层拆分，确保每个Manager类职责单一清晰
- Manager通过依赖注入方式访问PasteboardService共享数据，避免Manager之间直接依赖
- 保持现有接口和实现逻辑不变，仅重构结构，降低回归风险
- 分6步逐步迁移，每步控制在2000行以内，符合提交约束

预期收益：
- 单文件复杂度降低约85%（从5569行降至800行）
- 单元测试覆盖率提升至80%以上
- 编译依赖优化，修改某Manager只需重新编译该模块
- 代码复用性提高，各Manager可独立使用