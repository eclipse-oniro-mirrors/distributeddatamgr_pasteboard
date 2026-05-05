# ohos-pasteboard 测试用例

## 测试覆盖概览

| 分类 | 测试数量 | 覆盖命令 |
|----------|------------|------------------|
| 解析器测试 | 15 | set-data、has-data-type |
| 输出打印测试 | 3 | 所有（JSON 输出格式验证） |
| 错误处理测试 | 12 | 所有（错误码验证） |
| 执行器测试 | 10 | 所有（命令注册、帮助） |
| 集成测试 | 8 | 所有（工作流验证） |
| **总计** | **46** | **所有 6 个命令** |

## 命令测试矩阵

### set-data

| 测试用例 | 命令示例 | 说明 | 权限 | 前置依赖 | 预期结果 |
|-----------|-----------------|-------------|------------|------------|-----------------|
| 仅文本 | `ohos-pasteboard set-data --text "Hello World"` | 设置纯文本内容 | 无 | 无 | 成功：`{type:"result",status:"success",data:{primaryMimeType:"text/plain",recordCount:1}}` |
| 仅 HTML | `ohos-pasteboard set-data --html "<p>Hello</p>"` | 设置 HTML 内容 | 无 | 无 | 成功：`{type:"result",status:"success",data:{primaryMimeType:"text/html",recordCount:1}}` |
| 仅 URI | `ohos-pasteboard set-data --uri "file:///path/to/file"` | 设置 URI 内容 | 无 | 无 | 成功：`{type:"result",status:"success",data:{primaryMimeType:"text/uri",recordCount:1}}` |
| 混合参数 | `ohos-pasteboard set-data --text "plain" --html "<p>html</p>" --uri "file:///test"` | 设置多种数据类型 | 无 | 无 | 成功：多记录剪贴板 |
| 无参数 | `ohos-pasteboard set-data` | 缺少必需数据 | 无 | 无 | 失败：`ERR_ARG_INVALID` |
| 缺少值 | `ohos-pasteboard set-data --text` | 参数缺少值 | 无 | 无 | 失败：`ERR_ARG_INVALID` |
| 无效参数 | `ohos-pasteboard set-data --invalid "value"` | 未知参数 | 无 | 无 | 失败：`ERR_ARG_INVALID` |
| 重复参数 | `ohos-pasteboard set-data --text "first" --text "second"` | 多次设置相同参数 | 无 | 无 | 成功：多个条目 |

### get-data

| 测试用例 | 命令示例 | 说明 | 权限 | 前置依赖 | 预期结果 |
|-----------|-----------------|-------------|------------|------------|-----------------|
| 获取数据 | `ohos-pasteboard get-data` | 读取剪贴板内容 | `ohos.permission.READ_PASTEBOARD` | 无 | 成功：`{type:"result",status:"success",data:{recordCount:1,mimeTypes:[...],records:[...]}}` |
| 设置文本后获取 | `ohos-pasteboard set-data --text "test"`; `ohos-pasteboard get-data` | 设置文本后获取 | `ohos.permission.READ_PASTEBOARD` | set-data | 成功，包含文本数据 |
| 设置 HTML 后获取 | `ohos-pasteboard set-data --html "<p>test</p>"`; `ohos-pasteboard get-data` | 设置 HTML 后获取 | `ohos.permission.READ_PASTEBOARD` | set-data | 成功，包含 HTML 数据 |
| 设置 URI 后获取 | `ohos-pasteboard set-data --uri "file:///test"`; `ohos-pasteboard get-data` | 设置 URI 后获取 | `ohos.permission.READ_PASTEBOARD` | set-data | 成功，包含 URI 数据 |
| 空剪贴板获取 | `ohos-pasteboard clear-data`; `ohos-pasteboard get-data` | 清空后获取 | `ohos.permission.READ_PASTEBOARD` | clear-data | 成功：空记录 |

### clear-data

| 测试用例 | 命令示例 | 说明 | 权限 | 前置依赖 | 预期结果 |
|-----------|-----------------|-------------|------------|------------|-----------------|
| 清空数据 | `ohos-pasteboard clear-data` | 清空剪贴板所有数据 | 无 | 无 | 成功：`{type:"result",status:"success",data:{message:"Clear paste data successfully"}}` |
| 设置后清空 | `ohos-pasteboard set-data --text "test"`; `ohos-pasteboard clear-data` | 设置数据后清空 | 无 | set-data | 成功 |

### has-data

| 测试用例 | 命令示例 | 说明 | 权限 | 前置依赖 | 预期结果 |
|-----------|-----------------|-------------|------------|------------|-----------------|
| 检查状态 | `ohos-pasteboard has-data` | 检查剪贴板是否有内容 | 无 | 无 | 成功：`{type:"result",status:"success",data:{hasData:boolean,message:"true/false"}}` |
| 设置后检查 | `ohos-pasteboard set-data --text "test"`; `ohos-pasteboard has-data` | 设置后检查 | 无 | set-data | 成功：`hasData:true` |
| 清空后检查 | `ohos-pasteboard clear-data`; `ohos-pasteboard has-data` | 清空后检查 | 无 | clear-data | 成功：`hasData:false` |

