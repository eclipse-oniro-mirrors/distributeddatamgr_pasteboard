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

    loadLibraryWithPermissionCheck("pasteboard_ani", "@ohos.pasteboard");

    export const MIMETYPE_TEXT_URI: string = 'text/uri';
    export const MIMETYPE_TEXT_HTML: string = 'text/html';
    export const MIMETYPE_TEXT_PLAIN: string = 'text/plain';
    export const MIMETYPE_PIXELMAP: string = 'pixelMap';
    export const MIMETYPE_TEXT_WANT: string = 'text/want';
    export const MAX_RECORD_NUM: number = 512;
    export type ValueType = string | image.PixelMap | Want | ArrayBuffer;

    export enum Pattern {
        URL,
        NUMBER,
        EMAIL_ADDRESS,
        HTTP_URL,
        FLIGHT_NUMBER
    }

    export enum FileConflictOptions {
        OVERWRITE,
        SKIP,
        RENAME
    }

    export enum ProgressIndicator {
        NONE,
        DEFAULT
    }

    export interface ProgressInfo {
        progress: number;
    }

    export interface ProgressSignal {
        cancel(): void;
    }

    export interface GetDataParams {
        destUri?: string;
        fileConflictOptions?: FileConflictOptions;
        progressIndicator?: ProgressIndicator;
        progressListener?: (progress: ProgressInfo) => void;
        progressSignal?: ProgressSignal;
    }

    export native function createDataTypeValue(mimeType: string, value: ValueType): PasteData;
    export native function createDataRecord(data: Record<string, ValueType>): PasteData;
    export native function getSystemPasteboard(): SystemPasteboard;
    export native function createRecord(mimeType: string, value: ValueType): PasteDataRecord;

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
        htmlText: string;
        want: Want;
        pixelMap: image.PixelMap;
        data: Record<string, ArrayBuffer>;
        toPlainText(): string;
        addEntry(mimeType: string, value: ValueType): void;
        getValidTypes(types: Array<string>): Array<string>;
    }

    export interface PasteData {
        addRecord(record: PasteDataRecord): void;
        addRecord(mimeType: string, value: ValueType): void;
        getRecordCount(): number;
        getRecord(index: number): PasteDataRecord;
        setProperty(property: PasteDataProperty): void;
        getProperty(): PasteDataProperty;
        getMimeTypes(): Array<string>;
        getPrimaryHtml(): string;
        getPrimaryText(): string;
        getPrimaryUri(): string;
        getPrimaryMimeType(): string;
        getPrimaryWant(): Want;
        getPrimaryPixelMap(): image.PixelMap;
        getTag(): string;
        hasType(mimeType: string): boolean;
        removeRecordAt(index: number): void;
        replaceRecordAt(index: number, record: PasteDataRecord): void;
        pasteStart(): void;
        pasteComplete(): void;
    }

    export interface SystemPasteboard {
        hasDataType(mimeType: string): boolean;
        setData(data: PasteData, callback: AsyncCallback<void>): void;
        setData(data: PasteData): Promise<void>;
        clearData(callback: AsyncCallback<void>): void;
        clearData(): Promise<void>;
        getDataSync(): PasteData;
        getDataSource(): string;
        onUpdate(callback: () => void): void;
        offUpdate(callback?: () => void): void;
        onRemoteUpdate(callback: () => void): void;
        offRemoteUpdate(callback?: () => void): void;
        hasData(callback: AsyncCallback<boolean>): void;
        hasData(): Promise<boolean>;
        hasDataSync(): boolean;
        hasRemoteData(): boolean;
        isRemoteData(): boolean;
        clearDataSync(): void;
        setDataSync(data: PasteData): void;
        getChangeCount(): number;
        getMimeTypes(callback: AsyncCallback<Array<string>>): void;
        getMimeTypes(): Promise<Array<string>>;
        detectPatterns(patterns: Array<Pattern>): Promise<Array<Pattern>>;
        setAppShareOptions(shareOptions: ShareOption): void;
        removeAppShareOptions(): void;
        getDataWithProgress(params: GetDataParams): Promise<PasteData>;
    }
}

export class PasteDataRecordImpl implements pasteboard.PasteDataRecord {
    mimeType: string = "";
    plainText: string = "";
    uri: string = "";
    htmlText: string = "";
    want: Want | null = null;
    pixelMap: image.PixelMap | null = null;
    data: Record<string, ArrayBuffer> = {};

    toPlainText(): string {
        return this.plainText;
    }

    addEntry(mimeType: string, value: pasteboard.ValueType): void {
        console.println("PasteDataRecordImpl addEntry");
    }

    getValidTypes(types: Array<string>): Array<string> {
        let validTypes: Array<string> = [];
        for (let type of types) {
            if (type.length > 0) {
                validTypes.push(type);
            }
        }
        return validTypes;
    }
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
    native getMimeTypes(): Array<string>;
    native getPrimaryHtml(): string;
    native getPrimaryText(): string;
    native getPrimaryUri(): string;
    native getPrimaryMimeType(): string;
    native getPrimaryWant(): Want;
    native getPrimaryPixelMap(): image.PixelMap;
    native getTag(): string;
    native hasType(mimeType: string): boolean;
    native removeRecordAt(index: number): void;
    native replaceRecordAt(index: number, record: pasteboard.PasteDataRecord): void;
    native pasteStart(): void;
    native pasteComplete(): void;

