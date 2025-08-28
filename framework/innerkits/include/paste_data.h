/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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
#ifndef PASTE_BOARD_DATA_H
#define PASTE_BOARD_DATA_H

#include "paste_data_record.h"
#include "pasteboard_event_common.h"

namespace OHOS {
namespace MiscServices {
enum ShareOption : int32_t { InApp = 0, LocalDevice, CrossDevice };
enum ScreenEvent : int32_t { Default = 0, ScreenLocked, ScreenUnlocked };
struct API_EXPORT PasteDataProperty : public TLVWriteable, public TLVReadable {
    PasteDataProperty() = default;
    ~PasteDataProperty();
    explicit PasteDataProperty(const PasteDataProperty &property);
    PasteDataProperty &operator=(const PasteDataProperty &property);
    AAFwk::WantParams additions;
    std::vector<std::string> mimeTypes;
    std::string tag;
    std::int64_t timestamp;
    bool localOnly;
    ShareOption shareOption;
    uint32_t tokenId = 0;
    bool isRemote = false;
    std::string bundleName;
    std::string setTime;
    ScreenEvent screenStatus = ScreenEvent::Default;
    int32_t appIndex = 0;

    bool EncodeTLV(WriteOnlyBuffer &buffer) const override;
    bool DecodeTLV(ReadOnlyBuffer &buffer) override;
    size_t CountTLV() const override;
};

class API_EXPORT PasteData : public TLVWriteable, public TLVReadable, public Parcelable {
public:
    static constexpr const std::uint32_t MAX_RECORD_NUM = 512;
    PasteData();
    ~PasteData();
    PasteData(const PasteData &data);
    PasteData &operator=(const PasteData &data);
    explicit PasteData(std::vector<std::shared_ptr<PasteDataRecord>> records);

    void AddHtmlRecord(const std::string &html);
    void AddKvRecord(const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer);
    void AddPixelMapRecord(std::shared_ptr<OHOS::Media::PixelMap> pixelMap);
    void AddTextRecord(const std::string &text);
    void AddUriRecord(const OHOS::Uri &uri);
    void AddWantRecord(std::shared_ptr<OHOS::AAFwk::Want> want);
    void AddRecord(std::shared_ptr<PasteDataRecord> record);
    void AddRecord(const PasteDataRecord &record);
    std::vector<std::string> GetMimeTypes();
    std::vector<std::string> GetReportMimeTypes();
    DataDescription GetReportDescription();
    std::shared_ptr<std::string> GetPrimaryHtml();
    std::shared_ptr<OHOS::Media::PixelMap> GetPrimaryPixelMap();
    std::shared_ptr<std::string> GetPrimaryText();
    std::shared_ptr<OHOS::Uri> GetPrimaryUri();
    std::shared_ptr<std::string> GetPrimaryMimeType();
    std::shared_ptr<OHOS::AAFwk::Want> GetPrimaryWant();
    std::shared_ptr<PasteDataRecord> GetRecordAt(std::size_t index) const;
    std::shared_ptr<PasteDataRecord> GetRecordById(uint32_t recordId) const;
    std::size_t GetRecordCount() const;
    bool RemoveRecordAt(std::size_t number);
    bool ReplaceRecordAt(std::size_t number, std::shared_ptr<PasteDataRecord> record);
    bool HasMimeType(const std::string &mimeType);
    PasteDataProperty GetProperty() const;
    void SetProperty(const PasteDataProperty &property);
    ShareOption GetShareOption();
    void SetShareOption(ShareOption shareOption);
    uint32_t GetTokenId();
    int32_t GetOriginTokenId();
    void SetTokenId(uint32_t tokenId);
    std::vector<std::shared_ptr<PasteDataRecord>> AllRecords() const;
    bool IsDraggedData() const;
    void SetDraggedDataFlag(bool isDraggedData);
    bool IsLocalPaste() const;
    void SetLocalPasteFlag(bool isLocalPaste);

    void SetBundleInfo(const std::string &bundleName, int32_t appIndex = 0);
    std::string GetBundleName() const;
    int32_t GetAppIndex() const;
    void SetOriginAuthority(const std::pair<std::string, int32_t> &bundleIndex);
    std::pair<std::string, int32_t> GetOriginAuthority() const;
    void SetRemote(bool isRemote);
    bool IsRemote() const;
    void SetTime(const std::string &time);
    std::string GetTime();
    void SetScreenStatus(ScreenEvent screenStatus);
    ScreenEvent GetScreenStatus();
    void SetTag(const std::string &tag);
    std::string GetTag();
    void SetAdditions(const AAFwk::WantParams &additions);
    void SetAddition(const std::string &key, AAFwk::IInterface *value);
    void SetLocalOnly(bool localOnly);
    bool GetLocalOnly();
    void SetFileSize(int64_t fileSize);
    int64_t GetFileSize() const;

    bool Marshalling(Parcel &parcel) const override;
    static PasteData *Unmarshalling(Parcel &parcel);
    bool EncodeTLV(WriteOnlyBuffer &buffer) const override;
    bool DecodeTLV(ReadOnlyBuffer &buffer) override;
    size_t CountTLV() const override;

    bool IsValid() const;
    void SetInvalid();

    void SetDelayData(bool isDelay);
    bool IsDelayData() const;
    void SetDelayRecord(bool isDelay);
    bool IsDelayRecord() const;
    void SetDataId(uint32_t dataId);
    uint32_t GetDataId() const;
    uint32_t GetRecordId() const;
    void SetPasteId(const std::string &pasteId);
    std::string GetPasteId() const;
    std::string GetDeviceId() const;

    static void ShareOptionToString(ShareOption shareOption, std::string &out);
    static std::string CreatePasteId(const std::string &name, uint32_t sequence);
    static bool IsValidPasteId(const std::string &pasteId);
    static bool IsValidShareOption(int32_t shareOption);
    static std::string WEBVIEW_PASTEDATA_TAG;
    static constexpr const char *DISTRIBUTEDFILES_TAG = "distributedfiles";
    static constexpr const char *PATH_SHARE = "/data/storage/el2/share/r/";
    static constexpr const char *IMG_LOCAL_URI = "file:///";
    static constexpr const char *SHARE_PATH_PREFIX = "/mnt/hmdfs/";
    static constexpr const char *SHARE_PATH_PREFIX_ACCOUNT = "/account/merge_view/services/";
    static constexpr const char *DOCS_LOCAL_TAG = "/docs/";
    static constexpr size_t URI_BATCH_SIZE = 10000;
    static constexpr uint32_t MAX_REPORT_RECORD_NUM = 30;
    static constexpr int32_t INVALID_TOKEN_ID = -1;
    int64_t rawDataSize_ = 0;
    std::string deviceId_;

private:
    bool valid_ = true;
    bool isDraggedData_ = false;
    bool isLocalPaste_ = false; // local in app paste
    bool isDelayData_ = false;
    bool isDelayRecord_ = false;
    uint32_t dataId_ = 0;
    uint32_t recordId_ = 0;
    PasteDataProperty props_;
    std::vector<std::shared_ptr<PasteDataRecord>> records_;
    std::pair<std::string, int32_t> originAuthority_;
    std::string pasteId_;
 
    void RefreshMimeProp();
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_DATA_H
