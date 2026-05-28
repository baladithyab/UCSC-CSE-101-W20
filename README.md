# UCSC CSE 101 — Algorithms (Winter 2020, Patricia Lopez)

This is my coursework for CSE 101 at UC Santa Cruz. The repo contains:

```
hw1/   — assignments    (Bard, Iambic-pentameter detector)
hw2/   — assignments    (N-Queens backtracker)        ← compiled to WASM for the live demo
hw3/   — assignments    (AVL tree wordrange)
hw4/   — assignments    (Six-degrees BFS over IMDb)
bst-codes/                  — BST tutorial code
binary-heap/                — Binary heap tutorial code
linket-list/                — Linked-list tutorial code
recursion-through-stacks/   — Recursion-via-stack tutorial code
graph-qns/                  — Graph problems
bruteforce-questions/       — Brute-force exercises

embeds-wasm/                — Wasm shim layer for the interactive demos
embeds/                     — Static-HTML harnesses & build outputs (CI-generated)
scripts/build-embeds.sh     — CI build script (Emscripten compile + asset prep)
web.codeseys.json           — Manifest for codeseys.io project-embeds pipeline
.github/workflows/          — Calls baladithyab/web-embed-workflows@main
```

## Live demo

[https://codeseys.io/projects/cse-101](https://codeseys.io/projects/cse-101)

The N-Queens demo runs **the original course C++ unmodified** (`hw2/NQueens/Board.cpp` + `hw2/NQueens/Queens.cpp`) compiled to WebAssembly via Emscripten. The wasm shim in [`embeds-wasm/nqueens/nqueens-wasm.cpp`](embeds-wasm/nqueens/nqueens-wasm.cpp) wraps those classes in a step-by-step iterator API so the JS visualizer can drive the algorithm one queen-placement at a time.

## How the build pipeline works

1. Every push to `master` triggers `.github/workflows/build-web-asset.yml`, which calls the reusable workflow at [`baladithyab/web-embed-workflows@main`](https://github.com/baladithyab/web-embed-workflows).
2. The reusable workflow reads `web.codeseys.json`. Because `build.tools` declares `"emsdk"`, it sets up the Emscripten SDK (cached after first run) before running `scripts/build-embeds.sh`.
3. `build-embeds.sh` compiles the C++ + shim into `embeds/nqueens.{js,wasm,html}`.
4. The workflow uploads everything in `embeds/` to Cloudflare R2 at `cse-101/<git-sha>/`, authenticated via OIDC token (no static API keys).
5. The personal site re-syncs and the new `delivery.url` shows up on the project page.

## Running the demos locally

```bash
# Install Emscripten (one-time)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source emsdk_env.sh

# Build embeds
cd ..
EMBED_SOURCE_DIR=embeds ./scripts/build-embeds.sh

# Serve embeds/ over any static HTTP server, e.g.:
python3 -m http.server -d embeds 8080
# Open http://localhost:8080/nqueens.html

# Smoke test (Node-based, validates correctness):
node embeds-wasm/nqueens/smoke-test.cjs
```
