# ohos-pasteboard

## Overview

Pasteboard management CLI tool for OpenHarmony/HarmonyOS. This tool provides command-line interface for reading, writing, clearing and querying pasteboard data through the PasteboardClient API.

## Features

- **Set pasteboard data**: Set HTML, URI or plain text content to pasteboard
- **Get pasteboard data**: Retrieve pasteboard data in JSON format
- **Clear pasteboard**: Clear all pasteboard data
- **Check pasteboard status**: Query if pasteboard has data or specific MIME type
- **Remote data check**: Check if pasteboard contains data from distributed devices
- **Data source query**: Get the bundle name of the data source application

## Dependencies

### System Capability
- **Service**: PasteboardService (SA)
- **Client API**: PasteboardClient (inner_kits)
- **Component**: foundation/distributeddatamgr/pasteboard

### Permissions

| Permission | Required Commands | Description |
|------------|------------------|-------------|
| `ohos.permission.READ_PASTEBOARD` | `get-data` | Required for reading pasteboard data |
| `ohos.permission.SECURE_PASTE` | `get-data` | Alternative permission for secure paste operations |
| None | All other commands | No permission required for set, clear, has, and get-data-source operations |

**Note**: `set-data` does not require explicit permission based on code analysis. However, system app identity may be checked in certain scenarios.

## Basic Usage

```bash
ohos-pasteboard <command> [options]
```

## Command List

| Command | Description | Parameters | Permission | Prerequisites |
|---------|-------------|------------|------------|---------------|
| `set-data` | Set pasteboard data | --html, --uri, --text (at least one) | None | None |
| `get-data` | Get pasteboard data in JSON | None | READ_PASTEBOARD or SECURE_PASTE | None |
| `clear-data` | Clear pasteboard | None | None | None |
| `has-data` | Check if pasteboard has data | None | None | None |
| `has-data-type` | Check specific MIME type | --type (required), --timeout (optional) | None | None |
| `has-remote-data` | Check for remote data | None | None | None |
| `get-data-source` | Get data source bundle name | None | None | None |
| `help` | Show help information | [command] (optional) | None | None |

## Prerequisites Explanation

- **None**: The command can be executed directly without any prerequisites
- **Data Required**: Pasteboard must contain data for the command to return meaningful results

## Command Details

### 1. set-data

Set pasteboard data with HTML, URI or plain text content.

**Parameters**:
- `--html <string>`: HTML content (optional)
- `--uri <string>`: URI content (optional)
- `--text <string>`: Plain text content (optional)

**Rules**:
- At least one parameter must be provided
- The first parameter determines the primary MIME type:
  - `--text` → `text/plain`
  - `--html` → `text/html`
  - `--uri` → `text/uri-list`
- Multiple parameters can be combined, additional records will be added

**Examples**:
```bash
# Set plain text only (primary type: text/plain)
ohos-pasteboard set-data --text "Hello World"

# Set HTML only (primary type: text/html)
ohos-pasteboard set-data --html "<p>Hello World</p>"

# Set URI only (primary type: text/uri-list)
ohos-pasteboard set-data --uri "file:///path/to/file"

# Set multiple types - first --html determines primary type
ohos-pasteboard set-data --html "<p>Hello</p>" --text "Hello"

# Set multiple types - first --text determines primary type
ohos-pasteboard set-data --text "Hello" --uri "file:///path/to/file"
```

### 2. get-data

Get pasteboard data in structured JSON format.

**Parameters**: None

**Permission Required**: 
- `ohos.permission.READ_PASTEBOARD` OR
- `ohos.permission.SECURE_PASTE`

**Output Format**:
```json
{
  "status": "success",
  "data": {
    "records": [
      {
        "mimeType": "text/plain",
        "plainText": "Hello World"
      }
    ],
    "property": {
      "bundleName": "com.example.app",
      "timestamp": 1234567890,
      "tag": "",
      "isRemote": false
    },
    "recordCount": 1,
    "mimeTypes": ["text/plain"]
  },
  "errCode": "",
  "errMsg": ""
}
```

**Examples**:
```bash
# Get pasteboard data (requires permission)
ohos-pasteboard get-data
```

### 3. clear-data

Clear all pasteboard data.

**Parameters**: None

**Examples**:
```bash
# Clear pasteboard
ohos-pasteboard clear-data
```

### 4. has-data

Check if pasteboard contains any data.

**Parameters**: None

**Output**:
```json
{
  "status": "success",
  "data": {
    "hasData": true,
    "message": "Pasteboard contains data"
  },
  "errCode": "",
  "errMsg": ""
}
```