### has-data-type

| 测试用例 | 命令示例 | 说明 | 权限 | 前置依赖 | 预期结果 |
|-----------|-----------------|-------------|------------|------------|-----------------|
| 检查纯文本 | `ohos-pasteboard has-data-type --type text/plain` | 检查纯文本类型 | 无 | 无 | 成功：`{type:"result",status:"success",data:{hasType:boolean,type:"text/plain"}}` |
| 检查 HTML | `ohos-pasteboard has-data-type --type text/html` | 检查 HTML 类型 | 无 | 无 | 成功，包含 HTML 检查结果 |
| 检查 URI | `ohos-pasteboard has-data-type --type text/uri` | 检查 URI 类型 | 无 | 无 | 成功，包含 URI 检查结果 |
| 带超时 | `ohos-pasteboard has-data-type --type text/plain --timeout 1000` | 1000毫秒超时检查 | 无 | 无 | 成功：包含 timeout 字段 |
| 最小超时（1ms） | `ohos-pasteboard has-data-type --type text/plain --timeout 1` | 最小超时值 | 无 | 无 | 成功 |
| 最大超时（5000ms） | `ohos-pasteboard has-data-type --type text/plain --timeout 5000` | 最大超时值 | 无 | 无 | 成功 |
| 无效超时（0） | `ohos-pasteboard has-data-type --type text/plain --timeout 0` | 超时值低于范围 | 无 | 无 | 失败：`ERR_ARG_OUT_OF_RANGE` |
| 超范围（6000） | `ohos-pasteboard has-data-type --type text/plain --timeout 6000` | 超时值高于范围 | 无 | 无 | 失败：`ERR_ARG_OUT_OF_RANGE` |
| 缺少类型 | `ohos-pasteboard has-data-type` | 必填参数缺失 | 无 | 无 | 失败：`ERR_ARG_MISSING` |
| 类型不匹配 | `ohos-pasteboard set-data --text "test"`; `ohos-pasteboard has-data-type --type image/png` | 类型不存在 | 无 | set-data | 成功：`hasType:false` |
| 清空后检查 | `ohos-pasteboard clear-data`; `ohos-pasteboard has-data-type --type text/plain` | 清空后检查类型 | 无 | clear-data | 成功：`hasType:false` |

### has-remote-data

| 测试用例 | 命令示例 | 说明 | 权限 | 前置依赖 | 预期结果 |
|-----------|-----------------|-------------|------------|------------|-----------------|
| 检查远端数据 | `ohos-pasteboard has-remote-data` | 检查数据是否来自远端设备 | 无 | 无 | 成功：`{type:"result",status:"success",data:{hasRemoteData:boolean,message:"true/false"}}` |
| 本地数据检查 | `ohos-pasteboard set-data --text "test"`; `ohos-pasteboard has-remote-data` | 设置本地数据后检查 | 无 | set-data | 成功：`hasRemoteData:false` |

### 帮助命令

| 测试用例 | 命令示例 | 说明 | 权限 | 前置依赖 | 预期结果 |
|-----------|-----------------|-------------|------------|------------|-----------------|
| 全局帮助 | `ohos-pasteboard --help` | 显示全局帮助 | 无 | 无 | 帮助输出到 stderr，stdout 为空 |
| 空参数 | `ohos-pasteboard` | 无参数调用 | 无 | 无 | 帮助输出到 stderr，stdout 为空 |
| 命令帮助 | `ohos-pasteboard set-data --help` | 显示 set-data 帮助 | 无 | 无 | 帮助输出到 stderr，stdout 为空 |
| has-data-type 帮助 | `ohos-pasteboard has-data-type --help` | 显示 has-data-type 帮助 | 无 | 无 | 帮助输出到 stderr，stdout 为空 |

### 错误场景

| 测试用例 | 命令示例 | 说明 | 权限 | 前置依赖 | 预期结果 |
|-----------|-----------------|-------------|------------|------------|-----------------|
| 未知命令 | `ohos-pasteboard invalid-cmd` | 无效命令名 | 无 | 无 | 失败：`{type:"result",status:"failed",errCode:"ERR_CMD_INVALID",errMsg:"Unknown command: invalid-cmd",suggestion:"..."}` |

## 集成测试工作流

### 工作流 1：完整操作序列

```bash
# 1. 检查初始状态
ohos-pasteboard has-data

# 2. 设置文本数据
ohos-pasteboard set-data --text "workflow-test"

# 3. 验证数据存在
ohos-pasteboard has-data

# 4. 检查特定类型
ohos-pasteboard has-data-type --type text/plain

# 5. 获取数据
ohos-pasteboard get-data

# 6. 清空剪贴板
ohos-pasteboard clear-data

# 7. 验证已清空
ohos-pasteboard has-data
```

