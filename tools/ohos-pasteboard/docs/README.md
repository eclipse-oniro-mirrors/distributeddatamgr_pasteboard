# ohos-pasteboard

## 概述

剪贴板命令行工具，支持对剪贴板数据的读取、写入和查询操作。适用于设置或获取 HTML、URI 或纯文本数据。不适用于非文本数据类型的设置或获取。

## 功能列表

- **设置剪贴板数据**：将 HTML、URI 或纯文本内容写入系统剪贴板
- **获取剪贴板数据**：读取文本类型的剪贴板内容
- **清空剪贴板数据**：清空剪贴板数据内容
- **检查剪贴板状态**：检查剪贴板是否有数据
- **检查数据类型**: 检查剪贴板是否有指定类型的数据
- **检查远端数据**：检查剪贴板是否有远端设备的数据

## 依赖

### 系统能力

- `PasteboardClient` - 系统剪贴板服务客户端
- `PasteData` - 剪贴板数据结构
- `PasteDataRecord` - 单条剪贴板记录

### 权限

| 权限 | 命令 | 说明 |
|-----------|----------|-------------|
| `ohos.permission.READ_PASTEBOARD` | get-data | 读取剪贴板内容所需权限 |

## 基本用法

```bash
ohos-pasteboard <command> [options]
ohos-pasteboard --help
ohos-pasteboard <command> --help
```

## 命令列表

| 命令 | 说明 | 参数 | 权限 | 前置依赖 |
|---------|-------------|------------|-------------|--------------|
| set-data | 设置剪贴板数据，支持 HTML、URI 或纯文本内容 | `--text <string>`、`--html <string>`、`--uri <string>`（至少提供一个） | 无 | 无 |
| get-data | 读取文本类型的剪贴板数据 | 无 | `ohos.permission.READ_PASTEBOARD` | 无 |
| clear-data | 清空剪贴板数据内容 | 无 | 无 | 无 |
| has-data | 检查剪贴板是否有数据 | 无 | 无 | 无 |
| has-data-type | 检查剪贴板是否有指定类型的数据 | `--type <string>`（必填） | 无 | 无 |
| has-remote-data | 检查剪贴板是否有远端设备数据 | 无 | 无 | 无 |

**前置依赖说明**：
- **无**：命令可直接执行，无需前置条件

## 输出格式

所有命令将 JSON 输出到 stdout，结构如下：

### 成功响应
```json
{
  "type": "result",
  "status": "success",
  "data": {
    // 命令特定数据
  }
}
```

### 失败响应
```json
{
  "type": "result",
  "status": "failed",
  "errCode": "ERR_XXX",
  "errMsg": "错误描述",
  "suggestion": "建议的下一步操作"
}
```

### 错误码

| 错误码 | 说明 |
|------------|-------------|
| `ERR_ARG_INVALID` | 参数值无效 |
| `ERR_ARG_MISSING` | 缺少必需参数 |
| `ERR_CMD_INVALID` | 未知命令 |
| `ERR_CROSS_BORDER` | 屏幕状态不匹配 |
| `ERR_DATA_EXPIRED` | 剪贴板数据已过期 |
| `ERR_DATA_INVALID` | 数据格式无效 |
| `ERR_DATA_SIZE` | 数据大小超限 |
| `ERR_DESERIALIZATION` | 数据反序列化失败 |
| `ERR_GET_DATA_FAILED` | 获取数据失败 |
| `ERR_GET_REMOTE` | 获取远端数据失败 |
| `ERR_INTERNAL_ERROR` | 内部系统错误 |
| `ERR_INVALID_PARAM` | 参数无效 |
| `ERR_INVALID_USER` | 用户 ID 无效 |
| `ERR_NO_DATA` | 剪贴板无数据 |
| `ERR_NO_USER_DATA` | 当前用户无剪贴板数据 |
| `ERR_PERMISSION_DENIED` | 权限不足 |
| `ERR_REMOTE_EXCEPTION` | 远端设备异常 |
| `ERR_REMOTE_TASK` | 远端任务失败 |
| `ERR_SERIALIZATION` | 数据序列化失败 |
| `ERR_SERVICE_UNAVAILABLE` | 剪贴板服务不可用 |
| `ERR_SET_DATA_FAILED` | 设置数据失败 |
| `ERR_TASK_PROCESSING` | 任务正在处理中 |
| `ERR_TIMEOUT` | 操作超时 |

## 示例

### set-data

```bash
# 将纯文本数据写入剪贴板
ohos-pasteboard set-data --text "Hello World"

# 将 HTML 数据写入剪贴板
ohos-pasteboard set-data --html "<html><p>Hello</p></html>"

# 将 URI 数据写入剪贴板
ohos-pasteboard set-data --uri "file:///path/to/file"

# 同时设置多种数据类型（创建多记录剪贴板）
ohos-pasteboard set-data --text "Hello" --html "<p>Hello</p>"
```

### get-data

```bash
# 读取剪贴板内容
ohos-pasteboard get-data

# 输出示例：
{
  "type": "result",
  "status": "success",
  "data": {
    "recordCount": 1,
    "mimeTypes": ["text/plain"],
    "records": [
      {
        "mimeType": "text/plain",
        "plainText": "Hello World"
      }
    ]
  }
}
```

### clear-data

```bash
# 清空剪贴板所有数据
ohos-pasteboard clear-data

# 输出示例：
{
  "type": "result",
  "status": "success",
  "data": {}
}
```

### has-data

```bash
# 检查剪贴板是否有内容
ohos-pasteboard has-data

# 输出示例（有数据）：
{
  "type": "result",
  "status": "success",
  "data": {
    "hasData": true
  }
}
```

### has-data-type

```bash
# 检查是否包含纯文本数据
ohos-pasteboard has-data-type --type text/plain

# 检查是否包含 HTML 数据
ohos-pasteboard has-data-type --type text/html

# 检查是否包含 URI 数据
ohos-pasteboard has-data-type --type text/uri

# 输出示例：
{
  "type": "result",
  "status": "success",
  "data": {
    "hasType": true,
    "type": "text/plain"
  }
}
```

### has-remote-data

```bash
# 检查剪贴板数据是否来自远端设备
ohos-pasteboard has-remote-data

# 输出示例（本地数据）：
{
  "type": "result",
  "status": "success",
  "data": {
    "hasRemoteData": false
  }
}
```

## 典型工作流

```bash
# 1. 检查剪贴板状态
ohos-pasteboard has-data

# 2. 设置新数据
ohos-pasteboard set-data --text "测试内容"

# 3. 验证数据已设置
ohos-pasteboard has-data-type --type text/plain

# 4. 获取数据
ohos-pasteboard get-data

# 5. 完成后清空
ohos-pasteboard clear-data
```

## 安装

CLI 工具安装于 OpenHarmony 设备的 `/system/bin/cli_tool/executable/ohos-pasteboard`。

## 构建配置

- **构建目标**：`ohos-pasteboard`
- **子系统**：`distributeddatamgr`
- **部件**：`pasteboard`
