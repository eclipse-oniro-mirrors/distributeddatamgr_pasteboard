# 多前台用户剪贴板管理测试设计文档

## 1. 测试目标

验证UserContextResolver实现的多用户剪贴板管理功能，确保在多前台用户场景下剪贴板数据的正确隔离和访问。

## 2. 测试范围

### 2.1 核心功能改动
- **UserContextResolver类** - 用户上下文解析
- **GetCurrentAccountId改造** - 从查询活跃用户改为解析调用者用户
- **ResolveMainDisplayUserId** - 主显示器用户解析
- **ClearByResolvedUser** - 按用户清理剪贴板数据
- **用户切换事件处理** - 新增清理旧用户数据
- **DumpHistory/DumpData多用户支持** - 支持多前台用户数据展示
- **AppExit userId支持** - 按用户退出应用
- **ClearUriOnUninstall userId支持** - 按用户清理URI

### 2.2 UserContextSource来源类型
```cpp
enum class UserContextSource : int32_t {
    CALLER = 0,           // 调用者用户
    MAIN_DISPLAY,         // 主显示器用户
    FOREGROUND,           // 前台用户
    USER_SWITCHED_NEW,    // 用户切换-新用户
    USER_SWITCHED_OLD,    // 用户切换-旧用户
    USER_STOPPING,        // 用户停止
    PACKAGE_REMOVED,      // 应用卸载
};
```

## 3. 测试环境准备

### 3.1 设备要求
- 支持多用户的OpenHarmony设备（如RK3568）
- 至少配置2个用户账号（主用户+访客用户）

### 3.2 用户配置
```
用户100（主用户）：
  - 用户ID: 100
  - 显示ID: 0（主显示器）
  - 状态：前台活跃

用户101（访客用户）：
  - 用户ID: 101  
  - 显示ID: 1（副显示器）
  - 状态：前台活跃
```

### 3.3 应用安装
- 安装剪贴板测试应用（能进行复制粘贴操作）
- 安装日志查看工具
- 安装hdc命令行工具

## 4. 功能测试场景

### 4.1 UserContextResolver基础功能测试

#### TC-001: ResolveCallingUser测试
**目的**：验证调用者用户上下文解析正确性

**步骤**：
1. 主用户（100）调用剪贴板API
2. 访客用户（101）调用剪贴板API  
3. 检查解析的用户ID

**预期结果**：
```log
[INFO] ResolveCallingUser: uid=20010001, userId=100, isValid=true
[INFO] ResolveCallingUser: uid=20010101, userId=101, isValid=true
```

#### TC-002: ResolveMainDisplayUser测试
**目的**：验证主显示器用户解析

**步骤**：
1. 主显示器（displayId=0）活跃用户为100
2. 调用ResolveMainDisplayUser
3. 验证返回userId=100

**预期结果**：
```log
[INFO] ResolveMainDisplayUser: displayId=0, userId=100, isValid=true
```

#### TC-003: ResolveForegroundUsers测试  
**目的**：验证多前台用户解析

**步骤**：
1. 主用户100在主显示器前台
2. 访客用户101在副显示器前台
3. 调用ResolveForegroundUsers

**预期结果**：
```log
[INFO] ResolveForegroundUsers: found 2 users
[INFO] - UserId=100, displayId=0, source=FOREGROUND
[INFO] - UserId=101, displayId=1, source=FOREGROUND
```

### 4.2 用户切换场景测试

#### TC-004: 用户切换数据清理测试
**目的**：验证用户切换时旧用户数据清理

**步骤**：
1. 用户100复制文本"Hello from user 100"
2. 切换到用户101  
3. 观察日志中的清理操作
4. 用户100重新激活，验证剪贴板是否保留

**预期日志**：
```log
[INFO] user id switched: new=101, old=100
[INFO] ClearByResolvedUser: userId=100, clearing clipboard data
[INFO] SetSwitch end, newUser=101, oldUser=100
```

**验证点**：
- 旧用户剪贴板数据已清理
- 新用户剪贴板为空或保持自己的数据

#### TC-005: 用户停止清理测试
**目的**：验证用户停止事件处理

**步骤**：
1. 用户101停止运行（退出登录）
2. 观察清理操作

**预期日志**：
```log  
[INFO] user id is stopping: userId=101
[INFO] ClearByResolvedUser: userId=101, clearing clipboard data
```

### 4.3 多用户并发访问测试

#### TC-006: 双用户同时复制粘贴
**目的**：验证多用户并发剪贴板操作隔离性

