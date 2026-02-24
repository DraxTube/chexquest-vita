#!/bin/bash
set -e

ASSETS="${1:-assets}"
OUT="${2:-sce_sys}"

mkdir -p "$OUT/livearea/contents"

sanitize() {
    local input="$1" output="$2"

    if [ ! -f "$input" ]; then
        echo "SKIP: $input"
        return 0
    fi

    echo "Processing: $input → $output"

    # Copia il file
    cp "$input" "$output"

    # Strip metadata con optipng
    optipng -strip all -quiet "$output"

    # Strip extra con pngquant
    pngquant --force --quality=95-100 --strip \
        --output "${output}.tmp" -- "$output" 2>/dev/null || true
    if [ -f "${output}.tmp" ]; then
        mv "${output}.tmp" "$output"
    fi

    echo "  OK: $(stat -c%s "$output") bytes"
}

sanitize "$ASSETS/icon0.png"   "$OUT/icon0.png"
sanitize "$ASSETS/bg.png"      "$OUT/livearea/contents/bg.png"
sanitize "$ASSETS/startup.png" "$OUT/livearea/contents/startup.png"
sanitize "$ASSETS/pic0.png"    "$OUT/pic0.png"

echo "=== Done ==="
find "$OUT" -name "*.png" -exec file {} \;
