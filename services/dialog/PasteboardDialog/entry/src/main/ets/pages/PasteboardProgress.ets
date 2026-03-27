/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
import { AttributeUpdater } from '@ohos.arkui.modifier';
import resourceManager from '@ohos.resourceManager';
import { BusinessError } from '@ohos.base';
import intl from '@ohos.intl';
import componentUtils from '@ohos.arkui.componentUtils';
import measure from '@ohos.measure';
import promptAction from '@ohos.promptAction';
import EnvironmentCallback from '@ohos.app.ability.EnvironmentCallback';
import common from '@ohos.app.ability.common';
import { Configuration } from '@ohos.app.ability.Configuration';
import ConfigurationConstant from '@ohos.app.ability.ConfigurationConstant';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import { ComponentContent, LengthMetrics } from '@ohos.arkui.node';
import window from '@ohos.window';
import rpc from '@ohos.rpc';
import unifiedDataChannel from '@ohos.data.unifiedDataChannel';
import uniformDataStruct from '@ohos.data.uniformDataStruct';
import uniformTypeDescriptor from '@ohos.data.uniformTypeDescriptor';
import CommonConstants, {
  ProgressPromptText,
  ProgressErrorPromptText,
  SignalValue
} from '../common/constants/CommonConstants';
import GlobalContext from '../PasteboardProgressAbility/GlobalContext';

const TAG = 'PasteboardProgressAbility: ';
const DIALOG_MARGIN: number = getNumberByResourceId($r('sys.float.padding_level8').id, 16);
const PERCENTAGE_CALCULATION: number = 100;
const DIALOG_MAX_WIDTH: number = 400;
const DIALOG_MAX_HEIGHT_PERCENTAGE: number = 0.9;
const PROGRESS_SIGNAL_IPC_CODE: number = 0;
const PROGRESS_SIGNAL_IPC_CODE_IDL: number = 1;

function getNumberByResourceId(resourceId: number, defaultValue: number, allowZero?: boolean): number {
  try {
    let sourceValue: number = resourceManager.getSystemResourceManager().getNumber(resourceId);
    if (sourceValue > 0 || allowZero) {
      return sourceValue;
    } else {
      return defaultValue;
    }
  } catch (error) {
    console.error(TAG + `GetNumberByResourceId error, code: ${error.code}, message: ${error.message}`);
    return defaultValue;
  }
}

class TextModifier extends AttributeUpdater<TextAttribute, TextInterface> {
  initializeModifier(instance: TextAttribute): void {
  }
}

class ProgressModifier extends AttributeUpdater<ProgressAttribute, ProgressInterface> {
  initializeModifier(instance: ProgressAttribute): void {
  }
}

class WantInfo {
  public parameters: Object[]

  constructor(parameters: Object[]) {
    this.parameters = parameters
  }
}

class Params {
  public currentNum: number = 0;
  public promptText: Resource;
  public winCloseCallback: Callback<boolean>;
  public updateConfiguration: Callback<void>;

  constructor(currentNum: number, promptText: Resource, winCloseCallback: Callback<boolean>,
    updateConfiguration: Callback<void>) {
    this.currentNum = currentNum;
    this.promptText = promptText;
    this.winCloseCallback = winCloseCallback;
    this.updateConfiguration = updateConfiguration;
  }
}

let storage = LocalStorage.getShared();

@Entry(storage)
@Component
export struct Index {
  @LocalStorageLink('want') want: WantInfo = new WantInfo([]);
  @LocalStorageLink('win') win: window.Window = {} as window.Window;
  @LocalStorageLink('globalContext') globalContext: GlobalContext = new GlobalContext();
  @LocalStorageLink('serviceContext') serviceContext: common.ServiceExtensionContext | undefined = undefined;
  private ctx: UIContext | undefined = this.getUIContext();
  private params: Params | undefined = undefined;
  private contentNode: ComponentContent<Params> | undefined = undefined;
  private timer: number | null = null;
  private pollInterval: number = 50;
  private timeoutInterval: number = 10000;
  private unChangeProgressTimes: number = 0;
  private isTimeoutError: boolean = false;
  private latestUpdateTime: number = 0;