    addRecord(record: pasteboard.PasteDataRecord): void {
        return this.addRecordByPasteDataRecord(record);
    }

    addRecord(mimeType: string, value: pasteboard.ValueType): void {
        return this.addRecordByTypeValue(mimeType, value);
    }

    getMimeTypesImpl(): Array<string> {
        return this.getMimeTypes();
    }

    getPrimaryHtmlImpl(): string {
        return this.getPrimaryHtml();
    }

    getPrimaryTextImpl(): string {
        return this.getPrimaryText();
    }

    getPrimaryUriImpl(): string {
        return this.getPrimaryUri();
    }

    getPrimaryMimeTypeImpl(): string {
        return this.getPrimaryMimeType();
    }

    getPrimaryWantImpl(): Want {
        return this.getPrimaryWant();
    }

    getPrimaryPixelMapImpl(): image.PixelMap {
        return this.getPrimaryPixelMap();
    }

    getTagImpl(): string {
        return this.getTag();
    }

    hasTypeImpl(mimeType: string): boolean {
        return this.hasType(mimeType);
    }

    removeRecordAtImpl(index: number): void {
        return this.removeRecordAt(index);
    }

    replaceRecordAtImpl(index: number, record: pasteboard.PasteDataRecord): void {
        return this.replaceRecordAt(index, record);
    }
}

export class SystemPasteboardImpl implements pasteboard.SystemPasteboard {
    native hasDataType(mimeType: string): boolean;
    native nativeSetData(data: pasteboard.PasteData): number;
    native nativeClearData(): void;
    native getDataSync(): pasteboard.PasteData;
    native getDataSource(): string;
    native onUpdate(callback: () => void): void;
    native offUpdate(callback: () => void): void;
    native onRemoteUpdate(callback: () => void): void;
    native offRemoteUpdate(callback: () => void): void;
    native hasData(): boolean;
    native hasDataSync(): boolean;
    native hasRemoteData(): boolean;
    native isRemoteData(): boolean;
    native clearDataSync(): void;
    native setDataSync(data: pasteboard.PasteData): void;
    native getChangeCount(): number;
    native getMimeTypes(): Array<string>;
    native detectPatterns(patterns: Array<pasteboard.Pattern>): Array<pasteboard.Pattern>;
    native setAppShareOptions(shareOptions: pasteboard.ShareOption): void;
    native removeAppShareOptions(): void;
    native getDataWithProgress(params: pasteboard.GetDataParams): pasteboard.PasteData;

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

    hasDataPromise(): Promise<boolean> {
        console.println("promise => hasData enter")
        return new Promise<boolean>((resolve: (v: PromiseLike<boolean>) => void,
            reject: (error: Error) => void): void => {
            let p = taskpool.execute((): boolean => {
                return this.hasDataSync()
            })
            p.then((e: Any): void => {
                let r = e as boolean
                console.println("promise => hasData return:" + r)
                resolve(Promise.resolve(r))
            }).catch((e: Error): void => {
                console.log("promise => hasData failed:", e)
                reject(e)
            })
        })
    }

    getMimeTypesPromise(): Promise<Array<string>> {
        console.println("promise => getMimeTypes enter")
        return new Promise<Array<string>>((resolve: (v: PromiseLike<Array<string>>) => void,
            reject: (error: Error) => void): void => {
            let p = taskpool.execute((): Array<string> => {
                return this.getMimeTypes()
            })
            p.then((e: Any): void => {
                let r = e as Array<string>
                console.println("promise => getMimeTypes return:" + r.length)
                resolve(Promise.resolve(r))
            }).catch((e: Error): void => {
                console.log("promise => getMimeTypes failed:", e)
                reject(e)
            })
        })
    }

    detectPatternsPromise(patterns: Array<pasteboard.Pattern>): Promise<Array<pasteboard.Pattern>> {
        console.println("promise => detectPatterns enter")
        return new Promise<Array<pasteboard.Pattern>>((resolve: (v: PromiseLike<Array<pasteboard.Pattern>>) => void,
            reject: (error: Error) => void): void => {
            let p = taskpool.execute((): Array<pasteboard.Pattern> => {
                return this.detectPatterns(patterns)
            })
            p.then((e: Any): void => {
                let r = e as Array<pasteboard.Pattern>
                console.println("promise => detectPatterns return:" + r.length)
                resolve(Promise.resolve(r))
            }).catch((e: Error): void => {
                console.log("promise => detectPatterns failed:", e)
                reject(e)
            })
        })
    }

    getDataWithProgressPromise(params: pasteboard.GetDataParams): Promise<pasteboard.PasteData> {
        console.println("promise => getDataWithProgress enter")
        return new Promise<pasteboard.PasteData>((resolve: (v: PromiseLike<pasteboard.PasteData>) => void,
            reject: (error: Error) => void): void => {
            let p = taskpool.execute((): pasteboard.PasteData => {
                return this.getDataWithProgress(params)
            })
            p.then((e: Any): void => {
                let r = e as pasteboard.PasteData
                console.println("promise => getDataWithProgress return")
                resolve(Promise.resolve(r))
            }).catch((e: Error): void => {
                console.log("promise => getDataWithProgress failed:", e)
                reject(e)
            })
        })
    }
}
