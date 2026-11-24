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
const MAX_APP_NAME_WIDTH = 20;
const TOAST_BOTTOM_PADDING = 80;

interface ToastStorageItem {
  toastText: string;
  bottomPadding: number;
}

function truncateAppName(name: string): string {
  let width = 0;
  let result = '';
  for (let i = 0; i < name.length; i++) {
    const charWidth = name.charCodeAt(i) > 127 ? 2 : 1;
    if (width + charWidth > MAX_APP_NAME_WIDTH) {
      return result + '…';
    }
    result += name[i];
    width += charWidth;
  }
  return name;
}

export default class ToastExtensionAbility extends ServiceExtensionAbility {
  onCreate(want: Want): void {
    hilog.info(0, TAG, `${DBG} onCreate`);
  }

  onRequest(want: Want, startId: number): void {
    hilog.info(0, TAG, `${DBG} onRequest startId=${startId}`);
    let appName: string = want.parameters?.appName as string;
    if (!appName) {
      appName = 'unknown';
    }
    appName = truncateAppName(appName);
    hilog.info(0, TAG, `${DBG} onRequest appName after truncate = ${appName}`);
    const toastText: string =
      this.context.resourceManager.getStringByNameSync('pasteboard_access_tip', appName);
    hilog.info(0, TAG, `${DBG} onRequest toastText = ${toastText}`);
    this.createToastWindow(`PasteboardToast${startId}`, {
      toastText: toastText,
      bottomPadding: TOAST_BOTTOM_PADDING
    });
  }

  private async updateBottomPadding(
    win: window.Window, d: display.Display, storageItem: ToastStorageItem
  ): Promise<void> {
    try {
      const avoidArea = await win.getWindowAvoidArea(
        window.AvoidAreaType.TYPE_NAVIGATION_INDICATOR);
      const navBarHeightVp = avoidArea.bottomRect.height / d.densityPixels;
      storageItem.bottomPadding = TOAST_BOTTOM_PADDING + navBarHeightVp;
      hilog.info(0, TAG, `${DBG} navBarHeight=${avoidArea.bottomRect.height}`);
    } catch (e) {
      hilog.warn(0, TAG, `${DBG} getWindowAvoidArea failed`);
    }
  }

  private async setupWindow(
    win: window.Window, d: display.Display, storage: LocalStorage
  ): Promise<void> {
    await win.moveWindowTo(0, 0);
    await win.resize(d.width, d.height);
    await win.loadContent('pages/toastIndex', storage);
    await win.setWindowBackgroundColor(BG_COLOR);
    await win.setWindowTouchable(false);
    await win.showWindow();
    hilog.info(0, TAG, `${DBG} showWindow success`);
  }

  private async createToastWindow(
    name: string, storageItem: ToastStorageItem
  ): Promise<void> {
    hilog.info(0, TAG, `${DBG} createToastWindow begin, name=${name}`);
    let win: window.Window | null = null;
    let isDestroyed = false;
    const destroyToast = async () => {
      if (isDestroyed) {
        return;
      }
      isDestroyed = true;
      display.off('change', displayChangeCallback);
      if (win) {
        await win.destroyWindow();
      }
      this.context.terminateSelf();
    };
    const initialW = display.getDefaultDisplaySync().width;
    const initialH = display.getDefaultDisplaySync().height;
    const displayChangeCallback = (id: number) => {
      const curr = display.getDefaultDisplaySync();
      if (curr.width !== initialW || curr.height !== initialH) {
        hilog.info(0, TAG, `${DBG} display rotated, destroying toast`);
        destroyToast();
      }
    };
    try {
      const d = display.getDefaultDisplaySync();
      hilog.info(0, TAG, `${DBG} display w=${d.width} h=${d.height}`);
      win = await window.createWindow({
        ctx: this.context,
        name,
        windowType: window.WindowType.TYPE_SYSTEM_TOAST
      });
      await this.updateBottomPadding(win, d, storageItem);
      const storage: LocalStorage = new LocalStorage(storageItem);
      await this.setupWindow(win, d, storage);
      display.on('change', displayChangeCallback);
      setTimeout(() => {
        hilog.info(0, TAG, `${DBG} setTimeout fired`);
        destroyToast();
      }, SHOW_TOAST_TIME);
    } catch (err) {
      hilog.error(0, TAG, `${DBG} FAILED err=${JSON.stringify(err)}`);
      destroyToast();
    }
  }

  onDestroy(): void {
    hilog.info(0, TAG, `${DBG} onDestroy`);
  }
}