  aboutToAppear(): void {
    const promptTextPre: string = this.want.parameters['promptText'];
    const promptText: Resource = this.want.parameters['isRemote'] ?
    $r(`app.string.${ProgressPromptText[promptTextPre]}`, this.want.parameters['remoteDeviceName']) :
    $r(`app.string.${ProgressPromptText[promptTextPre]}`);
    this.params =
      new Params(0, promptText, (isNeedCancelTask: boolean) => {
        this.closeDialog(isNeedCancelTask)
      }, () => this.updateConfiguration());
    this.contentNode = new ComponentContent<Params>(this.ctx as UIContext, wrapBuilder(buildProgress), this.params);
    let options: promptAction.BaseDialogOptions = {
      autoCancel: false,
      onWillDismiss: (dismissDialogAction: DismissDialogAction) => {
        console.info(TAG + `reason: ${JSON.stringify(dismissDialogAction.reason)}`);
        if (dismissDialogAction.reason === DismissReason.PRESS_BACK) {
          return;
        }
        dismissDialogAction.dismiss();
      },
      onDidDisappear: () => {
        this.onDialogDisappear();
      }
    };
    this.getUIContext()
      .getPromptAction()
      .openCustomDialog(this.contentNode, options)
      .then(() => {
        this.queryProgressData();
      })
      .catch((error: BusinessError) => {
        console.error(TAG + `OpenCustomDialog args error code is ${error.code}, message is ${error.message}`);
      })
  }

  queryProgressData() {
    let options: unifiedDataChannel.Options = {
      key: this.want.parameters['progressKey']
    };
    try {
      unifiedDataChannel.queryData(options, (err, data) => {
        if (err || !data[0]?.getRecords()?.length) {
          this.unChangeProgressTimes++;
          if (err) {
            console.error(TAG + `Failed to query data. code is ${err.code}, message is ${err.message}`);
          }
          if (this.unChangeProgressTimes > this.timeoutInterval / this.pollInterval) {
            this.handleTimeoutError();
          } else {
            this.pollQuery();
          }
          return;
        }
        let records = data[0].getRecords();
        let record = records[0].getValue() as uniformDataStruct.PlainText;
        console.info(TAG + `Succeeded in querying data. record.textContent = ${record.textContent}`);
        const currentNum = Number(record.textContent) || 0;
        const updateTime = Number(record.abstract) || 0;
        if ((this.params as Params).currentNum === currentNum && this.latestUpdateTime == updateTime) {
          this.unChangeProgressTimes++;
        } else {
          this.unChangeProgressTimes = 0;
        }
        if (this.unChangeProgressTimes > this.timeoutInterval / this.pollInterval) {
          this.handleTimeoutError();
        } else {
          this.latestUpdateTime = updateTime;
          (this.params as Params).currentNum = currentNum;
          this.contentNode?.update(this.params);
          if (currentNum < PERCENTAGE_CALCULATION) {
            this.pollQuery();
          }
        }
      })
    } catch (error) {
      console.error(TAG + `Query data throws an exception. code is ${error.code}, message is ${error.message}`);
    }
  }

  private pollQuery(): void {
    this.timer = setTimeout(() => {
      this.queryProgressData();
    }, this.pollInterval);
  }

  private handleTimeoutError(): void {
    this.isTimeoutError = true;
    const promptTextPre: string = this.want.parameters['promptText'];
    (this.params as Params).promptText = $r(`app.string.${ProgressErrorPromptText[promptTextPre]}`);
    this.contentNode?.update(this.params);
    console.error(TAG + `unChangeProgressTimes is ${this.unChangeProgressTimes}`);
  }

  private onDialogDisappear(): void {
    console.info(TAG + 'on disappear');
    clearTimeout(this.timer);
    this.ctx = undefined;
    this.contentNode = undefined;
    this.params = undefined;
  }

  closeDialog(isNeedCancelTask: boolean): void {
    console.info(TAG + 'closeDialog');
    if (this.contentNode === undefined) {
      console.info(TAG + 'already closed');
      return;
    }
    let promptAction = (this.ctx as UIContext).getPromptAction();
    if (!promptAction) {
      console.error(TAG + 'getPromptAction or closeDialog is null.');
      return;
    }
    promptAction.closeCustomDialog(this.contentNode);
    this.contentNode.dispose();
    this.win.destroyWindow();
    this.globalContext.dialogSet.delete(this.want.parameters?.['progressKey']);
    console.info(TAG + `dialogSet size is ${this.globalContext.dialogSet.size}`);
    if (this.globalContext.dialogSet.size === 0) {
      this.serviceContext?.terminateSelf();
    }
    if (isNeedCancelTask) {
      this.cancelTask();
      this.cancelTaskIdl();
    }
  }