**Examples**:
```bash
# Check if pasteboard has data
ohos-pasteboard has-data
```

### 5. has-data-type

Check if pasteboard contains data of a specific MIME type.

**Parameters**:
- `--type <string>`: MIME type to check (required)
  - Common types: `text/plain`, `text/html`, `text/uri-list`, `image/png`
- `--timeout <int>`: Timeout in milliseconds (optional)
  - Range: 1-5000
  - Recommended: Use timeout for potentially slow operations

**Examples**:
```bash
# Check for plain text data
ohos-pasteboard has-data-type --type text/plain

# Check for HTML data
ohos-pasteboard has-data-type --type text/html

# Check for URI data
ohos-pasteboard has-data-type --type text/uri-list

# Check with timeout (3 seconds)
ohos-pasteboard has-data-type --type text/html --timeout 3000

# Check with minimum timeout
ohos-pasteboard has-data-type --type text/plain --timeout 1
```

### 6. has-remote-data

Check if pasteboard contains data from distributed devices.

**Parameters**: None

**Examples**:
```bash
# Check for remote data
ohos-pasteboard has-remote-data
```

### 7. get-data-source

Get the bundle name of the application that set the pasteboard data.

**Parameters**: None

**Examples**:
```bash
# Get data source application
ohos-pasteboard get-data-source
```

### 8. help

Show help information for the tool or specific command.

**Parameters**:
- `[command]`: Optional command name for detailed help

**Examples**:
```bash
# Show general help
ohos-pasteboard help
ohos-pasteboard --help

# Show help for specific command
ohos-pasteboard help set-data
ohos-pasteboard set-data --help

# Show help for get command
ohos-pasteboard help get-data

# Show help for has-data-type command
ohos-pasteboard help has-data-type
```

## Error Handling

The tool follows a standardized error response format:

```json
{
  "status": "error",
  "data": "",
  "errCode": "ERR_XXX",
  "errMsg": "Detailed error description",
  "suggestion": "Suggested next operation"
}
```

**Common Error Codes**:

| Error Code | Description |
|------------|-------------|
| `ERR_ARG_MISSING` | Missing required parameter |
| `ERR_ARG_INVALID` | Invalid parameter value |
| `ERR_ARG_OUT_OF_RANGE` | Parameter value out of valid range |
| `ERR_PERMISSION_DENIED` | Permission not granted |
| `ERR_INTERNAL_ERROR` | Internal system error |
| `ERR_GET_DATA_FAILED` | Failed to get pasteboard data |
| `ERR_SET_DATA_FAILED` | Failed to set pasteboard data |
| `ERR_GET_SOURCE_FAILED` | Failed to get data source |

## Output Format

**stdout**: Single-line JSON response
**stderr**: Debug and error logs

The tool ensures clean output separation:
- Business data is output to stdout in JSON format
- Debug information and errors are output to stderr

## Installation Path

The CLI tool is installed at:
```
/system/bin/ohos-pasteboard
```

## Typical Usage Scenarios

### Scenario 1: Basic Pasteboard Operations
```bash
# Set text
ohos-pasteboard set-data --text "Test message"

# Verify content
ohos-pasteboard has-data
ohos-pasteboard get-data

# Clear
ohos-pasteboard clear-data
```

### Scenario 2: Multi-Type Data
```bash
# Set HTML with text backup
ohos-pasteboard set-data --html "<h1>Title</h1>" --text "Title"

# Check types
ohos-pasteboard has-data-type --type text/html
ohos-pasteboard has-data-type --type text/plain
```

### Scenario 3: Distributed Pasteboard
```bash
# Check for remote data
ohos-pasteboard has-remote-data

# Get source if remote
ohos-pasteboard get-data-source
```

### Scenario 4: Type-Specific Query with Timeout
```bash
# Check for image data with timeout
ohos-pasteboard has-data-type --type image/png --timeout 2000
```

## Technical Notes

1. **Primary MIME Type**: The first parameter in `set-data` determines the primary record type
2. **Timeout Usage**: Use timeout parameter when checking types that may require remote data fetch
3. **Permission Configuration**: Add permissions in module.json5 for `get-data` command
4. **JSON Output**: All outputs are in standard JSON format for easy parsing
5. **No Interactive Input**: All operations are non-interactive and complete immediately

## Limitations

- Maximum 512 records per pasteboard operation (system limit)
- Timeout range: 1-5000 milliseconds
- Not suitable for real-time pasteboard monitoring
- Not suitable for bulk data transfer operations