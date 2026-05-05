# PasteboardClient API 错误码清单

## 概述

本文档整理了 `PasteboardClient` 三个核心 API 在客户端和服务端可能返回的所有错误码：
- `SetPasteData()`
- `GetPasteData()`
- `GetDataSource()`

错误码定义见 `pasteboard/utils/native/include/pasteboard_error.h`

---

## 错误码定义（完整列表）

| 错误码 | 名称 | 数值（十六进制） | 说明 |
|-------|------|-----------------|------|
| E_OK | 成功 | 0x1A80000 | 操作成功 |
| INVALID_RETURN_VALUE_ERROR | 返回值无效 | 0x1A80001 | 返回值无效 |
| INVALID_PARAM_ERROR | 参数无效 | 0x1A80002 | 参数错误 |
| SERIALIZATION_ERROR | 序列化失败 | 0x1A80003 | TLV序列化失败 |
| DESERIALIZATION_ERROR | 反序列化失败 | 0x1A80004 | TLV反序列化失败 |
| OBTAIN_SERVER_SA_ERROR | 获取服务失败 | 0x1A80005 | 无法获取PasteboardService SA |
| OTHER_ERROR | 其他错误 | 0x1A80006 | 未知错误 |
| CROSS_BORDER_ERROR | 跨边界错误 | 0x1A80007 | 屏幕状态跨边界 |
| PERMISSION_VERIFICATION_ERROR | 权限验证失败 | 0x1A80008 | 无READ_PASTEBOARD权限 |
| PARAM_ERROR | 参数错误 | 0x1A80009 | 参数错误 |
| TIMEOUT_ERROR | 超时 | 0x1A8000A | 操作超时 |
| CANCELED | 已取消 | 0x1A8000B | 操作已取消 |
| EXCEEDING_LIMIT_EXCEPTION | 超限 | 0x1A8000C | 数据大小超限 |
| TASK_PROCESSING | 任务处理中 | 0x1A8000D | 任务正在处理 |
| PROHIBIT_COPY | 禁止复制 | 0x1A8000E | 禁止复制操作 |
| UNKNOWN_ERROR | 未知错误 | 0x1A8000F | 未知错误 |
| BACKUP_EXCEPTION | 备份异常 | 0x1A80010 | 备份失败 |
| REMOTE_EXCEPTION | 远程异常 | 0x1A80011 | 远程设备异常 |
| INVALID_DATA_ERROR | 数据无效 | 0x1A80012 | 数据无效 |
| NO_DATA_ERROR | 无数据 | 0x1A80013 | 粘贴板无数据 |
| INVALID_USERID_ERROR | 用户ID无效 | 0x1A80014 | 用户ID错误 |
| REMOTE_TASK_ERROR | 远程任务错误 | 0x1A80015 | 远程任务失败 |
| INVALID_EVENT_ERROR | 事件无效 | 0x1A80016 | 分布式事件无效 |
| GET_REMOTE_DATA_ERROR | 获取远程数据失败 | 0x1A80017 | 获取远程数据失败 |
| SEND_BROADCAST_ERROR | 发送广播失败 | 0x1A80018 | 发送广播失败 |
| SYNC_DATA_ERROR | 同步数据失败 | 0x1A80019 | 同步数据失败 |
| URI_GRANT_ERROR | URI授权失败 | 0x1A8001A | URI授权失败 |
| DP_LOAD_SERVICE_ERROR | DP服务加载失败 | 0x1A8001B | DeviceProfile服务加载失败 |
| INVALID_OPTION_ERROR | ShareOption无效 | 0x1A8001C | ShareOption无效 |
| INVALID_OPERATION_ERROR | 操作无效 | 0x1A8001D | 操作无效 |
| BUTT_ERROR | 边界错误 | 0x1A8001E | 边界错误 |
| NO_TRUST_DEVICE_ERROR | 无可信设备 | 0x1A8001F | 无可信设备 |
| NO_USER_DATA_ERROR | 无用户数据 | 0x1A80020 | 用户无粘贴数据 |
| DATA_EXPIRED_ERROR | 数据过期 | 0x1A80021 | 粘贴数据已过期 |
| INVALID_DATA_SIZE | 数据大小无效 | 0x1A80022 | 数据大小错误 |
| REMOTE_DATA_SIZE_EXCEEDED | 远程数据超限 | 0x1A80023 | 远程数据大小超限 |

---

## 一、SetPasteData 错误码分析

### 1.1 Client端 (pasteboard_client.cpp:761-807)

