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

export default class CommonConstants {
  //PASTEBOARD page :
  public static readonly IMAGE_COUNT:number = 167;
  public static readonly IMG_ANIMATOR_OVER_DURATION:number = 1000;
  public static readonly IMG_ANIMATOR_NORMAL_DURATION:number = 30;
  public static readonly SWITCH_STATUS_OPEN:string = '1';
  public static readonly SWITCH_STATUS_CLOSE:string = '0';
  public static readonly GENERAL_PHONE_GAP_LENGTH:number = 16;
  public static readonly PC_PAD_GAP_LENGTH:number = 24;
  public static readonly PC_LIST_HEIGHT:number = 48;
  public static readonly PHONE_LIST_HEIGHT:number = 56;
  public static readonly TEXT_LIST_ALIGN_DISTANCE:number = 12;
  public static readonly PC_BORDER_RADIUS:number = 16;
  public static readonly NON_PC_BORDER_RADIUS:number = 20;
  public static readonly PASTEBOARD_DESC_TEXT_MARGIN_BOTTOM:number = 32;
  public static readonly PASTEBOARD_DESC_TEXT_MARGIN_TOP:number = 24;
  public static readonly PASTEBOARD_SWITCH_WIDTH:number = 36;
  public static readonly PASTEBOARD_SWITCH_HEIGHT:number = 20;
  public static readonly ITEM_LIST_PADDING_TOP_PC: number = 13;
  public static readonly ITEM_LIST_PADDING_BOTTOM_PC: number = 13;
  public static readonly ITEM_LIST_PADDING_TOP: number = 17;
  public static readonly ITEM_LIST_PADDING_BOTTOM: number = 18;
  public static readonly FONT_SIZE_SCALE_PARAM: string = 'persist.sys.font_scale_for_user0';
  public static readonly ANIMATOR_HEIGHT_PHONE: number = 192;
  public static readonly ANIMATOR_HEIGHT_PC: number = 207;
  public static readonly ANIMATOR_HEIGHT_PAD: number = 212;
  public static readonly ANIMATOR_WIDTH_PHONE: number = 290;
  public static readonly ANIMATOR_WIDTH_PC: number = 312;
  public static readonly ANIMATOR_WIDTH_PAD: number = 320;

  // Tips
  public static readonly TIPS_TYPE:string = '119';
  public static readonly FUN_NUM = 'SF-10044822_f101';
  // Symbol
  public static readonly SYMBOL_INFO_CIRCLE:string = '14vp';
  public static readonly SYMBOL_MARGIN_RIGHT:number = 4;
  // Update prompt text
  public static readonly UPDATE_PROMPT_LINE_HEIGHT:number = 16;
  public static readonly UPDATE_PROMPT_MARGIN_TOP:number = 9;
  // IPC callback descriptor
  public static readonly COMP_DIALOG_CALLBACK:string = 'ohos.miscservices.dialog.callback';
  // IPC callback descriptor idl
  public static readonly COMP_DIALOG_CALLBACK_IDL:string = 'OHOS.MiscServices.IPasteboardSignal';

  public static readonly DISABLE_DISTRIBUTED_PASTEBOARD:string = 'const.pasteboard.disable_crossdevice_clipboard';
}

export enum FontSizeScale {
  SM = 0.85,
  MD = 1.0,
  DEFAULT = MD,
  L = 1.15,
  XL = 1.3,
  XXL = 1.45,
  XXL1 = 1.75,
  XXL2 = 2,
  XXL3 = 3.2,
}

export enum ProgressPromptText {
  PromptText_PasteBoard_Remote = 'prompttext_pasteboard_remote',
  PromptText_PasteBoard_Local = 'prompttext_pasteboard_local',
  PromptText_Udmf_Import = 'prompttext_udmf_import',
  PromptText_Udmf_Move = 'prompttext_udmf_move',
  PromptText_Udmf_Copy = 'prompttext_udmf_copy',
  PromptText_Udmf_Download = 'prompttext_udmf_download',
  PromptText_Udmf_Format = 'prompttext_udmf_format',
}

export enum ProgressErrorPromptText {
  PromptText_PasteBoard_Remote = 'prompttext_pasteboard_error',
  PromptText_PasteBoard_Local = 'prompttext_pasteboard_error',
  PromptText_Udmf_Import = 'prompttext_udmf_import_error',
  PromptText_Udmf_Move = 'prompttext_udmf_move_error',
  PromptText_Udmf_Copy = 'prompttext_udmf_copy_error',
  PromptText_Udmf_Download = 'prompttext_udmf_download_error',
  PromptText_Udmf_Format = 'prompttext_udmf_format_error',
}

export enum SignalValue {
  CANCEL_PROGRESS_TASK = '1',
  CANCEL_TIMEOUT_ERROR = '2',
}