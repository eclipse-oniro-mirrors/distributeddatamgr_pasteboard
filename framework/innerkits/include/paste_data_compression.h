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

#ifndef PASTE_DATA_COMPRESSION_H
#define PASTE_DATA_COMPRESSION_H

#include <memory>
#include <vector>
#include "compression_utils.h"
#include "paste_data.h"
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
    
    static CompressionResult CompressPasteData(PasteData& pasteData)
    {
        CompressionResult result;
        
        // Serialize to binary
        std::vector<uint8_t> serialized = SerializePasteData(pasteData);
        result.originalSize = serialized.size();
        
        if (serialized.size() < COMPRESS_THRESHOLD) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, 
                "Data too small, skip compression: %{public}zu bytes", serialized.size());
            return result;
        }
        
        // Compress
        std::vector<uint8_t> compressed = CompressionUtils::Compress(serialized);
        result.compressedSize = compressed.size();
        result.compressed = compressed.size() < serialized.size();
        result.ratio = result.compressed ? 
            (float)compressed.size() / serialized.size() : 1.0f;
        
        if (result.compressed) {
            // Store compressed data
            pasteData.SetCompressedData(compressed);
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
                "PasteData compressed: %{public}zu -> %{public}zu bytes (%.1f%%)",
                result.originalSize, result.compressedSize, result.ratio * 100);
        }
        
        return result;
    }
    
    static bool DecompressPasteData(PasteData& pasteData)
    {
        if (!pasteData.HasCompressedData()) {
            return true; // Not compressed
        }
        
        std::vector<uint8_t> compressed = pasteData.GetCompressedData();
        
        // Decompress
        std::vector<uint8_t> decompressed = CompressionUtils::Decompress(compressed);
        
        // Deserialize
        bool success = DeserializePasteData(pasteData, decompressed);
        
        if (success) {
            pasteData.ClearCompressedData();
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
                "PasteData decompressed: %{public}zu -> %{public}zu bytes",
                compressed.size(), decompressed.size());
        } else {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to decompress PasteData");
        }
        
        return success;
    }
    
    static size_t EstimateMemoryUsage(const PasteData& pasteData)
    {
        size_t total = sizeof(PasteData);
        
        // Add record sizes
        for (const auto& record : pasteData.AllRecords()) {
            total += EstimateRecordSize(record);
        }
        
        // Add metadata
        total += pasteData.GetMimeType().size();
        total += pasteData.GetLabel().size();
        
        return total;
    }
    
private:
    static std::vector<uint8_t> SerializePasteData(const PasteData& pasteData)
    {
        std::vector<uint8_t> data;
        
        // Simple serialization (in production, use proper serialization)
        // 1. Number of records (4 bytes)
        uint32_t recordCount = pasteData.AllRecords().size();
        data.insert(data.end(), 
            reinterpret_cast<uint8_t*>(&recordCount),
            reinterpret_cast<uint8_t*>(&recordCount) + sizeof(recordCount));
        
        // 2. Each record
        for (const auto& record : pasteData.AllRecords()) {
            SerializeRecord(record, data);
        }
        
        // 3. Metadata
        SerializeString(pasteData.GetMimeType(), data);
        SerializeString(pasteData.GetLabel(), data);
        
        return data;
    }
    
    static bool DeserializePasteData(PasteData& pasteData, const std::vector<uint8_t>& data)
    {
        if (data.size() < sizeof(uint32_t)) {
            return false;
        }
        
        size_t offset = 0;
        
        // 1. Number of records
        uint32_t recordCount;
        memcpy(&recordCount, data.data() + offset, sizeof(recordCount));
        offset += sizeof(recordCount);
        
        // 2. Each record
        for (uint32_t i = 0; i < recordCount; i++) {
            auto record = DeserializeRecord(data, offset);
            if (record) {
                pasteData.AddRecord(record);
            }
        }
        
        // 3. Metadata
        std::string mimeType = DeserializeString(data, offset);
        std::string label = DeserializeString(data, offset);
        
        pasteData.SetMimeType(mimeType);
        pasteData.SetLabel(label);
        
        return true;
    }
    
    static void SerializeRecord(const std::shared_ptr<PasteDataRecord>& record, 
                                 std::vector<uint8_t>& data)
    {
        // Record type (1 byte)
        uint8_t type = static_cast<uint8_t>(record->GetType());
        data.push_back(type);
        
        // Record data
        switch (record->GetType()) {
            case PasteDataType::TEXT:
                SerializeString(record->GetPlainText(), data);
                break;
            case PasteDataType::HTML:
                SerializeString(record->GetHtmlText(), data);
                break;
            // ... other types
        }
    }
    
    static std::shared_ptr<PasteDataRecord> DeserializeRecord(const std::vector<uint8_t>& data,
                                                                 size_t& offset)
    {
        if (offset >= data.size()) {
            return nullptr;
        }
        
        uint8_t type = data[offset++];
        
        switch (static_cast<PasteDataType>(type)) {
            case PasteDataType::TEXT: {
                std::string text = DeserializeString(data, offset);
                return PasteDataRecord::NewTextRecord(text);
            }
            case PasteDataType::HTML: {
                std::string html = DeserializeString(data, offset);
                return PasteDataRecord::NewHtmlRecord(html);
            }
            default:
                return nullptr;
        }
    }
    
    static void SerializeString(const std::string& str, std::vector<uint8_t>& data)
    {
        uint32_t size = str.size();
        data.insert(data.end(),
            reinterpret_cast<uint8_t*>(&size),
            reinterpret_cast<uint8_t*>(&size) + sizeof(size));
        data.insert(data.end(), str.begin(), str.end());
    }
    
    static std::string DeserializeString(const std::vector<uint8_t>& data, size_t& offset)
    {
        if (offset + sizeof(uint32_t) > data.size()) {
            return "";
        }
        
        uint32_t size;
        memcpy(&size, data.data() + offset, sizeof(size));
        offset += sizeof(size);
        
        if (offset + size > data.size()) {
            return "";
        }
        
        std::string str(reinterpret_cast<const char*>(data.data() + offset), size);
        offset += size;
        
        return str;
    }
    
    static size_t EstimateRecordSize(const std::shared_ptr<PasteDataRecord>& record)
    {
        size_t size = sizeof(PasteDataRecord);
        
        switch (record->GetType()) {
            case PasteDataType::TEXT:
                size += record->GetPlainText().size();
                break;
            case PasteDataType::HTML:
                size += record->GetHtmlText().size();
                break;
            default:
                break;
        }
        
        return size;
    }
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTE_DATA_COMPRESSION_H