  async cancelTask() {
    const option = new rpc.MessageOption();
    const data = new rpc.MessageSequence();
    const reply = new rpc.MessageSequence();
    try {
      const proxy = this.want.parameters['ipcCallback'].value as rpc.IRemoteObject;
      await data.writeInterfaceToken(CommonConstants.COMP_DIALOG_CALLBACK);
      await data.writeString(this.isTimeoutError ? SignalValue.CANCEL_TIMEOUT_ERROR : SignalValue.CANCEL_PROGRESS_TASK);
      proxy.sendMessageRequest(PROGRESS_SIGNAL_IPC_CODE, data, reply, option);
    } catch (err) {
      console.error(TAG + `hap sendMessageRequest failed: ${JSON.stringify(err)}`);
    } finally {
      data.reclaim();
      reply.reclaim();
    }
  }

  async cancelTaskIdl() {
    const option = new rpc.MessageOption();
    const data = new rpc.MessageSequence();
    const reply = new rpc.MessageSequence();
    try {
      const proxy = this.want.parameters['ipcCallback'].value as rpc.IRemoteObject;
      await data.writeInterfaceToken(CommonConstants.COMP_DIALOG_CALLBACK_IDL);
      await data.writeString(this.isTimeoutError ? SignalValue.CANCEL_TIMEOUT_ERROR : SignalValue.CANCEL_PROGRESS_TASK);
      proxy.sendMessageRequest(PROGRESS_SIGNAL_IPC_CODE_IDL, data, reply, option);
    } catch (err) {
      console.error(TAG + `hap sendMessageRequest failed: ${JSON.stringify(err)}`);
    } finally {
      data.reclaim();
      reply.reclaim();
    }
  }

  updateConfiguration(): void {
    if (this.contentNode === undefined) {
      console.info(TAG + 'already closed');
      return;
    }
    this.contentNode?.updateConfiguration();
  }

  build() {
  }
}

@Builder
function buildProgress($$: Params) {
  PasteboardProgress({
    currentNum: $$.currentNum,
    promptText: $$.promptText,
    closeCallback: $$.winCloseCallback,
    updateConfiguration: $$.updateConfiguration,
  })
}

@ComponentV2
struct PasteboardProgress {
  @Local winWidth: number = 0;
  @Local maxHeight: number = 0;
  @Local oneLineOverFlow: boolean = false;
  private promptTextMarginRight: number = 17;
  @Param currentNum: number = 0;
  @Monitor('currentNum')
  currentNumChange(){
    this.currentPercentageChange()
  }
  @Param @Require promptText: Resource;
  @Monitor('promptText')
  PromptTextChange() {
    this.onPromptTextChange()
  }
  @Local idleWidth: Number = 100;
  @Monitor('idleWidth')
  IdleWidthChanged(){
    this.onIdleWidthChanged()
  }
  @Param closeCallback: Callback<boolean> = () =>{
  };
  @Param updateConfiguration:Callback<void> = () =>{
  };
  private progressModifier: ProgressModifier = new ProgressModifier();
  private percentageTextModifier: TextModifier = new TextModifier();
  private numberFormat: intl.NumberFormat = new intl.NumberFormat('', { style: 'percent' });
  private currentPercentage: string = this.numberFormat.format(0);
  private currentPercentageNum: number = 0;
  private callbackId: number = 0;
  private context = getContext(this) as common.UIAbilityContext;
  private systemLanguage: string | undefined = this.context.config.language;
  private systemColorMode: ConfigurationConstant.ColorMode | undefined = this.context.config.colorMode;