```cpp
int32_t PasteboardClient::SetPasteData(PasteData &pasteData, 
    std::shared_ptr<PasteboardDelayGetter> delayGetter,
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters)
```

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **OBTAIN_SERVER_SA_ERROR** | `proxyService == nullptr` | 772 | 无法获取PasteboardService SA代理 |
| **WritePasteData返回的错误** | `WritePasteData失败` | 785 | 写入数据失败，可能返回：<br>- SERIALIZATION_ERROR<br>- INVALID_DATA_SIZE<br>- INVALID_DATA_ERROR |
| **ConvertErrCode转换的错误** | `proxyService->SetPasteData返回` | 788-794, 797 | 服务端返回的错误码经ConvertErrCode转换 |

**ConvertErrCode转换规则**：
- `ERR_INVALID_VALUE` → `SERIALIZATION_ERROR`
- `ERR_INVALID_DATA` → `SERIALIZATION_ERROR`
- `ERR_OK` → `E_OK`
- 其他错误码 → 保持不变

### 1.2 Service端 (pasteboard_service.cpp:2820-2855)

```cpp
int32_t PasteboardService::SetPasteData(int fd, int64_t rawDataSize, 
    const std::vector<uint8_t> &buffer,
    const sptr<IPasteboardDelayGetter> &delayGetter, 
    const sptr<IPasteboardEntryGetter> &entryGetter)
```

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **INVALID_PARAM_ERROR** | `fd < 0` | 2824 | 文件描述符无效 |
| **INVALID_DATA_SIZE** | `rawDataSize <= 0` 或 `rawDataSize > max` | 2833 | 数据大小无效（<=0或超过限制） |
| **INVALID_DATA_ERROR** | `WritePasteData失败` | 2839 | WritePasteData返回非E_OK（详见下表） |
| **NO_DATA_ERROR** | `Decode失败` | 2841 | TLV反序列化失败 |
| **SaveData返回的错误** | `SaveData失败` | 2850 | 保存数据失败 |

**WritePasteData可能返回的错误码** (pasteboard_service.cpp:2767-2800):

| 错误码 | 触发条件 | 行号 |
|-------|---------|------|
| **INVALID_DATA_SIZE** | `actualSize < 0` 或 `rawDataSize > actualSize` | 2776 |
| **INVALID_DATA_ERROR** | `mmap失败` | 2782 |
| **INVALID_DATA_ERROR** | `rawData == nullptr` | 2789 |

---

## 二、GetPasteData 错误码分析

### 2.1 Client端 (pasteboard_client.cpp:289-327)

```cpp
int32_t PasteboardClient::GetPasteData(PasteData &pasteData)
```

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **OBTAIN_SERVER_SA_ERROR** | `proxyService == nullptr` | 304 | 无法获取PasteboardService SA代理 |
| **ConvertErrCode转换的错误** | `proxyService->GetPasteData返回的realErrCode` | 313 | 服务端返回的错误码经ConvertErrCode转换 |
| **GetDataReport返回的错误** | `ret != E_OK` | 320 | GetData失败返回的错误码 |
| **SERIALIZATION_ERROR** | `ProcessPasteData失败` | 321-323 | 处理数据失败（TLV反序列化错误） |

### 2.2 Service端 (pasteboard_service.cpp:1251-1327)

```cpp
int32_t PasteboardService::GetPasteData(int &fd, int64_t &size, 
    std::vector<uint8_t> &rawData,
    const std::string &pasteId, int32_t &syncTime, int32_t &realErrCode)
```

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **INVALID_PARAM_ERROR** | `!PasteData::IsValidPasteId(pasteId)` | 1256 | pasteId格式无效 |
| **SERIALIZATION_ERROR** | `AshmemCreate失败` | 1262 | 创建共享内存失败 |
| **GetPasteDataInner返回的错误** | `GetPasteDataInner失败` | 1259 | 内部获取数据失败（详见下表） |

**GetPasteDataInner可能返回的错误码** (pasteboard_service.cpp:1284-1327):

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **PERMISSION_VERIFICATION_ERROR** | `!VerifyPermission(tokenId)` | 1301 | 无READ_PASTEBOARD权限（非开发者模式） |
| **GetData返回的错误** | `GetData失败` | 1308, 1312-1318 | 获取数据失败（详见下表） |
| **SERIALIZATION_ERROR** | `Encode失败` | 1337 | TLV序列化失败 |
| **SERIALIZATION_ERROR** | `WriteRawData失败` | 1346 | 写入原始数据失败 |
| **SERIALIZATION_ERROR** | `AshmemCreate失败` | 1352 | 创建共享内存失败 |

**GetData可能返回的错误码** (pasteboard_service.cpp:1424-1467):

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **GetLocalData返回的错误** | `GetLocalData失败` | 1444, 1465 | 获取本地数据失败（详见下表） |
| **GetRemoteData返回的错误** | `GetRemoteData失败` | 1451, 1465 | 获取远程数据失败（详见下表） |
| **REMOTE_DATA_SIZE_EXCEEDED** | 远程数据大小超限 | 1452 | 远程数据超过限制 |
| **URI_GRANT_ERROR** | URI授权失败 | 1467 | CheckAndGrantRemoteUri失败 |

