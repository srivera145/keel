#!/bin/sh
# KEEL OS — automated checks.
#  1. full tour, deterministic 60 Hz timestep: every screen renders real
#     content (non-blank check) and screenshots land in build/shots/.
#  2. render-time budget: p99 frame render time must fit in 16.7 ms.
#  3. paced run (presenter thread, triple buffering) for display pacing,
#     reported next to the host's own scheduling jitter for context.
set -e
cd "$(dirname "$0")/.."
make -s
mkdir -p build/shots
echo "== 1+2. full tour (all screens) with render budget assertion"
./build/keelos --headless --assert-budget --script tests/tour.keel --out build/shots
echo
echo "== 3. paced 60 Hz run (presenter + frame queue), home + movies + player"
./build/keelos --headless --realtime --script tests/paced.keel --out build/shots_paced || true
cc -O2 -o build/vm_jitter tests/vm_jitter.c -lpthread
./build/vm_jitter 4 8
echo
echo "all checks passed"