  async aboutToAppear(): Promise<void> {
    console.info(TAG + 'PasteboardProgress aboutToAppear');
    let applicationContext: common.ApplicationContext = this.context.getApplicationContext();
    let configurationChange = (newConfig: Configuration) => this.onConfigurationUpdatedCall(newConfig);

    let environmentCallback: EnvironmentCallback = {
      onConfigurationUpdated(newConfig: Configuration) {
        configurationChange(newConfig);
      },
      onMemoryLevel(level: AbilityConstant.MemoryLevel) {
        console.info(TAG + `onMemoryLevel level: ${level}`);
      }
    }

    try {
      this.callbackId = applicationContext.on('environment', environmentCallback);
    } catch (err) {
      console.error(TAG + `Failed to register applicationContext. Code is ${err.code}, message is ${err.message}`);
    }
  }

  onConfigurationUpdatedCall(newConfig: Configuration): void {
    console.info(TAG + `onConfigurationUpdated newConfig: ${JSON.stringify(newConfig)}`);
    if (this.systemLanguage !== newConfig.language) {
      console.info(TAG + `systemLanguage from ${this.systemLanguage} changed to ${newConfig.language}`);
      this.systemLanguage = newConfig.language;
    }
    if (this.systemColorMode !== newConfig.colorMode) {
      console.info(TAG + `systemColorMode from ${this.systemColorMode} changed to ${newConfig.colorMode}`);
      this.systemColorMode = newConfig.colorMode;
    }
    this.updateConfiguration();
  }

  aboutToDisappear(): void {
    console.info(TAG + 'aboutToDisappear applicationContext off environment.');
    let applicationContext: common.ApplicationContext = this.context.getApplicationContext();
    try {
      applicationContext.off('environment', this.callbackId);
      this.callbackId = 0;
    } catch (err) {
      console.error(TAG + `Failed to unregister applicationContext. Code is ${err.code}, message is ${err.message}`);
    }
  }

  currentPercentageChange(): void {
    if (this.currentNum <= 0) {
      this.currentPercentageNum = 0;
      this.currentPercentage = '';
    } else if (this.currentPercentageNum > this.currentNum || this.currentNum > 100) {
      console.info(TAG +
        `current num exception. currentPercentageNum: ${this.currentPercentageNum} currentNum: ${this.currentNum}`);
      return;
    }
    console.info(TAG + `current num currentNum. ${this.currentNum}`);
    this.currentPercentageNum = Math.round(this.currentNum);
    this.currentPercentage = this.numberFormat.format(this.currentNum / PERCENTAGE_CALCULATION);
    this.percentageTextModifier.updateConstructorParams(this.currentPercentage);
    this.progressModifier.attribute?.value(this.currentPercentageNum);
    this.progressModifier.updateConstructorParams({ value: this.currentPercentageNum });
    if (this.currentPercentageNum === PERCENTAGE_CALCULATION) {
      setTimeout(() => this.closeCallback(false), 300);
    }
  }

  onPromptTextChange() {
    this.onIdleWidthChanged();
  }

  onIdleWidthChanged() {
    let itemLenWidthPX1: SizeOptions = measure.measureTextSize({
      textContent: this.promptText,
      fontSize: 14,
      textAlign: TextAlign.Start,
      fontWeight: FontWeight.Regular
    });
    let itemLenWidthPX2: SizeOptions = measure.measureTextSize({
      textContent: this.currentPercentage,
      fontSize: 14,
      fontWeight: FontWeight.Regular
    })
    if (this.idleWidth >
      px2vp((itemLenWidthPX1.width as number) + (itemLenWidthPX2.width as number)) + this.promptTextMarginRight) {
      this.oneLineOverFlow = false
    } else {
      this.oneLineOverFlow = true
    }
  }

  build() {
    RelativeContainer() {
      Row() {
        this.pastedProgressLoading()
      }
      .width(this.winWidth)
      .constraintSize({ maxWidth: DIALOG_MAX_WIDTH, maxHeight: this.maxHeight })
      .margin({
        left: $r('sys.float.padding_level8'),
        right: $r('sys.float.padding_level8')
      })
      .borderRadius($r('sys.float.ohos_id_corner_radius_panel'))
      .backgroundBlurStyle(BlurStyle.COMPONENT_ULTRA_THICK)
      .alignRules({
        center: { anchor: '__container__', align: VerticalAlign.Center },
        middle: { anchor: '__container__', align: HorizontalAlign.Center }
      })
    }
    .onSizeChange((oldValue: SizeOptions, newValue: SizeOptions) => {
      console.info(TAG + 'dialog size change. width: ' + newValue.width + ' height: ' + newValue.height);
      if (Number(newValue.width) === 0 || Number(newValue.height) === 0) {
        return;
      }
      this.winWidth = Number(newValue.width) - DIALOG_MARGIN * 2;
      this.maxHeight = Number(newValue.height) * DIALOG_MAX_HEIGHT_PERCENTAGE - 28;
    })
    .height('100%')
    .width('100%')
  }