**GetLocalData可能返回的错误码** (pasteboard_service.cpp:1651-1700):

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **NO_DATA_ERROR** | `clips_中无用户数据` | 1658 | 用户粘贴板无数据 |
| **IsDataValid返回的错误** | `IsDataValid失败` | 1661-1664 | 数据有效性验证失败（详见下表） |
| **INVALID_USERID_ERROR** | `copyTime_中无用户` | 1679 | 用户ID无效 |

**IsDataValid可能返回的错误码** (pasteboard_service.cpp:1003-1040):

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **INVALID_PARAM_ERROR** | `pasteData无效或IsDraggedData` | 1007 | 数据无效或为拖拽数据 |
| **DATA_EXPIRED_ERROR** | `IsDataAged()` | 1011 | 数据已过期 |
| **CROSS_BORDER_ERROR** | `屏幕状态跨边界` | 1017 | 设置数据时的屏幕状态大于当前屏幕状态 |
| **PERMISSION_VERIFICATION_ERROR** | `ShareOption::InApp且tokenId不匹配` | 1023 | 应用内分享但调用者不是设置者 |
| **INVALID_DATA_ERROR** | `ShareOption无效` | 1036 | ShareOption类型无效 |

**GetRemoteData可能返回的错误码** (pasteboard_service.cpp:1575-1607):

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **REMOTE_TASK_ERROR** | `task == nullptr` | 1580 | 远程任务创建失败 |
| **TASK_PROCESSING** | `isPasting且等待失败` | 1590 | 任务正在处理中，等待失败 |
| **INVALID_EVENT_ERROR** | `distEvt != event` | 1596 | 分布式事件不匹配 |
| **GetRemotePasteData返回的错误** | GetRemotePasteData失败 | 1606 | 获取远程粘贴数据失败（详见下表） |

**GetRemotePasteData可能返回的错误码** (pasteboard_service.cpp:1609-1649):

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **value->errorCode** | GetDistributedData返回的错误 | 1644, 1646 | 分布式数据获取失败 |
| **TIMEOUT_ERROR** | `value == nullptr` | 1648 | 获取远程数据超时 |

---

## 三、GetDataSource 错误码分析

### 3.1 Client端 (pasteboard_client.cpp:1068-1078)

```cpp
int32_t PasteboardClient::GetDataSource(std::string &bundleName)
```

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **OBTAIN_SERVER_SA_ERROR** | `proxyService == nullptr` | 1073 | 无法获取PasteboardService SA代理 |
| **ConvertErrCode转换的错误** | `proxyService->GetDataSource返回` | 1077 | 服务端返回的错误码经ConvertErrCode转换 |

### 3.2 Service端 (pasteboard_service.cpp:2736-2757)

```cpp
int32_t PasteboardService::GetDataSource(std::string &bundleName)
```

| 错误码 | 触发条件 | 行号 | 说明 |
|-------|---------|------|------|
| **INVALID_USERID_ERROR** | `userId == ERROR_USERID` | 2740 | 当前账户ID无效 |
| **NO_USER_DATA_ERROR** | `clips_.Find(userId)失败` | 2744 | 用户粘贴板无数据 |
| **REMOTE_EXCEPTION** | `data->IsRemote()` | 2748 | 数据来源于远程设备（无法获取本地bundleName） |
| **INVALID_DATA_ERROR** | `bundleName.empty()` 或长度超限 | 2754 | 无法获取应用包名或包名无效 |

---

## 四、错误码分类总结

### 4.1 按来源分类

| 类别 | 错误码 | 说明 |
|------|-------|------|
| **权限错误** | PERMISSION_VERIFICATION_ERROR | 无READ_PASTEBOARD权限或ShareOption验证失败 |
| **数据错误** | NO_DATA_ERROR, NO_USER_DATA_ERROR, INVALID_DATA_ERROR, DATA_EXPIRED_ERROR | 粘贴板无数据或数据无效 |
| **序列化错误** | SERIALIZATION_ERROR, DESERIALIZATION_ERROR | TLV序列化/反序列化失败 |
| **参数错误** | INVALID_PARAM_ERROR, INVALID_DATA_SIZE | 参数或数据大小无效 |
| **服务错误** | OBTAIN_SERVER_SA_ERROR | 无法获取PasteboardService |
| **远程错误** | REMOTE_EXCEPTION, REMOTE_TASK_ERROR, GET_REMOTE_DATA_ERROR | 远程设备相关错误 |
| **超时错误** | TIMEOUT_ERROR | 操作超时 |
| **状态错误** | CROSS_BORDER_ERROR, TASK_PROCESSING | 屏幕状态或任务状态错误 |

### 4.2 按API分类

