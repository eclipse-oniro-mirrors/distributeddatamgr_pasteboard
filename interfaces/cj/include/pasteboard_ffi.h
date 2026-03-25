/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PASTEBOARD_FFI_H
#define PASTEBOARD_FFI_H

#include "cj_ffi/cj_common_ffi.h"

extern "C" {
#define OUT_OF_RANGE 12900001

typedef struct {
    CArrString mimeTypes;
    char *tag;
    int64_t timestamp;
    bool localOnly;
    int32_t shareOption;
} CPasteDataProperty;

typedef struct {
    char *htmlText;
    char *mimeType;
    char *plainText;
    char *uri;
    int64_t pixelMap;
} CPasteDataRecord;

FFI_EXPORT RetDataI64 FfiOHOSCreateStringPasteData(const char *mimeType, const char *value);
FFI_EXPORT RetDataI64 FfiOHOSCreatePixelMapPasteData(const char *mimeType, int64_t pixelMapId);
FFI_EXPORT RetDataI64 FfiOHOSCreateArrayBufPasteData(const char *mimeType, uint8_t *buffPtr, int64_t bufferSize);
FFI_EXPORT RetDataI64 FfiOHOSCreateStringPasteDataRecord(
    const char *mimeType, const char *value, CPasteDataRecord *retPtr);
FFI_EXPORT RetDataI64 FfiOHOSCreatePixelMapPasteDataRecord(
    const char *mimeType, int64_t pixelMapId, CPasteDataRecord *retPtr);
FFI_EXPORT RetDataI64 FfiOHOSCreateArrayBufPasteDataRecord(
    const char *mimeType, uint8_t *buffPtr, int64_t bufferSize, CPasteDataRecord *retPtr);
FFI_EXPORT RetDataCString FfiOHOSPasteDataRecordToPlainText(int64_t id);
FFI_EXPORT RetDataCString FfiOHOSPasteDataGetPrimaryText(int64_t id);
FFI_EXPORT RetDataCString FfiOHOSPasteDataGetPrimaryHtml(int64_t id);
FFI_EXPORT RetDataCString FfiOHOSPasteDataGetPrimaryUri(int64_t id);
FFI_EXPORT RetDataI64 FfiOHOSPasteDataGetPrimaryPixelMap(int64_t id);
FFI_EXPORT RetDataCString FfiOHOSPasteDataGetPrimaryMimeType(int64_t id);
FFI_EXPORT int32_t FfiOHOSPasteDataGetProperty(int64_t id, CPasteDataProperty *retPtr);
FFI_EXPORT int32_t FfiOHOSPasteDataSetProperty(
    int64_t id, CArrString mimeTypes, const char *tag, int64_t timestamp, bool localOnly, int32_t shareOption);
FFI_EXPORT RetDataCString FfiOHOSPasteDataGetTag(int64_t id);
FFI_EXPORT RetDataBool FfiOHOSPasteDataHasType(int64_t id, const char *mimeTypes);
FFI_EXPORT int32_t FfiOHOSPasteDataAddRecord(int64_t id, int64_t recordId);
FFI_EXPORT int32_t FfiOHOSPasteDataAddMimeTypeRecord(int64_t id, const char *mimeType, const char *value);
FFI_EXPORT int32_t FfiOHOSPasteDataAddPixelMapRecord(int64_t id, const char *mimeType, int64_t pixelMapId);
FFI_EXPORT int32_t FfiOHOSPasteDataAddArrayRecord(
    int64_t id, const char *mimeType, uint8_t *buffPtr, int64_t bufferSize);
FFI_EXPORT RetDataCArrString FfiOHOSPasteDataGetMimeTypes(int64_t id);
FFI_EXPORT RetDataI64 FfiOHOSPasteDataGetRecord(int64_t id, int32_t index, CPasteDataRecord *retPtr);
FFI_EXPORT RetDataUI FfiOHOSPasteDataGetRecordCount(int64_t id);
FFI_EXPORT int32_t FfiOHOSPasteDataRemoveRecord(int64_t id, int32_t index);
FFI_EXPORT int32_t FfiOHOSPasteDataReplaceRecord(int64_t id, int64_t recordId, int32_t index);
FFI_EXPORT RetDataI64 FfiOHOSGetSystemPasteboard();
FFI_EXPORT int32_t FfiOHOSSystemPasteboardSetData(int64_t id, int64_t pasteDataId);
FFI_EXPORT RetDataI64 FfiOHOSSystemPasteboardGetData(int64_t id);
FFI_EXPORT RetDataBool FfiOHOSSystemPasteboardHasData(int64_t id);
FFI_EXPORT void FfiOHOSSystemPasteboardClearData(int64_t id);
FFI_EXPORT RetDataBool FfiOHOSSystemPasteboardIsRemoteData(int64_t id);
FFI_EXPORT RetDataBool FfiOHOSSystemPasteboardHasDataType(int64_t id, const char *mimeType);
FFI_EXPORT RetDataCString FfiOHOSSystemPasteboardGetDataSource(int64_t id);
}

#endif