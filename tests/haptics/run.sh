#!/bin/sh
# Host simulation of the DRV2605L driver and IQS9151 haptic paths (no Zephyr needed).
set -eu
cd "$(dirname "$0")/../.."
output=$(mktemp /tmp/nickey-haptic-test.XXXXXX)
trap 'rm -f "$output"' EXIT
cc -std=c11 -Wall -Wextra -Werror -Itests/haptics/fakes -Iinclude tests/haptics/test_haptics.c -o "$output"
"$output"
cc -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -Itests/haptics/fakes -Iinclude tests/haptics/test_iqs.c -o "$output"
"$output"
cc -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -DTEST_LEFT -Itests/haptics/fakes -Iinclude tests/haptics/test_iqs.c -o "$output"
"$output"
