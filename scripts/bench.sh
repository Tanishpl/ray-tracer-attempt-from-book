#!/usr/bin/env bash
# times the final scene 4 ways and prints a markdown table
# usage   scripts/bench.sh [width] [spp]
set -euo pipefail

cd "$(dirname "$0")/.."
WIDTH="${1:-400}"
SPP="${2:-32}"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

# make/ninja put the binary in build/, visual studio puts it in build/Release/
find_bin() {
  for b in build/raytracer build/raytracer.exe build/Release/raytracer.exe; do
    if [ -f "$b" ]; then echo "./$b"; return; fi
  done
}

BIN="$(find_bin)"
if [ -z "$BIN" ]; then
  echo "Building release version..."
  if ! { cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release -j; } > "$OUT/build.log" 2>&1; then
    cat "$OUT/build.log" >&2
    echo "Build failed, see above" >&2
    exit 1
  fi
  BIN="$(find_bin)"
fi
if [ -z "$BIN" ]; then
  echo "Couldn't find the raytracer binary after building" >&2
  exit 1
fi

if command -v nproc > /dev/null; then CORES=$(nproc); else CORES=$(sysctl -n hw.ncpu); fi

# prints "total min_busy max_busy" for one config
run() {
  local line
  line=$("$BIN" --width "$WIDTH" --spp "$SPP" --quiet --out "$OUT/img.ppm" "$@" \
    | sed -n 's/Rendered in \([0-9.]*\) s\. Thread busy time min \([0-9.]*\) s, max \([0-9.]*\) s.*/\1 \2 \3/p')
  if [ -z "$line" ]; then
    echo "Render failed with options $*" >&2
    exit 1
  fi
  echo "$line"
}

row() {
  local label="$1"; shift
  local res t lo hi
  res=$(run "$@")
  read -r t lo hi <<< "$res"
  printf "| %s | %.2f | %.1fx | %.2f / %.2f |\n" "$label" "$t" "$(awk "BEGIN { print $base / $t }")" "$lo" "$hi"
}

echo "Using $BIN"
echo "Final scene, ${WIDTH}px wide, ${SPP} samples per pixel, ${CORES} hardware threads"
echo

res=$(run --threads 1 --no-bvh)
read -r base _ _ <<< "$res"

echo "| Configuration | Time (s) | Speedup | Thread busy min / max (s) |"
echo "|---|---|---|---|"
printf "| 1 thread, no BVH | %.2f | 1.0x | |\n" "$base"
row "1 thread, BVH" --threads 1
row "$CORES threads, BVH, equal bands" --threads "$CORES" --schedule bands
row "$CORES threads, BVH, atomic row counter" --threads "$CORES" --schedule dynamic
