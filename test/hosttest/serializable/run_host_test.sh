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
# Host-side build + run + coverage loop for the Serializable module.
#
# This is the "single command" an AI (or a developer, or CI) runs to verify a
# change to framework/framework/serializable/serializable.cpp without a device:
#
#     ./run_host_test.sh
#
# Exit code 0  => all tests passed AND line coverage >= COVERAGE_MIN.
# Exit code 1  => a test failed.
# Exit code 2  => coverage gate not met.
# Exit code 3  => build/toolchain problem.
#
# Env overrides:
#   COVERAGE_MIN   minimum line-coverage percentage to pass (default 90)
#   CXX            host C++ compiler with gcov support (default g++)
#   GCOV           matching gcov binary (default gcov-12 to match g++-12)
#
# We use the system g++/gcov toolchain, not the OHOS LLVM toolchain under
# /opt/llvm: its coverage runtime is built against OHOS libc and will not link a
# plain host binary. g++ + gcov is the portable host loop.

set -uo pipefail

# --- Locate ourselves and the OHOS source root -------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# .../foundation/distributeddatamgr/pasteboard/test/hosttest/serializable -> code root
CODE_ROOT="$(cd "${SCRIPT_DIR}/../../../../../.." && pwd)"
PASTEBOARD_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

# --- Configurable knobs ------------------------------------------------------
COVERAGE_MIN="${COVERAGE_MIN:-90}"
CXX="${CXX:-g++}"
GCOV="${GCOV:-gcov-12}"

# --- Source locations --------------------------------------------------------
GTEST_ROOT="${CODE_ROOT}/third_party/googletest/googletest"
CJSON_ROOT="${CODE_ROOT}/third_party/cJSON"
SERIALIZABLE_SRC="${PASTEBOARD_ROOT}/framework/framework/serializable/serializable.cpp"
SERIALIZABLE_INC="${PASTEBOARD_ROOT}/framework/framework/include"
API_INC="${PASTEBOARD_ROOT}/framework/framework/include"
TEST_SRC="${SCRIPT_DIR}/serializable_host_test.cpp"

BUILD_DIR="${SCRIPT_DIR}/.build"
BIN="${BUILD_DIR}/serializable_host_test"

fail() { echo "[FAIL] $*" >&2; }
info() { echo "[INFO] $*"; }

# --- Preflight ---------------------------------------------------------------
for tool in "${CXX}" "${GCOV}"; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        fail "required tool not found: ${tool}"
        exit 3
    fi
done
for f in "${GTEST_ROOT}/src/gtest-all.cc" "${CJSON_ROOT}/cJSON.c" "${SERIALIZABLE_SRC}" "${TEST_SRC}"; do
    if [[ ! -f "${f}" ]]; then
        fail "missing source: ${f}"
        exit 3
    fi
done

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

# --- Compile -----------------------------------------------------------------
# Coverage instrumentation (--coverage) is applied ONLY to serializable.cpp (the
# unit under test); gtest and cJSON are compiled without it so they do not
# pollute the coverage numbers or the gate. Building serializable.o inside
# BUILD_DIR keeps the .gcno/.gcda next to it.
info "compiling cJSON (no coverage)"
"${CXX}" -c -x c "${CJSON_ROOT}/cJSON.c" -I"${CJSON_ROOT}" -O0 -g \
    -o "${BUILD_DIR}/cJSON.o" || { fail "cJSON compile failed"; exit 3; }

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

info "compiling serializable.cpp (WITH coverage)"
( cd "${BUILD_DIR}" && "${CXX}" -c "${SERIALIZABLE_SRC}" \
    -I"${SERIALIZABLE_INC}" -I"${API_INC}" -I"${CJSON_ROOT}" \
    -std=c++17 -O0 -g --coverage \
    -o serializable.o ) || { fail "serializable compile failed"; exit 3; }

info "compiling test"
"${CXX}" -c "${TEST_SRC}" \
    -I"${SERIALIZABLE_INC}" -I"${API_INC}" -I"${CJSON_ROOT}" \
    -I"${GTEST_ROOT}/include" -std=c++17 -O0 -g \
    -o "${BUILD_DIR}/test.o" || { fail "test compile failed"; exit 3; }

info "linking"
"${CXX}" --coverage \
    "${BUILD_DIR}/test.o" "${BUILD_DIR}/serializable.o" \
    "${BUILD_DIR}/cJSON.o" "${BUILD_DIR}/gtest-all.o" "${BUILD_DIR}/gtest_main.o" \
    -lpthread -o "${BIN}" || { fail "link failed"; exit 3; }

# --- Run ---------------------------------------------------------------------
info "running tests"
# --gtest_output= (empty) suppresses any inherited/default xml report so we
# don't drop stray files in-tree; --gtest_color=yes keeps the output readable.
"${BIN}" --gtest_color=yes --gtest_output=
TEST_RC=$?
if [[ ${TEST_RC} -ne 0 ]]; then
    fail "unit tests failed (rc=${TEST_RC})"
    exit 1
fi

# --- Coverage ----------------------------------------------------------------
info "computing coverage"
# gcov reads the .gcno (compile) + .gcda (run) beside serializable.o.
COV_LINE="$( cd "${BUILD_DIR}" && "${GCOV}" -n serializable.cpp 2>/dev/null \
    | grep -A1 "serializable.cpp'" | grep "Lines executed" )"
echo "  ${COV_LINE}"

LINE_COV="$(echo "${COV_LINE}" | grep -oE "[0-9]+\.[0-9]+" | head -1)"

if [[ -z "${LINE_COV}" ]]; then
    fail "could not parse coverage output"
    exit 3
fi

info "serializable.cpp line coverage: ${LINE_COV}% (min ${COVERAGE_MIN}%)"

# --- Gate --------------------------------------------------------------------
if awk "BEGIN{exit !(${LINE_COV} >= ${COVERAGE_MIN})}"; then
    echo "[PASS] tests green and coverage ${LINE_COV}% >= ${COVERAGE_MIN}%"
    exit 0
else
    fail "coverage ${LINE_COV}% below gate ${COVERAGE_MIN}%"
    exit 2
fi
