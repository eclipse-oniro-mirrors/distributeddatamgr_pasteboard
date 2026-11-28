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

#ifndef PASTEBOARD_VIRTUAL_ENTRY_H
#define PASTEBOARD_VIRTUAL_ENTRY_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OHOS {
namespace MiscServices {

constexpr uint32_t MAX_VIRTUAL_FILE_DEPTH = 64;
constexpr size_t MAX_VIRTUAL_FILE_COUNT = 4096;
constexpr size_t MAX_VIRTUAL_FILE_ID_BYTES = 1024;
constexpr size_t MAX_VIRTUAL_FILE_NAME_BYTES = 255;

inline bool IsValidFileId(const std::string &id)
{
    return !id.empty() && id.size() <= MAX_VIRTUAL_FILE_ID_BYTES &&
           id.find_first_of("/\\") == std::string::npos && id.find('\0') == std::string::npos;
}

inline bool IsValidFileName(const std::string &name)
{
    return !name.empty() && name.size() <= MAX_VIRTUAL_FILE_NAME_BYTES &&
           name != "." && name != ".." &&
           name.find_first_of("/\\") == std::string::npos && name.find('\0') == std::string::npos;
}

struct FuseNode {
    std::string id;
    std::string name;
    int32_t type = 0;        // 0=file, 1=dir
    int64_t size = -1;       // -1 for directories
    int64_t modifyTime = 0;
    uint32_t recordId = 0;
    std::vector<FuseNode> children;

    bool IsFile() const { return type == 0; }
    bool IsDir() const { return type == 1; }

    const FuseNode *FindChild(const std::string &childName) const
    {
        for (const auto &child : children) {
            if (child.name == childName) {
                return &child;
            }
        }
        return nullptr;
    }

    const FuseNode *FindByPath(const std::string &path) const
    {
        if (path.empty()) {
            return this;
        }
        const FuseNode *current = this;
        size_t start = 0;
        while (start < path.size()) {
            size_t end = path.find('/', start);
            std::string segment = (end == std::string::npos)
                ? path.substr(start) : path.substr(start, end - start);
            current = current->FindChild(segment);
            if (current == nullptr) {
                return nullptr;
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
        return current;
    }

    const FuseNode *FindById(const std::string &targetId) const
    {
        if (IsFile() && id == targetId) {
            return this;
        }
        for (const auto &child : children) {
            if (auto *found = child.FindById(targetId); found != nullptr) {
                return found;
            }
        }
        return nullptr;
    }

    bool HasValidFileSizes(uint32_t depth = 0) const
    {
        if (depth > MAX_VIRTUAL_FILE_DEPTH) {
            return false;
        }
        if (IsFile()) {
            return size >= 0;
        }
        for (const auto &child : children) {
            if (!child.HasValidFileSizes(depth + 1)) {
                return false;
            }
        }
        return true;
    }
};

std::vector<uint8_t> SerializeTree(const FuseNode &root);
std::shared_ptr<FuseNode> DeserializeTree(const std::vector<uint8_t> &blob);

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_VIRTUAL_ENTRY_H