**步骤**：
1. 用户100在主显示器复制"Text A"
2. 用户101在副显示器复制"Text B"  
3. 各用户分别粘贴
4. 验证数据隔离

**预期结果**：
```
用户100粘贴结果：Text A（自己的数据）
用户101粘贴结果：Text B（自己的数据）
```

**日志验证**：
```log
[INFO] GetPasteData: userId=100, returning user-specific data
[INFO] GetPasteData: userId=101, returning user-specific data  
```

#### TC-007: 多用户应用退出测试
**目的**：验证AppExit按用户清理

**步骤**：
1. 用户100的应用A（pid=1234）注册剪贴板观察者
2. 用户101的应用B（pid=5678）注册剪贴板观察者
3. 应用A退出
4. 验证只清理用户100的观察者

**预期日志**：
```log
[INFO] AppExit: pid=1234, userId=100
[INFO] RemoveObserverByPid: userId=100, pid=1234  
[INFO] RemoveObserverByPid: userId=101, pid=5678 (not removed)
```

### 4.4 应用卸载场景测试

#### TC-008: 按用户清理URI权限
**目的**：验证ClearUriOnUninstall按用户清理

**步骤**：
1. 用户100的应用C安装，复制包含URI的数据
2. 用户101的应用D安装，复制包含URI的数据
3. 卸载应用C（用户100）
4. 验证只清理用户100的URI权限

**预期日志**：
```log
[INFO] ClearUriOnUninstall: tokenId=xxx, userId=100
[INFO] clear uri, tokenId=xxx, userId=100  
```

## 5. 数据展示测试

### 5.1 Dump命令测试

#### TC-009: DumpData多用户展示
**目的**：验证dump命令展示多用户数据

**步骤**：
1. 用户100复制"User 100 data"
2. 用户101复制"User 101 data"  
3. 执行`hdc shell "dumpsys pasteboard"`

**预期输出**：
```
UserId: 100
PasteData: User 100 data
  - MIME: text/plain
  - TokenId: xxx

UserId: 101  
PasteData: User 101 data
  - MIME: text/plain
  - TokenId: yyy
```

#### TC-010: DumpHistory多用户历史
**目的**：验证历史记录展示多用户

**步骤**：
1. 用户100执行多次复制操作
2. 用户101执行多次复制操作
3. 执行`hdc shell "dumpsys pasteboard --history"`

**预期输出**：
```
UserId: 100
History:
  - 2026-05-17 10:00: Copy "text1" from app A
  - 2026-05-17 10:05: Copy "text2" from app B

UserId: 101
History:  
  - 2026-05-17 10:02: Copy "data1" from app C
  - 2026-05-17 10:08: Copy "data2" from app D
```

## 6. 边界和异常测试

### 6.1 错误场景测试

#### TC-011: 无效用户ID处理
**目的**：验证ERROR_USERID处理

**步骤**：
1. 模拟无效的uid调用
2. 验证返回ERROR_USERID=-1

**预期日志**：
```log
[ERROR] resolve calling user failed, uid=-1, ret=xxx
[ERROR] clear resolved user failed, userId invalid
```

#### TC-012: 用户切换失败处理
**目的**：验证切换失败时不影响系统

**步骤**：
1. 模拟用户切换事件失败
2. 验证系统稳定性

**预期结果**：
- 不清理旧用户数据
- 系统继续运行

### 6.2 性能测试

#### TC-013: 多用户并发性能
**目的**：验证多用户场景性能

**步骤**：
1. 5个前台用户同时操作
2. 测试响应时间

**预期指标**：
- 单次复制操作 < 50ms
- 用户切换清理 < 100ms
- ResolveForegroundUsers < 10ms

## 7. 兼容性测试

### 7.1 向后兼容测试

#### TC-014: 单用户场景兼容
**目的**：验证单用户场景功能正常

**步骤**：
1. 只激活用户100
2. 执行常规剪贴板操作

**预期结果**：
- 功能与之前版本一致
- 日志显示单用户处理

#### TC-015: API兼容性测试  
**目的**：验证原有API仍然正常

**步骤**：
1. 测试现有应用使用剪贴板API
2. 验证无兼容性问题

## 8. 测试执行步骤

### 8.1 基础测试流程
```bash
# 1. 编译并部署
hb build pasteboard -i --skip-prebuilts
hdc file send libpasteboard_service.z.so /system/lib/

# 2. 重启剪贴板服务
hdc shell "restart pasteboard_service"

# 3. 配置多用户
hdc shell "accountmanager create-user guest"

# 4. 启动日志监控
hdc shell "hilog -r && hilog -t PasteboardService"

# 5. 执行测试场景
# 根据上述TC执行对应操作

# 6. 检查日志和结果
hdc shell "hilog | grep -E 'userId|ClearBy|Resolve'"
hdc shell "dumpsys pasteboard"
```

