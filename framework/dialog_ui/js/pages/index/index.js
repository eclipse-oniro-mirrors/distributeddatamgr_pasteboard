/*
    Copyright (c) 2022 Huawei Device Co., Ltd.
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
import prompt from '@system.prompt';
import router from '@ohos.router';
var EVENT_CANCEL = "EVENT_CANCEL";
var EVENT_INIT = "EVENT_INIT";
var EVENT_VALUE = "value";
export default {
    data: {
        appName : router.getParams().appName,
        deviceType: router.getParams().deviceType,
    },
    onInit() {
        console.info('getParams: ' + JSON.stringify(router.getParams()));
        callNativeHandler(EVENT_INIT, EVENT_VALUE);
    },
    onZombie() {
        console.error('Zombie dialog!');
        prompt.showToast({
            message: this.$t("strings.zombie")
        });
    },
    onCancel() {
        console.info('onCancel: ' + EVENT_CANCEL);
        // start the timer to prevent myself being a zombie dialog
        setTimeout(this.onZombie, 5000);
        prompt.showToast({
            message: this.$t("strings.cancel")
        });
        callNativeHandler(EVENT_CANCEL, EVENT_VALUE);
    }
};