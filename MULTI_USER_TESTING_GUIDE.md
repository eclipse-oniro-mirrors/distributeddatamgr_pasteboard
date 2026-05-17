# 多前台用户剪贴板管理测试指南

## 测试场景和预期结果

### 场景1：用户切换

**操作步骤**：切换用户（从用户100切换到用户101）

**预期现象**：
- 用户100的剪贴板数据被清理
- 用户101的剪贴板为新状态（空或有该用户自己的数据）

**预期日志**：
```
ResolveCallingUser: uid=xxx, userId=100
user id switched: new=101, old=100
ClearByResolvedUser: userId=100
ClearInner: userId=100, bundleName=PasteboardService
ClearInner: found data for userId=100, erasing
ClearByResolvedUser: calling ClearInner for userId=100
ClearByResolvedUser completed: userId=100
SetSwitch end, newUser=101, oldUser=100
```

### 场景2：用户复制文本

**操作步骤**：用户100复制文本"Hello"

**预期现象**：
- 剪贴板数据成功存储到用户100的数据区域
- 用户101看不到用户100的数据

**预期日志**：
```
ResolveCallingUser: uid=xxx
ResolveCallingUser success: uid=xxx, userId=100
GetCurrentAccountId: return userId=100, isValid=true
GetAppInfo: tokenId=xxx
GetAppInfo: resolved userId=100 from calling user, isValid=true
GetAppInfo HAP: bundleName=com.example.app, userId=100, appIndex=0
GetAppInfo complete: bundleName=com.example.app, userId=100, tokenType=0
SetPasteData相关日志（已有）
```

### 场景3：用户粘贴数据

**操作步骤**：用户100粘贴数据

**预期现象**：
- 只能获取用户100自己的剪贴板数据
- 不会获取到用户101的数据

**预期日志**：
```
ResolveCallingUser: uid=xxx
ResolveCallingUser success: uid=xxx, userId=100
GetCurrentAccountId: return userId=100, isValid=true
GetAppInfo: tokenId=xxx
GetAppInfo: resolved userId=100 from calling user, isValid=true
GetPasteData success. appInfo.userId=100
GetPasteData相关日志（已有）
```

### 场景4：多用户同时操作

**操作步骤**：
1. 用户100复制文本"A"
2. 用户101复制文本"B"
3. 用户100粘贴
4. 用户101粘贴

**预期现象**：
- 用户100粘贴结果为"A"
- 用户101粘贴结果为"B"
- 数据完全隔离

**预期日志**：
```
# 用户100复制
ResolveCallingUser success: uid=xxx, userId=100
GetAppInfo: resolved userId=100 from calling user, isValid=true

# 用户101复制
ResolveCallingUser success: uid=xxx, userId=101
GetAppInfo: resolved userId=101 from calling user, isValid=true

# 用户100粘贴
ResolveCallingUser success: uid=xxx, userId=100
GetPasteData success. appInfo.userId=100

# 用户101粘贴
ResolveCallingUser success: uid=xxx, userId=101
GetPasteData success. appInfo.userId=101
```

### 场景5：用户停止

**操作步骤**：用户101停止（退出登录）

**预期现象**：
- 用户101的剪贴板数据被清理
- 用户100的数据不受影响

**预期日志**：
```
user id is stopping: userId=101
MakeEventContext: source=5, userId=101, isValid=true
ClearByResolvedUser: userId=101
ClearInner: userId=101, bundleName=PasteboardService
ClearByResolvedUser completed: userId=101
```

### 场景6：应用卸载

**操作步骤**：卸载用户100的应用A（该应用之前复制了包含URI的数据）

**预期现象**：
- 只清理用户100的URI权限
- 不影响用户101的剪贴板数据

**预期日志**：
```
ClearUriOnUninstall: tokenId=xxx
ResolveCallingUser: uid=xxx
ResolveCallingUser success: uid=xxx, userId=100
ClearUriOnUninstall: resolved userId=100 for tokenId=xxx
clear uri, tokenId=xxx, userId=100
```

### 场景7：应用退出

**操作步骤**：用户100的应用B（pid=1234）退出，该应用曾注册剪贴板观察者

**预期现象**：
- 只移除用户100的观察者
- 不影响用户101的观察者

**预期日志**：
```
pid=1234 exit, userId=100
RemoveObserverByPid: removing observer for userId=100, pid=1234
RemoveObserverByPid: no observer found for userId=101, pid=1234
AppExit completed for userId=100, pid=1234
```

### 场景8：主显示器用户查询

**操作步骤**：剪贴板服务初始化或查询主显示器用户

**预期现象**：
- 正确识别主显示器（displayId=0）的活跃用户

**预期日志**：
```
ResolveMainDisplayUser: displayId=0
ResolveMainDisplayUser success: displayId=0, userId=100
ResolveMainDisplayUserId: return userId=100, isValid=true
```

### 场景9：多前台用户查询（DumpData）

**操作步骤**：执行`dumpsys pasteboard`查看剪贴板数据

**预期现象**：
- 显示所有前台用户的剪贴板数据
- 每个用户数据清晰标识userId

**预期日志**：
```
ResolveForegroundUsers: start query
ResolveForegroundUsers: userId=100, displayId=0
ResolveForegroundUsers: userId=101, displayId=1
ResolveForegroundUsers success: found 2 users
```

**预期输出**：
```
UserId: 100
PasteData: [用户100的数据]

UserId: 101
PasteData: [用户101的数据]
```

### 场景10：历史记录查询（DumpHistory）

**操作步骤**：查看剪贴板历史记录

**预期现象**：
- 显示所有前台用户的历史记录
- 按用户分组展示

**预期日志**：
```
ResolveForegroundUsers: start query
ResolveForegroundUsers success: found 2 users
DumpHistory(int32_t userId) for userId=100
DumpHistory(int32_t userId) for userId=101
```

## 如何查看日志

### 方法1：实时监控日志
```bash
hdc shell hilog | grep -E "userId|ClearBy|Resolve|GetAppInfo"
```

### 方法2：查看特定模块日志
```bash
hdc shell hilog -t PasteboardService
```

### 方法3：查看历史日志
```bash
hdc shell hilog -r && hdc shell hilog -t PasteboardService > pasteboard.log
```

## 关键日志标识

| 日志关键字 | 含义 |
|-----------|------|
| ResolveCallingUser success | 成功解析调用者用户 |
| ResolveMainDisplayUser success | 成功解析主显示器用户 |
| ResolveForegroundUsers success | 成功解析前台用户列表 |
| ClearByResolvedUser | 按用户清理剪贴板数据 |
| ClearInner | 清理内部数据 |
| user id switched | 用户切换事件 |
| GetAppInfo: resolved userId | 从调用者解析userId |
| RemoveObserverByPid | 移除用户观察者 |
| ClearUriOnUninstall | 按用户清理URI |

## 日志级别说明

- **HILOGI（INFO）**：关键操作流程日志，生产环境可见
- **HILOGD（DEBUG）**：调试日志，需要开启debug级别
- **HILOGE（ERROR）**：错误日志，始终可见

## 测试要点总结

1. **用户切换**：观察清理旧用户数据的完整流程
2. **数据隔离**：不同userId的操作日志互不干扰
3. **清理操作**：所有清理操作明确标识userId
4. **解析过程**：所有解析过程有成功和失败日志
5. **观察者管理**：按userId移除观察者
6. **URI清理**：按userId清理URI权限
7. **多用户并发**：日志清晰区分不同用户

---

**版本**: v2.0（简化版）  
**更新**: 增强日志后重写  
**日期**: 2026-05-17