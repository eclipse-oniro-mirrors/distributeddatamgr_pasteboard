/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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
// @ts-nocheck
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index';
import pasteboard from '@ohos.pasteboard';
import image from '@ohos.multimedia.image';
import Want from '@ohos.app.ability.Want';
import List from '@ohos.util.List';

const myType = 'my-mime-type';
const allTypes = [pasteboard.MIMETYPE_TEXT_PLAIN, pasteboard.MIMETYPE_TEXT_HTML, pasteboard.MIMETYPE_TEXT_URI,
  pasteboard.MIMETYPE_TEXT_WANT, pasteboard.MIMETYPE_PIXELMAP, myType
];

describe('PasteBoardJSTest', function () {
  beforeAll(async function () {
    console.info('beforeAll');
  });

  afterAll(async function () {
    console.info('afterAll');
  });

  /**
   * @tc.name      pasteboard_promise_test1
   * @tc.desc      Adds PlainTextData
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test1', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData1 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData1);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    const types = pasteData.getMimeTypes();
    expect('text/plain').assertEqual(types[0]);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData1 = data;
      expect(pasteData1.getRecordCount()).assertEqual(1);
      const primaryText = pasteData1.getPrimaryText();
      expect(primaryText).assertEqual(textData1);
      expect(pasteboard.MAX_RECORD_NUM).assertEqual(512);
      expect(pasteData1.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_PLAIN);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test2
   * @tc.desc      Adds PlainTextData = ''
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test2', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData2 = '';
    const pasteData = pasteboard.createPlainTextData(textData2);
    await systemPasteboard.setPasteData(pasteData);
    const res2 = await systemPasteboard.hasPasteData();
    expect(res2).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const recordCount2 = data.getRecordCount();
      expect(recordCount2).assertEqual(1);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test3
   * @tc.desc      Adds PlainTextData = 'Hello 中国!@#$%^&*()_+{}\?.'
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test3', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData3 = 'Hello 中国!@#$%^&*()_+{}?.';
    const pasteData = pasteboard.createPlainTextData(textData3);
    await systemPasteboard.setPasteData(pasteData);
    const res3 = await systemPasteboard.hasPasteData();
    expect(res3).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData3 = data;
      expect(pasteData3.getRecordCount()).assertEqual(1);
      const primaryText = pasteData3.getPrimaryText();
      expect(primaryText).assertEqual(textData3);
      expect(pasteData3.hasMimeType(pasteboard.MIMETYPE_TEXT_PLAIN)).assertEqual(true);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test4
   * @tc.desc      Adds 300K PlainTextData
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test4', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    let textData4 = '';
    for (let i = 0; i < 300; i++) {
      textData4 = textData4 + 'A';
    }
    const pasteData = pasteboard.createPlainTextData(textData4);
    await systemPasteboard.setPasteData(pasteData);
    const res4 = await systemPasteboard.hasPasteData();
    expect(res4).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData4 = data;
      expect(pasteData4.getRecordCount()).assertEqual(1);
      const primaryText = pasteData4.getPrimaryText();
      expect(primaryText).assertEqual(textData4);
      expect(pasteData4.hasMimeType(pasteboard.MIMETYPE_TEXT_PLAIN)).assertEqual(true);
      expect(pasteData4.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_PLAIN);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test5
   * @tc.desc      Adds htmlText
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test5', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText5 = '<html><head></head><body>Hello World!</body></html>';
    const pasteData = pasteboard.createHtmlData(htmlText5);
    await systemPasteboard.setPasteData(pasteData);
    const res5 = await systemPasteboard.hasPasteData();
    expect(res5).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData5 = data;
      expect(pasteData5.getRecordCount()).assertEqual(1);
      const primaryHtml6 = pasteData5.getPrimaryHtml();
      expect(primaryHtml6).assertEqual(htmlText5);
      expect(pasteData5.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
      expect(pasteData5.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test6
   * @tc.desc      Adds htmlText = ''
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test6', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText6 = '';
    const pasteData = pasteboard.createHtmlData(htmlText6);
    await systemPasteboard.setPasteData(pasteData);
    const res6 = await systemPasteboard.hasPasteData();
    expect(res6).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      const primaryHtml6 = data.getPrimaryHtml();
      expect(primaryHtml6).assertEqual(htmlText6);
      expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
      expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test7
   * @tc.desc      Adds htmlText = 'Hello 中国!@#$%^&*()_+{}\?.'
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test7', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText = 'Hello 中国!@#$%^&*()_+{}?.';
    const pasteData = pasteboard.createHtmlData(htmlText);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
      expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
      expect(data.getPrimaryHtml()).assertEqual(htmlText);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test8
   * @tc.desc      Adds uriText
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test8', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText8 = 'https://www.baidu.com/';
    const pasteData = pasteboard.createUriData(uriText8);
    await systemPasteboard.setPasteData(pasteData);
    const res8 = await systemPasteboard.hasPasteData();
    expect(res8).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      const primaryUri8 = data.getPrimaryUri();
      expect(primaryUri8).assertEqual(uriText8);
      expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_URI)).assertEqual(true);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test9
   * @tc.desc      Adds uriText = ''
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test9', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText9 = '';
    const pasteData = pasteboard.createUriData(uriText9);
    await systemPasteboard.setPasteData(pasteData);
    const res9 = await systemPasteboard.hasPasteData();
    expect(res9).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      expect(data.getPrimaryUri()).assertEqual(uriText9);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test10
   * @tc.desc      Set uriText = 'Hello //'
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test10', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText10 = 'Hello//';
    const pasteData = pasteboard.createUriData(uriText10);
    await systemPasteboard.setPasteData(pasteData);
    const res10 = await systemPasteboard.hasPasteData();
    expect(res10).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_URI)).assertEqual(true);
      expect(data.getPrimaryUri()).assertEqual(uriText10);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test11
   * @tc.desc      Adds want
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test11', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const want11 = {
      bundleName: 'com.example.myapplication8',
      abilityName: 'com.example.myapplication8.MainAbility',
    };
    const pasteData = pasteboard.createWantData(want11);
    await systemPasteboard.setPasteData(pasteData);
    const res11 = await systemPasteboard.hasPasteData();
    expect(res11).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      const primaryWant = data.getPrimaryWant();
      expect(want11.bundleName).assertEqual(primaryWant.bundleName);
      expect(want11.abilityName).assertEqual(primaryWant.abilityName);
      expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_WANT)).assertEqual(true);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test12
   * @tc.desc      Adds one record(s)
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test12', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData12 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData12);
    await systemPasteboard.setPasteData(pasteData);
    const res12 = await systemPasteboard.hasPasteData();
    expect(res12).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      let recordText12 = data.getRecordAt(0).plainText;
      expect(recordText12).assertEqual(textData12);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test13
   * @tc.desc      Adds 2 record(s)
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test13', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData130 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData130);
    const textData131 = 'Hello World1';
    pasteData.addTextRecord(textData131);
    await systemPasteboard.setPasteData(pasteData);
    const res13 = await systemPasteboard.hasPasteData();
    expect(res13).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(2);
      const dataRecord130 = data.getRecordAt(0);
      const dataRecord131 = data.getRecordAt(1);
      expect(dataRecord130.plainText).assertEqual(textData131);
      expect(dataRecord131.plainText).assertEqual(textData130);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test14
   * @tc.desc      Adds 15 record(s)
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test14', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData140 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData140);
    let textData14 = '';
    for (let i = 1; i < 15; i++) {
      textData14 = 'Hello World';
      textData14 = textData14 + i;
      pasteData.addTextRecord(textData14);
    }
    await systemPasteboard.setPasteData(pasteData);
    const res14 = await systemPasteboard.hasPasteData();
    expect(res14).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(15);
      const dataRecord14 = data.getRecordAt(14);
      expect(dataRecord14.plainText).assertEqual(textData140);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test15
   * @tc.desc      Adds 30 record(s)
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test15', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData150 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData150);
    let textData15 = '';
    for (let i = 1; i < 30; i++) {
      textData15 = 'Hello World';
      textData15 = textData15 + i;
      pasteData.addTextRecord(textData15);
    }
    await systemPasteboard.setPasteData(pasteData);
    const res15 = await systemPasteboard.hasPasteData();
    expect(res15).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(30);
      const dataRecord15 = data.getRecordAt(0);
      expect(dataRecord15.plainText).assertEqual('Hello World29');
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test16
   * @tc.desc      Adds 31 record(s)
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test16', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData160 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData160);
    let textData16 = '';
    for (let i = 1; i < 31; i++) {
      textData16 = 'Hello World';
      textData16 = textData16 + i;
      pasteData.addTextRecord(textData16);
    }
    await systemPasteboard.setPasteData(pasteData);
    const res16 = await systemPasteboard.hasPasteData();
    expect(res16).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const recordCount16 = data.getRecordCount();
      expect(recordCount16).assertEqual(31);
      const dataRecord16 = data.getRecordAt(0);
      expect(dataRecord16.plainText).assertEqual('Hello World30');
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test17
   * @tc.desc      Adds PlainText,HtmlText,UriText
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test17', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData17 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData17);
    const htmlText17 = '<html><head></head><body>Hello World!</body></html>';
    pasteData.addHtmlRecord(htmlText17);
    const uriText17 = 'https://www.baidu.com/';
    pasteData.addUriRecord(uriText17);
    const want17 = {
      bundleName: 'com.example.myapplication8',
      abilityName: 'com.example.myapplication8.MainAbility',
    };
    pasteData.addWantRecord(want17);
    await systemPasteboard.setPasteData(pasteData);
    const res17 = await systemPasteboard.hasPasteData();
    expect(res17).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(4);
      const wantRecord17 = data.getPrimaryWant();
      expect(wantRecord17.bundleName).assertEqual(want17.bundleName);
      expect(wantRecord17.abilityName).assertEqual(want17.abilityName);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test18
   * @tc.desc      Delete one PlainTextData
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test18', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData18 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData18);
    await systemPasteboard.setPasteData(pasteData);
    const res18 = await systemPasteboard.hasPasteData();
    expect(res18).assertEqual(true);
    const pasteData18 = await systemPasteboard.getPasteData();
    expect(pasteData18.getRecordCount()).assertEqual(1);
    expect(pasteData18.removeRecordAt(0)).assertEqual(true);
    await systemPasteboard.setPasteData(pasteData18);
    systemPasteboard.getPasteData().then((data) => {
      const recordCount18 = data.getRecordCount();
      expect(recordCount18).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test19
   * @tc.desc      Delete one htmlText
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test19', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText19 = '<html><head></head><body>Hello World!</body></html>';
    const pasteData = pasteboard.createHtmlData(htmlText19);
    await systemPasteboard.setPasteData(pasteData);
    const res19 = await systemPasteboard.hasPasteData();
    expect(res19).assertEqual(true);
    const pasteData19 = await systemPasteboard.getPasteData();
    expect(pasteData19.getRecordCount()).assertEqual(1);
    expect(pasteData19.removeRecordAt(0)).assertEqual(true);
    await systemPasteboard.setPasteData(pasteData19);
    systemPasteboard.getPasteData().then((data) => {
      const recordCount19 = data.getRecordCount();
      expect(recordCount19).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test20
   * @tc.desc      Delete one uriText
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test20', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText20 = 'https://www.baidu.com/';
    const pasteData = pasteboard.createUriData(uriText20);
    await systemPasteboard.setPasteData(pasteData);
    const res20 = await systemPasteboard.hasPasteData();
    expect(res20).assertEqual(true);
    const pasteData20 = await systemPasteboard.getPasteData();
    expect(pasteData20.getRecordCount()).assertEqual(1);
    expect(pasteData20.removeRecordAt(0)).assertEqual(true);
    await systemPasteboard.setPasteData(pasteData20);
    systemPasteboard.getPasteData().then((data) => {
      const recordCount20 = data.getRecordCount();
      expect(recordCount20).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test21
   * @tc.desc      Delete one want
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test21', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const want21 = {
      bundleName: 'com.example.myapplication8',
      abilityName: 'com.example.myapplication8.MainAbility',
    };
    const pasteData = pasteboard.createWantData(want21);
    await systemPasteboard.setPasteData(pasteData);
    const res21 = await systemPasteboard.hasPasteData();
    expect(res21).assertEqual(true);
    const pasteData21 = await systemPasteboard.getPasteData();
    expect(pasteData21.getRecordCount()).assertEqual(1);
    expect(pasteData21.removeRecordAt(0)).assertEqual(true);
    await systemPasteboard.setPasteData(pasteData21);
    systemPasteboard.getPasteData().then((data) => {
      const recordCount21 = data.getRecordCount();
      expect(recordCount21).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test22
   * @tc.desc      Deletes 300K PlainTextData
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test22', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    let textData22 = '';
    for (let i = 0; i < 300; i++) {
      textData22 = textData22 + 'A';
    }
    const pasteData = pasteboard.createPlainTextData(textData22);
    await systemPasteboard.setPasteData(pasteData);
    const res22 = await systemPasteboard.hasPasteData();
    expect(res22).assertEqual(true);
    const pasteData22 = await systemPasteboard.getPasteData();
    expect(pasteData22.getRecordCount()).assertEqual(1);
    expect(pasteData22.removeRecordAt(0)).assertEqual(true);
    await systemPasteboard.setPasteData(pasteData22);
    systemPasteboard.getPasteData().then((data) => {
      const recordCount22 = data.getRecordCount();
      expect(recordCount22).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test23
   * @tc.desc      Deletes 30 record(s)
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test23', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData230 = 'Hello World';
    const pasteData = pasteboard.createPlainTextData(textData230);
    let textData23 = '';
    for (let i = 1; i < 30; i++) {
      textData23 = 'Hello World';
      textData23 = textData23 + i;
      pasteData.addTextRecord(textData23);
    }
    await systemPasteboard.setPasteData(pasteData);
    const res23 = await systemPasteboard.hasPasteData();
    expect(res23).assertEqual(true);
    const pasteData23 = await systemPasteboard.getPasteData();
    expect(pasteData23.getRecordCount()).assertEqual(30);
    for (let i = 0; i < 30; i++) {
      expect(pasteData23.removeRecordAt(0)).assertEqual(true);
    }
    expect(pasteData23.getRecordCount()).assertEqual(0);
    systemPasteboard.setPasteData(pasteData23).then(() => {
      systemPasteboard.getPasteData().then((data) => {
        let recordCount23 = data.getRecordCount();
        expect(recordCount23).assertEqual(0);
        done();
      });
    });
  });

  /**
   * @tc.name      pasteboard_promise_test24
   * @tc.desc      Deletes replaced record
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test24', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData24 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData24);
    await systemPasteboard.setPasteData(pasteData);
    const res24 = await systemPasteboard.hasPasteData();
    expect(res24).assertEqual(true);
    const pasteData24 = await systemPasteboard.getPasteData();
    expect(pasteData24.getRecordCount()).assertEqual(1);
    const textData241 = 'Hello World1';
    const pasteDataRecord = pasteboard.createPlainTextRecord(textData241);
    const replace = pasteData24.replaceRecordAt(0, pasteDataRecord);
    expect(replace).assertEqual(true);
    const primaryText = pasteData24.getPrimaryText();
    expect(primaryText).assertEqual(textData241);
    expect(pasteData24.hasMimeType(pasteboard.MIMETYPE_TEXT_PLAIN)).assertEqual(true);
    const dataRecord = pasteData24.getRecordAt(0);
    expect(dataRecord.plainText).assertEqual(textData241);
    expect(pasteData24.removeRecordAt(0)).assertEqual(true);
    systemPasteboard.setPasteData(pasteData24).then(() => {
      systemPasteboard.getPasteData().then((data) => {
        const recordCount24 = data.getRecordCount();
        expect(recordCount24).assertEqual(0);
        done();
      });
    });
  });

  /**
   * @tc.name      pasteboard_promise_test25
   * @tc.desc      Deletes 文本、uri、html
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test25', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData25 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData25);
    const htmlText25 = '<html><head></head><body>Hello World!</body></html>';
    pasteData.addHtmlRecord(htmlText25);
    const uriText25 = 'https://www.baidu.com/';
    pasteData.addUriRecord(uriText25);
    await systemPasteboard.setPasteData(pasteData);
    const res25 = await systemPasteboard.hasPasteData();
    expect(res25).assertEqual(true);
    const pasteData25 = await systemPasteboard.getPasteData();
    expect(pasteData25.getRecordCount()).assertEqual(3);
    expect(pasteData25.removeRecordAt(0)).assertEqual(true);
    expect(pasteData25.removeRecordAt(0)).assertEqual(true);
    expect(pasteData25.removeRecordAt(0)).assertEqual(true);
    systemPasteboard.setPasteData(pasteData25).then(() => {
      systemPasteboard.getPasteData().then((data) => {
        const recordCount25 = data.getRecordCount();
        expect(recordCount25).assertEqual(0);
        done();
      });
    });
  });

  /**
   * @tc.name      pasteboard_promise_test26
   * @tc.desc      Replaces 文本 record
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test26', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData26 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData26);
    await systemPasteboard.setPasteData(pasteData);
    const res26 = await systemPasteboard.hasPasteData();
    expect(res26).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData26 = data;
      expect(pasteData26.getRecordCount()).assertEqual(1);
      const textData261 = 'Hello World1';
      const pasteDataRecord = pasteboard.createPlainTextRecord(textData261);
      const replace26 = pasteData26.replaceRecordAt(0, pasteDataRecord);
      expect(replace26).assertEqual(true);
      const primaryText26 = pasteData26.getPrimaryText();
      expect(primaryText26).assertEqual(textData261);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test27
   * @tc.desc      Replaces htmlText record
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test27', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText27 = '<html><head></head><body>Hello World!</body></html>';
    const pasteData = pasteboard.createHtmlData(htmlText27);
    await systemPasteboard.setPasteData(pasteData);
    const res27 = await systemPasteboard.hasPasteData();
    expect(res27).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData27 = data;
      expect(pasteData27.getRecordCount()).assertEqual(1);
      const htmlText27 = '<html><head></head><body>Hello World 1</body></html>';
      const pasteDataRecord = pasteboard.createHtmlTextRecord(htmlText27);
      const replace27 = pasteData27.replaceRecordAt(0, pasteDataRecord);
      expect(replace27).assertEqual(true);
      const primaryHtml271 = pasteData27.getPrimaryHtml();
      expect(primaryHtml271).assertEqual(htmlText27);
      expect(pasteData27.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
      const primaryHtml272 = pasteData27.getPrimaryHtml();
      expect(primaryHtml272).assertEqual(htmlText27);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test28
   * @tc.desc      Replaces uri record
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test28', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText28 = 'https://www.baidu.com/';
    const pasteData = pasteboard.createUriData(uriText28);
    await systemPasteboard.setPasteData(pasteData);
    const res28 = await systemPasteboard.hasPasteData();
    expect(res28).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData28 = data;
      expect(pasteData28.getRecordCount()).assertEqual(1);
      const uriText281 = 'https://www.baidu.com/1';
      const pasteDataRecord = pasteboard.createUriRecord(uriText281);
      const replace28 = pasteData28.replaceRecordAt(0, pasteDataRecord);
      expect(replace28).assertEqual(true);
      const primaryUri1 = pasteData28.getPrimaryUri();
      expect(primaryUri1).assertEqual(uriText281);
      expect(pasteData28.hasMimeType(pasteboard.MIMETYPE_TEXT_URI)).assertEqual(true);
      const primaryUri2 = pasteData28.getPrimaryUri();
      expect(primaryUri2).assertEqual(uriText281);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test29
   * @tc.desc      Replaces want record
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test29', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const wantText29 = {
      bundleName: 'com.example.myapplication3',
      abilityName: 'com.example.myapplication3.MainAbility',
    };
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_WANT, wantText29);
    await systemPasteboard.setPasteData(pasteData);
    const res29 = await systemPasteboard.hasPasteData();
    expect(res29).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData29 = data;
      expect(pasteData29.getRecordCount()).assertEqual(1);
      const wantText291 = {
        bundleName: 'com.example.myapplication30',
        abilityName: 'com.example.myapplication30.MainAbility',
      };
      const pasteDataRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_WANT, wantText291);
      const replace29 = pasteData29.replaceRecordAt(0, pasteDataRecord);
      expect(replace29).assertEqual(true);
      const primaryWant29 = pasteData29.getPrimaryWant();
      expect(primaryWant29.bundleName).assertEqual(wantText291.bundleName);
      expect(primaryWant29.abilityName).assertEqual(wantText291.abilityName);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test30
   * @tc.desc      Replaces 300k文本 record
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test30', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    let textData30 = '';
    for (let i = 0; i < 300; i++) {
      textData30 = textData30 + 'A';
    }
    const pasteData = pasteboard.createPlainTextData(textData30);
    await systemPasteboard.setPasteData(pasteData);
    const res30 = await systemPasteboard.hasPasteData();
    expect(res30).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData30 = data;
      expect(pasteData30.getRecordCount()).assertEqual(1);
      const textData301 = 'Hello World1';
      const pasteDataRecord = pasteboard.createPlainTextRecord(textData301);
      const replace = pasteData30.replaceRecordAt(0, pasteDataRecord);
      expect(replace).assertEqual(true);
      const primaryText = pasteData30.getPrimaryText();
      expect(primaryText).assertEqual(textData301);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test31
   * @tc.desc      Clears pasteBoard, gets record count
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test31', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData31 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData31);
    await systemPasteboard.setPasteData(pasteData);
    const res31 = await systemPasteboard.hasPasteData();
    expect(res31).assertEqual(true);
    const pasteData31 = await systemPasteboard.getPasteData();
    expect(pasteData31.getRecordCount()).assertEqual(1);
    await systemPasteboard.clearData();
    systemPasteboard.getPasteData().then((data) => {
      const pasteData2 = data;
      const recordCount = pasteData2.getRecordCount();
      expect(recordCount).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test32
   * @tc.desc      Adds Property
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test32', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData32 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData32);
    await systemPasteboard.setPasteData(pasteData);
    const res32 = await systemPasteboard.hasPasteData();
    expect(res32).assertEqual(true);
    const pasteData32 = await systemPasteboard.getPasteData();
    expect(pasteData32.getRecordCount()).assertEqual(1);
    const pasteDataProperty = pasteData32.getProperty();
    expect(pasteDataProperty.shareOption).assertEqual(pasteboard.ShareOption.CrossDevice);
    pasteDataProperty.shareOption = pasteboard.ShareOption.InApp;
    pasteData32.setProperty(pasteDataProperty);
    expect(pasteData32.getProperty().shareOption).assertEqual(pasteboard.ShareOption.InApp);
    done();
  });

  /**
   * @tc.name      pasteboard_promise_test33
   * @tc.desc      Clears pasteBoard and check property
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test33', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData33 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData33);
    await systemPasteboard.setPasteData(pasteData);
    const res33 = await systemPasteboard.hasPasteData();
    expect(res33).assertEqual(true);
    systemPasteboard.getPasteData().then(async (data) => {
      expect(data.getRecordCount()).assertEqual(1);
      await systemPasteboard.clearData();
      const newPasteData = await systemPasteboard.getPasteData();
      expect(newPasteData.getProperty().shareOption).assertEqual(pasteboard.ShareOption.CrossDevice);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test34
   * @tc.desc      打开内容变化通知功能
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test34', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    systemPasteboard.on('update', contentChanges);
    const textData34 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData34);
    await systemPasteboard.setPasteData(pasteData);
    const res34 = await systemPasteboard.hasPasteData();
    expect(res34).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      expect(data.removeRecordAt(0)).assertEqual(true);
      expect(data.getRecordCount()).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test35
   * @tc.desc      清除剪切板内的文本数据项
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test35', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData35 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData35);
    await systemPasteboard.setPasteData(pasteData);
    const res35 = await systemPasteboard.hasPasteData();
    expect(res35).assertEqual(true);
    const pasteData35 = await systemPasteboard.getPasteData();
    expect(pasteData35.getRecordCount()).assertEqual(1);
    await systemPasteboard.clearData();
    systemPasteboard.getPasteData().then((data) => {
      const recordCount35 = data.getRecordCount();
      expect(recordCount35).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test36
   * @tc.desc      清除剪切板内的uri数据项
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test36', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText36 = 'https://www.baidu.com/';
    const pasteData = pasteboard.createUriData(uriText36);
    await systemPasteboard.setPasteData(pasteData);
    const res36 = await systemPasteboard.hasPasteData();
    expect(res36).assertEqual(true);
    const pasteData36 = await systemPasteboard.getPasteData();
    expect(pasteData36.getRecordCount()).assertEqual(1);
    await systemPasteboard.clearData();
    systemPasteboard.getPasteData().then((data) => {
      const recordCount36 = data.getRecordCount();
      expect(recordCount36).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test37
   * @tc.desc      清除剪切板内的html数据项
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test37', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText37 = '<html><head></head><body>Hello World!</body></html>';
    const pasteData = pasteboard.createHtmlData(htmlText37);
    await systemPasteboard.setPasteData(pasteData);
    const res37 = await systemPasteboard.hasPasteData();
    expect(res37).assertEqual(true);
    const pasteData37 = await systemPasteboard.getPasteData();
    expect(pasteData37.getRecordCount()).assertEqual(1);
    await systemPasteboard.clearData();
    systemPasteboard.getPasteData().then((data) => {
      const recordCount37 = data.getRecordCount();
      expect(recordCount37).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test38
   * @tc.desc      清除剪切板内的want数据项
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test38', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const myWant38 = {
      bundleName: 'com.example.myapplication55',
      abilityName: 'com.example.myapplication55.MainAbility',
    };
    const pasteData381 = pasteboard.createWantData(myWant38);
    await systemPasteboard.setPasteData(pasteData381);
    const res38 = await systemPasteboard.hasPasteData();
    expect(res38).assertEqual(true);
    const pasteData38 = await systemPasteboard.getPasteData();
    expect(pasteData38.getRecordCount()).assertEqual(1);
    await systemPasteboard.clearData();
    systemPasteboard.getPasteData().then((data) => {
      const recordCount38 = data.getRecordCount();
      expect(recordCount38).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test39
   * @tc.desc      向剪切板内增加30条数据项，然后清除
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test39', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData390 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData390);
    let textData39 = '';
    for (let i = 1; i < 30; i++) {
      textData39 = 'Hello World';
      textData39 = textData39 + i;
      pasteData.addTextRecord(textData39);
    }
    await systemPasteboard.setPasteData(pasteData);
    const res39 = await systemPasteboard.hasPasteData();
    expect(res39).assertEqual(true);
    const pasteData39 = await systemPasteboard.getPasteData();
    expect(pasteData39.getRecordCount()).assertEqual(30);
    await systemPasteboard.clearData();
    systemPasteboard.getPasteData().then((data) => {
      const recordCount39 = data.getRecordCount();
      expect(recordCount39).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test40
   * @tc.desc      向剪贴板数据各增加5条文本、uri、html数据，然后清除
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test40', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData400 = 'Hello World0';
    const pasteData = pasteboard.createPlainTextData(textData400);
    let textData40 = '';
    for (let i = 1; i < 5; i++) {
      textData40 = 'Hello World';
      textData40 = textData40 + i;
      pasteData.addTextRecord(textData40);
    }
    let htmlText40 = '';
    for (let i = 0; i < 5; i++) {
      htmlText40 = '<html><head></head><body>Hello World!</body></html>';
      htmlText40 = htmlText40 + i;
      pasteData.addHtmlRecord(htmlText40);
    }
    let uriText40 = '';
    for (let i = 0; i < 5; i++) {
      uriText40 = 'https://www.baidu.com/';
      uriText40 = uriText40 + i;
      pasteData.addUriRecord(uriText40);
    }
    await systemPasteboard.setPasteData(pasteData);
    const res40 = await systemPasteboard.hasPasteData();
    expect(res40).assertEqual(true);
    const data40 = await systemPasteboard.getPasteData();
    expect(data40.getRecordCount()).assertEqual(15);
    await systemPasteboard.clearData();
    systemPasteboard.getPasteData().then((data) => {
      const recordCount40 = data.getRecordCount();
      expect(recordCount40).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test41
   * @tc.desc      更新剪贴板数据，查询剪贴板存在剪贴板数据
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test41', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData41 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData41);
    await systemPasteboard.setPasteData(pasteData);
    const res41 = await systemPasteboard.hasPasteData();
    expect(res41).assertEqual(true);
    const pasteData41 = await systemPasteboard.getPasteData();
    expect(pasteData41.getRecordCount()).assertEqual(1);
    const textData411 = 'Hello World1';
    const pasteDataRecord = pasteboard.createPlainTextRecord(textData411);
    const replace41 = pasteData41.replaceRecordAt(0, pasteDataRecord);
    expect(replace41).assertEqual(true);
    await systemPasteboard.setPasteData(pasteData41);
    systemPasteboard.hasPasteData().then(async (data) => {
      expect(data).assertEqual(true);
      const newData41 = await systemPasteboard.getPasteData();
      expect(newData41.getPrimaryText()).assertEqual(textData411);
      const newPasteDataRecord = newData41.getRecordAt(0);
      expect(newPasteDataRecord.plainText).assertEqual(textData411);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test42
   * @tc.desc      删除所有的剪贴板数据，查询剪贴板不存在剪贴板数据
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test42', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData42 = 'Hello World!';
    const data42 = pasteboard.createPlainTextData(textData42);
    await systemPasteboard.setPasteData(data42);
    const res42 = await systemPasteboard.hasPasteData();
    expect(res42).assertEqual(true);
    const pasteData42 = await systemPasteboard.getPasteData();
    const recordCount = pasteData42.getRecordCount();
    expect(recordCount).assertEqual(1);
    expect(pasteData42.removeRecordAt(0)).assertEqual(true);
    expect(pasteData42.getRecordCount()).assertEqual(0);
    const newData42 = await systemPasteboard.getPasteData();
    expect(newData42.getRecordCount()).assertEqual(1);
    done();
  });

  /**
   * @tc.name      pasteboard_promise_test43
   * @tc.desc      将文本数据强制转换为文本
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test43', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData43 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData43);
    await systemPasteboard.setPasteData(pasteData);
    const res43 = await systemPasteboard.hasPasteData();
    expect(res43).assertEqual(true);
    systemPasteboard.getPasteData().then(async (data) => {
      const pasteData143 = data;
      expect(pasteData143.getRecordCount()).assertEqual(1);
      const pasteDataRecord = pasteData143.getRecordAt(0);
      const text43 = await pasteDataRecord.convertToText();
      expect(text43).assertEqual(textData43);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test44
   * @tc.desc      将一条含有特殊字符、中英混杂的文本数据强制转换为文本
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test44', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData44 = 'Hello 中国!@#$%^&*()_+{}?.';
    const pasteData = pasteboard.createPlainTextData(textData44);
    await systemPasteboard.setPasteData(pasteData);
    const res44 = await systemPasteboard.hasPasteData();
    expect(res44).assertEqual(true);
    const pasteData44 = await systemPasteboard.getPasteData();
    expect(pasteData44.getRecordCount()).assertEqual(1);
    const pasteDataRecord = pasteData44.getRecordAt(0);
    pasteDataRecord.convertToText((err, text) => {
      if (err) {
        console.info('f_test44 pasteDataRecord.convertToText error: ' + error);
      } else {
        expect(textData44).assertEqual(text);
        done();
      }
    });
  });

  /**
   * @tc.name      pasteboard_promise_test45
   * @tc.desc      将一条超长文本数据 (大小为301K)强制转换为文本
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test45', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    let textData45 = '';
    for (let i = 0; i < 301; i++) {
      textData45 = textData45 + 'A';
    }
    const pasteData = pasteboard.createPlainTextData(textData45);
    await systemPasteboard.setPasteData(pasteData);
    const res45 = await systemPasteboard.hasPasteData();
    expect(res45).assertEqual(true);
    const pasteData45 = await systemPasteboard.getPasteData();
    expect(pasteData45.getRecordCount()).assertEqual(1);
    const pasteDataRecord = pasteData45.getRecordAt(0);
    pasteDataRecord.convertToText((err, text) => {
      if (err) {
        console.info('f_test45 pasteDataRecord.convertToText error: ' + error);
      } else {
        expect(textData45).assertEqual(text);
        done();
      }
    });
  });

  /**
   * @tc.name      pasteboard_promise_test46
   * @tc.desc      将uri数据强制转换为文本
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test46', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText46 = 'https://www.baidu.com/';
    const pasteData = pasteboard.createUriData(uriText46);
    await systemPasteboard.setPasteData(pasteData);
    const res46 = await systemPasteboard.hasPasteData();
    expect(res46).assertEqual(true);
    let pasteData46 = await systemPasteboard.getPasteData();
    expect(pasteData46.getRecordCount()).assertEqual(1);
    let pasteDataRecord = pasteData46.getRecordAt(0);
    pasteDataRecord
      .convertToText()
      .then((text) => {
        expect(uriText46).assertEqual(text);
        done();
      })
      .catch((error) => {
        console.info('f_test46 pasteDataRecord.convertToText error: ' + error);
      });
  });

  /**
   * @tc.name      pasteboard_promise_test47
   * @tc.desc      复制文本、uri格式
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test47', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData470 = 'Hello World0';
    const pasteData = pasteboard.createPlainTextData(textData470);
    const uriText47 = pasteboard.createUriRecord('https://www.baidu.com/');
    pasteData.addRecord(uriText47);
    await systemPasteboard.setPasteData(pasteData);
    const res47 = await systemPasteboard.hasPasteData();
    expect(res47).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const recordCount47 = data.getRecordCount();
      expect(recordCount47).assertEqual(2);
      const pasteDataRecord1 = data.getRecordAt(0);
      const pasteDataRecord2 = data.getRecordAt(1);
      expect(pasteDataRecord1.uri).assertEqual(uriText47.uri);
      expect(pasteDataRecord2.plainText).assertEqual(textData470);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test48
   * @tc.desc      关闭内容变化通知功能：向剪贴板数据增加、删除等html数据项
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test48', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    systemPasteboard.off('update', contentChanges);
    const htmlText48 = '<html><head></head><body>Hello World!</body></html>';
    const pasteData = pasteboard.createHtmlData(htmlText48);
    await systemPasteboard.setPasteData(pasteData);
    const res48 = await systemPasteboard.hasPasteData();
    expect(res48).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      expect(data.removeRecordAt(0)).assertEqual(true);
      expect(data.getRecordCount()).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test49
   * @tc.desc      创建pixelMap
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test49', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const buffer49 = new ArrayBuffer(128);
    const opt49 = {
      size: { height: 5, width: 5 },
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    const pixelMap = await image.createPixelMap(buffer49, opt49);
    expect(pixelMap.getPixelBytesNumber()).assertEqual(100);
    const pasteData49 = pasteboard.createData(pasteboard.MIMETYPE_PIXELMAP, pixelMap);
    await systemPasteboard.setPasteData(pasteData49);
    const res49 = await systemPasteboard.hasPasteData();
    expect(res49).assertEqual(true);
    systemPasteboard.getPasteData().then(async (newPasteData) => {
      const recordCount49 = newPasteData.getRecordCount();
      expect(recordCount49).assertEqual(1);
      const newPixelMap49 = newPasteData.getPrimaryPixelMap();
      const PixelMapBytesNumber = newPixelMap49.getPixelBytesNumber();
      expect(PixelMapBytesNumber).assertEqual(100);
      const imageInfo = await newPixelMap49.getImageInfo();
      expect(imageInfo.size.height === 5 && imageInfo.size.width === 5).assertEqual(true);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test50
   * @tc.desc      创建kv Record
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test50', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const dataXml50 = new ArrayBuffer(512);
    let int32view50 = new Int32Array(dataXml50);
    for (let i = 0; i < int32view50.length; i++) {
      int32view50[i] = 65535 + i;
    }
    const pasteDataRecord = pasteboard.createRecord('app/xml', dataXml50);
    const dataJpg50 = new ArrayBuffer(256);
    pasteDataRecord.data['image/ipg'] = dataJpg50;
    const pasteData50 = pasteboard.createHtmlData('application/xml');
    const replace = pasteData50.replaceRecordAt(0, pasteDataRecord);
    expect(replace).assertEqual(true);
    await systemPasteboard.setPasteData(pasteData50);
    const res50 = await systemPasteboard.hasPasteData();
    expect(res50).assertEqual(true);
    systemPasteboard.getPasteData().then((newPasteData) => {
      const recordCount50 = newPasteData.getRecordCount();
      expect(recordCount50).assertEqual(1);
      let newPasteDataRecord = newPasteData.getRecordAt(0);
      let newAppXml50 = newPasteDataRecord.data['app/xml'];
      let newImageIpg50 = newPasteDataRecord.data['image/ipg'];
      expect(newAppXml50.byteLength === 512 && newImageIpg50.byteLength === 256).assertEqual(true);
      let newAppXmlView50 = new Int32Array(newAppXml50);
      let newImageIpgView50 = new Int32Array(newImageIpg50);
      for (let i = 0; i < newAppXmlView50.length; i++) {
        console.info('newAppXml[' + i + '] = ' + newAppXmlView50[i]);
      }
      for (let i = 0; i < newImageIpgView50.length; i++) {
        console.info('newImageIpg[' + i + '] = ' + newImageIpg50[i]);
      }
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test51
   * @tc.desc      测试addPixelMapRecord
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test51', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const buffer51 = new ArrayBuffer(128);
    const opt51 = {
      size: { height: 5, width: 5 },
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    const pasteData51 = pasteboard.createHtmlData('application/xml');
    const pixelMap = await image.createPixelMap(buffer51, opt51);
    expect(pixelMap.getPixelBytesNumber() === 100).assertEqual(true);
    pasteData51.addRecord(pasteboard.MIMETYPE_PIXELMAP, pixelMap);
    await systemPasteboard.setPasteData(pasteData51);
    const res51 = await systemPasteboard.hasPasteData();
    expect(res51).assertEqual(true);
    systemPasteboard.getPasteData().then(async (newPasteData) => {
      const recordCount51 = newPasteData.getRecordCount();
      expect(recordCount51).assertEqual(2);
      const newPixelMap51 = newPasteData.getPrimaryPixelMap();
      const PixelMapBytesNumber51 = newPixelMap51.getPixelBytesNumber();
      expect(PixelMapBytesNumber51).assertEqual(100);
      const newHtmlData51 = newPasteData.getRecordAt(1);
      expect(newHtmlData51.htmlText).assertEqual('application/xml');
      const imageInfo51 = await newPixelMap51.getImageInfo();
      expect(imageInfo51.size.height === 5 && imageInfo51.size.width === 5).assertEqual(true);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test52
   * @tc.desc      支持512个record
   * @tc.type      Function
   * @tc.require   AR000HEECB
   */
  it('pasteboard_promise_test52', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const dataHtml52 = new ArrayBuffer(256);
    const htmlText52 = '<html><head></head><body>Hello!</body></html>';
    const uriText52 = 'https://www.baidu.com/';
    const wantText52 = {
      bundleName: 'com.example.myapplication3',
      abilityName: 'com.example.myapplication3.MainAbility',
    };
    const plainText52 = '';
    const pasteData52 = pasteboard.createData('x'.repeat(1024), dataHtml52);
    const record52 = pasteData52.getRecordAt(0);
    record52.htmlText = htmlText52;
    record52.plainText = plainText52;
    record52.uri = uriText52;
    record52.want = wantText52;
    const buffer52 = new ArrayBuffer(128);
    const opt52 = {
      size: { height: 5, width: 5 },
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    const pixelMap = await image.createPixelMap(buffer52, opt52);
    record52.pixelMap = pixelMap;
    pasteData52.replaceRecordAt(0, record52);
    for (let i = 0; i < 511; i++) {
      pasteData52.addRecord(record52);
    }
    await systemPasteboard.setPasteData(pasteData52);
    const res52 = await systemPasteboard.hasPasteData();
    expect(res52).assertTrue();
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(512);
      expect(data.getRecordAt(0).mimeType).assertEqual('x'.repeat(1024));
      expect(data.getPrimaryWant().bundleName).assertEqual(wantText52.bundleName);
      expect(data.getRecordAt(253).htmlText).assertEqual(htmlText52);
      expect(data.getRecordAt(511).plainText).assertEqual(plainText52);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test53
   * @tc.desc      html
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test53', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "<!DOCTYPE html><html><head><title>" +
    "的厚爱hi哦</title></head><body><h2>恶风无关痛痒和</h2>" +
    "<p>Greg任何人https://exampsaole.com问我的<a href=\"https://exaeqdwerfmple.com\">" +
    "如果qwiuyhw@huedqw.dsh站</a>。</p></body></html>";
    const pasteData = pasteboard.createHtmlData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    const patterns = [pasteboard.Pattern.EMAIL_ADDRESS, pasteboard.Pattern.NUMBER];
    systemPasteboard.detectPatterns(patterns).then((data) => {
      const patternsRight = [pasteboard.Pattern.EMAIL_ADDRESS];
      expect(data.sort().join('')).assertEqual(patternsRight.sort().join(''));
      done();
    }).catch((error)=>{
      console.error('promise_test53: systemPasteboard.detectPatterns promise error:' + error.message);
      return;
    });
  });

  /**
   * @tc.name      pasteboard_promise_test54
   * @tc.desc      plaintext
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test54', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "部分人的十点半：\n" +
    "「而飞过海」\n" +
    "方法：\n" +
    "https://pr5yyye-drseyive.u54yk.cwerfe/s/42e1ewed77f3dab4" +
    "网gest加尔文iqru发的我ui哦计划任务i文化人:\n" +
    "~b0043fg3423tddj~";
    const pasteData = pasteboard.createPlainTextData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    const patterns = [pasteboard.Pattern.EMAIL_ADDRESS,
      pasteboard.Pattern.URL, pasteboard.Pattern.NUMBER];
    systemPasteboard.detectPatterns(patterns).then((data) => {
      const patternsRight = [pasteboard.Pattern.NUMBER, pasteboard.Pattern.URL];
      expect(data.sort().join('')).assertEqual(patternsRight.sort().join(''));
      done();
    }).catch((error)=>{
      console.error('promise_test54: systemPasteboard.detectPatterns promise error:' + error.message);
      return;
    });
  });

  /**
   * @tc.name      pasteboard_promise_test55
   * @tc.desc      Add html with local uri
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test55', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText5 = "<html><head></head><body><div class='item'><img " +
        "src='file:///com.example.webview/data/storage/el1/base/test.png'></div></body></html>";
    const pasteData = pasteboard.createHtmlData(htmlText5);
    await systemPasteboard.setPasteData(pasteData);
    const res5 = await systemPasteboard.hasPasteData();
    expect(res5).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData5 = data;
      expect(pasteData5.getRecordCount()).assertEqual(1);
      const primaryHtml6 = pasteData5.getPrimaryHtml();
      expect(primaryHtml6).assertEqual(htmlText5);
      expect(pasteData5.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
      expect(pasteData5.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test56
   * @tc.desc      Add html with distributed uri
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test56', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText5 = "<html><head></head><body><div class='item'><img src='file://com.byy.testdpb/data/storage/el2/" +
        "distributedfiles/.remote_share/data/storage/el2/base/haps/entry/cache/t1.jpg'></div></body></html>";
    const pasteData = pasteboard.createHtmlData(htmlText5);
    await systemPasteboard.setPasteData(pasteData);
    const res5 = await systemPasteboard.hasPasteData();
    expect(res5).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData5 = data;
      expect(pasteData5.getRecordCount()).assertEqual(1);
      const primaryHtml6 = pasteData5.getPrimaryHtml();
      expect(primaryHtml6).assertEqual(htmlText5);
      expect(pasteData5.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
      expect(pasteData5.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test57
   * @tc.desc      Add html with uris
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_promise_test57', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText5 = "<html><head></head><body><div class='item'><img src='file://com.byy.testdpb/data/storage/el2/" +
        "distributedfiles/.remote_share/data/storage/el2/base/haps/entry/cache/t1.jpg'></div>" +
        "<div class='item'><img src='file:///com.example.webview/data/storage/el1/base/test.png'></div></body></html>";
    const pasteData = pasteboard.createHtmlData(htmlText5);
    await systemPasteboard.setPasteData(pasteData);
    const res5 = await systemPasteboard.hasPasteData();
    expect(res5).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData5 = data;
      expect(pasteData5.getRecordCount()).assertEqual(1);
      const primaryHtml6 = pasteData5.getPrimaryHtml();
      expect(primaryHtml6).assertEqual(htmlText5);
      expect(pasteData5.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
      expect(pasteData5.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test58
   * @tc.desc      test addEntry and getData
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test58', 0, async function (done) {
    const htmlText = "<html><head></head><body>Hello World!</body></html>";
    const plainData = "Hello World!";
    const uriText = "https://www.AACCSVDSVSDV.com/";
    const buffer58 = new ArrayBuffer(128);
    const opt58 = {
      size: { height: 5, width: 5 },
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    const pixelMap = await image.createPixelMap(buffer58, opt58);
    const pasteRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_HTML, htmlText);
    pasteRecord.addEntry(pasteboard.MIMETYPE_TEXT_PLAIN, plainData);
    pasteRecord.addEntry(pasteboard.MIMETYPE_TEXT_URI, uriText);
    pasteRecord.addEntry(pasteboard.MIMETYPE_PIXELMAP, pixelMap);

    const html = await pasteRecord.getData(pasteboard.MIMETYPE_TEXT_HTML);
    expect(html.toString()).assertEqual(htmlText.toString());
    const plain = await pasteRecord.getData(pasteboard.MIMETYPE_TEXT_PLAIN);
    expect(plain.toString()).assertEqual(plainData.toString());
    const uri = await pasteRecord.getData(pasteboard.MIMETYPE_TEXT_URI);
    expect(uri.toString()).assertEqual(uriText.toString());
    const pixel = await pasteRecord.getData(pasteboard.MIMETYPE_PIXELMAP);
    const PixelMapBytesNumber = pixel.getPixelBytesNumber();
    expect(PixelMapBytesNumber).assertEqual(100);
    const imageInfo = await pixel.getImageInfo();
    expect(imageInfo.size.height === 5 && imageInfo.size.width === 5).assertEqual(true);

    done();
  });

  /**
   * @tc.name      pasteboard_promise_test59
   * @tc.desc      Single style with createData(Record) function.
   * @tc.type      Function
   * @tc.require   API 14
   */
  it('pasteboard_promise_test59', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();

    let inputData = 'Hello world.';
    let outputData = await copyPasteWithRecord(pasteboard.MIMETYPE_TEXT_PLAIN, inputData, systemPasteboard);
    expect(outputData).assertEqual(inputData);

    inputData = '<p>Hello world.</p>';
    outputData = await copyPasteWithRecord(pasteboard.MIMETYPE_TEXT_HTML, inputData, systemPasteboard);
    expect(outputData).assertEqual(inputData);

    inputData = 'file://abc/def.png';
    outputData = await copyPasteWithRecord(pasteboard.MIMETYPE_TEXT_URI, inputData, systemPasteboard);
    expect(outputData).assertEqual(inputData);

    let inputWant = {
      deviceId: '',
      bundleName: 'test.bundle.name',
      abilityName: 'test.ability,name',
      moduleName: 'test.module.name',
    };
    outputData = await copyPasteWithRecord(pasteboard.MIMETYPE_TEXT_WANT, inputWant, systemPasteboard);
    expect(outputData.bundleName).assertEqual(inputWant.bundleName);

    let inputPixelMap = await buildPixelMap();
    outputData = await copyPasteWithRecord(pasteboard.MIMETYPE_PIXELMAP, inputPixelMap, systemPasteboard);
    expect(outputData.getImageInfoSync().size.width).assertEqual(inputPixelMap.getImageInfoSync().size.width);
    expect(outputData.getImageInfoSync().size.height).assertEqual(inputPixelMap.getImageInfoSync().size.height);

    let inputArrayBuffer = string2ArrayBuffer('Hello world.');
    outputData = await copyPasteWithRecord('my-mime-type', inputArrayBuffer, systemPasteboard);
    expect(outputData.toString()).assertEqual(inputArrayBuffer.toString());

    await systemPasteboard.clearData();
    done();
  });

  /**
   * @tc.name      pasteboard_promise_test60
   * @tc.desc      Multi style with createData(Record) function.
   * @tc.type      Function
   * @tc.require   API 14
   */
  it('pasteboard_promise_test60', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();

    let record = await initRecordData();
    await systemPasteboard.setData(pasteboard.createData(record));

    systemPasteboard.getData().then(async (outputData) => {
      const recordCount = outputData.getRecordCount();
      console.log('actual recordCount: ' + recordCount);
      expect(recordCount).assertEqual(1);

      const outRecord = outputData.getRecord(0);
      const outTypes = outRecord.getValidTypes(allTypes);
      console.log('outTypes: ' + outTypes.toString());

      for (const type of outTypes) {
        console.log('check type: ' + type);
        const outValue = await outRecord.getData(type);
        expect(outValue != null).assertTrue();
        const inValue = record[type];
        if (type === pasteboard.MIMETYPE_TEXT_WANT) {
          console.log('actual bundleName: ' + outValue.bundleName);
          expect(outValue.bundleName.length > 0).assertTrue();
        } else if (type === pasteboard.MIMETYPE_PIXELMAP) {
          expect(outValue !== null && outValue !== undefined).assertTrue();
        } else {
          console.log('type: ' + type + ', actual strValue: ' + outValue.toString() + ', expect strValue: ' + inValue.toString());
          expect(outValue.toString()).assertEqual(inValue.toString());
        }
      }

      await systemPasteboard.clearData();
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test61
   * @tc.desc      Multi style with addEntry function.
   * @tc.type      Function
   * @tc.require   API 14
   */
  it('pasteboard_promise_test61', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();

    let record = await initRecordData();
    let recordTmp = await initRecordData('temp');

    let inData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_HTML, 'temp value');

    for (const item of Object.entries(record)) {
      if (item[0] === pasteboard.MIMETYPE_TEXT_PLAIN) {
        continue;
      }
      let inRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_PLAIN, recordTmp[pasteboard.MIMETYPE_TEXT_PLAIN]);
      inRecord.addEntry(pasteboard.MIMETYPE_TEXT_PLAIN, record[pasteboard.MIMETYPE_TEXT_PLAIN]);
      inRecord.addEntry(item[0], recordTmp[item[0]]);
      inRecord.addEntry(item[0], item[1]);
      inData.replaceRecord(0, inRecord);
      await systemPasteboard.setData(inData);

      console.log('copy success');
      const outputData = await systemPasteboard.getData();

      console.log('actual recordCount: ' + outputData.getRecordCount());
      expect(outputData.getRecordCount()).assertEqual(1);
      const outRecord = outputData.getRecord(0);

      const inTypes = [pasteboard.MIMETYPE_TEXT_PLAIN, item[0]];
      console.log('actual validTypes: ' + outRecord.getValidTypes(inTypes).toString());
      expect(outRecord.getValidTypes(inTypes).toString()).assertEqual(inTypes.toString());
      console.log('actual mimeType: ' + outRecord.mimeType);
      expect(outRecord.mimeType).assertEqual(pasteboard.MIMETYPE_TEXT_PLAIN);
      console.log('actual plainText: ' + outRecord.plainText);
      expect(outRecord.plainText).assertEqual(record[pasteboard.MIMETYPE_TEXT_PLAIN]);

      const outValue = await outRecord.getData(item[0]);
      if (item[0] === pasteboard.MIMETYPE_TEXT_WANT) {
        console.log('actual bundleName: ' + outValue.bundleName);
        expect(outValue.bundleName).assertEqual(item[1].bundleName);
      } else if (item[0] === pasteboard.MIMETYPE_PIXELMAP) {
        console.log('actual width: ' + outValue.getImageInfoSync().size.width);
        expect(outValue.getImageInfoSync().size.width).assertEqual(item[1].getImageInfoSync().size.width);
        expect(outValue.getImageInfoSync().size.height).assertEqual(item[1].getImageInfoSync().size.height);
      } else {
        console.log('type: ' + item[0] + ', actual strValue: ' + outValue.toString());
        expect(outValue.toString()).assertEqual(item[1].toString());
      }
    }

    await systemPasteboard.clearData();
    done();
  });

  /**
   * @tc.name      pasteboard_promise_test62
   * @tc.desc      Paste record's getValidType function.
   * @tc.type      Function
   * @tc.require   API 14
   */
  it('pasteboard_promise_test62', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();

    let record = await initRecordData();
    await systemPasteboard.setData(pasteboard.createData(record));

    const outputData = await systemPasteboard.getData();
    const recordCount = outputData.getRecordCount();
    console.log('actual recordCount: ' + recordCount);
    expect(recordCount).assertEqual(1);

    const outRecord = outputData.getRecord(0);
    let outTypes = outRecord.getValidTypes(allTypes);
    console.log('actual outTypes1: ' + outTypes.toString());
    expect(outTypes.toString()).assertEqual(allTypes.toString());

    const partlyTypes = [pasteboard.MIMETYPE_TEXT_HTML, myType];
    outTypes = outRecord.getValidTypes(partlyTypes);
    console.log('actual outTypes2: ' + outTypes.toString());
    expect(outTypes.toString()).assertEqual(partlyTypes.toString());

    outTypes = outRecord.getValidTypes([]);
    console.log('actual outTypes3: ' + outTypes.toString());
    expect(outTypes.length).assertEqual(0);

    await systemPasteboard.setData(pasteboard.createData({
      'text/plain' : 'hello world',
      'text/uri' : 'file://abc.def.png',
    }));
    const outRecord1 = (await systemPasteboard.getData()).getRecord(0);
    outTypes = outRecord1.getValidTypes(allTypes);
    console.log('actual outTypes4: ' + outTypes.toString());
    expect(outTypes.toString()).assertEqual([pasteboard.MIMETYPE_TEXT_PLAIN, pasteboard.MIMETYPE_TEXT_URI].toString());

    await systemPasteboard.clearData();
    done();
  });

  async function copyPasteWithRecord(mimeType, value, systemPasteboard) {
    let record = {};
    record[mimeType] = value;
    let data = pasteboard.createData(record);
    await systemPasteboard.setData(data);

    let outData = await systemPasteboard.getData();
    if (outData.getRecordCount() !== 1) {
      return '';
    }
    let pasteRecord = outData.getRecord(0);
    let validMimeType = pasteRecord.getValidTypes([mimeType]);
    return await pasteRecord.getData(validMimeType.pop());
  }

  async function buildPixelMap() {
    let buffer = new ArrayBuffer(500);
    let realSize = {height: 5, width: 100};
    let opt = {
      size: realSize,
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    return await image.createPixelMap(buffer, opt);
  }

  function string2ArrayBuffer(input) {
    let arr = [];
    for (let index = 0; index < input.length; index++) {
      arr.push(input.charCodeAt(index));
    }
    let arrayBuffer = new Uint8Array(arr).buffer;
    return arrayBuffer;
  }

  async function initRecordData(temp) {
    const inputPlainText = 'Hello world.' + (temp ? temp : '');
    const inputHtml = '<p>Hello world.' + (temp ? temp : '') + '</p>';
    const inputUri = 'file://abc/def' + (temp ? temp : '') + '.png';
    const inputWant = {
      deviceId: '',
      bundleName: 'test.bundle.name' + (temp ? temp : ''),
      abilityName: 'test.ability,name' + (temp ? temp : ''),
      moduleName: 'test.module.name' + (temp ? temp : ''),
    };
    const inputPixelMap = await buildPixelMap();
    const inputArrayBuffer = string2ArrayBuffer('Hello world.' + (temp ? temp : ''));

    let record = {};
    record[pasteboard.MIMETYPE_TEXT_PLAIN] = inputPlainText;
    record[pasteboard.MIMETYPE_TEXT_HTML] = inputHtml;
    record[pasteboard.MIMETYPE_TEXT_URI] = inputUri;
    record[pasteboard.MIMETYPE_TEXT_WANT] = inputWant;
    record[pasteboard.MIMETYPE_PIXELMAP] = inputPixelMap;
    record[myType] = inputArrayBuffer;

    return record;
  }

  /**
   * @tc.name      pasteboard_promise_test63
   * @tc.desc      html
   * @tc.type      Function
   * @tc.require   AR20241012964265
   */
  it('pasteboard_promise_test63', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "<!DOCTYPE html><html><head><title>" +
    "的厚爱hi哦</title></head><body><h2>恶风无关痛痒和</h2>" +
    "<p>Greg任何人https://exampsaole.com问我的<a href=\"https://exaeqdwerfmple.com\">" +
    "如果qwiuyhw@huedqw.dsh站</a>。</p></body></html>";
    const pasteData = pasteboard.createHtmlData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getMimeTypes().then((data) => {
      expect(data.length).assertEqual(1);
      expect(data[0]).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test47
   * @tc.desc      复制文本、uri格式
   * @tc.type      Function
   * @tc.require   AR20241012964265
   */
  it('pasteboard_promise_test64', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData64 = 'Hello World0';
    const pasteData = pasteboard.createPlainTextData(textData64);
    const uriText64 = pasteboard.createUriRecord('https://www.baidu.com/');
    pasteData.addRecord(uriText64);
    const htmlText64 = '<html><head></head><body>Hello World 1</body></html>';
    const pasteDataRecord = pasteboard.createHtmlTextRecord(htmlText64);
    pasteData.addRecord(pasteDataRecord);
    await systemPasteboard.setPasteData(pasteData);

    systemPasteboard.getMimeTypes().then((data) => {
      expect(data.length).assertEqual(3);
      const expectedMimeTypes = new Set([pasteboard.MIMETYPE_TEXT_PLAIN, pasteboard.MIMETYPE_TEXT_HTML, pasteboard.MIMETYPE_TEXT_URI]);
      expect(Array.from(data).every(type => expectedMimeTypes.has(type))).assertEqual(true);
      done();
    });
  });

  /**
   *  The callback function is used for pasteboard content changes
   */
  function contentChanges() {
    console.info('#EVENT: The content is changed in the pasteboard');
  }

  /**
   * @tc.name      pasteboard_promise_test65
   * @tc.desc      打开远端内容变化通知功能
   * @tc.type      Function
   * @tc.require   AR000H5I1D
   */
  it('pasteboard_promise_test65', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    systemPasteboard.onRemoteUpdate(contentChanges);
    const textData65 = 'Hello World!';
    const pasteData = pasteboard.createPlainTextData(textData65);
    await systemPasteboard.setPasteData(pasteData);
    const res66 = await systemPasteboard.hasPasteData();
    expect(res66).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      expect(data.removeRecordAt(0)).assertEqual(true);
      expect(data.getRecordCount()).assertEqual(0);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_promise_test66
   * @tc.desc      关闭远端内容变化通知功能：向剪贴板数据增加、删除等html数据项
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_promise_test66', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    systemPasteboard.offRemoteUpdate(contentChanges);
    const htmlText66 = '<html><head></head><body>Hello World!</body></html>';
    const pasteData = pasteboard.createHtmlData(htmlText66);
    await systemPasteboard.setPasteData(pasteData);
    const res66 = await systemPasteboard.hasPasteData();
    expect(res66).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      expect(data.removeRecordAt(0)).assertEqual(true);
      expect(data.getRecordCount()).assertEqual(0);
      done();
    });
  });
});