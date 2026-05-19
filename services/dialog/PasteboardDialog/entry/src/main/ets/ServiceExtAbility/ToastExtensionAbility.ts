/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
import rpc from '@ohos.rpc';
import hilog from '@ohos.hilog';
import promptAction from '@ohos.promptAction';
import ServiceExtensionAbility from '@ohos.app.ability.ServiceExtensionAbility';
import type Want from '@ohos.application.Want';
import GlobalContext from './GlobalParam';

const TAG = 'ToastExtensionAbility';
const INFO = '读取了剪贴板信息';

class ToastStub extends rpc.RemoteObject {
  constructor(des: string) {
    super(des);
  }
}

export default class ToastExtensionAbility extends ServiceExtensionAbility {
  onCreate(want: Want): void {
    hilog.info(0, TAG, 'onCreate');
    GlobalContext.getInstance().context = this.context;
  }

  onConnect(want: Want): ToastStub {
    hilog.info(0, TAG, 'onConnect');
    let showInfo: string = want.parameters.appName + INFO;
    promptAction.showToast({message:showInfo});
    return new ToastStub('PasteboardToast');
  }

  onRequest(want: Want, startId: number): void {
    hilog.info(0, TAG, 'onRequest');
    this.onConnect(want);
  }

  onDisconnet(): void {
    hilog.info(0, TAG, 'onDisconnet');
    this.onDestroy();
  }

  onDestroy(): void {
    hilog.info(0, TAG, 'onDestroy');
    GlobalContext.getInstance().context.terminateSelf();
  }
}
