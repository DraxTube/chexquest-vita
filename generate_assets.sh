#!/bin/bash
# generate_assets.sh — Genera assets LiveArea compatibili con PS Vita
set -e

command -v convert >/dev/null 2>&1 || { echo "Serve ImageMagick: sudo apt install imagemagick"; exit 1; }

rm -rf sce_sys
mkdir -p sce_sys/livearea/contents

TITLE=""
LOGO=""

# Scarica se non esistono già
if [ ! -f /tmp/chex_title.png ]; then
    echo "Scaricamento title screen..."
    curl -sL --max-time 30 -o /tmp/chex_title.png \
        "https://tcrf.net/images/1/11/Chex_Quest-title.png" 2>/dev/null || true
fi

if [ ! -f /tmp/chex_logo.png ]; then
    echo "Scaricamento logo..."
    curl -sL --max-time 30 \
        -A "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" \
        -o /tmp/chex_logo_raw \
        "https://static.wikia.nocookie.net/chexquest/images/b/b7/Chex-Quest-Logo.png/revision/latest?cb=20250226015616" 2>/dev/null || true
    if [ -s /tmp/chex_logo_raw ]; then
        convert /tmp/chex_logo_raw -depth 8 -strip PNG32:/tmp/chex_logo.png 2>/dev/null || true
        rm -f /tmp/chex_logo_raw
    fi
fi

[ -s /tmp/chex_title.png ] && file /tmp/chex_title.png | grep -qi "image" && TITLE="/tmp/chex_title.png"
[ -s /tmp/chex_logo.png ] && file /tmp/chex_logo.png | grep -qi "image" && LOGO="/tmp/chex_logo.png"
[ -z "$LOGO" ] && LOGO="$TITLE"
[ -z "$TITLE" ] && TITLE="$LOGO"

echo "TITLE=$TITLE  LOGO=$LOGO"

# icon0.png — 128x128
if [ -n "$LOGO" ]; then
    convert "$LOGO" -resize 128x128^ -gravity center -extent 128x128 \
        -depth 8 -strip PNG32:sce_sys/icon0.png
else
    convert -size 128x128 xc:'#2E8B57' -fill '#FFD700' -gravity center \
        -pointsize 28 -annotate +0-8 'CQ' -fill white -pointsize 11 \
        -annotate +0+16 'CHEX QUEST' -depth 8 -strip PNG32:sce_sys/icon0.png
fi

# pic0.png — 960x544
if [ -n "$TITLE" ]; then
    convert "$TITLE" -resize 960x544^ -gravity center -extent 960x544 \
        -depth 8 -strip PNG32:sce_sys/pic0.png
else
    convert -size 960x544 xc:'#1a1a2e' -fill '#FFD700' -gravity center \
        -pointsize 72 -annotate +0-40 'CHEX QUEST' -fill white -pointsize 28 \
        -annotate +0+30 'PS Vita Edition' -depth 8 -strip PNG32:sce_sys/pic0.png
fi

# bg.png — 840x500
if [ -n "$TITLE" ]; then
    convert "$TITLE" -resize 840x500^ -gravity center -extent 840x500 \
        -depth 8 -strip PNG32:sce_sys/livearea/contents/bg.png
else
    convert -size 840x500 xc:'#0f3460' -fill white -gravity center \
        -pointsize 48 -annotate +0-20 'CHEX QUEST' -fill '#cccccc' -pointsize 20 \
        -annotate +0+30 'PS Vita' -depth 8 -strip PNG32:sce_sys/livearea/contents/bg.png
fi

# startup.png — 280x158
if [ -n "$LOGO" ]; then
    convert "$LOGO" -resize 280x158^ -gravity center -extent 280x158 \
        -depth 8 -strip PNG32:sce_sys/livearea/contents/startup.png
else
    convert -size 280x158 xc:'#e94560' -fill white -gravity center \
        -pointsize 36 -annotate +0-8 'PLAY' -fill '#FFD700' -pointsize 14 \
        -annotate +0+22 'CHEX QUEST' -depth 8 -strip PNG32:sce_sys/livearea/contents/startup.png
fi

# template.xml
cat > sce_sys/livearea/contents/template.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<livearea style="a1" format-ver="01.00" content-rev="1">
  <livearea-bg>
    <image>bg.png</image>
  </livearea-bg>
  <gate>
    <startup-image>startup.png</startup-image>
  </gate>
</livearea>
EOF

echo ""
echo "=== Assets generati ==="
for f in sce_sys/icon0.png sce_sys/pic0.png \
         sce_sys/livearea/contents/bg.png \
         sce_sys/livearea/contents/startup.png \
         sce_sys/livearea/contents/template.xml; do
    if [ -s "$f" ]; then
        echo "OK: $f ($(wc -c < "$f") bytes)"
    else
        echo "ERRORE: $f"
    fi
done

identify sce_sys/icon0.png sce_sys/pic0.png \
    sce_sys/livearea/contents/bg.png \
    sce_sys/livearea/contents/startup.png 2>/dev/null

echo "Done!"
