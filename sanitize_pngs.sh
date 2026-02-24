#!/bin/bash
set -e

ASSETS="${1:-assets}"
OUT="${2:-sce_sys}"

mkdir -p "$OUT/livearea/contents"

sanitize() {
    local input="$1" output="$2" w="$3" h="$4"

    if [ ! -f "$input" ]; then
        echo "SKIP: $input (not found)"
        return 0
    fi

    echo "Processing: $input → $output (${w}x${h})"

    # Step 1: ffmpeg — resize + strip metadata (always available on GH runners)
    ffmpeg -y -loglevel error \
        -i "$input" \
        -vf "scale=${w}:${h}:flags=lanczos" \
        -pix_fmt rgb24 \
        -update 1 \
        "$output"

    # Step 2: pngquant — strip ALL chunks, force clean PNG
    if command -v pngquant &>/dev/null; then
        pngquant --force --quality=95-100 --strip \
            --output "${output}.tmp" -- "$output" 2>/dev/null || true
        if [ -f "${output}.tmp" ]; then
            mv "${output}.tmp" "$output"
        fi
    fi

    # Step 3: optipng — final cleanup
    if command -v optipng &>/dev/null; then
        optipng -strip all -quiet "$output" 2>/dev/null || true
    fi

    local size
    size=$(stat -c%s "$output" 2>/dev/null || stat -f%z "$output" 2>/dev/null || echo "?")
    echo "  OK: $size bytes"
}

# Vita required dimensions
sanitize "$ASSETS/icon0.png"   "$OUT/icon0.png"                       128  128
sanitize "$ASSETS/bg.png"      "$OUT/livearea/contents/bg.png"        840  500
sanitize "$ASSETS/startup.png" "$OUT/livearea/contents/startup.png"   280  158
sanitize "$ASSETS/pic0.png"    "$OUT/pic0.png"                        960  544

echo ""
echo "=== PNG sanitization complete ==="
find "$OUT" -name "*.png" -exec file {} \;
