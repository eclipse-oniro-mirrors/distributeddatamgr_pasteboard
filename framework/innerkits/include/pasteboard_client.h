/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef PASTE_BOARD_CLIENT_H
#define PASTE_BOARD_CLIENT_H

#include <condition_variable>
#include <functional>
#include <singleton.h>

#include "i_pasteboard_service.h"
#include "paste_data.h"
#include "paste_data_record.h"
#include "pasteboard_delay_getter.h"
#include "pasteboard_observer.h"
#include "pasteboard_progress_signal.h"
#include "refbase.h"
#include "unified_data.h"
#include "want.h"

namespace OHOS {
namespace MiscServices {
class API_EXPORT PasteboardSaDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit PasteboardSaDeathRecipient();
    ~PasteboardSaDeathRecipient() = default;
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;

private:
    DISALLOW_COPY_AND_MOVE(PasteboardSaDeathRecipient);
};

enum ProgressStatus {
    NORMAL_PASTE = 0,
    CANCEL_PASTE = 1,
    PASTE_TIME_OUT = 2,
};

enum FileConflictOption {
    FILE_OVERWRITE = 0,
    FILE_SKIP = 1,
    FILE_RENAME = 2
};

enum ProgressIndicator {
    NONE_PROGRESS_INDICATOR = 0,
    DEFAULT_PROGRESS_INDICATOR = 1
};

struct PasteDataFromServiceInfo {
    pid_t pid;
    std::string currentPid;
    std::string currentId;
};

struct ProgressReportLintener {
    void (*OnProgressFail)(int32_t result);
};

struct ProgressInfo {
    int percentage;
};

struct ProgressListener {
    void (*ProgressNotify)(std::shared_ptr<ProgressInfo> progressInfo);
};

struct GetDataParams {
    std::string destUri;
    enum FileConflictOption fileConflictOption;
    enum ProgressIndicator progressIndicator;
    struct ProgressListener listener;
    std::shared_ptr<ProgressSignalClient> progressSignal;
};

class API_EXPORT PasteboardClient : public DelayedSingleton<PasteboardClient> {
    DECLARE_DELAYED_SINGLETON(PasteboardClient);

public:
    DISALLOW_COPY_AND_MOVE(PasteboardClient);

    /**
     * CreateHtmlTextRecord
     * @descrition Create Html Text Record.
     * @param std::string text.
     * @return PasteDataRecord.
     */
    std::shared_ptr<PasteDataRecord> CreateHtmlTextRecord(const std::string &text);

    /**
     * CreatePlainTextRecord
     * @descrition Create Plaint Text Record.
     * @param std::string text.
     * @return PasteDataRecord.
     */
    std::shared_ptr<PasteDataRecord> CreatePlainTextRecord(const std::string &text);

    /**
     * CreatePixelMapRecord
     * @descrition Create PixelMap Record.
     * @param OHOS::Media::PixelMap pixelMap.
     * @return PasteDataRecord.
     */
    std::shared_ptr<PasteDataRecord> CreatePixelMapRecord(std::shared_ptr<OHOS::Media::PixelMap> pixelMap);

    /**
     * CreateUriRecord
     * @descrition Create Uri Text Record.
     * @param OHOS::Uri uri.
     * @return PasteDataRecord.
     */
    std::shared_ptr<PasteDataRecord> CreateUriRecord(const OHOS::Uri &uri);

    /**
     * CreateWantRecord
     * @descrition Create Plaint Want Record.
     * @param OHOS::AAFwk::Want want.
     * @return PasteDataRecord.
     */
    std::shared_ptr<PasteDataRecord> CreateWantRecord(std::shared_ptr<OHOS::AAFwk::Want> want);

    /**
     * CreateKvRecord
     * @descrition Create Kv Record.
     * @param std::string mimeType
     * @param std::vector<uint8_t> arrayBuffer
     * @return PasteDataRecord.
     */
    std::shared_ptr<PasteDataRecord> CreateKvRecord(
        const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer);

    /**
     * CreateMultiDelayRecord
     * @descrition Create Multi DelayRecord.
     * @param std::vector<std::string> mimeTypes
     * @param std::shared_ptr<UDMF::EntryGetter> entryGetter
     * @return PasteDataRecord.
     */
    std::shared_ptr<PasteDataRecord> CreateMultiDelayRecord(
        std::vector<std::string> mimeTypes, const std::shared_ptr<UDMF::EntryGetter> entryGetter);

