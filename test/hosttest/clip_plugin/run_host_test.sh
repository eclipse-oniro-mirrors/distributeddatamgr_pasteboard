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
# Host-side build + run + coverage loop for ClipPlugin / DefaultClip.
# Shallow-dependency module needing a single-header shim (pasteboard_hilog.h,
# pasteboard_event_dfx.h). Links the real serializable.cpp for GlobalEvent's
# Marshal/Unmarshal. Coverage is measured on clip_plugin.cpp + default_clip.cpp.
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
SHIM_INC="${SCRIPT_DIR}/shim"                                    # fake seam (must be first)
CLIP_INC="${PASTEBOARD_ROOT}/framework/framework/clip"
CLIP_PARENT_INC="${PASTEBOARD_ROOT}/framework/framework"       # for "clip/default_clip.h"
FW_INC="${PASTEBOARD_ROOT}/framework/framework/include"
CLIP_SRC="${PASTEBOARD_ROOT}/framework/framework/clip/clip_plugin.cpp"
DEFAULT_SRC="${PASTEBOARD_ROOT}/framework/framework/clip/default_clip.cpp"
SER_SRC="${PASTEBOARD_ROOT}/framework/framework/serializable/serializable.cpp"
TEST_SRC="${SCRIPT_DIR}/clip_plugin_host_test.cpp"

BUILD_DIR="${SCRIPT_DIR}/.build"
BIN="${BUILD_DIR}/clip_plugin_host_test"

fail() { echo "[FAIL] $*" >&2; }
info() { echo "[INFO] $*"; }

for tool in "${CXX}" "${GCOV}"; do
    command -v "${tool}" >/dev/null 2>&1 || { fail "required tool not found: ${tool}"; exit 3; }
done
for f in "${GTEST_ROOT}/src/gtest-all.cc" "${CJSON_ROOT}/cJSON.c" "${CLIP_SRC}" "${DEFAULT_SRC}" \
         "${SER_SRC}" "${TEST_SRC}" "${SHIM_INC}/pasteboard_hilog.h" "${SHIM_INC}/pasteboard_event_dfx.h"; do
    [[ -f "${f}" ]] || { fail "missing source: ${f}"; exit 3; }
done

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

# shim FIRST so it shadows the real hilog/event_dfx headers.
UUT_INC=(-I"${SHIM_INC}" -I"${CLIP_INC}" -I"${CLIP_PARENT_INC}" -I"${FW_INC}" -I"${CJSON_ROOT}")

info "compiling cJSON (no coverage)"
"${CXX}" -c -x c "${CJSON_ROOT}/cJSON.c" -I"${CJSON_ROOT}" -O0 -g \
    -o "${BUILD_DIR}/cJSON.o" || { fail "cJSON compile failed"; exit 3; }

info "compiling serializable.cpp (dependency, no coverage)"
"${CXX}" -c "${SER_SRC}" -I"${FW_INC}" -I"${CJSON_ROOT}" -std=c++17 -O0 -g \
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

info "compiling clip_plugin.cpp + default_clip.cpp (WITH coverage)"
( cd "${BUILD_DIR}" && \
  "${CXX}" -c "${CLIP_SRC}" "${UUT_INC[@]}" -std=c++17 -O0 -g --coverage -o clip_plugin.o && \
  "${CXX}" -c "${DEFAULT_SRC}" "${UUT_INC[@]}" -std=c++17 -O0 -g --coverage -o default_clip.o ) \
    || { fail "unit-under-test compile failed"; exit 3; }

info "compiling test"
"${CXX}" -c "${TEST_SRC}" "${UUT_INC[@]}" -I"${GTEST_ROOT}/include" \
    -std=c++17 -O0 -g -o "${BUILD_DIR}/test.o" || { fail "test compile failed"; exit 3; }

info "linking"
"${CXX}" --coverage \
    "${BUILD_DIR}/test.o" "${BUILD_DIR}/clip_plugin.o" "${BUILD_DIR}/default_clip.o" \
    "${BUILD_DIR}/serializable.o" "${BUILD_DIR}/cJSON.o" \
    "${BUILD_DIR}/gtest-all.o" "${BUILD_DIR}/gtest_main.o" \
    -lpthread -o "${BIN}" || { fail "link failed"; exit 3; }

info "running tests"
"${BIN}" --gtest_color=yes --gtest_output=
TEST_RC=$?
[[ ${TEST_RC} -eq 0 ]] || { fail "unit tests failed (rc=${TEST_RC})"; exit 1; }

info "computing coverage"
total_lines=0
covered_lines=0
for gcno in clip_plugin default_clip; do
    src_base="${gcno}.cpp"
    line="$( cd "${BUILD_DIR}" && "${GCOV}" -n "${gcno}.gcno" 2>/dev/null \
        | grep -A1 "${src_base}'" | grep "Lines executed" | head -1 )"
    pct="$(echo "${line}" | grep -oE "[0-9]+\.[0-9]+" | head -1)"
    n="$(echo "${line}" | grep -oE "of [0-9]+" | grep -oE "[0-9]+")"
    echo "  ${src_base}: ${line}"
    if [[ -n "${pct}" && -n "${n}" ]]; then
        total_lines=$((total_lines + n))
        c="$(awk "BEGIN{printf \"%d\", (${pct}/100.0)*${n} + 0.5}")"
        covered_lines=$((covered_lines + c))
    fi
done

[[ "${total_lines:-0}" -gt 0 ]] || { fail "could not parse coverage output"; exit 3; }

LINE_COV="$(awk "BEGIN{printf \"%.2f\", (${covered_lines}*100.0)/${total_lines}}")"
info "clip_plugin combined line coverage: ${LINE_COV}% (min ${COVERAGE_MIN}%)"

if awk "BEGIN{exit !(${LINE_COV} >= ${COVERAGE_MIN})}"; then
    echo "[PASS] tests green and coverage ${LINE_COV}% >= ${COVERAGE_MIN}%"
    exit 0
else
    fail "coverage ${LINE_COV}% below gate ${COVERAGE_MIN}%"
    exit 2
fi
