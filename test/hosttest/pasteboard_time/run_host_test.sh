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
# Host-side build + run + coverage loop for the PasteBoardTime module.
# Shallow-dependency module: the only non-stdlib include is c_utils' singleton.h
# (header-only, host-compilable), reached via an include path -- no shim needed.
# See ../serializable/README.md for the pattern and toolchain notes.
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
CUTILS_INC="${CODE_ROOT}/commonlibrary/c_utils/base/include"
TIME_INC="${PASTEBOARD_ROOT}/utils/native/include"
TIME_SRC="${PASTEBOARD_ROOT}/utils/native/src/pasteboard_time.cpp"
TEST_SRC="${SCRIPT_DIR}/pasteboard_time_host_test.cpp"

BUILD_DIR="${SCRIPT_DIR}/.build"
BIN="${BUILD_DIR}/pasteboard_time_host_test"

fail() { echo "[FAIL] $*" >&2; }
info() { echo "[INFO] $*"; }

for tool in "${CXX}" "${GCOV}"; do
    command -v "${tool}" >/dev/null 2>&1 || { fail "required tool not found: ${tool}"; exit 3; }
done
for f in "${GTEST_ROOT}/src/gtest-all.cc" "${TIME_SRC}" "${TEST_SRC}" "${CUTILS_INC}/singleton.h"; do
    [[ -f "${f}" ]] || { fail "missing source: ${f}"; exit 3; }
done

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

UUT_INC=(-I"${TIME_INC}" -I"${CUTILS_INC}")

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

info "compiling pasteboard_time.cpp (WITH coverage)"
( cd "${BUILD_DIR}" && "${CXX}" -c "${TIME_SRC}" "${UUT_INC[@]}" \
    -std=c++17 -O0 -g --coverage -o pasteboard_time.o ) \
    || { fail "unit-under-test compile failed"; exit 3; }

info "compiling test"
"${CXX}" -c "${TEST_SRC}" "${UUT_INC[@]}" -I"${GTEST_ROOT}/include" \
    -std=c++17 -O0 -g -o "${BUILD_DIR}/test.o" || { fail "test compile failed"; exit 3; }

info "linking"
"${CXX}" --coverage \
    "${BUILD_DIR}/test.o" "${BUILD_DIR}/pasteboard_time.o" \
    "${BUILD_DIR}/gtest-all.o" "${BUILD_DIR}/gtest_main.o" \
    -lpthread -o "${BIN}" || { fail "link failed"; exit 3; }

info "running tests"
"${BIN}" --gtest_color=yes --gtest_output=
TEST_RC=$?
[[ ${TEST_RC} -eq 0 ]] || { fail "unit tests failed (rc=${TEST_RC})"; exit 1; }

info "computing coverage"
COV_LINE="$( cd "${BUILD_DIR}" && "${GCOV}" -n pasteboard_time.gcno 2>/dev/null \
    | grep -A1 "pasteboard_time.cpp'" | grep "Lines executed" | head -1 )"
echo "  ${COV_LINE}"
LINE_COV="$(echo "${COV_LINE}" | grep -oE "[0-9]+\.[0-9]+" | head -1)"

[[ -n "${LINE_COV}" ]] || { fail "could not parse coverage output"; exit 3; }
info "pasteboard_time.cpp line coverage: ${LINE_COV}% (min ${COVERAGE_MIN}%)"

if awk "BEGIN{exit !(${LINE_COV} >= ${COVERAGE_MIN})}"; then
    echo "[PASS] tests green and coverage ${LINE_COV}% >= ${COVERAGE_MIN}%"
    exit 0
else
    fail "coverage ${LINE_COV}% below gate ${COVERAGE_MIN}%"
    exit 2
fi
