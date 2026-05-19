/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import deviceInfo from '@ohos.deviceInfo';

const TYPE_PC: string = '2in1';
const TYPE_PAD: string = 'tablet';
const TYPE_PHONE: string = 'phone';
const TYPE_DEFAULT: string = 'default';

export const IS_PC: boolean = deviceInfo.deviceType === TYPE_PC;

export const IS_PAD: boolean = deviceInfo.deviceType === TYPE_PAD;

export const IS_PHONE: boolean = [TYPE_PHONE, TYPE_DEFAULT].includes(deviceInfo.deviceType);