### 工作流 2：多种数据类型

```bash
# 1. 设置 HTML 数据
ohos-pasteboard set-data --html "<p>html-test</p>"

# 2. 检查 HTML 类型
ohos-pasteboard has-data-type --type text/html

# 3. 获取数据
ohos-pasteboard get-data

# 4. 设置 URI 数据
ohos-pasteboard set-data --uri "file:///test/path"

# 5. 检查 URI 类型
ohos-pasteboard has-data-type --type text/uri

# 6. 获取数据
ohos-pasteboard get-data
```

### 工作流 3：超时测试

```bash
# 1. 设置数据
ohos-pasteboard set-data --text "timeout-test"

# 2. 带超时检查
ohos-pasteboard has-data-type --type text/plain --timeout 1000

# 3. 检查不匹配类型并带超时
ohos-pasteboard has-data-type --type image/jpeg --timeout 500
```

### 工作流 4：混合参数

```bash
# 1. 同时设置三种类型
ohos-pasteboard set-data --text "plain-text" --html "<p>html</p>" --uri "file:///test"

# 2. 验证每种类型存在
ohos-pasteboard has-data-type --type text/plain
ohos-pasteboard has-data-type --type text/html
ohos-pasteboard has-data-type --type text/uri

# 3. 获取所有记录
ohos-pasteboard get-data
```

### 工作流 5：顺序操作

```bash
# 1. 设置并获取文本
ohos-pasteboard set-data --text "seq-test-1"
ohos-pasteboard get-data

# 2. 设置并获取 HTML
ohos-pasteboard set-data --html "<p>seq-test-2</p>"
ohos-pasteboard get-data

# 3. 设置并获取 URI
ohos-pasteboard set-data --uri "file:///seq-test-3"
ohos-pasteboard get-data

# 4. 清空并验证
ohos-pasteboard clear-data
ohos-pasteboard has-data
```

## 参数验证测试

### 超时范围验证

| 超时值 | 有效？ | 范围 | 测试结果 |
|---------------|--------|-------|-------------|
| 0 | ❌ 否 | 1-5000 | `ERR_ARG_OUT_OF_RANGE` |
| 1 | ✅ 是 | 最小值 | 成功 |
| 100 | ✅ 是 | 范围内 | 成功 |
| 1000 | ✅ 是 | 范围内 | 成功 |
| 3000 | ✅ 是 | 范围内 | 成功 |
| 5000 | ✅ 是 | 最大值 | 成功 |
| 6000 | ❌ 否 | 超过最大值 | `ERR_ARG_OUT_OF_RANGE` |

### MIME 类型值

| MIME 类型 | 测试中使用 | 说明 |
|-----------|---------------|-------|
| text/plain | ✅ 是 | 主要文本类型 |
| text/html | ✅ 是 | HTML 类型 |
| text/uri | ✅ 是 | URI 类型 |
| image/png | ✅ 是 | 不匹配测试 |
| image/jpeg | ✅ 是 | 时不匹配测试 |

## JSON 输出验证

所有测试用例验证 JSON 输出结构：

```json
// 成功格式
{
  "type": "result",
  "status": "success",
  "data": { ... }
}

// 失败格式
{
  "type": "result",
  "status": "failed",
  "errCode": "ERR_XXX",
  "errMsg": "...",
  "suggestion": "..."
}
```

**验证检查项**：
- `type` 字段始终等于 `"result"`
- `status` 为 `"success"` 或 `"failed"`（不是 `"error"`）
- 失败响应不包含 `data` 字段
- 失败响应必须包含 `errCode`、`errMsg`、`suggestion`
- 所有输出使用 2 空格缩进

## 测试执行

```bash
# 构建测试
hb build pasteboard -t --no-prebuilt-sdk --ignore-api-check --skip-partlist-check --skip-download --skip-prebuilts

# 推送到设备
hdc file send out/standard/test/tests/unittest/pasteboard/pasteboard/ExecuteCommandTest /data/local/tmp/pasteboard_test/

# 运行测试
hdc shell "cd /data/local/tmp/pasteboard_test && ./ExecuteCommandTest"

# 预期输出
[==========] 46 tests from 5 test suites ran.
[  PASSED  ] 46 tests.
```

## 测试套件分解

| 测试套件 | 测试数量 | 覆盖范围 |
|------------|------------|----------|
| ParserTest | 15 | 参数解析验证 |
| PrinterTest | 3 | JSON 输出格式验证 |
| ErrorHandlerTest | 12 | 错误码和消息验证 |
| ExecutorTest | 10 | 命令注册和执行 |
| IntegrationTest | 8 | 完整工作流验证 |