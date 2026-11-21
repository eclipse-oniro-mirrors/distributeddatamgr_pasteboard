/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef PASTE_BOARD_PASTEBOARD_SERV_IPC_INTERFACE_CODE_H
#define PASTE_BOARD_PASTEBOARD_SERV_IPC_INTERFACE_CODE_H

/* SAID: 3701 */
namespace OHOS {
namespace Security {
namespace PasteboardServ {
enum PasteboardServiceInterfaceCode {
    GET_PASTE_DATA = 0,
    HAS_PASTE_DATA = 1,
    SET_PASTE_DATA = 2,
    CLEAR_ALL = 3,
    SUBSCRIBE_OBSERVER = 4,
    UNSUBSCRIBE_OBSERVER = 5,
    UNSUBSCRIBE_ALL_OBSERVER = 6,
    IS_REMOTE_DATA = 7,
    GET_DATA_SOURCE = 8,
    HAS_DATA_TYPE = 9,
    SET_GLOBAL_SHARE_OPTION = 10,
    REMOVE_GLOBAL_SHARE_OPTION = 11,
    GET_GLOBAL_SHARE_OPTION = 12,
    SET_APP_SHARE_OPTIONS = 13,
    REMOVE_APP_SHARE_OPTIONS = 14,
    PASTE_START = 15,
    PASTE_COMPLETE = 16,
    REGISTER_CLIENT_DEATH_OBSERVER = 17,
    DETECT_PATTERNS = 18,
    GET_RECORD_VALUE = 19,
    GET_MIME_TYPES = 20,
    GET_REMOTE_DEVICE_NAME = 21,
    SHOW_PROGRESS = 22,
    GET_CHANGE_COUNT = 23,
    SUBSCRIBE_ENTITY_OBSERVER = 24,
    UNSUBSCRIBE_ENTITY_OBSERVER = 25,
};

enum PasteboardObserverInterfaceCode {
    ON_PASTE_BOARD_CHANGE = 0,
    ON_PASTE_BOARD_EVENT = 1,
};

enum PasteboardDisposableInterfaceCode {
    ON_TEXT_RECEIVED = 0,
};

enum EntityObserverInterfaceCode {
    ON_RECOGNITION_EVENT = 0,
};

enum PasteboardEntryGetterInterfaceCode {
    GET_RECORD_VALUE_BY_TYPE = 0,
};

enum PasteboardFuseGetterInterfaceCode {
    ON_READ = 0,
    ON_READ_ASYNC = 1,
    ON_STAT = 2,
    ON_CANCEL = 3,
};
} // namespace PasteboardServ
} // namespace Security
} // namespace OHOS
#endif // PASTE_BOARD_PASTEBOARD_SERV_IPC_INTERFACE_CODE_H
