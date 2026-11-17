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

#include "cj_ffi/cj_common_ffi.h"

extern "C" {
FFI_EXPORT int FfiOHOSCreateStringPasteData = 0;
FFI_EXPORT int FfiOHOSCreatePixelMapPasteData = 0;
FFI_EXPORT int FfiOHOSCreateArrayBufPasteData = 0;
FFI_EXPORT int FfiOHOSCreateStringPasteDataRecord = 0;
FFI_EXPORT int FfiOHOSCreatePixelMapPasteDataRecord = 0;
FFI_EXPORT int FfiOHOSCreateArrayBufPasteDataRecord = 0;
FFI_EXPORT int FfiOHOSPasteDataRecordToPlainText = 0;
FFI_EXPORT int FfiOHOSPasteDataGetPrimaryText = 0;
FFI_EXPORT int FfiOHOSPasteDataGetPrimaryHtml = 0;
FFI_EXPORT int FfiOHOSPasteDataGetPrimaryUri = 0;
FFI_EXPORT int FfiOHOSPasteDataGetPrimaryPixelMap = 0;
FFI_EXPORT int FfiOHOSPasteDataGetPrimaryMimeType = 0;
FFI_EXPORT int FfiOHOSPasteDataGetProperty = 0;
FFI_EXPORT int FfiOHOSPasteDataSetProperty = 0;
FFI_EXPORT int FfiOHOSPasteDataGetTag = 0;
FFI_EXPORT int FfiOHOSPasteDataHasType = 0;
FFI_EXPORT int FfiOHOSPasteDataAddRecord = 0;
FFI_EXPORT int FfiOHOSPasteDataAddMimeTypeRecord = 0;
FFI_EXPORT int FfiOHOSPasteDataAddPixelMapRecord = 0;
FFI_EXPORT int FfiOHOSPasteDataAddArrayRecord = 0;
FFI_EXPORT int FfiOHOSPasteDataGetMimeTypes = 0;
FFI_EXPORT int FfiOHOSPasteDataGetRecord = 0;
FFI_EXPORT int FfiOHOSPasteDataGetRecordV2 = 0;
FFI_EXPORT int FfiOHOSPasteDataGetRecordCount = 0;
FFI_EXPORT int FfiOHOSPasteDataRemoveRecord = 0;
FFI_EXPORT int FfiOHOSPasteDataReplaceRecord = 0;
FFI_EXPORT int FfiOHOSGetSystemPasteboard = 0;
FFI_EXPORT int FfiOHOSGetSystemPasteboardV2 = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardSetData = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardGetData = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardHasData = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardClearData = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardIsRemoteData = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardHasDataType = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardGetDataSource = 0;
FFI_EXPORT int FfiOHOSCreateMultiTypePasteData = 0;
FFI_EXPORT int FfiOHOSFreeCString = 0;
FFI_EXPORT int FfiOHOSFreeCArrString = 0;
FFI_EXPORT int FfiOHOSFreePasteDataProperty = 0;
FFI_EXPORT int FfiOHOSFreePasteDataRecord = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardSetDataV2 = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardGetDataV2 = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardIsRemoteDataV2 = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardHasDataTypeV2 = 0;
FFI_EXPORT int FfiOHOSSystemPasteboardGetDataSourceV2 = 0;
}