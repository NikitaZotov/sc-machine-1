#!/usr/bin/env bash

set -eo pipefail

lcov -c -d . -o coverage.info
lcov -a base_coverage.info -a coverage.info -o coverage.info
lcov -r coverage.info "/usr/*" -o coverage.info
