# Chex Quest per PS Vita

Port di Chex Quest per PlayStation Vita basato su Chocolate Doom.

## Compilazione

La compilazione avviene automaticamente tramite GitHub Actions.
Ogni push su `main` genera un file VPK scaricabile dagli Artifacts.

### Compilazione locale

```bash
export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:$PATH

# Clona Chocolate Doom
git clone --branch chocolate-doom-3.0.1 \
  https://github.com/chocolate-doom/chocolate-doom.git

# Applica patch
bash vita/patch_chex.sh

# Compila
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake ..
make -j$(nproc)
