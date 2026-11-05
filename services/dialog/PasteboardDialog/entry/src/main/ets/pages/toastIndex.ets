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

const TAG = 'toastIndex';
const DBG = '[TOAST_DBG]';
const TOAST_BOTTOM_PADDING = 68;

let storage = LocalStorage.getShared();

@Entry(storage)
@Component
struct Index {
  @LocalStorageLink('toastText') toastText: string = '';

  aboutToAppear() {
    hilog.info(0, TAG, `${DBG} aboutToAppear toastText=${this.toastText}`);
  }

  build() {
    Row() {
      Column() {
        Text(this.toastText)
          .fontSize(14)
          .fontWeight(FontWeight.Regular)
          .fontColor($r('app.color.white'))
          .fontFamily($r('app.string.typeface'))
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.None })
          .wordBreak(WordBreak.BREAK_ALL)
          .maxFontScale(2)
          .accessibilityText(this.toastText)
      }
      .constraintSize({ minHeight: 36 })
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .backgroundColor($r('app.color.grey'))
      .borderRadius(18)
      .padding({ left: 16, right: 16 })
    }
    .alignItems(VerticalAlign.Bottom)
    .justifyContent(FlexAlign.Center)
    .height('100%')
    .width('100%')
    .padding({ bottom: TOAST_BOTTOM_PADDING })
  }
}