| API | 可能返回的错误码数量 | 主要错误类型 |
|-----|-------------------|-------------|
| **SetPasteData** | 5种 | 参数错误、数据错误、序列化错误 |
| **GetPasteData** | 15种 | 权限错误、数据错误、序列化错误、远程错误、超时错误 |
| **GetDataSource** | 4种 | 用户错误、数据错误、远程错误 |

---

## 五、CLI工具错误处理建议

### 5.1 SetPasteData 错误处理

```cpp
int32_t ret = client->SetPasteData(pasteData);
if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
    switch (ret) {
        case PasteboardError::OBTAIN_SERVER_SA_ERROR:
            return PrintError("ERR_SERVICE_UNAVAILABLE", "Pasteboard service unavailable");
        case PasteboardError::INVALID_PARAM_ERROR:
            return PrintError("ERR_INVALID_PARAM", "Invalid parameter");
        case PasteboardError::INVALID_DATA_SIZE:
            return PrintError("ERR_DATA_SIZE", "Data size exceeds limit");
        case PasteboardError::INVALID_DATA_ERROR:
            return PrintError("ERR_DATA_INVALID", "Invalid paste data");
        case PasteboardError::NO_DATA_ERROR:
            return PrintError("ERR_NO_DATA", "No data to set");
        default:
            return PrintError("ERR_SET_DATA_FAILED", "Failed to set data, code: " + to_string(ret));
    }
}
```

### 5.2 GetPasteData 错误处理

```cpp
int32_t ret = client->GetPasteData(pasteData);
if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
    switch (ret) {
        case PasteboardError::OBTAIN_SERVER_SA_ERROR:
            return PrintError("ERR_SERVICE_UNAVAILABLE", "Pasteboard service unavailable");
        case PasteboardError::PERMISSION_VERIFICATION_ERROR:
            return PrintError("ERR_PERMISSION_DENIED", "Permission denied. Add READ_PASTEBOARD in module.json5");
        case PasteboardError::NO_DATA_ERROR:
            return PrintError("ERR_NO_DATA", "Pasteboard is empty");
        case PasteboardError::DATA_EXPIRED_ERROR:
            return PrintError("ERR_DATA_EXPIRED", "Paste data has expired");
        case PasteboardError::SERIALIZATION_ERROR:
            return PrintError("ERR_SERIALIZATION", "Data serialization error");
        case PasteboardError::TIMEOUT_ERROR:
            return PrintError("ERR_TIMEOUT", "Operation timeout");
        case PasteboardError::REMOTE_EXCEPTION:
            return PrintError("ERR_REMOTE", "Remote device exception");
        default:
            return PrintError("ERR_GET_DATA_FAILED", "Failed to get data, code: " + to_string(ret));
    }
}
```

### 5.3 GetDataSource 错误处理

```cpp
int32_t ret = client->GetDataSource(bundleName);
if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
    switch (ret) {
        case PasteboardError::OBTAIN_SERVER_SA_ERROR:
            return PrintError("ERR_SERVICE_UNAVAILABLE", "Pasteboard service unavailable");
        case PasteboardError::INVALID_USERID_ERROR:
            return PrintError("ERR_INVALID_USER", "Invalid user ID");
        case PasteboardError::NO_USER_DATA_ERROR:
            return PrintError("ERR_NO_USER_DATA", "No paste data for current user");
        case PasteboardError::REMOTE_EXCEPTION:
            return PrintError("ERR_REMOTE_DATA", "Data from remote device, cannot get source");
        case PasteboardError::INVALID_DATA_ERROR:
            return PrintError("ERR_INVALID_BUNDLE", "Cannot get bundle name");
        default:
            return PrintError("ERR_GET_SOURCE_FAILED", "Failed to get data source, code: " + to_string(ret));
    }
}
```

---

## 六、关键注意事项

### 6.1 权限要求

- **GetPasteData**: 需要 `ohos.permission.READ_PASTEBOARD`
- **SetPasteData**: 无权限要求（任何应用都可设置）
- **GetDataSource**: 无权限要求（任何应用都可查询）

### 6.2 数据生命周期

- 粘贴数据会过期（默认超时时间由 `agedTime_` 控制）
- 过期数据会返回 `DATA_EXPIRED_ERROR`
- 开发者模式下可绕过某些权限检查

### 6.3 远程数据特殊处理

- 远程数据无法通过 `GetDataSource` 获取 bundleName（返回 `REMOTE_EXCEPTION`)
- 远程数据可能因网络原因超时（返回 `TIMEOUT_ERROR`)
- 远程数据大小可能超限（返回 `REMOTE_DATA_SIZE_EXCEEDED`)

---

**文档生成日期**: 2026-04-30  
**源代码版本**: foundation/distributeddatamgr/pasteboard (master分支)  
**分析范围**: pasteboard_client.cpp + pasteboard_service.cpp