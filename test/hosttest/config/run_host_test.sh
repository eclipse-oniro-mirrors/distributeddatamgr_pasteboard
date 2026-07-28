#!/usr/bin/env bash
#
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Host-side build + run + coverage loop for the Config module.
# "Composition" sample: Config is a Serializable, so this links the real
# config.cpp against the real serializable.cpp + cJSON -- no shim, no fake.
# Coverage is measured on config.cpp only (the unit under test).
#
# Single command:  ./run_host_test.sh
# Exit: 0 pass+coverage ok | 1 test fail | 2 coverage below gate | 3 build error
# Env: COVERAGE_MIN (default 90), CXX (default g++), GCOV (gcov-12)

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CODE_ROOT="$(cd "${SCRIPT_DIR}/../../../../../.." && pwd)"
PASTEBOARD_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

COVERAGE_MIN="${COVERAGE_MIN:-90}"
CXX="${CXX:-g++}"
GCOV="${GCOV:-gcov-12}"

GTEST_ROOT="${CODE_ROOT}/third_party/googletest/googletest"
CJSON_ROOT="${CODE_ROOT}/third_party/cJSON"
CONFIG_INC="${PASTEBOARD_ROOT}/services/load/include"
SER_INC="${PASTEBOARD_ROOT}/framework/framework/include"
CONFIG_SRC="${PASTEBOARD_ROOT}/services/load/src/config.cpp"
SER_SRC="${PASTEBOARD_ROOT}/framework/framework/serializable/serializable.cpp"
TEST_SRC="${SCRIPT_DIR}/config_host_test.cpp"

BUILD_DIR="${SCRIPT_DIR}/.build"
BIN="${BUILD_DIR}/config_host_test"

fail() { echo "[FAIL] $*" >&2; }
info() { echo "[INFO] $*"; }

for tool in "${CXX}" "${GCOV}"; do
    command -v "${tool}" >/dev/null 2>&1 || { fail "required tool not found: ${tool}"; exit 3; }
done
for f in "${GTEST_ROOT}/src/gtest-all.cc" "${CJSON_ROOT}/cJSON.c" "${CONFIG_SRC}" \
         "${SER_SRC}" "${TEST_SRC}"; do
    [[ -f "${f}" ]] || { fail "missing source: ${f}"; exit 3; }
done

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

UUT_INC=(-I"${CONFIG_INC}" -I"${SER_INC}" -I"${CJSON_ROOT}")

info "compiling cJSON (no coverage)"
"${CXX}" -c -x c "${CJSON_ROOT}/cJSON.c" -I"${CJSON_ROOT}" -O0 -g \
    -o "${BUILD_DIR}/cJSON.o" || { fail "cJSON compile failed"; exit 3; }

info "compiling serializable.cpp (dependency, no coverage)"
"${CXX}" -c "${SER_SRC}" -I"${SER_INC}" -I"${CJSON_ROOT}" -std=c++17 -O0 -g \
    -o "${BUILD_DIR}/serializable.o" || { fail "serializable compile failed"; exit 3; }

# googletest is large and identical across suites, so reuse a shared prebuilt
# copy when HOSTTEST_GTEST_CACHE points to one (run_all.sh sets this). Otherwise
# build it here and, if a cache dir is set, populate it for later suites.
if [[ -n "${HOSTTEST_GTEST_CACHE:-}" && -f "${HOSTTEST_GTEST_CACHE}/gtest-all.o" \
      && -f "${HOSTTEST_GTEST_CACHE}/gtest_main.o" ]]; then
    info "reusing cached googletest (${HOSTTEST_GTEST_CACHE})"
    cp "${HOSTTEST_GTEST_CACHE}/gtest-all.o" "${HOSTTEST_GTEST_CACHE}/gtest_main.o" "${BUILD_DIR}/"
else
    info "compiling googletest (no coverage)"
    "${CXX}" -c "${GTEST_ROOT}/src/gtest-all.cc" "${GTEST_ROOT}/src/gtest_main.cc" \
        -I"${GTEST_ROOT}/include" -I"${GTEST_ROOT}" -std=c++17 -O0 -g || \
        { fail "gtest compile failed"; exit 3; }
    mv gtest-all.o gtest_main.o "${BUILD_DIR}/" 2>/dev/null
    if [[ -n "${HOSTTEST_GTEST_CACHE:-}" ]]; then
        mkdir -p "${HOSTTEST_GTEST_CACHE}"
        cp "${BUILD_DIR}/gtest-all.o" "${BUILD_DIR}/gtest_main.o" "${HOSTTEST_GTEST_CACHE}/"
    fi
fi

info "compiling config.cpp (WITH coverage)"
( cd "${BUILD_DIR}" && "${CXX}" -c "${CONFIG_SRC}" "${UUT_INC[@]}" \
    -std=c++17 -O0 -g --coverage -o config.o ) \
    || { fail "unit-under-test compile failed"; exit 3; }

info "compiling test"
"${CXX}" -c "${TEST_SRC}" "${UUT_INC[@]}" -I"${GTEST_ROOT}/include" \
    -std=c++17 -O0 -g -o "${BUILD_DIR}/test.o" || { fail "test compile failed"; exit 3; }

info "linking"
"${CXX}" --coverage \
    "${BUILD_DIR}/test.o" "${BUILD_DIR}/config.o" "${BUILD_DIR}/serializable.o" \
    "${BUILD_DIR}/cJSON.o" "${BUILD_DIR}/gtest-all.o" "${BUILD_DIR}/gtest_main.o" \
    -lpthread -o "${BIN}" || { fail "link failed"; exit 3; }

info "running tests"
"${BIN}" --gtest_color=yes --gtest_output=
TEST_RC=$?
[[ ${TEST_RC} -eq 0 ]] || { fail "unit tests failed (rc=${TEST_RC})"; exit 1; }

info "computing coverage"
COV_LINE="$( cd "${BUILD_DIR}" && "${GCOV}" -n config.gcno 2>/dev/null \
    | grep -A1 "config.cpp'" | grep "Lines executed" | head -1 )"
echo "  ${COV_LINE}"
LINE_COV="$(echo "${COV_LINE}" | grep -oE "[0-9]+\.[0-9]+" | head -1)"

[[ -n "${LINE_COV}" ]] || { fail "could not parse coverage output"; exit 3; }
info "config.cpp line coverage: ${LINE_COV}% (min ${COVERAGE_MIN}%)"

if awk "BEGIN{exit !(${LINE_COV} >= ${COVERAGE_MIN})}"; then
    echo "[PASS] tests green and coverage ${LINE_COV}% >= ${COVERAGE_MIN}%"
    exit 0
else
    fail "coverage ${LINE_COV}% below gate ${COVERAGE_MIN}%"
    exit 2
fi
