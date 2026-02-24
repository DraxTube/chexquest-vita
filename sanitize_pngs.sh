#!/bin/bash
set -e

ASSETS="${1:-assets}"
OUT="${2:-sce_sys}"

mkdir -p "$OUT/livearea/contents"

sanitize() {
    local input="$1" output="$2" w="$3" h="$4"
    if [ ! -f "$input" ]; then
        echo "SKIP: $input (not found)"
        return 1
    fi
    echo "Processing: $input → $output (${w}x${h})"

    # Passo 1: ImageMagick — ridimensiona, rimuovi TUTTO
    convert "$input" \
        -resize "${w}x${h}!" \
        -strip \
        -interlace none \
        -define png:exclude-chunks=all \
        -define png:include-chunk=none \
        -define png:compression-filter=0 \
        -define png:compression-strategy=0 \
        -define png:compression-level=9 \
        -depth 8 \
        -type TrueColor \
        "PNG24:${output}"

    # Passo 2: optipng — rimuovi qualsiasi chunk rimasto
    if command -v optipng &>/dev/null; then
        optipng -strip all -quiet "$output" 2>/dev/null || true
    fi

    # Passo 3: pngquant (fallback extra sicurezza)
    if command -v pngquant &>/dev/null; then
        pngquant --force --quality=90-100 --strip \
            --output "${output}.tmp" "$output" 2>/dev/null
        if [ -f "${output}.tmp" ]; then
            mv "${output}.tmp" "$output"
        fi
    fi

    local size
    size=$(stat -c%s "$output" 2>/dev/null || stat -f%z "$output" 2>/dev/null || echo "?")
    echo "  OK: $size bytes"
}

# Dimensioni obbligatorie Vita
sanitize "$ASSETS/icon0.png"   "$OUT/icon0.png"                       128  128
sanitize "$ASSETS/startup.png" "$OUT/livearea/contents/startup.png"   280  158
sanitize "$ASSETS/bg.png"      "$OUT/livearea/contents/bg.png"        840  500
sanitize "$ASSETS/pic0.png"    "$OUT/pic0.png"                        960  544

echo ""
echo "=== PNG sanitization complete ==="
echo "Output in: $OUT/"
find "$OUT" -name "*.png" -exec sh -c 'echo "  $(file "$1")"' _ {} \;
