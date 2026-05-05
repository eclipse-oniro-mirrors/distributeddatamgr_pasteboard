# Test Cases for ohos-pasteboard CLI

## Test Coverage

This document covers all 7 CLI commands with all parameter combinations in structured table format.

## Command Test Cases

### 1. set-data

| Command Example | Description | Permission | Prerequisites |
|-----------------|-------------|------------|---------------|
| `ohos-pasteboard set-data --text "Hello World"` | Set plain text only, primary type: text/plain | None | None |
| `ohos-pasteboard set-data --html "<p>Hello</p>"` | Set HTML only, primary type: text/html | None | None |
| `ohos-pasteboard set-data --uri "file:///path/to/file"` | Set URI only, primary type: text/uri-list | None | None |
| `ohos-pasteboard set-data --text ""` | Set empty text (should fail) | None | None |
| `ohos-pasteboard set-data --html ""` | Set empty HTML (should fail) | None | None |
| `ohos-pasteboard set-data --uri ""` | Set empty URI (should fail) | None | None |
| `ohos-pasteboard set-data` | No parameters (should fail) | None | None |
| `ohos-pasteboard set-data --html "<p>Hello</p>" --text "Hello"` | Set HTML + text, HTML is primary | None | None |
| `ohos-pasteboard set-data --text "Hello" --html "<p>Hello</p>"` | Set text + HTML, text is primary | None | None |
| `ohos-pasteboard set-data --text "Hello" --uri "file:///path"` | Set text + URI, text is primary | None | None |
| `ohos-pasteboard set-data --uri "file:///path" --text "Hello"` | Set URI + text, URI is primary | None | None |
| `ohos-pasteboard set-data --html "<p>Hi</p>" --uri "file:///path"` | Set HTML + URI, HTML is primary | None | None |
| `ohos-pasteboard set-data --uri "file:///path" --html "<p>Hi</p>"` | Set URI + HTML, URI is primary | None | None |
| `ohos-pasteboard set-data --html "<p>H</p>" --uri "file:///p" --text "T"` | Set all three, HTML is primary | None | None |
| `ohos-pasteboard set-data --text "T" --html "<p>H</p>" --uri "file:///p"` | Set all three, text is primary | None | None |
| `ohos-pasteboard set-data --uri "file:///p" --text "T" --html "<p>H</p>"` | Set all three, URI is primary | None | None |
| `ohos-pasteboard set-data --text "Long text content with multiple words"` | Set long text | None | None |
| `ohos-pasteboard set-data --html "<html><body><h1>Title</h1></body></html>"` | Set complex HTML | None | None |
| `ohos-pasteboard set-data --uri "datashare:///com.example.provider/table"` | Set DataShare URI | None | None |

### 2. get-data

| Command Example | Description | Permission | Prerequisites |
|-----------------|-------------|------------|---------------|
| `ohos-pasteboard get-data` | Get pasteboard data (success case) | READ_PASTEBOARD or SECURE_PASTE | Clipboard has data |
| `ohos-pasteboard get-data` | Get pasteboard data (permission denied) | None | Clipboard has data |
| `ohos-pasteboard get-data` | Get pasteboard data (empty pasteboard) | READ_PASTEBOARD or SECURE_PASTE | Clipboard is empty |

### 3. clear-data

| Command Example | Description | Permission | Prerequisites |
|-----------------|-------------|------------|---------------|
| `ohos-pasteboard clear-data` | Clear pasteboard (success) | None | Clipboard has data |
| `ohos-pasteboard clear-data` | Clear pasteboard (already empty) | None | Clipboard is empty |

### 4. has-data

| Command Example | Description | Permission | Prerequisites |
|-----------------|-------------|------------|---------------|
| `ohos-pasteboard has-data` | Check pasteboard status (has data) | None | Clipboard has data |
| `ohos-pasteboard has-data` | Check pasteboard status (empty) | None | Clipboard is empty |

### 5. has-data-type

