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
import router from '@ohos.router'
import prompt from '@system.prompt';
var EVENT_CANCEL = "EVENT_CANCEL";
var EVENT_VALUE = "value";
export default {
    onInit() {
    },
    onShow() {
        const options = {
            duration: 800,
            easing: 'linear',
            iterations: 'Infinity'
        };
        const frames = [
            {
                transform: {
                    rotate: '0deg'
                }
            },
            {
                transform: {
                    rotate: '360deg'
                }
            }
        ];
        this.animation = this.$element('loading-img').animate(frames, options);
        this.animation.play();
    },
    onCancel(msg){
        callNativeHandler(EVENT_CANCEL, EVENT_VALUE);
        prompt.showToast({
            message: msg
        });
    }
};