  @Builder
  pastedProgressLoading() {
    if (this.oneLineOverFlow) {
      Column() {
        this.showPromptText(2, 2, undefined)
        this.BuilderPercentageText(2)
        Row() {
          this.showProgress(1)
          this.cancelButton()
        }
      }
      .onSizeChange((oldValue: SizeOptions, newValue: SizeOptions) => {
        this.idleWidth =
          newValue.width as number - px2vp(componentUtils.getRectangleById('close_button').size.width) - 24
      })
      .alignItems(HorizontalAlign.Start)
      .margin({
        left: 24,
        right: 24,
        top: 33.5,
        bottom: 24.5
      })
    } else {
      Row() {
        Column() {
          Row() {
            this.showPromptText(1, undefined, 1)
            this.BuilderPercentageText(2)
          }
          .margin({ left: 3, bottom: 4.5 })
          .onSizeChange((oldValue: SizeOptions, newValue: SizeOptions) => {
            this.idleWidth = newValue.width as number
          })
          .justifyContent(FlexAlign.Start)

          this.showProgress(undefined)
        }
        .alignItems(HorizontalAlign.Start)
        .layoutWeight(1)

        this.cancelButton()
      }
      .alignItems(VerticalAlign.Bottom)
      .margin({
        left: 24,
        right: 24,
        top: 33.5,
        bottom: 24.5
      })
    }
  }

  @Builder
  showPromptText(maxLines: number, maxFontScale: number | undefined, layoutWeight: number | undefined) {
    Text(this.promptText)
      .fontSize(14)
      .fontFamily('HarmonyHeiTi')
      .fontWeight(FontWeight.Regular)
      .fontColor($r('sys.color.font_primary'))
      .constraintSize({ minHeight: 20 })
      .wordBreak(WordBreak.BREAK_ALL)
      .textAlign(TextAlign.Start)
      .textOverflow({ overflow: TextOverflow.Ellipsis })
      .margin({ end: LengthMetrics.vp(this.promptTextMarginRight) })
      .maxLines(maxLines)
      .maxFontScale(maxFontScale)
      .layoutWeight(layoutWeight)
  }

  @Builder
  BuilderPercentageText(maxFontScale: number | undefined) {
    Text(this.currentPercentage)
      .attributeModifier(this.percentageTextModifier)
      .fontSize(14)
      .fontWeight(FontWeight.Regular)
      .fontFamily('HarmonyHeiTi')
      .fontColor($r('sys.color.font_secondary'))
      .lineHeight(19)
      .maxLines(1)
      .constraintSize({ minHeight: 20 })
      .wordBreak(WordBreak.BREAK_ALL)
      .maxFontScale(maxFontScale)
      .onSizeChange(() => {
        this.onIdleWidthChanged()
      })
  }

  @Builder
  showProgress(layoutWeight: number | undefined) {
    Progress({ value: this.currentPercentageNum, type: ProgressType.Linear })
      .attributeModifier(this.progressModifier)
      .color($r('sys.color.comp_background_emphasize'))
      .height(24)
      .value(this.currentPercentageNum)
      .style({ enableSmoothEffect: true })
      .layoutWeight(layoutWeight)
  }

  @Builder
  cancelButton() {
    Button({ buttonStyle: ButtonStyleMode.TEXTUAL }) {
      SymbolGlyph($r('sys.symbol.xmark_circle_fill'))
        .fontColor([$r('sys.color.ohos_id_color_foreground')])
        .fontSize('24vp')
    }
    .id('close_button')
    .opacity(0.4)
    .margin({ start: LengthMetrics.vp(16) })
    .size({ width: 24, height: 24 })
    .accessibilityText($r('app.string.progress_cancel_btn'))
    .onClick(() => {
      console.warn(TAG + 'pastedProgressLoading on click');
      this.closeCallback(true);
    })
  }
}
