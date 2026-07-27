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
# Aggregator for all host-side test suites under test/hosttest/.
# This is the intended single verification entry point for an AI change->verify
# loop and for CI: it runs every <module>/run_host_test.sh and fails if any
# suite fails.
#
#   ./run_all.sh
#
# Exit 0 = every suite passed (tests green + coverage gates met).
# Exit 1 = at least one suite failed (see the per-suite output above).
#
# Any env var (e.g. COVERAGE_MIN) is passed through to each suite.
#
# Speed: googletest (~5s to compile) is identical across suites, so this builds
# it ONCE into a temp cache dir and exports HOSTTEST_GTEST_CACHE; each runner
# reuses it instead of recompiling. Set HOSTTEST_GTEST_CACHE yourself to reuse a
# cache across invocations. NOTE: suites share fixed .build/ dirs, so do NOT run
# two aggregator/suite invocations concurrently in the same tree.

set -uo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CODE_ROOT="$(cd "${SCRIPT_DIR}/../../../../.." && pwd)"
GTEST_ROOT="${CODE_ROOT}/third_party/googletest/googletest"

# --- Build the shared googletest cache once (unless caller supplied one) -----
CACHE_OWNED=0
if [[ -z "${HOSTTEST_GTEST_CACHE:-}" ]]; then
    HOSTTEST_GTEST_CACHE="$(mktemp -d)"
    CACHE_OWNED=1
fi
export HOSTTEST_GTEST_CACHE
if [[ ! -f "${HOSTTEST_GTEST_CACHE}/gtest-all.o" || ! -f "${HOSTTEST_GTEST_CACHE}/gtest_main.o" ]]; then
    if [[ -f "${GTEST_ROOT}/src/gtest-all.cc" ]]; then
        echo ">>> prebuilding shared googletest cache (${HOSTTEST_GTEST_CACHE})"
        ( cd "${HOSTTEST_GTEST_CACHE}" && \
          "${CXX:-g++}" -c "${GTEST_ROOT}/src/gtest-all.cc" "${GTEST_ROOT}/src/gtest_main.cc" \
            -I"${GTEST_ROOT}/include" -I"${GTEST_ROOT}" -std=c++17 -O0 -g ) \
          || echo ">>> WARNING: gtest prebuild failed; suites will build it themselves"
    fi
fi

cleanup() { [[ ${CACHE_OWNED} -eq 1 ]] && rm -rf "${HOSTTEST_GTEST_CACHE}"; }
trap cleanup EXIT

declare -a PASSED=()
declare -a FAILED=()

# Discover suites: any immediate subdir containing an executable run_host_test.sh.
for runner in "${SCRIPT_DIR}"/*/run_host_test.sh; do
    [[ -f "${runner}" ]] || continue
    suite="$(basename "$(dirname "${runner}")")"
    echo "============================================================"
    echo ">>> host suite: ${suite}"
    echo "============================================================"
    if bash "${runner}"; then
        PASSED+=("${suite}")
    else
        FAILED+=("${suite} (rc=$?)")
    fi
    echo
done

echo "============================================================"
echo "host test summary"
echo "  passed: ${#PASSED[@]}  [${PASSED[*]:-}]"
echo "  failed: ${#FAILED[@]}  [${FAILED[*]:-}]"
echo "============================================================"

[[ ${#FAILED[@]} -eq 0 ]] && exit 0 || exit 1
