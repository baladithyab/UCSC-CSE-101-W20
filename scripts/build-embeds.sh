#!/usr/bin/env bash
# scripts/build-embeds.sh
#
# Pre-upload transform for CSE 101 demos:
#   - Compiles nqueens.cpp + Board.cpp + Queens.cpp + nqueens-wasm.cpp
#     into embeds/nqueens.js + embeds/nqueens.wasm using Emscripten.
#   - Copies embeds-wasm/nqueens/nqueens.html into embeds/.
#   - Same pattern for AVL Wordrange and Sixdegrees BFS once those land.
#
# Run by the reusable workflow when web.codeseys.json declares:
#   build:
#     script: scripts/build-embeds.sh
#     tools: [emsdk]
#
# Env vars provided by the workflow:
#   EMBED_SLUG        — manifest.slug
#   EMBED_VERSION     — short git sha
#   EMBED_SOURCE_DIR  — directory the workflow will rsync to R2 (default 'embeds')
set -euo pipefail

# Resolve source-dir; the workflow defaults to 'embeds' for this repo
# but we want the script to be runnable locally too.
DEST="${EMBED_SOURCE_DIR:-embeds}"
mkdir -p "$DEST"

echo "Build script: cse-101 embeds → $DEST/ (slug=$EMBED_SLUG, version=$EMBED_VERSION)"

# ---------- N-Queens (HW2) -----------------------------------------------
# We compile the original course sources from hw2/NQueens/ unmodified —
# Board.cpp + Queens.cpp — together with our wasm shim layer in
# embeds-wasm/nqueens/. The shim is the only file that uses Emscripten
# macros; the original CSE 101 sources are untouched.
NQ_SRC_DIR="hw2/NQueens"
NQ_SHIM_DIR="embeds-wasm/nqueens"
if [ -f "$NQ_SHIM_DIR/nqueens-wasm.cpp" ] && [ -d "$NQ_SRC_DIR" ]; then
  echo
  echo "── N-Queens ───────────────────────────────────────────────"
  if ! command -v emcc >/dev/null 2>&1; then
    echo "::error::emcc not on PATH; the workflow's emsdk setup step must run first"
    echo "  (declare 'tools: [emsdk]' under build: in web.codeseys.json)"
    exit 1
  fi
  echo "emcc: $(emcc --version | head -1)"

  emcc \
    "$NQ_SRC_DIR/Board.cpp" \
    "$NQ_SRC_DIR/Queens.cpp" \
    "$NQ_SHIM_DIR/nqueens-wasm.cpp" \
    -I "$NQ_SRC_DIR" \
    -I "$NQ_SHIM_DIR" \
    -O3 \
    -std=c++17 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME='createNqueensModule' \
    -s ENVIRONMENT='web' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=4194304 \
    -s STACK_SIZE=1048576 \
    -s EXPORTED_FUNCTIONS='[
      "_nqueens_init",
      "_nqueens_step",
      "_nqueens_solve_all",
      "_nqueens_num_queens",
      "_nqueens_queen_row",
      "_nqueens_queen_col",
      "_nqueens_done",
      "_nqueens_has_solution",
      "_nqueens_size"
    ]' \
    -s EXPORTED_RUNTIME_METHODS='["HEAPU8"]' \
    -s SINGLE_FILE=0 \
    --closure 0 \
    -o "$DEST/nqueens.js"

  # Copy the HTML harness alongside the wasm bundle.
  cp "$NQ_SHIM_DIR/nqueens.html" "$DEST/nqueens.html"

  echo "  ✓ $DEST/nqueens.js   ($(stat -c%s "$DEST/nqueens.js" 2>/dev/null || stat -f%z "$DEST/nqueens.js") bytes)"
  echo "  ✓ $DEST/nqueens.wasm ($(stat -c%s "$DEST/nqueens.wasm" 2>/dev/null || stat -f%z "$DEST/nqueens.wasm") bytes)"
  echo "  ✓ $DEST/nqueens.html"
else
  echo "::warning::missing $NQ_SRC_DIR or $NQ_SHIM_DIR; skipping N-Queens build"
fi

# ---------- AVL Wordrange (HW3) -----------------------------------------
# Compiles hw3/Wordrange/AVL.cpp + the wasm shim into avl-wordrange.{js,wasm}.
AVL_SRC_DIR="hw3/Wordrange"
AVL_SHIM_DIR="embeds-wasm/avl-wordrange"
if [ -f "$AVL_SHIM_DIR/avl-wordrange-wasm.cpp" ] && [ -d "$AVL_SRC_DIR" ]; then
  echo
  echo "── AVL Wordrange ──────────────────────────────────────────"

  emcc \
    "$AVL_SRC_DIR/AVL.cpp" \
    "$AVL_SHIM_DIR/avl-wordrange-wasm.cpp" \
    -I "$AVL_SRC_DIR" \
    -I "$AVL_SHIM_DIR" \
    -O3 \
    -std=c++17 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME='createAvlModule' \
    -s ENVIRONMENT='web' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=4194304 \
    -s STACK_SIZE=1048576 \
    -s EXPORTED_FUNCTIONS='[
      "_avl_reset",
      "_avl_insert",
      "_avl_range",
      "_avl_count",
      "_avl_root_idx",
      "_avl_node_left",
      "_avl_node_right",
      "_avl_node_parent",
      "_avl_node_height",
      "_avl_node_ldesc",
      "_avl_node_rdesc",
      "_avl_node_depth",
      "_avl_node_key",
      "_malloc",
      "_free"
    ]' \
    -s EXPORTED_RUNTIME_METHODS='["UTF8ToString","stringToUTF8","lengthBytesUTF8","HEAPU8","ccall","cwrap"]' \
    -s SINGLE_FILE=0 \
    --closure 0 \
    -o "$DEST/avl-wordrange.js"

  cp "$AVL_SHIM_DIR/avl-wordrange.html" "$DEST/avl-wordrange.html"

  echo "  ✓ $DEST/avl-wordrange.js   ($(stat -c%s "$DEST/avl-wordrange.js" 2>/dev/null || stat -f%z "$DEST/avl-wordrange.js") bytes)"
  echo "  ✓ $DEST/avl-wordrange.wasm ($(stat -c%s "$DEST/avl-wordrange.wasm" 2>/dev/null || stat -f%z "$DEST/avl-wordrange.wasm") bytes)"
  echo "  ✓ $DEST/avl-wordrange.html"
else
  echo "::warning::missing $AVL_SRC_DIR or $AVL_SHIM_DIR; skipping AVL Wordrange build"
fi

# ---------- Sixdegrees BFS (HW4) — TODO ---------------------------------
# Future: compile hw4/Sixdegrees/graph.cpp + a wasm shim.

echo
echo "Build script complete"
echo "  Output: $DEST/"
ls -la "$DEST"
