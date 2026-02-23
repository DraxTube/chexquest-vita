#!/bin/bash
# generate_assets.sh - Genera gli asset grafici per PS Vita LiveArea
set -e

mkdir -p sce_sys/livearea/contents

# Colori tema Chex Quest
BG_COLOR='#1a5c2a'        # Verde scuro (tema Chex)
ACCENT_COLOR='#e94560'     # Rosso per il pulsante
TEXT_COLOR='#ffffff'
GOLD_COLOR='#FFD700'

echo "=== Generazione Asset PS Vita ==="

# === icon0.png (128x128) - Icona nella bolla ===
echo "Creando icon0.png..."
convert -size 128x128 xc:'#1a5c2a' \
  -fill '#2E8B57' -draw "roundrectangle 8,8 120,120 12,12" \
  -fill '#FFD700' -gravity center \
  -font DejaVu-Sans-Bold -pointsize 32 \
  -annotate +0-15 'CQ' \
  -fill white -pointsize 12 \
  -annotate +0+18 'CHEX' \
  -annotate +0+32 'QUEST' \
  -fill '#e94560' -pointsize 8 \
  -annotate +0+48 'PS VITA' \
  sce_sys/icon0.png
echo "  -> icon0.png creato (128x128)"

# === pic0.png (960x544) - Sfondo info app ===
echo "Creando pic0.png..."
convert -size 960x544 \
  'gradient:#0a2a0a-#1a5c2a' \
  \( -size 960x544 xc:none \
     -fill 'rgba(255,215,0,0.05)' \
     -draw "circle 480,272 480,100" \
     -draw "circle 200,400 200,350" \
     -draw "circle 750,150 750,100" \) \
  -composite \
  -fill '#FFD700' -gravity center \
  -font DejaVu-Sans-Bold -pointsize 72 \
  -annotate +0-80 'CHEX QUEST' \
  -fill white -pointsize 32 \
  -annotate +0+10 'PS Vita Edition' \
  -fill '#e94560' -pointsize 20 \
  -annotate +0+60 'OPL3 Music • Full Sound • Touch Controls' \
  -fill 'rgba(255,255,255,0.3)' -pointsize 14 \
  -gravity south -annotate +0+20 'doomgeneric port v1.0' \
  sce_sys/pic0.png
echo "  -> pic0.png creato (960x544)"

# === bg.png (840x500) - Sfondo LiveArea ===
echo "Creando bg.png..."
convert -size 840x500 \
  'gradient:#0f3460-#1a5c2a' \
  \( -size 840x500 xc:none \
     -fill 'rgba(46,139,87,0.15)' \
     -draw "circle 420,250 420,50" \) \
  -composite \
  -fill 'rgba(255,215,0,0.6)' -gravity center \
  -font DejaVu-Sans-Bold -pointsize 48 \
  -annotate +0-40 'CHEX QUEST' \
  -fill 'rgba(255,255,255,0.4)' -pointsize 20 \
  -annotate +0+20 'PS Vita' \
  sce_sys/livearea/contents/bg.png
echo "  -> bg.png creato (840x500)"

# === startup.png (280x158) - Pulsante Start ===
echo "Creando startup.png..."
convert -size 280x158 xc:none \
  -fill '#e94560' -draw "roundrectangle 4,4 276,154 16,16" \
  -fill '#c0392b' -draw "roundrectangle 4,80 276,154 0,16" \
  -fill '#e94560' -draw "roundrectangle 4,4 276,148 16,16" \
  -fill white -gravity center \
  -font DejaVu-Sans-Bold -pointsize 36 \
  -annotate +0-10 'START' \
  -fill '#FFD700' -pointsize 14 \
  -annotate +0+25 'CHEX QUEST' \
  sce_sys/livearea/contents/startup.png
echo "  -> startup.png creato (280x158)"

# === Verifica ===
echo ""
echo "=== Asset generati ==="
echo "sce_sys/"
ls -la sce_sys/icon0.png sce_sys/pic0.png
echo "sce_sys/livearea/contents/"
ls -la sce_sys/livearea/contents/

# Verifica dimensioni
for f in sce_sys/icon0.png sce_sys/pic0.png sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/startup.png; do
  dims=$(identify -format "%wx%h" "$f" 2>/dev/null || echo "unknown")
  echo "  $f: $dims"
done

echo ""
echo "=== Fatto! ==="