### 8.2 日志关键字监控
```bash
# 监控用户上下文解析
hilog | grep "ResolveCallingUser\|ResolveMainDisplayUser\|ResolveForegroundUsers"

# 监控用户切换
hilog | grep "user id switched\|ClearByResolvedUser\|SetSwitch end"

# 监控数据隔离  
hilog | grep "userId=100\|userId=101"

# 监控清理操作
hilog | grep "clear uri\|ClearByResolvedUser"
```

## 9. 测试数据准备

### 9.1 测试应用清单
```
- ClipboardTestApp：基本复制粘贴功能
- MultiUserTestApp：支持指定userId的剪贴板操作  
- UriTestApp：包含URI的剪贴板数据
- ObserverTestApp：注册剪贴板观察者
```

### 9.2 测试数据类型
```cpp
// 文本数据
PasteData record1;
record1.AddEntry("text/plain", "Test data for user X");

// URI数据  
PasteData record2;
record2.AddUri("file:///data/user/X/test.txt");

// 多媒体数据
PasteData record3;
record3.AddPixelMap(pixelMap);
```

## 10. 预期观察到的关键现象

### 10.1 用户切换时
✅ 看到新旧用户ID日志
✅ 旧用户剪贴板数据被清理
✅ 新用户剪贴板状态正确初始化

### 10.2 多用户并发时
✅ 每个用户只能访问自己的数据
✅ 用户间数据完全隔离
✅ 操作记录按用户分开记录

### 10.3 应用卸载时  
✅ 只清理卸载应用所在用户的URI权限
✅ 不影响其他用户的剪贴板数据

### 10.4 Dump命令输出
✅ 显示所有前台用户的数据
✅ 每个用户数据清晰标识userId
✅ 历史记录按用户分组

### 10.5 性能表现
✅ 用户切换无明显卡顿
✅ 多用户操作响应及时
✅ 内存使用合理增长

## 11. 测试通过标准

### 11.1 功能标准
- 所有TC-001至TC-015测试场景通过
- 无数据泄露或错乱
- 日志输出完整准确

### 11.2 性能标准  
- 用户切换响应 < 100ms
- 多用户并发无阻塞
- 内存增长 < 50MB/用户

### 11.3 稳定性标准
- 连续运行24小时无异常
- 1000次用户切换无错误
- 多用户压力测试稳定

## 12. 测试报告模板

### 12.1 测试执行记录表
| TC编号 | 测试场景 | 执行时间 | 结果 | 日志验证 | 备注 |
|--------|----------|----------|------|----------|------|
| TC-001 | ResolveCallingUser | 2026-05-17 | PASS | ✓ |  |
| TC-002 | ResolveMainDisplayUser | 2026-05-17 | PASS | ✓ |  |
| ... | ... | ... | ... | ... | ... |

### 12.2 问题记录表
| 问题ID | 发现时间 | TC编号 | 问题描述 | 严重程度 | 状态 | 修复版本 |
|--------|----------|--------|----------|----------|------|----------|
| ISSUE-001 | 2026-05-17 | TC-004 | 切换后旧用户数据未清理 | High | Fixed | v2.0 |

## 13. 自动化测试建议

### 13.1 自动化脚本示例
```python
def test_user_switch_cleanup():
    # 创建用户101
    hdc_shell("accountmanager create-user guest")
    
    # 用户100复制数据
    switch_to_user(100)
    copy_text("Hello from user 100")
    
    # 切换到用户101
    switch_to_user(101)
    
    # 检查清理日志
    log = hdc_shell("hilog | grep ClearByResolvedUser")
    assert "userId=100" in log
    
    # 用户100数据应为空或不存在
    paste_result = paste_text()
    assert paste_result != "Hello from user 100"
```

### 13.2 持续集成配置
```yaml
# CI配置
test_multi_user_pasteboard:
  stage: test
  script:
    - hb build pasteboard -t --skip-prebuilts
    - python3 test_multi_user.py
  artifacts:
    paths:
      - test_results/
      - logs/
```

---

**文档版本**: v1.0  
**编写日期**: 2026-05-17  
**适用版本**: feat/multi-foreground-user-pasteboard分支  
**测试负责人**: yangxiaodong41