    /**
     * CreateHtmlData
     * @descrition Create Html Paste Data.
     * @param std::string text  .
     * @return PasteData.
     */
    std::shared_ptr<PasteData> CreateHtmlData(const std::string &htmlText);

    /**
     * CreatePlainTextData
     * @descritionCreate Plain Text Paste Data.
     * @param std::string text .
     * @return PasteData.
     */
    std::shared_ptr<PasteData> CreatePlainTextData(const std::string &text);

    /**
     * CreatePixelMapData
     * @descrition Create PixelMap Paste Data.
     * @param OHOS::Media::PixelMap pixelMap .
     * @return PasteData.
     */
    std::shared_ptr<PasteData> CreatePixelMapData(std::shared_ptr<OHOS::Media::PixelMap> pixelMap);

    /**
     * CreateUriData
     * @descrition Create Uri Paste Data.
     * @param OHOS::Uri uri .
     * @return PasteData.
     */
    std::shared_ptr<PasteData> CreateUriData(const OHOS::Uri &uri);

    /**
     * CreateWantData
     * @descrition Create Want Paste Data.
     * @param OHOS::AAFwk::Want want .
     * @return PasteData.
     */
    std::shared_ptr<PasteData> CreateWantData(std::shared_ptr<OHOS::AAFwk::Want> want);

    /**
     * CreateKvData
     * @descrition Create Kv Paste Data.
     * @param std::string mimeType
     * @param std::vector<uint8_t> arrayBuffer
     * @return PasteData.
     */
    std::shared_ptr<PasteData> CreateKvData(const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer);

    /**
     * CreateMultiTypeData
     * @descrition Create multi-type Data.
     * @param std::map<std::string, EntryValue> typeValueMap
     * @param recordMimeType record's default mimeType
     * @return PasteData.
     */
    std::shared_ptr<PasteData> CreateMultiTypeData(
       std::shared_ptr<std::map<std::string, std::shared_ptr<EntryValue>>> typeValueMap,
       const std::string &recordMimeType = "");

    /**
     * CreateMultiTypeDelayData
     * @descrition Create delayed multi-type Data.
     * @param std::vector<std::string> utdTypes
     * @param std::shared_ptr<UDMF::EntryGetter> entryGetter
     * @return PasteData.
     */
    std::shared_ptr<PasteData> CreateMultiTypeDelayData(std::vector<std::string> mimeTypes,
        std::shared_ptr<UDMF::EntryGetter> entryGetter);

    /**
     * GetChangeCount
     * @descrition get clip changed count from the pasteboard.
     * @param changeCount the changeCount of the PasteData.
     * @return int32_t.
     */
    int32_t GetChangeCount(uint32_t &changeCount);
   
    /**
     * GetRecordValueByType
     * @descrition get entry value from the pasteboard.
     * @param dataId the dataId of the PasteData.
     * @param recordId the recordId of the PasteRecord.
     * @param value the value of the PasteDataEntry.
     * @return int32_t.
     */
    int32_t GetRecordValueByType(uint32_t dataId, uint32_t recordId, PasteDataEntry& value);

    /**
     * GetPasteData
     * @descrition get paste data from the pasteboard.
     * @param pasteData the object of the PasteDate.
     * @return int32_t.
     */
    int32_t GetPasteData(PasteData &pasteData);

    /**
     * GetMimeTypes
     * @descrition get mime types from the pasteboard.
     * @return Returns MimeTypes
     */
    std::vector<std::string> GetMimeTypes();

    /**
     * HasPasteData
     * @descrition check paste data exist in the pasteboard.
     * @return bool. True exists, false does not exist
     */
    bool HasPasteData();

    /**
     * Clear
     * @descrition Clear Current pasteboard data.
     * @return void.
     */
    void Clear();

    /**
     * SetPasteData
     * @descrition set paste data to the pasteboard.
     * @param pasteData the object of the PasteData.
     * @param pasteData the object of the PasteboardDelayGetter.
     * @param pasteData the map of the EntryGetter.
     * @return int32_t.
     */
    int32_t SetPasteData(PasteData &pasteData, std::shared_ptr<PasteboardDelayGetter> delayGetter = nullptr,
        std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters = {});

    /**
     * SetPasteData
     * @descrition set paste data to the pasteboard.
     * @param unifiedData the object of the PasteDate.
     * @return int32_t.
     */
    int32_t SetUnifiedData(
        const UDMF::UnifiedData &unifiedData, std::shared_ptr<PasteboardDelayGetter> delayGetter = nullptr);

