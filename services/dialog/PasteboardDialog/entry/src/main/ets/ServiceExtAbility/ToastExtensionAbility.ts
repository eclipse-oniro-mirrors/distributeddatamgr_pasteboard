/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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
import hilog from '@ohos.hilog';
import window from '@ohos.window';
import display from '@ohos.display';
import ServiceExtensionAbility from '@ohos.app.ability.ServiceExtensionAbility';
import type Want from '@ohos.application.Want';

const TAG = 'ToastExtensionAbility';
const DBG = '[TOAST_DBG]';
const BG_COLOR = '#00000000';
const SHOW_TOAST_TIME = 3000;

interface IRect {
  left: number;
  top: number;
  width: number;
  height: number;
}

interface ToastStorageItem {
  toastText: string;
}

export default class ToastExtensionAbility extends ServiceExtensionAbility {
  onCreate(want: Want): void {
    hilog.info(0, TAG, `${DBG} onCreate`);
  }

  onRequest(want: Want, startId: number): void {
    hilog.info(0, TAG, `${DBG} onRequest startId=${startId}`);
    let appName: string = want.parameters?.appName as string;
    hilog.info(0, TAG, `${DBG} onRequest appName from want = ${appName}`);
    if (!appName) {
      hilog.warn(0, TAG, `${DBG} onRequest appName is null, fallback to unknown`);
      appName = 'unknown';
    }
    const toastText: string = this.context.resourceManager.getStringByNameSync('pasteboard_access_tip', appName);
    hilog.info(0, TAG, `${DBG} onRequest toastText = ${toastText}`);
    const displayClass = display.getDefaultDisplaySync();
    hilog.info(0, TAG, `${DBG} display w=${displayClass.width} h=${displayClass.height}`);
    const storageItem: ToastStorageItem = {
      toastText: toastText
    };
    const rect: IRect = {
      left: 0,
      top: 0,
      width: displayClass.width,
      height: displayClass.height
    };
    this.createToastWindow(`PasteboardToast${startId}`, rect, storageItem);
  }

  private async createToastWindow(name: string, rect: IRect, storageItem: ToastStorageItem): Promise<void> {
    hilog.info(0, TAG, `${DBG} createToastWindow begin, name=${name}`);
    let win: window.Window | null = null;
    const displayChangeCallback = (id: number) => {
      const d = display.getDefaultDisplaySync();
      hilog.info(0, TAG, `${DBG} display changed, w=${d.width} h=${d.height}`);
      if (win) {
        win.resize(d.width, d.height);
      }
    };
    try {
      hilog.info(0, TAG, `${DBG} createToastWindow: calling window.createWindow TYPE_SYSTEM_TOAST`);
      win = await window.createWindow({
        ctx: this.context,
        name,
        windowType: window.WindowType.TYPE_SYSTEM_TOAST
      });
      hilog.info(0, TAG, `${DBG} createToastWindow: window.createWindow success`);
      const storage: LocalStorage = new LocalStorage(storageItem);
      await win.moveWindowTo(rect.left, rect.top);
      await win.resize(rect.width, rect.height);
      await win.loadContent('pages/toastIndex', storage);
      hilog.info(0, TAG, `${DBG} createToastWindow: loadContent success`);
      await win.setWindowBackgroundColor(BG_COLOR);
      await win.setWindowTouchable(false);
      await win.showWindow();
      hilog.info(0, TAG, `${DBG} createToastWindow: showWindow success, toast should be visible`);
      display.on('change', displayChangeCallback);
      hilog.info(0, TAG, `${DBG} createToastWindow: display change listener registered`);
      setTimeout(async () => {
        hilog.info(0, TAG, `${DBG} createToastWindow: setTimeout fired, destroying window`);
        display.off('change', displayChangeCallback);
        hilog.info(0, TAG, `${DBG} createToastWindow: display listener unregistered`);
        if (win) {
          await win.destroyWindow();
          hilog.info(0, TAG, `${DBG} createToastWindow: destroyWindow done`);
        }
        this.context.terminateSelf();
        hilog.info(0, TAG, `${DBG} createToastWindow: terminateSelf called`);
      }, SHOW_TOAST_TIME);
    } catch (err) {
      hilog.error(0, TAG, `${DBG} createToastWindow: FAILED err=${JSON.stringify(err)}`);
      display.off('change', displayChangeCallback);
      this.context.terminateSelf();
    }
  }

  onDestroy(): void {
    hilog.info(0, TAG, `${DBG} onDestroy`);
  }
}
