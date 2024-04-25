/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'
import pasteboard from '@ohos.pasteboard'
import UDC from '@ohos.data.unifiedDataChannel';

var plainTextData = new UDC.UnifiedData();
var sumGetUnifiedDataTime = 0;

const BASE_CONUT = 100;

describe('PasteBoardPerfJSTest', function () {
    beforeAll(async function () {
        console.info('beforeAll');
    })

    afterAll(async function () {
        console.info('afterAll');
    })

    function computeAverageTime(startTime, baseCount, message) {
        let endTime = new Date().getTime();
        let averageTime = ((endTime - startTime) * 1000) / baseCount;
        console.info(message + averageTime);
    }

    function computeAverageGetTime(baseCount, message) {
        let averageTime = (sumGetUnifiedDataTime * 1000) / baseCount;
        console.info(message + averageTime);
    }

    function getPlainTextData(dataType) {
        let plainText = new UDC.PlainText();
        plainText.details = {
            Key: 'plainText',
            Value: 'plainText',
        };
        plainText.textContent = 'textContent';
        plainText.abstract = 'abstract';
        plainTextData.addRecord(plainText);
        return plainTextData;
    }

    /**
     * @tc.name      setUnifiedData_delay_performance_test_001
     * @tc.desc      setUnifiedData delay interface performance test
     * @tc.type      PERFORMANCE
     */
    it('setUnifiedData_delay_performance_test_001', 0, async function (done) {
        console.info('setUnifiedData_delay_performance_test_001 begin');
        const systemPasteboard = pasteboard.getSystemPasteboard();
        let properties = new UDC.UnifiedDataProperties();
        properties.getDelayData = getPlainTextData;
        plainTextData.properties = properties;
        await systemPasteboard.setUnifiedData(plainTextData);
        function setUnifiedDataDelayPerfTest(index) {
            systemPasteboard.setUnifiedData(plainTextData).then(() => {
                if (index < BASE_CONUT) {
                    setUnifiedDataDelayPerfTest(index + 1);
                } else {
                    computeAverageTime(startTime, BASE_CONUT, "setUnifiedData_delay_performance_test_001 averageTime:");
                    done();
                }
            });
        }
        let startTime = new Date().getTime();
        setUnifiedDataDelayPerfTest(0);
        console.info('setUnifiedData_delay_performance_test_001 end');
    })

    /**
     * @tc.name      getUnifiedData_delay_performance_test_001
     * @tc.desc      getUnifiedData delay interface performance test
     * @tc.type      PERFORMANCE
     */
    it('getUnifiedData_delay_performance_test_001', 0, async function (done) {
        console.info('getUnifiedData_delay_performance_test_001 begin');
        const systemPasteboard = pasteboard.getSystemPasteboard();
        let properties = new UDC.UnifiedDataProperties();
        properties.getDelayData = getPlainTextData;
        plainTextData.properties = properties;
        await systemPasteboard.setUnifiedData(plainTextData);
        function getUnifiedDataDelayPerfTest(index) {
            systemPasteboard.setUnifiedData(plainTextData).then(() => {
                let beginGetTime = new Date().getTime();
                systemPasteboard.getUnifiedData().then((data) => {
                    let endGetTime = new Date().getTime();
                    sumGetUnifiedDataTime += (endGetTime - beginGetTime);
                });
                if (index < BASE_CONUT) {
                    getUnifiedDataDelayPerfTest(index + 1);
                } else {
                    computeAverageTime(BASE_CONUT, "getUnifiedData_delay_performance_test_001 averageTime:");
                    done();
                }
            });
        }
        let startTime = new Date().getTime();
        getUnifiedDataDelayPerfTest(0);
        console.info('getUnifiedData_delay_performance_test_001 end');
    })
});