    /**
     * SetPasteData
     * @descrition set paste data to the pasteboard.
     * @param unifiedData the object of the PasteDate.
     * @return int32_t.
     */
    int32_t GetUnifiedData(UDMF::UnifiedData &unifiedData);

    /**
     * SetUdsdData
     * @descrition set unified data with uds entries to the pasteboard.
     * @param unifiedData the object of the PasteDate.
     * @return int32_t.
     */
    int32_t SetUdsdData(const UDMF::UnifiedData &unifiedData);

    /**
     * GetUnifiedDataWithEntry
     * @descrition get unified data with uds entries from the pasteboard.
     * @param unifiedData the object of the PasteDate.
     * @return int32_t.
     */
    int32_t GetUdsdData(UDMF::UnifiedData &unifiedData);

    /**
     * IsRemoteData
     * @descrition check if remote data.
     * @return bool. True is remote data, else false.
     */
    bool IsRemoteData();

    /**
     * GetDataSource
     * @descrition Obtain the package name of the data source application.
     * @param std::string bundleName The package name of the application.
     * @return int32_t.
     */
    int32_t GetDataSource(std::string &bundleName);

    /**
     * HasDataType
     * @descrition Check if there is data of the specified type in the pasteboard.
     * @param std::string mimeType Specified mimetype.
     * @return bool. True exists, false does not exist
     */
    bool HasDataType(const std::string &mimeType);

    /**
     * DetectPatterns
     * @description Checks the specified patterns contained in clipboard, and removes if not found.
     * @param patternsToCheck A reference to an set of Pattern to check against the clipboard.
     * @return Returns DetectPatterns.
     */
    std::set<Pattern> DetectPatterns(const std::set<Pattern> &patternsToCheck);

    /**
     * Subscribe
     * @descrition
     * @param type observer type
     * @param observer pasteboard change callback.
     * @return void.
     */
    void Subscribe(PasteboardObserverType type, sptr<PasteboardObserver> callback);

    /**
     * AddPasteboardChangedObserver
     * @descrition
     * @param observer pasteboard change callback.
     * @return void.
     */
    void AddPasteboardChangedObserver(sptr<PasteboardObserver> callback);

    /**
     * AddPasteboardEventObserver
     * @descrition
     * @param observer pasteboard event(read or change) callback.
     * @return void.
     */
    void AddPasteboardEventObserver(sptr<PasteboardObserver> callback);

    /**
     * Unsubscribe
     * @descrition
     * @param type observer type
     * @param observer pasteboard change callback.
     * @return void.
     */
    void Unsubscribe(PasteboardObserverType type, sptr<PasteboardObserver> callback);

    /**
     * RemovePasteboardChangedObserver
     * @descrition
     * @param observer pasteboard change callback.
     * @return void.
     */
    void RemovePasteboardChangedObserver(sptr<PasteboardObserver> callback);

    /**
     * RemovePasteboardEventObserver
     * @descrition
     * @param observer pasteboard event callback.
     * @return void.
     */
    void RemovePasteboardEventObserver(sptr<PasteboardObserver> callback);

    /**
     * SetGlobalShareOption
     * @descrition Set globalShareOptions.
     * @param globalShareOption globalShareOptions
     * @return int32_t
     */
    int32_t SetGlobalShareOption(const std::map<uint32_t, ShareOption> &globalShareOptions);

    /**
     * RemoveGlobalShareOption
     * @descrition Remove globalShareOptions.
     * @param tokenId tokenIds
     * @return int32_t
     */
    int32_t RemoveGlobalShareOption(const std::vector<uint32_t> &tokenIds);

    /**
     * GetGlobalShareOption
     * @descrition Get globalShareOptions.
     * @param tokenId tokenIds
     * @return globalShareOptions
     */
    std::map<uint32_t, ShareOption> GetGlobalShareOption(const std::vector<uint32_t> &tokenIds);

    /**
     * SetAppShareOptions
     * @description Sets a unified ShareOptions for the application.
     * @param shareOptions shareOptions
     * @return result
     */
    int32_t SetAppShareOptions(const ShareOption &shareOptions);

    /**
     * RemoveAppShareOptions
     * @description Removes the ShareOptions for the application.
     * @return result
     */
    int32_t RemoveAppShareOptions();

    /**
     * OnRemoteSaDied
     * @descrition
     * @param object systemAbility proxy object
     * @return void.
     */
    void OnRemoteSaDied(const wptr<IRemoteObject> &object);

    /**
     * LoadSystemAbilitySuccess
     * @descrition inherit SystemAbilityLoadCallbackStub override LoadSystemAbilitySuccess
     * @param remoteObject systemAbility proxy object.
     * @return void.
     */
    void LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject);

