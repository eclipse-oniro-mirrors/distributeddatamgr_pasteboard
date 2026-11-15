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

let storage = LocalStorage.getShared();

@Entry(storage)
@Component
struct Index {
  @LocalStorageLink('toastText') toastText: string = '';
  @LocalStorageLink('bottomPadding') bottomPadding: number = 80;

  aboutToAppear() {
    hilog.info(0, TAG, `${DBG} aboutToAppear toastText=${this.toastText}`);
  }

  build() {
    Row() {
      GridRow({
        columns: { xs: 4, sm: 6 },
        gutter: { x: 24 },
        breakpoints: { value: ['600vp', '840vp'] }
      }) {
        GridCol({ span: { xs: 4, sm: 6 } }) {
          Row() {
            Column() {
              Text(this.toastText)
                .fontSize(14)
                .fontWeight(FontWeight.Regular)
                .fontColor($r('sys.color.font_primary'))
                .fontFamily($r('app.string.typeface'))
                .maxLines(2)
                .textOverflow({ overflow: TextOverflow.Ellipsis })
                .wordBreak(WordBreak.BREAK_ALL)
                .maxFontScale(2)
            }
            .constraintSize({ minHeight: 36, maxWidth: 400 })
            .justifyContent(FlexAlign.Center)
            .alignItems(HorizontalAlign.Center)
            .backgroundColor($r('sys.color.comp_background_primary'))
            .backgroundBlurStyle(BlurStyle.COMPONENT_ULTRA_THICK)
            .borderRadius(18)
            .shadow(ShadowStyle.OUTER_DEFAULT_MD)
            .padding({ left: '16vp', right: '16vp', top: '8vp', bottom: '8vp' })
            .accessibilityText(this.toastText)
            .accessibilityLevel('no-hide-descendants')
          }
          .justifyContent(FlexAlign.Center)
          .width('100%')
        }
      }
    }
    .alignItems(VerticalAlign.Bottom)
    .height('100%')
    .width('100%')
    .backgroundColor(Color.Transparent)
    .padding({ bottom: this.bottomPadding, left: 24, right: 24 })
  }
}
