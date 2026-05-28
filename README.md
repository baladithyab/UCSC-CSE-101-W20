# UCSC CSE 101 — Algorithms (Winter 2020)

My labs and programming assignments from CSE 101 (Algorithms / Abstract Data Types) at UCSC, taught Winter 2020 by Patricia Lopez.

The repo holds the original C++ source for each homework (`hw1/` through `hw4/`), plus extras for binary heaps, BSTs, linked lists, graph problems, and recursion-through-stacks tutorials.

## Live demos

The `embeds/` directory holds two interactive visualizations of the algorithms:

- **N-Queens backtracker** (HW2) — animated column-by-column placement
- **AVL Wordrange** (HW3) — string-keyed self-balancing BST with rotation animation

These run on [codeseys.io/projects/cse-101](https://codeseys.io/projects/cse-101) via the [`codeseys-embed` discovery topic](https://codeseys.io/blog/personal-website-embed-architecture/) — push to master rebuilds them automatically.

## Layout

| Path | What |
|---|---|
| `hw1/` … `hw4/` | Per-homework C++ source + writeups |
| `bst-codes/`, `binary-heap/`, `linket-list/` | Standalone ADT implementations |
| `graph-qns/`, `bruteforce-questions/`, `recursion-through-stacks/` | In-class problem sets |
| `embeds/` | Interactive web demos (rendered on codeseys.io) |
| `web.codeseys.json` | Embed manifest (consumed by the personal-site discovery script) |