    /**
     * LoadSystemAbilityFail
     * @descrition inherit SystemAbilityLoadCallbackStub override LoadSystemAbilityFail
     * @return void.
     */
    void LoadSystemAbilityFail();

    /**
     * PasteStart
     * @descrition Utilized to notify pasteboard service while reading PasteData, in this case, the service will help to
     *     preserve the context and resources
     * @return void.
     */
    void PasteStart(const std::string &pasteId);

    /**
     * PasteComplete
     * @descrition Invoked to notify pasteboard service the utilization of PasteData has completed and occupied
     *     resources can be released for further usage
     * @return void.
     */
    void PasteComplete(const std::string &deviceId, const std::string &pasteId);

    /**
     * GetDataWithProgress
     * @descrition Get pastedata from the system pasteboard with system progress indicator.
     * @param pasteData the object of the PasteData.
     * @param params - Indicates the {@link GetDataParams}.
     * @returns int32_t
     */
    int32_t GetDataWithProgress(PasteData &pasteData, std::shared_ptr<GetDataParams> params);

    /**
     * GetUnifiedDataWithProgress
     * @descrition Get pastedata from the system pasteboard with system progress indicator.
     * @param unifiedData - the object of the PasteData.
     * @param params - Indicates the {@link GetDataParams}.
     * @returns int32_t
     */
    int32_t GetUnifiedDataWithProgress(UDMF::UnifiedData &unifiedData, std::shared_ptr<GetDataParams> params);

    /**
     * GetRemoteDeviceName
     * @descrition Obtain the remote device name.
     * @param std::string deviceName - the device name of the remote device.
     * @returns int32_t
     */
    int32_t GetRemoteDeviceName(std::string &deviceName, bool &isRemote);

    /**
     * ProgressMakeMessageInfo
     * @descrition Make progress message information and pull up hap.
     * @param std::string progressKey - the progressKey.
     * @param std::string signalKey - the signalKey.
     * @returns void
     */
    void ProgressMakeMessageInfo(const std::string &progressKey, const std::string &signalKey);

private:
    sptr<IPasteboardService> GetPasteboardService();
    sptr<IPasteboardService> GetPasteboardServiceProxy();
    static void RetainUri(PasteData &pasteData);
    static void SplitWebviewPasteData(PasteData &pasteData);
    static void GetProgressByProgressInfo(std::shared_ptr<ProgressInfo> progressInfo);
    static int32_t HandleProgressStatus(const std::string &signalKey,
       std::shared_ptr<ProgressReportLintener> progressReport);
    int32_t PullHapSignal(std::string &signalKey, std::shared_ptr<ProgressReportLintener> progressReport);
    static int32_t SetProgressWithoutFile(std::string &progressKey, std::shared_ptr<GetDataParams> params);
    static void ProgressSmoothToTwentyPercent(PasteData &pasteData, std::string &progressKey,
       std::shared_ptr<GetDataParams> params);
    static void RefreshUri(std::shared_ptr<PasteDataRecord> &record);
    int32_t GetPasteDataFromService(PasteData &pasteData, PasteDataFromServiceInfo &pasteDataFromServiceInfo,
       std::string progressKey, std::shared_ptr<GetDataParams> params);
    static void OnProgressAbnormal(int32_t result);
    void ProgressRadarReport(PasteData &pasteData, PasteDataFromServiceInfo &pasteDataFromServiceInfo);
    static int32_t ProgressAfterTwentyPercent(PasteData &pasteData, std::shared_ptr<GetDataParams> params,
       std::string progressKey);
    static int32_t CheckProgressParam(std::shared_ptr<GetDataParams> params);
    static sptr<IPasteboardService> pasteboardServiceProxy_;
    static std::mutex instanceLock_;
    bool constructing_ = false;
    static std::condition_variable proxyConVar_;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_{ nullptr };
    std::atomic<uint32_t> getSequenceId_ = 0;
    static std::atomic<bool> remoteTask_;
    static std::atomic<bool> isPasting_;
    class StaticDestoryMonitor {
    public:
        StaticDestoryMonitor() : destoryed_(false) {}
        ~StaticDestoryMonitor()
        {
            destoryed_ = true;
        }

        bool IsDestoryed() const
        {
            return destoryed_;
        }

    private:
        bool destoryed_;
    };
    static StaticDestoryMonitor staticDestoryMonitor_;
    void RebuildWebviewPasteData(PasteData &pasteData);
    void Init();
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_CLIENT_H
