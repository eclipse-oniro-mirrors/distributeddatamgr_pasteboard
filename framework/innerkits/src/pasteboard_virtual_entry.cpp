/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pasteboard_virtual_entry.h"

#include <cstring>

#include "securec.h"

namespace OHOS {
namespace MiscServices {

namespace {

void WriteU32(std::vector<uint8_t> &buf, uint32_t val)
{
    buf.insert(buf.end(), reinterpret_cast<const uint8_t *>(&val),
               reinterpret_cast<const uint8_t *>(&val) + sizeof(uint32_t));
}

void WriteI32(std::vector<uint8_t> &buf, int32_t val)
{
    buf.insert(buf.end(), reinterpret_cast<const uint8_t *>(&val),
               reinterpret_cast<const uint8_t *>(&val) + sizeof(int32_t));
}

void WriteI64(std::vector<uint8_t> &buf, int64_t val)
{
    buf.insert(buf.end(), reinterpret_cast<const uint8_t *>(&val),
               reinterpret_cast<const uint8_t *>(&val) + sizeof(int64_t));
}

void WriteString(std::vector<uint8_t> &buf, const std::string &str)
{
    WriteU32(buf, static_cast<uint32_t>(str.size()));
    buf.insert(buf.end(), str.begin(), str.end());
}

void SerializeNode(std::vector<uint8_t> &buf, const FuseNode &node)
{
    WriteString(buf, node.id);
    WriteString(buf, node.name);
    WriteI32(buf, node.type);
    WriteI64(buf, node.size);
    WriteI64(buf, node.modifyTime);
    WriteU32(buf, static_cast<uint32_t>(node.children.size()));
    for (const auto &child : node.children) {
        SerializeNode(buf, child);
    }
}

bool ReadU32(const std::vector<uint8_t> &buf, size_t &offset, uint32_t &val)
{
    if (offset + sizeof(uint32_t) > buf.size()) {
        return false;
    }
    if (memcpy_s(&val, sizeof(val), buf.data() + offset, sizeof(uint32_t)) != EOK) {
        return false;
    }
    offset += sizeof(uint32_t);
    return true;
}

bool ReadI32(const std::vector<uint8_t> &buf, size_t &offset, int32_t &val)
{
    if (offset + sizeof(int32_t) > buf.size()) {
        return false;
    }
    if (memcpy_s(&val, sizeof(val), buf.data() + offset, sizeof(int32_t)) != EOK) {
        return false;
    }
    offset += sizeof(int32_t);
    return true;
}

bool ReadI64(const std::vector<uint8_t> &buf, size_t &offset, int64_t &val)
{
    if (offset + sizeof(int64_t) > buf.size()) {
        return false;
    }
    if (memcpy_s(&val, sizeof(val), buf.data() + offset, sizeof(int64_t)) != EOK) {
        return false;
    }
    offset += sizeof(int64_t);
    return true;
}

bool ReadString(const std::vector<uint8_t> &buf, size_t &offset, std::string &val)
{
    uint32_t len = 0;
    if (!ReadU32(buf, offset, len)) {
        return false;
    }
    if (offset + len > buf.size()) {
        return false;
    }
    val.assign(reinterpret_cast<const char *>(buf.data() + offset), len);
    offset += len;
    return true;
}

bool DeserializeNode(const std::vector<uint8_t> &buf, size_t &offset,
    FuseNode &node, uint32_t depth, uint32_t &count)
{
    if (depth > MAX_VIRTUAL_FILE_DEPTH || count >= MAX_VIRTUAL_FILE_COUNT) {
        return false;
    }
    ++count;
    if (!ReadString(buf, offset, node.id) ||
        !ReadString(buf, offset, node.name) ||
        !ReadI32(buf, offset, node.type) ||
        !ReadI64(buf, offset, node.size) ||
        !ReadI64(buf, offset, node.modifyTime)) {
        return false;
    }
    if (node.type != 0 && node.type != 1) {
        return false;
    }
    if (node.type == 0 && node.size < 0) {
        return false;
    }
    uint32_t childCount = 0;
    if (!ReadU32(buf, offset, childCount)) {
        return false;
    }
    if (childCount > MAX_VIRTUAL_FILE_COUNT) {
        return false;
    }
    node.children.reserve(childCount);
    for (uint32_t i = 0; i < childCount; ++i) {
        FuseNode child;
        if (!DeserializeNode(buf, offset, child, depth + 1, count)) {
            return false;
        }
        node.children.emplace_back(std::move(child));
    }
    return true;
}

} // namespace

std::vector<uint8_t> SerializeTree(const FuseNode &root)
{
    std::vector<uint8_t> buf;
    SerializeNode(buf, root);
    return buf;
}

std::shared_ptr<FuseNode> DeserializeTree(const std::vector<uint8_t> &blob)
{
    if (blob.empty()) {
        return nullptr;
    }
    auto node = std::make_shared<FuseNode>();
    size_t offset = 0;
    uint32_t count = 0;
    if (!DeserializeNode(blob, offset, *node, 0, count)) {
        return nullptr;
    }
    return node;
}

} // namespace MiscServices
} // namespace OHOS
