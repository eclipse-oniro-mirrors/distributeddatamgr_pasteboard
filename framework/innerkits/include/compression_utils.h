/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef COMPRESSION_UTILS_H
#define COMPRESSION_UTILS_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <memory>
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

class CompressionUtils {
public:
    static constexpr size_t COMPRESSION_THRESHOLD = 1024; // 1KB
    static constexpr int COMPRESSION_LEVEL = 6; // Balance between speed and ratio
    
    struct CompressedHeader {
        uint32_t originalSize;
        uint32_t compressedSize;
        uint8_t compressionType; // 0: none, 1: lz4, 2: zstd
        uint8_t reserved[3];
    };
    
    static std::vector<uint8_t> Compress(const std::vector<uint8_t>& data)
    {
        if (data.size() < COMPRESSION_THRESHOLD) {
            return data; // Small data, no compression
        }
        
        std::vector<uint8_t> compressed;
        CompressedHeader header;
        header.originalSize = data.size();
        header.compressedSize = 0;
        header.compressionType = 1; // LZ4
        
        // Simple RLE compression for demonstration
        // In production, use LZ4 or ZSTD
        compressed.resize(sizeof(CompressedHeader) + data.size() * 2);
        memcpy(compressed.data(), &header, sizeof(CompressedHeader));
        
        size_t compressedIdx = sizeof(CompressedHeader);
        size_t i = 0;
        
        while (i < data.size()) {
            uint8_t current = data[i];
            uint8_t count = 1;
            
            // Count consecutive identical bytes
            while (i + count < data.size() && 
                   data[i + count] == current && 
                   count < 255) {
                count++;
            }
            
            // Write: count (1 byte) + value (1 byte)
            if (compressedIdx + 2 <= compressed.size()) {
                compressed[compressedIdx++] = count;
                compressed[compressedIdx++] = current;
            }
            
            i += count;
        }
        
        header.compressedSize = compressedIdx - sizeof(CompressedHeader);
        memcpy(compressed.data(), &header, sizeof(CompressedHeader));
        
        compressed.resize(compressedIdx);
        
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, 
            "Compressed: %{public}zu -> %{public}zu bytes (%{public}.1f%%)",
            data.size(), compressed.size(),
            (float)compressed.size() * 100 / data.size());
        
        return compressed;
    }
    
    static std::vector<uint8_t> Decompress(const std::vector<uint8_t>& compressed)
    {
        if (compressed.size() < sizeof(CompressedHeader)) {
            return compressed; // Not compressed
        }
        
        CompressedHeader header;
        memcpy(&header, compressed.data(), sizeof(CompressedHeader));
        
        if (header.compressionType == 0) {
            // Not compressed
            return std::vector<uint8_t>(compressed.begin() + sizeof(CompressedHeader), 
                                        compressed.end());
        }
        
        std::vector<uint8_t> data;
        data.reserve(header.originalSize);
        
        // Simple RLE decompression
        size_t idx = sizeof(CompressedHeader);
        while (idx + 1 < compressed.size()) {
            uint8_t count = compressed[idx++];
            uint8_t value = compressed[idx++];
            
            for (uint8_t i = 0; i < count; i++) {
                data.push_back(value);
            }
        }
        
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
            "Decompressed: %{public}zu -> %{public}zu bytes",
            compressed.size(), data.size());
        
        return data;
    }
    
    static bool IsCompressed(const std::vector<uint8_t>& data)
    {
        if (data.size() < sizeof(CompressedHeader)) {
            return false;
        }
        
        CompressedHeader header;
        memcpy(&header, data.data(), sizeof(CompressedHeader));
        
        return header.compressionType > 0;
    }
    
    static size_t GetCompressedSize(const std::vector<uint8_t>& compressed)
    {
        if (compressed.size() < sizeof(CompressedHeader)) {
            return compressed.size();
        }
        
        CompressedHeader header;
        memcpy(&header, compressed.data(), sizeof(CompressedHeader));
        
        return header.originalSize;
    }
};

} // namespace MiscServices
} // namespace OHOS

#endif // COMPRESSION_UTILS_H