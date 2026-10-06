#!/usr/bin/env bash
# times the final scene 4 ways and prints a markdown table
# usage   scripts/bench.sh [width] [spp]
set -euo pipefail

cd "$(dirname "$0")/.."
WIDTH="${1:-400}"
SPP="${2:-32}"
BIN=./build/raytracer
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

if [ ! -x "$BIN" ]; then
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release > /dev/null
  cmake --build build -j > /dev/null
fi

if command -v nproc > /dev/null; then CORES=$(nproc); else CORES=$(sysctl -n hw.ncpu); fi

# prints "total min_busy max_busy" for one config
run() {
  "$BIN" --width "$WIDTH" --spp "$SPP" --quiet --out "$OUT/img.ppm" "$@" \
    | sed -n 's/Rendered in \([0-9.]*\) s\. Thread busy time min \([0-9.]*\) s, max \([0-9.]*\) s.*/\1 \2 \3/p'
}

echo "Final scene, ${WIDTH}px wide, ${SPP} samples per pixel, ${CORES} hardware threads"
echo
echo "| Configuration | Time (s) | Speedup | Thread busy min / max (s) |"
echo "|---|---|---|---|"

read -r base _ _ <<< "$(run --threads 1 --no-bvh)"
row() {
  local label="$1"; shift
  read -r t lo hi <<< "$(run "$@")"
  printf "| %s | %.2f | %.1fx | %.2f / %.2f |\n" "$label" "$t" "$(awk "BEGIN { print $base / $t }")" "$lo" "$hi"
}

printf "| 1 thread, no BVH | %.2f | 1.0x | |\n" "$base"
row "1 thread, BVH" --threads 1
row "$CORES threads, BVH, equal bands" --threads "$CORES" --schedule bands
row "$CORES threads, BVH, atomic row counter" --threads "$CORES" --schedule dynamic
