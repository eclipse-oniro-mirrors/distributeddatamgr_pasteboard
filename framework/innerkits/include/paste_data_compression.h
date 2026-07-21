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
 * 
 * NOTE: This is a demonstration implementation.
 * Actual integration requires adapting to PasteData API.
 */

#ifndef PASTE_DATA_COMPRESSION_H
#define PASTE_DATA_COMPRESSION_H

#include <memory>
#include <vector>
#include "compression_utils.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

class PasteDataCompression {
public:
    static constexpr size_t COMPRESS_THRESHOLD = 4 * 1024; // 4KB
    
    struct CompressionResult {
        bool compressed = false;
        size_t originalSize = 0;
        size_t compressedSize = 0;
        float ratio = 0.0f;
    };
    
    static CompressionResult CompressPasteData(std::vector<uint8_t>& data)
    {
        CompressionResult result;
        result.originalSize = data.size();
        
        if (data.size() < COMPRESS_THRESHOLD) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, 
                "Data too small, skip compression: %{public}zu bytes", data.size());
            return result;
        }
        
        std::vector<uint8_t> compressed = CompressionUtils::Compress(data);
        result.compressedSize = compressed.size();
        result.compressed = compressed.size() < data.size();
        result.ratio = result.compressed ? 
            (float)compressed.size() / data.size() : 1.0f;
        
        if (result.compressed) {
            data = compressed;
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
                "Data compressed: %{public}zu -> %{public}zu bytes (%.1f%%)",
                result.originalSize, result.compressedSize, result.ratio * 100);
        }
        
        return result;
    }
    
    static bool DecompressPasteData(std::vector<uint8_t>& data)
    {
        if (!CompressionUtils::IsCompressed(data)) {
            return true; // Not compressed
        }
        
        std::vector<uint8_t> decompressed = CompressionUtils::Decompress(data);
        
        if (decompressed.empty()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Decompression failed");
            return false;
        }
        
        data = decompressed;
        
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
            "Data decompressed: size=%{public}zu bytes", data.size());
        
        return true;
    }
    
    static size_t EstimateMemoryUsage(const std::vector<uint8_t>& data)
    {
        return data.size();
    }
    
    static size_t EstimateOriginalSize(const std::vector<uint8_t>& compressed)
    {
        return CompressionUtils::GetCompressedSize(compressed);
    }
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTE_DATA_COMPRESSION_H