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
  beforeAll(async function () {
    console.info('beforeAll');
  });

  afterAll(async function () {
    console.info('afterAll');
  });

  /**
   * @tc.name      pasteboard_callback_test1
   * @tc.desc      createPlainTextData test
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test1', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test1: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData1 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData1);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test1: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test1: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test1: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const primaryText1 = data.getPrimaryText();
            expect(primaryText1).assertEqual(textData1);
            expect(pasteboard.MAX_RECORD_NUM).assertEqual(512);
            expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_PLAIN)).assertEqual(true);
            expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_PLAIN);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test2
   * @tc.desc      createHtmlData test
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test2', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test2: systemPasteboard.clear callback error:' + err);
        return;
      }
      const htmlText2 = '<html><head></head><body>Hello World!</body></html>';
      const pasteData = pasteboard.createHtmlData(htmlText2);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test2: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test2: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test2: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const PrimaryHtml2 = data.getPrimaryHtml();
            console.info('f_test2: PrimaryHtml = ' + PrimaryHtml2);
            expect(PrimaryHtml2).assertEqual(htmlText2);
            expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
            expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test3
   * @tc.desc      测试异步callback调用+createHtmlData,htmlText = ''.
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test3', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test3: systemPasteboard.clear callback error:' + err);
        return;
      }
      const htmlText3 = '';
      const pasteData = pasteboard.createHtmlData(htmlText3);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test3: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test3: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test3: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const PrimaryHtml3 = data.getPrimaryHtml();
            console.info('f_test3: PrimaryHtml = ' + PrimaryHtml3);
            expect(PrimaryHtml3).assertEqual(htmlText3);
            expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_HTML)).assertEqual(true);
            expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_HTML);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test4
   * @tc.desc      createUriData test
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test4', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test4: systemPasteboard.clear callback error:' + err);
        return;
      }
      const uriText4 = '';
      const pasteData = pasteboard.createUriData(uriText4);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test4: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test4: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test4: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const PrimaryUri4 = data.getPrimaryUri();
            console.info('f_test4: PrimaryUri = ' + PrimaryUri4);
            expect(PrimaryUri4).assertEqual(uriText4);
            expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_URI)).assertEqual(true);
            expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_URI);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test5
   * @tc.desc      createWantData test
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test5', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test5: systemPasteboard.clear callback error:' + err);
        return;
      }
      const want5 = {
        bundleName: 'com.example.myapplication8',
        abilityName: 'com.example.myapplication8.MainAbility',
      };
      const pasteData = pasteboard.createWantData(want5);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test5: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test5: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test5: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const PrimaryWant5 = data.getPrimaryWant();
            expect(PrimaryWant5.bundleName).assertEqual(want5.bundleName);
            expect(PrimaryWant5.abilityName).assertEqual(want5.abilityName);
            expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_WANT)).assertEqual(true);
            expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_WANT);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test6
   * @tc.desc      addTextRecord test
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test6', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test6: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData6 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData6);
      const textData62 = 'Hello World1';
      pasteData.addTextRecord(textData62);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test6: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test6: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test6: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(2);
            const PrimaryText6 = data.getPrimaryText();
            expect(PrimaryText6).assertEqual(textData62);
            expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_PLAIN)).assertEqual(true);
            expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_PLAIN);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test7
   * @tc.desc      addTextRecord test
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test7', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test7: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData71 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData71);
      let textData7 = '';
      for (let i = 1; i < 15; i++) {
        textData7 = 'Hello World';
        textData7 = textData7 + i;
        pasteData.addTextRecord(textData7);
      }
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test7: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test7: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test7: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(15);
            const PrimaryText7 = data.getPrimaryText();
            expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_PLAIN)).assertEqual(true);
            expect(data.getPrimaryMimeType()).assertEqual(pasteboard.MIMETYPE_TEXT_PLAIN);
            expect(PrimaryText7).assertEqual('Hello World14');
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test8
   * @tc.desc      addHtmlRecord+addUriRecord+addWantRecord
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test8', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test8: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData8 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData8);
      const htmlText8 = '<html><head></head><body>Hello World!</body></html>';
      pasteData.addHtmlRecord(htmlText8);
      const uriText8 = '';
      pasteData.addUriRecord(uriText8);
      const want8 = {
        bundleName: 'com.example.myapplication8',
        abilityName: 'com.example.myapplication8.MainAbility',
      };
      pasteData.addWantRecord(want8);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test8: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test8: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test8: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(4);
            const MimeTypes8 = data.getMimeTypes();
            expect(MimeTypes8.length).assertEqual(4);
            const expectedMimeTypes = new Set([pasteboard.MIMETYPE_TEXT_PLAIN, pasteboard.MIMETYPE_TEXT_HTML,
              pasteboard.MIMETYPE_TEXT_URI, pasteboard.MIMETYPE_TEXT_WANT]);
            expect(Array.from(MimeTypes8).every(type => expectedMimeTypes.has(type))).assertEqual(true);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test9
   * @tc.desc      addHtmlRecord+addUriRecord+removeRecordAt
   * @tc.type      Function
   * @tc.require   AR000HEECD
   */
  it('pasteboard_callback_test9', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test9: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData9 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData9);
      const htmlText9 = '<html><head></head><body>Hello World!</body></html>';
      pasteData.addHtmlRecord(htmlText9);
      const uriText9 = '';
      pasteData.addUriRecord(uriText9);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test9: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test9: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test9: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(3);
            expect(data.removeRecordAt(0)).assertEqual(true);
            expect(data.getRecordCount()).assertEqual(2);
            systemPasteboard.setPasteData(data, (err, newData) => {
              if (err) {
                console.error('f_test9: systemPasteboard.setPasteData callback error:' + err);
                return;
              }
              systemPasteboard.getPasteData((err, data) => {
                if (err) {
                  console.error('f_test9: systemPasteboard.getPasteData callback error:' + err);
                  return;
                }
                expect(data.getRecordCount()).assertEqual(2);
                done();
              });
            });
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test10
   * @tc.desc      Add 30 TextRecords
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test10', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test10: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData101 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData101);
      let textData10 = '';
      for (let i = 1; i < 30; i++) {
        textData10 = 'Hello World';
        textData10 = textData10 + i;
        pasteData.addTextRecord(textData10);
      }
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test10: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test10: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test10: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(30);
            for (let i = 0; i < 30; i++) {
              expect(data.removeRecordAt(0)).assertEqual(true);
            }
            expect(data.getRecordCount()).assertEqual(0);
            systemPasteboard.setPasteData(data, (err, newData) => {
              if (err) {
                console.error('f_test10: systemPasteboard.setPasteData callback error:' + err);
                return;
              }
              systemPasteboard.getPasteData((err, data) => {
                if (err) {
                  console.error('f_test10: systemPasteboard.getPasteData callback error:' + err);
                  return;
                }
                expect(data.getRecordCount() == 0).assertEqual(true);
                done();
              });
            });
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test11
   * @tc.desc      Replace textRecord
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test11', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test11: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData11 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData11);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test11: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test11: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test11: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const textData111 = 'Hello World1';
            const pasteDataRecord11 = pasteboard.createPlainTextRecord(textData111);
            const replace11 = data.replaceRecordAt(0, pasteDataRecord11);
            expect(replace11).assertEqual(true);
            const primaryText11 = data.getPrimaryText();
            expect(primaryText11).assertEqual(textData111);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test12
   * @tc.desc      Replace htmlRecord
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test12', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test12: systemPasteboard.clear callback error:' + err);
        return;
      }
      const htmlText12 = '<html><head></head><body>Hello World!</body></html>';
      const pasteData = pasteboard.createHtmlData(htmlText12);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test12: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test12: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test12: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const htmlText121 = '<html><head></head><body>Hello World 1</body></html>';
            const pasteDataRecord12 = pasteboard.createHtmlTextRecord(htmlText121);
            const replace12 = data.replaceRecordAt(0, pasteDataRecord12);
            expect(replace12).assertEqual(true);
            expect(data.getRecordCount()).assertEqual(1);
            const primaryHtml12 = data.getPrimaryHtml();
            expect(primaryHtml12).assertEqual(htmlText121);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test13
   * @tc.desc      Replace wantRecord
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test13', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test13: systemPasteboard.clear callback error:' + err);
        return;
      }
      const wantText13 = {
        bundleName: 'com.example.myapplication3',
        abilityName: 'com.example.myapplication3.MainAbility',
      };
      const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_WANT, wantText13);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test13: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test13: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test13: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const getWantText13 = {
              bundleName: 'com.example.myapplication30',
              abilityName: 'com.example.myapplication30.MainAbility',
            };
            const pasteDataRecord13 = pasteboard.createRecord(pasteboard.MIMETYPE_TEXT_WANT, getWantText13);
            const replace13 = data.replaceRecordAt(0, pasteDataRecord13);
            expect(replace13).assertEqual(true);
            const primaryWant13 = data.getPrimaryWant();
            expect(data.hasMimeType(pasteboard.MIMETYPE_TEXT_WANT)).assertEqual(true);
            expect(primaryWant13.bundleName).assertEqual(getWantText13.bundleName);
            expect(primaryWant13.abilityName).assertEqual(getWantText13.abilityName);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test14
   * @tc.desc      get pasteData after clear wantData
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test14', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test14: systemPasteboard.clear callback error:' + err);
        return;
      }
      const wantText14 = {
        bundleName: 'com.example.myapplication3',
        abilityName: 'com.example.myapplication3.MainAbility',
      };
      const pasteData = pasteboard.createWantData(wantText14);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test14: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test14: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test14: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            systemPasteboard.clear((err, data) => {
              if (err) {
                console.error('f_test14: systemPasteboard.clear callback error:' + err);
                return;
              }
              systemPasteboard.getPasteData((err, data) => {
                if (err) {
                  console.error('f_test14: systemPasteboard.getPasteData callback error:' + err);
                  return;
                }
                expect(data.getRecordCount()).assertEqual(0);
                done();
              });
            });
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test15
   * @tc.desc      getProperty and setProperty test
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test15', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test15: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData15 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData15);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test15: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test15: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test15: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const pasteDataProperty15 = data.getProperty();
            expect(pasteDataProperty15.shareOption).assertEqual(pasteboard.ShareOption.CrossDevice);
            pasteDataProperty15.shareOption = pasteboard.ShareOption.InApp;
            data.setProperty(pasteDataProperty15);
            const getPasteDataProperty15 = data.getProperty();
            expect(getPasteDataProperty15.shareOption).assertEqual(pasteboard.ShareOption.InApp);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test16
   * @tc.desc      on test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test16', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test16: systemPasteboard.clear callback error:' + err);
        return;
      }
      systemPasteboard.on('update', contentChanges);
      const textData16 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData16);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test16: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test16: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data === true).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test16: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount() == 1).assertEqual(true);
            expect(data.removeRecordAt(0)).assertEqual(true);
            expect(data.getRecordCount() == 0).assertEqual(true);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test17
   * @tc.desc      off test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test17', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test17: systemPasteboard.clear callback error:' + err);
        return;
      }
      systemPasteboard.off('update', contentChanges);
      const textData17 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData17);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test17: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test17: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test17: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            expect(data.removeRecordAt(0)).assertEqual(true);
            expect(data.getRecordCount()).assertEqual(0);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test18
   * @tc.desc      createRecord test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test18', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear().then(() => {
      const buffer18 = new ArrayBuffer(128);
      const opt18 = {
        size: { height: 5, width: 5 },
        pixelFormat: 3,
        editable: true,
        alphaType: 1,
        scaleMode: 1,
      };
      const pasteData = pasteboard.createHtmlData('application/xml');
      image.createPixelMap(buffer18, opt18).then((pixelMap) => {
        expect(pixelMap.getPixelBytesNumber()).assertEqual(100);
        const pixelMapRecord18 = pasteboard.createRecord(pasteboard.MIMETYPE_PIXELMAP, pixelMap);
        pasteData.addRecord(pixelMapRecord18);
        systemPasteboard.setPasteData(pasteData).then(() => {
          systemPasteboard.hasPasteData().then((data) => {
            expect(data).assertEqual(true);
            systemPasteboard.getPasteData().then((newPasteData) => {
              const recordCount18 = newPasteData.getRecordCount();
              expect(recordCount18).assertEqual(2);
              const newPixelMap18 = newPasteData.getPrimaryPixelMap();
              const PixelMapBytesNumber18 = newPixelMap18.getPixelBytesNumber();
              expect(PixelMapBytesNumber18).assertEqual(100);
              const newHtmlData18 = newPasteData.getRecordAt(1);
              expect(newHtmlData18.htmlText).assertEqual('application/xml');
              newPixelMap18.getImageInfo().then((imageInfo) => {
                expect(imageInfo.size.height === 5 && imageInfo.size.width === 5).assertEqual(true);
                done();
              });
            });
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test19
   * @tc.desc      Add plainText、htmlText、uriText record
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test19', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test19: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData191 = 'Hello World0';
      const pasteData = pasteboard.createPlainTextData(textData191);
      let textData19 = '';
      for (let i = 1; i < 5; i++) {
        textData19 = 'Hello World';
        textData19 = textData19 + i;
        pasteData.addTextRecord(textData19);
      }
      let htmlText19 = '';
      for (let i = 0; i < 5; i++) {
        htmlText19 = '<html><head></head><body>Hello World!</body></html>';
        htmlText19 = htmlText19 + i;
        pasteData.addHtmlRecord(htmlText19);
      }
      let uriText19 = '';
      for (let i = 0; i < 5; i++) {
        uriText19 = 'https://www.baidu.com/';
        uriText19 = uriText19 + i;
        pasteData.addUriRecord(uriText19);
      }
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test19: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test19: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData(async (err, data) => {
            if (err) {
              console.error('f_test19: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(15);
            await systemPasteboard.clear();
            let newData19 = await systemPasteboard.getPasteData();
            expect(newData19.getRecordCount()).assertEqual(0);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test20
   * @tc.desc      convertToText test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test20', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test20: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData20 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData20);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test20: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test20: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData(async (err, data) => {
            if (err) {
              console.error('f_test20: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const dataRecord20 = data.getRecordAt(0);
            const text = await dataRecord20.convertToText();
            expect(text).assertEqual(textData20);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test21
   * @tc.desc      convertToText test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test21', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test21: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData21 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData21);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test21: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test21: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test21: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const dataRecord21 = data.getRecordAt(0);
            dataRecord21.convertToText((err, data) => {
              if (err) {
                console.error('f_test21: PasteDataRecord.convertToText callback error:' + err);
                return;
              }
              expect(data).assertEqual(textData21);
              done();
            });
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test22
   * @tc.desc      convertToText test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test22', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test22: systemPasteboard.clear callback error:' + err);
        return;
      }
      const textData22 = 'Hello 中国!@#$%^&*()_+{}\\?.';
      const pasteData = pasteboard.createPlainTextData(textData22);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test22: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test22: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData(async (err, data) => {
            if (err) {
              console.error('f_test22: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const dataRecord22 = data.getRecordAt(0);
            const text = await dataRecord22.convertToText();
            expect(text).assertEqual(textData22);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test23
   * @tc.desc      convertToText test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test23', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test23: systemPasteboard.clear callback error:' + err);
        return;
      }
      const uriText23 = 'https://www.baidu.com/';
      const pasteData = pasteboard.createUriData(uriText23);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test23: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test23: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData(async (err, data) => {
            if (err) {
              console.error('f_test23: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            const dataRecord23 = data.getRecordAt(0);
            const text = await dataRecord23.convertToText();
            expect(text).assertEqual(uriText23);
            done();
          });
        });
      });
    });
  });

  /**
   *  The callback function is used for pasteboard content changes
   */
  function contentChanges() {
    console.info('#EVENT: The content is changed in the pasteboard');
  }

  /**
   * @tc.name      pasteboard_callback_test24
   * @tc.desc      onRemoteUpdate test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test24', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test24: systemPasteboard.clear callback error:' + err);
        return;
      }
      systemPasteboard.onRemoteUpdate(contentChanges);
      const textData16 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData16);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test24: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test24: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data === true).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test24: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount() == 1).assertEqual(true);
            expect(data.removeRecordAt(0)).assertEqual(true);
            expect(data.getRecordCount() == 0).assertEqual(true);
            done();
          });
        });
      });
    });
  });

  /**
   * @tc.name      pasteboard_callback_test17
   * @tc.desc      offRemoteUpdate test
   * @tc.type      Function
   * @tc.require   AR000H5GKU
   */
  it('pasteboard_callback_test25', 0, async function (done) {
    const systemPasteboard = pasteboard.getSystemPasteboard();
    systemPasteboard.clear((err, data) => {
      if (err) {
        console.error('f_test25: systemPasteboard.clear callback error:' + err);
        return;
      }
      systemPasteboard.offRemoteUpdate(contentChanges);
      const textData17 = 'Hello World!';
      const pasteData = pasteboard.createPlainTextData(textData17);
      systemPasteboard.setPasteData(pasteData, (err, data) => {
        if (err) {
          console.error('f_test25: systemPasteboard.setPasteData callback error:' + err);
          return;
        }
        systemPasteboard.hasPasteData((err, data) => {
          if (err) {
            console.error('f_test25: systemPasteboard.hasPasteData callback error:' + err);
            return;
          }
          expect(data).assertEqual(true);
          systemPasteboard.getPasteData((err, data) => {
            if (err) {
              console.error('f_test25: systemPasteboard.getPasteData callback error:' + err);
              return;
            }
            expect(data.getRecordCount()).assertEqual(1);
            expect(data.removeRecordAt(0)).assertEqual(true);
            expect(data.getRecordCount()).assertEqual(0);
            done();
          });
        });
      });
    });
  });
});
