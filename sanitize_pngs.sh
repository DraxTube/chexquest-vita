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

    # Force RGB (no palette!), strip ALL metadata
    python3 -c "
from PIL import Image
img = Image.open('$input')
img = img.convert('RGB')
img.save('$output', 'PNG', optimize=True, pnginfo=None)
"

    # Extra strip with optipng (no pngquant — it creates palette!)
    if command -v optipng &>/dev/null; then
        optipng -strip all -quiet "$output"
    fi

    echo "  OK: $(stat -c%s "$output") bytes — $(file -b "$output")"
}

sanitize "$ASSETS/icon0.png"   "$OUT/icon0.png"
sanitize "$ASSETS/bg.png"      "$OUT/livearea/contents/bg.png"
sanitize "$ASSETS/startup.png" "$OUT/livearea/contents/startup.png"

echo ""
echo "=== Done ==="
find "$OUT" -name "*.png" -exec file {} \;