| Command Example | Description | Permission | Prerequisites |
|-----------------|-------------|------------|---------------|
| `ohos-pasteboard has-data-type --type text/plain` | Check for plain text (no timeout) | None | None |
| `ohos-pasteboard has-data-type --type text/html` | Check for HTML (no timeout) | None | None |
| `ohos-pasteboard has-data-type --type text/uri-list` | Check for URI list (no timeout) | None | None |
| `ohos-pasteboard has-data-type --type image/png` | Check for PNG image (no timeout) | None | None |
| `ohos-pasteboard has-data-type --type application/json` | Check for JSON data (no timeout) | None | None |
| `ohos-pasteboard has-data-type` | No type parameter (should fail) | None | None |
| `ohos-pasteboard has-data-type --type ""` | Empty type (should fail) | None | None |
| `ohos-pasteboard has-data-type --type text/plain --timeout 1` | Check with minimum timeout (1ms) | None | None |
| `ohos-pasteboard has-data-type --type text/plain --timeout 3000` | Check with 3 second timeout | None | None |
| `ohos-pasteboard has-data-type --type text/plain --timeout 5000` | Check with maximum timeout (5000ms) | None | None |
| `ohos-pasteboard has-data-type --type text/plain --timeout 0` | Timeout below minimum (should fail) (should fail) | None | None |
| `ohos-pasteboard has-data-type --type text/plain --timeout 5001` | Timeout above maximum (should fail) | None | None |
| `ohos-pasteboard has-data-type --type text/html --timeout 2000` | Check HTML with 2s timeout | None | None |
| `ohos-pasteboard has-data-type --type text/uri-list --timeout 4000` | Check URI with 4s timeout | None | None |
| `ohos-pasteboard has-data-type --type image/png --timeout 500` | Check PNG with 0.5s timeout | None | None |

### 6. has-remote-data

| Command Example | Description | Permission | Prerequisites |
|-----------------|-------------|------------|---------------|
| `ohos-pasteboard has-remote-data` | Check for remote data (present) | None | Clipboard has-remote-data |
| `ohos-pasteboard has-remote-data` | Check for remote data (none) | None | Clipboard has local data only |
| `ohos-pasteboard has-remote-data` | Check for remote data (empty pasteboard) | None | Clipboard is empty |

### 7. help

| Command Example | Description | Permission | Prerequisites |
|-----------------|-------------|------------|---------------|
| `ohos-pasteboard --help` | Show general help | None | None |
| `ohos-pasteboard --help` | Show general help (alternative) | None | None |
| `ohos-pasteboard --help set-data` | Show help for set-data | None | None |
| `ohos-pasteboard set-data --help` | Show help for set-data (alternative) | None | None |
| `ohos-pasteboard --help get-data` | Show help for get-data | None | None |
| `ohos-pasteboard get-data --help` | Show help for get-data (alternative) | None | None |
| `ohos-pasteboard --help clear-data` | Show help for clear-data | None | None |
| `ohos-pasteboard clear-data --help` | Show help for clear-data (alternative) | None | None |
| `ohos-pasteboard --help has-data` | Show help for has-data | None | None |
| `ohos-pasteboard has-data --help` | Show help for has-data (alternative) | None | None |
| `ohos-pasteboard --help has-data-type` | Show help for has-data-type | None | None |
| `ohos-pasteboard has-data-type --help` | Show help for has-data-type (alternative) | None | None |
| `ohos-pasteboard --help has-remote-data` | Show help for has-remote-data | None | None |
| `ohos-pasteboard has-remote-data --help` | Show help for has-remote-data (alternative) | None | None |
| `ohos-pasteboard --help invalid-command` | Show help for invalid command (should fail) | None | None |

## Test Workflow Examples

### Workflow 1: Basic Clipboard Operations
```bash
# Step 1: Check initial state
ohos-pasteboard has-data

# Step 2: Set text data
ohos-pasteboard set-data --text "Test message"

# Step 3: Verify data was set
ohos-pasteboard has-data
ohos-pasteboard has-data-type --type text/plain

# Step 4: Get data (requires permission)
ohos-pasteboard get-data

# Step 5: Clear data
ohos-pasteboard clear-data

# Step 6: Verify data was cleared
ohos-pasteboard has-data
```

### Workflow 2: Multi-Type Data
```bash
# Step 1: Set multi-type data
ohos-pasteboard set-data --html "<h1>Title</h1>" --text "Title"

# Step 2: Verify both types present
ohos-pasteboard has-data-type --type text/html
ohos-pasteboard has-data-type --type text/plain

# Step 3: Get and verify primary type
ohos-pasteboard get-data
```

### Workflow 3: Remote Data Check
```bash
# Check for remote data
ohos-pasteboard has-remote-data
```

### Workflow 4: Type Query with Timeout
```bash
# Step 1: Set some data
ohos-pasteboard set-data --text "Test"

# Step 2: Query with timeout
ohos-pasteboard has-data-type --type text/plain --timeout 10
ohos-pasteboard has-data-type --type text/html --timeout 3000
```

## Summary

- **Total Commands**: 7 (5 business commands + help)
- **Test Case Count**: 40+
- **Coverage**: All commands with all parameter combinations
- **Permission Coverage**: Both permission-required and non-permission scenarios
- **Error Case Coverage**: Invalid parameters, empty values, out-of-range values