/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { BusinessError } from '@ohos.base'
import { AsyncCallback } from '@ohos.base';
import Want from '@ohos.app.ability.Want';
import image from '@ohos.multimedia.image';

class Cleaner {
    static callback(cleaner: Cleaner): void {
        console.println("enter Cleaner.callback. ");
        cleaner.clean()
    }

    constructor(targetPtr: long) {
        this.targetPtr = targetPtr
    }

    native clean(): void

    private targetPtr: long = 0
}

class FinalizationAgent<T extends Object> {
    constructor(obj: T, ptr: long) {
        this.register(obj, ptr);
    }

    register(obj: T, ptr: long): void {
        this.unregisterToken = {};
        this.cleaner = new Cleaner(ptr);
        finalizer.register(obj, this.cleaner!, this.unregisterToken);
    }

    unregister(): void {
        finalizer.unregister(this.unregisterToken);
    }

    private cleaner: Cleaner | null = null;
    private unregisterToken: object = {};
}

let finalizer = new FinalizationRegistry<Cleaner>(Cleaner.callback)

export namespace pasteboard {

    loadLibrary("pasteboard_ani");

    export const MIMETYPE_TEXT_URI: string = 'text/uri';
    export const MIMETYPE_TEXT_HTML: string = 'text/html';
    export const MIMETYPE_TEXT_PLAIN: string = 'text/plain';
    export const MIMETYPE_PIXELMAP: string = 'pixelMap';
    export const MIMETYPE_TEXT_WANT: string = 'text/want';
    export type ValueType = string | image.PixelMap | Want | ArrayBuffer;

    export native function createDataTypeValue(mimeType: string, value: ValueType): PasteData;
    export native function createDataRecord(data: Record<string, ValueType>): PasteData;
    export native function getSystemPasteboard(): SystemPasteboard;

    export function createData(mimeType: string, value: ValueType): PasteData {
        return createDataTypeValue(mimeType, value);
    }

    export function createData(data: Record<string, ValueType>): PasteData {
        return createDataRecord(data);
    }

    export enum ShareOption {
        INAPP,
        LOCALDEVICE,
        CROSSDEVICE
    }

    export interface PasteDataProperty {
        tag: string;
        readonly timestamp: number;
        shareOption: ShareOption;
    }

    export interface PasteDataRecord {
        mimeType: string;
        plainText: string;
        uri: string;
    }

    export interface PasteData {
        addRecord(record: PasteDataRecord): void;
        addRecord(mimeType: string, value: ValueType): void;
        getRecordCount(): number;
        getRecord(index: number): PasteDataRecord;
        setProperty(property: PasteDataProperty): void;
        getProperty(): PasteDataProperty;
    }

    export interface SystemPasteboard {
        hasDataType(mimeType: string): boolean;
        setData(data: PasteData, callback: AsyncCallback<void>): void;
        setData(data: PasteData): Promise<void>;
        clearData(callback: AsyncCallback<void>): void;
        clearData(): Promise<void>;
        getDataSync(): PasteData;
        getDataSource(): string;
    }
}

export class PasteDataRecordImpl implements pasteboard.PasteDataRecord {
    mimeType: string = "";
    plainText: string = "";
    uri: string = "";
}

export class PasteDataPropertyImpl implements pasteboard.PasteDataProperty {
    tag: string = "";
    timestamp: number = 0;
    shareOption: pasteboard.ShareOption = pasteboard.ShareOption.CROSSDEVICE;
}

export class PasteDataImpl implements pasteboard.PasteData {
    private nativePtr: long = 0;
    private fzAgent: FinalizationAgent<PasteDataImpl>;
    constructor(nativePtr: long) {
        this.nativePtr = nativePtr;
        this.fzAgent = new FinalizationAgent<PasteDataImpl>(this, this.nativePtr);
    }

    unregisterCleaner(): void {
        this.fzAgent.unregister();
    }

    getNativePtr() : long {
        return this.nativePtr;
    }

    native addRecordByPasteDataRecord(record: pasteboard.PasteDataRecord): void;
    native addRecordByTypeValue(mimeType: string, value: pasteboard.ValueType): void;
    native getRecordCount(): number;
    native getRecord(index: number): PasteDataRecordImpl;
    native setProperty(property: pasteboard.PasteDataProperty): void;
    native getProperty(): pasteboard.PasteDataProperty;

    addRecord(record: pasteboard.PasteDataRecord): void {
        return this.addRecordByPasteDataRecord(record);
    }

    addRecord(mimeType: string, value: pasteboard.ValueType): void {
        return this.addRecordByTypeValue(mimeType, value);
    }
}

export class SystemPasteboardImpl implements pasteboard.SystemPasteboard {
    native hasDataType(mimeType: string): boolean;
    native nativeSetData(data: pasteboard.PasteData): number;
    native nativeClearData(): void;
    native getDataSync(): pasteboard.PasteData;
    native getDataSource(): string;

    setData(data: pasteboard.PasteData, callback: AsyncCallback<void>): void {
        console.println("setData enter")
        let p = taskpool.execute(this.nativeSetData, data)
        p.then((e: Any) => {
            let r = e as number
            console.println("setData return :" + r)
            let err: BusinessError<void>
            callback(err, undefined)
        }).catch((e: Any) => {
            let err = e as BusinessError<void>;
            callback(err, undefined);
        });
    }
    setData(data: pasteboard.PasteData): Promise<void> {
        console.println("promise => setData enter")
        return new Promise<void>((resolve: (v: PromiseLike<void>) => void, reject: (error: Error) => void): void => {
            let p = taskpool.execute(this.nativeSetData, data)
            p.then((e: Any): void => {
                let r = e as number
                console.println("promise => setData return:" + r)
                resolve(Promise.resolve())
            }).catch((e: Error): void => {
                console.log("promise => set failed:", e)
                reject(e)
            })
        })
    }
    clearData(callback: AsyncCallback<void>): void {
        console.println("clearData enter")
        let p = taskpool.execute((): number => {
            this.nativeClearData()
            return 0
        })
        p.then((e: Any) => {
            let r = e as number
            let err: BusinessError<void>
            callback(err, undefined)
        }).catch((e: Any) => {
            let err = e as BusinessError<void>;
            callback(err, undefined);
        });
    }
    clearData(): Promise<void> {
        console.println("promise => clearData enter")
        return new Promise<void>((resolve: (v: PromiseLike<void>) => void, reject: (error: Error) => void): void => {
            let p = taskpool.execute((): number => {
                this.nativeClearData()
                return 0
            })
            p.then((e: Any): void => {
                resolve(Promise.resolve())
            }).catch((e: Error): void => {
                console.log("promise => clear failed:", e)
                reject(e)
            })
        })
    }
}
