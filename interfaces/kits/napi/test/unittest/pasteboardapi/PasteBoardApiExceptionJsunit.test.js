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

describe('PasteBoardJSTest', function () {
  const myType = 'my-mime-type';
  const allTypes = [pasteboard.MIMETYPE_TEXT_PLAIN, pasteboard.MIMETYPE_TEXT_HTML, pasteboard.MIMETYPE_TEXT_URI,
    pasteboard.MIMETYPE_TEXT_WANT, pasteboard.MIMETYPE_PIXELMAP, myType
  ];

  beforeAll(async function () {
    console.info('beforeAll');
  });

  afterAll(async function () {
    console.info('afterAll');
  });

  /**
   * @tc.name      pasteboard_exception_test1
   * @tc.desc      自定义数据测试
   * @tc.type      Function
   * @tc.require   AR000HEECB
   */
  it('pasteboard_exception_test1', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    let pasteData = undefined;
    console.info('systemPasteboard clear data success');
    const dataUri = new ArrayBuffer(256);
    pasteData = pasteboard.createData('xxx', dataUri);
    const addUri = new ArrayBuffer(128);
    pasteData.addRecord('xxx', addUri);
    const recordUri = new ArrayBuffer(96);
    const pasteDataRecord = pasteboard.createRecord('xxx', recordUri);
    pasteData.addRecord(pasteDataRecord);
    await systemPasteboard.setPasteData(pasteData);
    console.info('Set pastedata success');
    const res = await systemPasteboard.hasPasteData();
    console.info('Check pastedata has data success, result: ' + res);
    expect(res).assertTrue();
    const data = await systemPasteboard.getPasteData();
    console.info('Get paste data success');
    expect(data.getRecordCount()).assertEqual(3);
    expect(data.getRecordAt(0).data['xxx'].byteLength).assertEqual(96);
    expect(data.getRecordAt(1).data['xxx'].byteLength).assertEqual(128);
    expect(data.getRecordAt(2).data['xxx'].byteLength).assertEqual(256);
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test2
   * @tc.desc      自定义数据测试
   * @tc.type      Function
   * @tc.require   AR000HEECB
   */
  it('pasteboard_exception_test2', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    console.info('systemPasteboard clear data success');
    let pasteData = undefined;
    let pasteRecord = undefined;
    const dataHtml = new ArrayBuffer(256);
    pasteData = pasteboard.createData('xy', dataHtml);
    expect(pasteData != undefined).assertTrue();
    pasteData.addRecord('x'.repeat(1024), dataHtml);
    expect(pasteData.getRecordCount()).assertEqual(2);
    pasteRecord = pasteboard.createRecord('xy2', dataHtml);
    expect(pasteRecord != undefined).assertTrue();
    pasteData.addRecord(pasteRecord);
    await systemPasteboard.setPasteData(pasteData);
    console.info('set pastedata success');
    const res = await systemPasteboard.hasPasteData();
    console.info('Check pastedata has data success, result: ' + res);
    expect(res).assertTrue();
    systemPasteboard.getPasteData().then((data) => {
      console.info('get paste data success');
      expect(data.getRecordCount()).assertEqual(3);
      expect(data.getRecordAt(0).mimeType).assertEqual('xy2');
      expect(data.getRecordAt(1).mimeType).assertEqual('x'.repeat(1024));
      expect(data.getRecordAt(2).mimeType).assertEqual('xy');
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test3
   * @tc.desc      自定义数据异常测试
   * @tc.type      Function
   * @tc.require   AR000HEECB
   */
  it('pasteboard_exception_test3', 0, async function (done) {
    console.info('pasteboard_exception_test1 start');
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    console.info('systemPasteboard clear data success');
    let pasteData = undefined;
    let pasteRecord = undefined;

    // test createData
    let dataHtml = new ArrayBuffer(256);
    try {
      pasteData = pasteboard.createData('x'.repeat(1025), dataHtml);
      expect(true === false).assertTrue();
    } catch (error) {
      console.info(error.code);
      console.info(error.message);
    }

    expect(pasteData).assertEqual(undefined);
    pasteData = pasteboard.createData('x'.repeat(1024), dataHtml);
    expect(pasteData != undefined).assertTrue();

    // test addRecord
    try {
      pasteData.addRecord('x'.repeat(1025), dataHtml);
      expect(true === false).assertTrue();
    } catch (error) {
      console.info(error.code);
      console.info(error.message);
    }
    expect(pasteData.getRecordCount()).assertEqual(1);
    pasteData.addRecord('x'.repeat(1024), dataHtml);
    expect(pasteData.getRecordCount()).assertEqual(2);

    let addHtml = new ArrayBuffer(128);
    try {
      pasteData.addRecord('x'.repeat(1025), addHtml);
      expect(true === false).assertTrue();
    } catch (error) {
      console.info(error.code);
      console.info(error.message);
    }
    expect(pasteData.getRecordCount()).assertEqual(2);
    pasteData.addRecord('x'.repeat(1024), addHtml);
    expect(pasteData.getRecordCount()).assertEqual(3);

    let recordHtml = new ArrayBuffer(64);
    try {
      pasteRecord = pasteboard.createRecord('x'.repeat(1025), recordHtml);
      expect(true === false).assertTrue();
    } catch (error) {
      console.info(error.code);
      console.info(error.message);
    }
    expect(pasteRecord).assertEqual(undefined);
    pasteRecord = pasteboard.createRecord('x'.repeat(1024), recordHtml);
    expect(pasteRecord != undefined).assertTrue();
    pasteData.addRecord(pasteRecord);
    expect(pasteData.getRecordCount()).assertEqual(4);
    await systemPasteboard.setPasteData(pasteData);
    console.info('set pastedata success');
    const res = await systemPasteboard.hasPasteData();
    console.info('Check pastedata has data success, result: ' + res);
    expect(res).assertTrue();
    systemPasteboard.getPasteData().then((data) => {
      console.info('get paste data success');
      expect(data.getRecordCount()).assertEqual(4);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test4
   * @tc.desc      一个record中多个数据类型：get primary html、pixelMap、want、text、uri
   * @tc.type      Function
   * @tc.require   AR000HEECB
   */
  it('pasteboard_exception_test4', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const dataHtml = new ArrayBuffer(256);
    const htmlText = '<html><head></head><body>Hello!</body></html>';
    const uriText = 'https://www.baidu.com/';
    const wantText = {
      bundleName: 'com.example.myapplication3',
      abilityName: 'com.example.myapplication3.MainAbility',
    };
    let plainText = '';
    const pasteData = pasteboard.createData('x'.repeat(1024), dataHtml);
    const record = pasteData.getRecordAt(0);
    record.htmlText = htmlText;
    record.plainText = plainText;
    record.uri = uriText;
    record.want = wantText;
    const buffer = new ArrayBuffer(128);
    const opt = {
      size: { height: 5, width: 5 },
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    const pixelMap = await image.createPixelMap(buffer, opt);
    record.pixelMap = pixelMap;
    pasteData.replaceRecordAt(0, record);
    await systemPasteboard.setPasteData(pasteData);
    const hasData = await systemPasteboard.hasPasteData();
    expect(hasData).assertTrue();
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      expect(data.getRecordAt(0).mimeType).assertEqual('x'.repeat(1024));
      expect(data.getPrimaryWant().bundleName).assertEqual(wantText.bundleName);
      expect(data.getPrimaryWant().abilityName).assertEqual(wantText.abilityName);
      let newPixelMap = data.getPrimaryPixelMap();
      newPixelMap.getImageInfo().then((imageInfo) => {
        expect(imageInfo.size.height).assertEqual(opt.size.height);
        expect(imageInfo.size.width).assertEqual(opt.size.width);
      });
      expect(data.getPrimaryUri()).assertEqual(uriText);
      expect(data.getPrimaryText()).assertEqual(plainText);
      expect(data.getPrimaryHtml()).assertEqual(htmlText);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test5
   * @tc.desc      Test CreateRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test5', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText = 'https://www.baidu.com/';
    const pasteData = pasteboard.createUriData(uriText);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      const pasteData1 = data;
      expect(pasteData1.getRecordCount()).assertEqual(1);
      const uriText1 = 'https://www.baidu.com/1';
      const pasteDataRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_URI, uriText1);
      const replace = pasteData1.replaceRecordAt(0, pasteDataRecord);
      expect(replace).assertEqual(true);
      const primaryUri1 = pasteData1.getPrimaryUri();
      expect(primaryUri1).assertEqual(uriText1);
      expect(pasteData1.hasMimeType(pasteboard.MIMETYPE_TEXT_URI)).assertEqual(true);
      const primaryUri2 = pasteData1.getPrimaryUri();
      expect(primaryUri2).assertEqual(uriText1);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test6
   * @tc.desc      Test CreateRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test6', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText6 = 'https://www.baidu.com/';
    const textData6 = 'Hello World!';
    const htmlText6 = '<html><head></head><body>Hello World!</body></html>';
    const wantText6 = {
      bundleName: 'com.example.myapplication3',
      abilityName: 'com.example.myapplication3.MainAbility',
    };
    const dataHtml6 = new ArrayBuffer(256);
    const buffer6 = new ArrayBuffer(128);
    const opt6 = {
      size: { height: 5, width: 5 },
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    const pixelMap = await image.createPixelMap(buffer6, opt6);
    const pasteData = pasteboard.createUriData(uriText6);

    try {
      let pasteDataRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_URI, uriText6);
      pasteData.addRecord(pasteDataRecord);
      pasteDataRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_PLAIN, textData6);
      pasteData.addRecord(pasteDataRecord);
      pasteDataRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_HTML, htmlText6);
      pasteData.addRecord(pasteDataRecord);
      pasteDataRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_WANT, wantText6);
      pasteData.addRecord(pasteDataRecord);
      pasteDataRecord = pasteboard.createRecord('x'.repeat(1022), dataHtml6);
      pasteData.addRecord(pasteDataRecord);
      pasteDataRecord = pasteboard.createRecord(pasteboard.MIMETYPE_PIXELMAP, pixelMap);
      pasteData.addRecord(pasteDataRecord);
    } catch (error) {
      expect(error.code === undefined).assertTrue();
      expect(error.message === undefined).assertTrue();
      expect(True === false).assertTrue();
    }
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(7);
      let dataRecord = data.getRecordAt(3);
      expect(dataRecord.htmlText).assertEqual(htmlText6);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test7
   * @tc.desc      Test CreateRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test7', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText = 'https://www.baidu.com/';
    const htmlText = '<html><head></head><body>Hello World!</body></html>';
    const pasteData = pasteboard.createUriData(uriText);

    try {
      const pasteDataRecord = pasteboard.createRecord('xxddxx', htmlText);
      pasteData.addRecord(pasteDataRecord);
      expect(true === false).assertTrue();
    } catch (error) {
      expect(error.code).assertEqual('401');
      expect(error.message).assertEqual('Parameter error. The mimeType is not an arraybuffer.');
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test8
   * @tc.desc      Test Create Uri Data
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test8', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    let uriText = 'https://www.baidu.com/';
    let pasteData = undefined;
    try {
      pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText);
    } catch (e) {
      expect(true === false).assertTrue();
    }
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      const dataRecord = data.getRecordAt(0);
      expect(dataRecord.uri).assertEqual(uriText);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test9
   * @tc.desc      Test Create htmlText Data
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test9', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const htmlText = '<html><head></head><body>Hello World!</body></html>';
    let pasteData = undefined;
    try {
      pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_HTML, htmlText);
    } catch (e) {
      expect(true === false).assertTrue();
    }
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      const dataRecord = data.getRecordAt(0);
      expect(dataRecord.htmlText).assertEqual(htmlText);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test10
   * @tc.desc      Test Create wantText Data
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test10', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const wantText = {
      bundleName: 'com.example.myapplication3',
      abilityName: 'com.example.myapplication3.MainAbility',
    };
    let pasteData = undefined;
    try {
      pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_WANT, wantText);
    } catch (e) {
      expect(true === false).assertTrue();
    }
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(1);
      let primaryWant = data.getPrimaryWant();
      expect(primaryWant.bundleName).assertEqual(wantText.bundleName);
      expect(primaryWant.abilityName).assertEqual(wantText.abilityName);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test11
   * @tc.desc      Test Create pixelMap Data
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test11', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const buffer = new ArrayBuffer(128);
    const opt = {
      size: { height: 5, width: 5 },
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    let pasteData = undefined;
    const pixelMap = await image.createPixelMap(buffer, opt);
    try {
      pasteData = pasteboard.createData(pasteboard.MIMETYPE_PIXELMAP, pixelMap);
    } catch (e) {
      expect(true === false).assertTrue();
    }
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getPasteData().then(async (data) => {
      expect(data.getRecordCount()).assertEqual(1);
      const primaryPixelMap = data.getPrimaryPixelMap();
      const PixelMapBytesNumber = primaryPixelMap.getPixelBytesNumber();
      expect(PixelMapBytesNumber).assertEqual(100);
      const imageInfo = await primaryPixelMap.getImageInfo();
      expect(imageInfo.size.height === 5 && imageInfo.size.width === 5).assertEqual(true);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test12
   * @tc.desc      Test CreateData throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test12', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    let dataHtml = new ArrayBuffer(256);
    let pasteData = undefined;
    try {
      pasteData = pasteboard.createData(pasteboard.MIMETYPE_PIXELMAP, dataHtml);
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code).assertEqual('401');
      expect(e.message).assertEqual('Parameter error. Actual mimeType is not mimetype_pixelmap.');
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test13
   * @tc.desc      Test Create KV Data
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test13', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    let dataHtml = new ArrayBuffer(256);
    let pasteData = undefined;
    try {
      pasteData = pasteboard.createData('x'.repeat(1034), dataHtml);
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '401').assertTrue();
      expect(e.message === 'Parameter error. The length of mimeType cannot be greater than 1024 bytes.').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test14
   * @tc.desc      Test addRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test14', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText14 = 'https://www.baidu.com/';
    const textData14 = 'Hello World!';
    const htmlText14 = '<html><head></head><body>Hello World!</body></html>';
    const wantText14 = {
      bundleName: 'com.example.myapplication3',
      abilityName: 'com.example.myapplication3.MainAbility',
    };
    const dataHtml14 = new ArrayBuffer(256);
    const buffer14 = new ArrayBuffer(128);
    const opt14 = {
      size: { height: 5, width: 5 },
      pixelFormat: 3,
      editable: true,
      alphaType: 1,
      scaleMode: 1,
    };
    const pixelMap = await image.createPixelMap(buffer14, opt14);
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText14);

    try {
      pasteData.addRecord(pasteboard.MIMETYPE_TEXT_HTML, htmlText14);
      pasteData.addRecord(pasteboard.MIMETYPE_TEXT_URI, uriText14);
      pasteData.addRecord(pasteboard.MIMETYPE_TEXT_PLAIN, textData14);
      pasteData.addRecord(pasteboard.MIMETYPE_PIXELMAP, pixelMap);
      pasteData.addRecord(pasteboard.MIMETYPE_TEXT_WANT, wantText14);
      pasteData.addRecord('x'.repeat(100), dataHtml14);
    } catch (error) {
      expect(true === false).assertTrue();
    }
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    systemPasteboard.getPasteData().then((data) => {
      expect(data.getRecordCount()).assertEqual(7);
      let dataRecord = data.getRecordAt(6);
      expect(dataRecord.uri).assertEqual(uriText14);
      let primaryPixelMap = data.getPrimaryPixelMap();
      let PixelMapBytesNumber = primaryPixelMap.getPixelBytesNumber();
      expect(PixelMapBytesNumber).assertEqual(100);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test15
   * @tc.desc      Test addRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test15', 0, async function (done) {
    let uriText = 'https://www.baidu.com/';
    let pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText);
    try {
      pasteData.addRecord('xxxx', uriText);
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '401').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test16
   * @tc.desc      Test addRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test16', 0, async function (done) {
    const uriText = 'https://www.baidu.com/';
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText);
    const num = 600;
    try {
      for (let i = 0; i < num-1; i++) {
        pasteData.addRecord(pasteboard.MIMETYPE_TEXT_URI, uriText);
      }
      expect(pasteData.getRecordCount()).assertEqual(num);
    } catch (e) {
      expect(e.code === '12900002').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test17
   * @tc.desc      Test getRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test17', 0, async function (done) {
    const uriText = 'https://www.baidu.com/';
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText);
    try {
      let dataRecord = pasteData.getRecord(0);
      expect(dataRecord.uri).assertEqual(uriText);
    } catch (e) {
      expect(true === false).assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test18
   * @tc.desc      Test getRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test18', 0, async function (done) {
    const uriText = 'https://www.baidu.com/';
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText);
    try {
      let dataRecord = pasteData.getRecord(5);
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '12900001').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test19
   * @tc.desc      Test replaceRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test19', 0, async function (done) {
    const uriText = 'https://www.baidu.com/';
    const uriText1 = 'https://www.baidu1.com/';
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText);
    const dataRecord = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_URI, uriText1);
    try {
      pasteData.replaceRecord(0, dataRecord);
      const record = pasteData.getRecord(0);
      expect(record.uri).assertEqual(uriText1);
    } catch (e) {
      expect(true === false).assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test20
   * @tc.desc      Test replaceRecord throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test20', 0, async function (done) {
    const uriText = 'https://www.baidu.com/';
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText);
    try {
      pasteData.replaceRecord(0, 'xxxxxx');
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '401').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test21
   * @tc.desc      Test setData
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test21', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const uriText = 'Hello//';
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_URI, uriText);
    await systemPasteboard.setData(pasteData);
    const data = await systemPasteboard.hasData();
    expect(data).assertEqual(true);
    systemPasteboard.getData().then((pasteData1) => {
      expect(pasteData1.getRecordCount()).assertEqual(1);
      expect(pasteData1.hasType(pasteboard.MIMETYPE_TEXT_URI)).assertEqual(true);
      expect(pasteData1.getPrimaryUri()).assertEqual(uriText);
      done();
    });
  });

  /**
   * @tc.name      pasteboard_exception_test22
   * @tc.desc      Test setData throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test22', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    try {
      systemPasteboard.setData('xxxxx');
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '401').assertTrue();
      expect(e.message === 'Parameter error. The Type of data must be pasteData.').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test23
   * @tc.desc      Test set property throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test23', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = 'Hello World!';
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, textData);
    const pasteDataProperty = pasteData.getProperty();
    expect(pasteDataProperty.shareOption).assertEqual(pasteboard.ShareOption.CrossDevice);
    pasteDataProperty.shareOption = pasteboard.ShareOption.InApp;
    pasteData.setProperty(pasteDataProperty);
    expect(pasteData.getProperty().shareOption).assertEqual(pasteboard.ShareOption.InApp);
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test24
   * @tc.desc      Test set property throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test24', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = 'Hello World!';
    const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, textData);
    try {
      const obj = { shareOption: 1 };
      pasteData.setProperty(obj);
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '401').assertTrue();
      expect(e.message === 'Parameter error. The type of property must be PasteDataProperty.').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test25
   * @tc.desc      Test createData throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test25', 0, async function (done) {
    const textData = 'Hello World!';
    const dataXml = new ArrayBuffer(512);
    try {
      const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, dataXml);
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '401').assertTrue();
      expect(e.message === 'Parameter error. The type of value must be string.').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test26
   * @tc.desc      Test createData throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test26', 0, async function (done) {
    const textData = 'Hello World!';
    const dataXml = new ArrayBuffer(512);
    try {
      const pasteData = pasteboard.createData('xxxxx', textData);
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '401').assertTrue();
      expect(e.message === 'Parameter error. The mimeType is not an arraybuffer.').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test27
   * @tc.desc      Test createData throw error
   * @tc.type      Function
   * @tc.require   I5TYVJ
   */
  it('pasteboard_exception_test27', 0, async function (done) {
    try {
      const pasteData = pasteboard.createData(pasteboard.MIMETYPE_PIXELMAP, {});
      expect(true === false).assertTrue();
    } catch (e) {
      expect(e.code === '401').assertTrue();
      expect(e.message === 'Parameter error. Actual mimeType is not mimetype_pixelmap.').assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test28
   * @tc.desc      异常值 非数组
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_exception_test28', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "部分人的十点半：\n" +
    "https://pr5yyye-drseyive.u54yk.cwerfe/s/42e1ewed77f3dab4" +
    "网gest加尔文iqru发的我ui哦计划任务i文化人:\n" +
    "~b0043fg3423tddj~";
    const pasteData = pasteboard.createPlainTextData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    const patterns = pasteboard.Pattern.EMAIL_ADDRESS;
    try {
      await systemPasteboard.detectPatterns(patterns);
    } catch (e) {
      expect(e.code == 401).assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test29
   * @tc.desc      异常值 传空
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_exception_test29', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "部分人的十点半：\n" +
    "https://pr5yyye-drseyive.u54yk.cwerfe/s/42e1ewed77f3dab4" +
    "网gest加尔文iqru发的我ui哦计划任务i文化人:\n" +
    "~b0043fg3423tddj~";
    const pasteData = pasteboard.createPlainTextData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    try {
      await systemPasteboard.detectPatterns();
    } catch (e) {
      expect(e.code == 401).assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test30
   * @tc.desc      异常值 数组内元素出错
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_exception_test30', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "部分人的十点半：\n" +
    "https://pr5yyye-drseyive.u54yk.cwerfe/s/42e1ewed77f3dab4" +
    "网gest加尔文iqru发的我ui哦计划任务i文化人:\n" +
    "~b0043fg3423tddj~";
    const pasteData = pasteboard.createPlainTextData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    const patterns = ["dsa", "fdsf", "da"];
    try {
      await systemPasteboard.detectPatterns(patterns);
    } catch (e) {
      expect(e.code == 401).assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test31
   * @tc.desc      异常值 参数个数异常
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_exception_test31', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "部分人的十点半：\n" +
    "https://pr5yyye-drseyive.u54yk.cwerfe/s/42e1ewed77f3dab4" +
    "网gest加尔文iqru发的我ui哦计划任务i文化人:\n" +
    "~b0043fg3423tddj~";
    const pasteData = pasteboard.createPlainTextData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    const patterns1 = [0, 1];
    const patterns2 = [1, 2];
    try {
      await systemPasteboard.detectPatterns(patterns1, patterns2);
    } catch (e) {
      expect(e.code == 401).assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test32
   * @tc.desc      异常值-非预期数字数组
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_exception_test32', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "<!DOCTYPE html><html><head><title>" +
    "，尽快改好Greg就就。、</title></head><body><h2>访如果如果</h2>" +
    "<p>搞了个<a href=\"https://grehtjeffxample.com\">" +
    "剖一个v给ioadhoa@wdoiewf.com</a>。</p></body></html>";
    const pasteData = pasteboard.createHtmlData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    const patterns1 = [0, 1, 23789, 238];
    try {
      await systemPasteboard.detectPatterns(patterns1);
    } catch (e) {
      expect(e.code == 401).assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test33
   * @tc.desc      异常值-空数组
   * @tc.type      Function
   * @tc.require   AR000H5HVI
   */
  it('pasteboard_exception_test33', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();
    const textData = "<!DOCTYPE html><html><head><title>" +
    "，尽快改好Greg就就。、</title></head><body><h2>访如果如果</h2>" +
    "<p>搞了个<a href=\"https://grehtjeffxample.com\">" +
    "剖一个v给ioadhoa@wdoiewf.com</a>。</p></body></html>";
    const pasteData = pasteboard.createHtmlData(textData);
    await systemPasteboard.setPasteData(pasteData);
    const res = await systemPasteboard.hasPasteData();
    expect(res).assertEqual(true);
    const patterns1 = [];
    try {
      await systemPasteboard.detectPatterns(patterns1);
    } catch (e) {
      expect(e.code == 401).assertTrue();
    }
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test34
   * @tc.desc      createData(Record) exception param.
   * @tc.type      Function
   * @tc.require   API 14
   */
  it('pasteboard_exception_test34', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();

    let record = await initRecordData();
    let exceptionRecord = {}
    exceptionRecord[pasteboard.MIMETYPE_TEXT_PLAIN] = record[pasteboard.MIMETYPE_PIXELMAP];
    await systemPasteboard.setData(pasteboard.createData(exceptionRecord));

    const outData = await systemPasteboard.getData();
    expect(outData.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_PLAIN);
    const outValue = outData.getPrimaryText();
    expect(outValue).assertUndefined();

    let exceptionRecord1 = {}
    exceptionRecord1[pasteboard.MIMETYPE_TEXT_WANT] = record[pasteboard.MIMETYPE_PIXELMAP];
    await systemPasteboard.setData(pasteboard.createData(exceptionRecord1));

    const outData1 = await systemPasteboard.getData();
    expect(outData1.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_WANT);
    const outValue1 = outData.getPrimaryWant();
    expect(outValue1).assertUndefined();

    let exceptionRecord2 = {}
    exceptionRecord2[pasteboard.MIMETYPE_TEXT_WANT] = record[pasteboard.MIMETYPE_TEXT_HTML];
    exceptionRecord2[pasteboard.MIMETYPE_PIXELMAP] = record[pasteboard.MIMETYPE_TEXT_URI];
    exceptionRecord2[pasteboard.MIMETYPE_TEXT_PLAIN] = record[myType];
    await systemPasteboard.setData(pasteboard.createData(exceptionRecord2));

    const outData2 = await systemPasteboard.getData();
    expect(outData2.getRecordCount()).assertEqual(1);
    const outRecord = outData2.getRecord(0);
    console.log('getValidTypes: ' + outRecord.getValidTypes(allTypes).toString());
    expect(outRecord.getValidTypes(allTypes).toString()).assertEqual(
      [pasteboard.MIMETYPE_TEXT_PLAIN, pasteboard.MIMETYPE_TEXT_WANT, pasteboard.MIMETYPE_PIXELMAP].toString()
    );

    await systemPasteboard.clearData();
    done();
  });

  /**
   * @tc.name      pasteboard_exception_test35
   * @tc.desc      addEntry exception param.
   * @tc.type      Function
   * @tc.require   API 14
   */
  it('pasteboard_exception_test35', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    await systemPasteboard.clearData();

    const record = await initRecordData();
    const pasteRecord =
      pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_PLAIN, record[pasteboard.MIMETYPE_TEXT_PLAIN]);
    try {
      pasteRecord.addEntry(pasteboard.MIMETYPE_TEXT_URI, record[pasteboard.MIMETYPE_PIXELMAP]);
    } catch (err) {
      expect(err.code).assertEqual('401');
    }

    try {
      pasteRecord.addEntry(pasteboard.MIMETYPE_PIXELMAP, record[pasteboard.MIMETYPE_TEXT_PLAIN]);
    } catch (err) {
      expect(err.code).assertEqual('401');
    }

    try {
      pasteRecord.addEntry(pasteboard.MIMETYPE_TEXT_PLAIN, record[myType]);
    } catch (err) {
      expect(err.code).assertEqual('401');
    }

    try {
      pasteRecord.addEntry(myType, record[pasteboard.MIMETYPE_TEXT_PLAIN]);
    } catch (err) {
      expect(err.code).assertEqual('401');
    }

    await systemPasteboard.clearData();
    done();
  });

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
});
