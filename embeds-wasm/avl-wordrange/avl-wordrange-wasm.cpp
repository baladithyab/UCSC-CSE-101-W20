// avl-wordrange-wasm.cpp
//
// Step-by-step wrapper around the unmodified CSE 101 HW3 AVL class
// (hw3/Wordrange/AVL.cpp + hw3/Wordrange/AVL.h), exposed to JavaScript
// via Emscripten extern "C".
//
// The original AVL exposes:
//   - insert(string)
//   - range(string, string) -> int
//   - printTree()
//
// For the live demo we want the JS visualizer to read the tree
// structure after each operation, plus a few accessors for rendering
// (key, height, balance factor, descendant counts).
//
// To do that without modifying AVL.h, we expose a flat-buffer
// traversal API: after any insert, JS calls `avl_count_nodes()` and
// then `avl_get_node(i, ...)` to walk the in-order traversal.

#include "AVL.h"

#include <emscripten/emscripten.h>
#include <string>
#include <vector>
#include <cstring>

namespace {

// Single global AVL — the demo only ever needs one tree at a time.
AVL* g_tree = nullptr;

// Snapshot of the tree as a flat list of nodes, populated by
// avl_snapshot(). Each node: key (string) + structural metadata.
struct NodeSnapshot {
    std::string key;
    int height;
    int ldesc;
    int rdesc;
    int parentIdx;   // -1 for the root
    int leftIdx;     // -1 if no left child
    int rightIdx;
    int depth;       // 0 for root, parent.depth+1 otherwise
};
std::vector<NodeSnapshot> g_snap;

// Buffer for returning strings to JS (key, range string, etc.).
std::string g_outBuf;

// Forward-declare access to the private root via reflection: AVL.h
// declares root as private. Rather than expose it, we expose only
// behaviour through the public API, and we write our own traversal by
// re-walking via insertion-only API. But we need the structure.
//
// Workaround: subclass AVL to expose a getter. We don't modify AVL.h.
class OpenAVL : public AVL {
public:
    Node* getRoot() {
        // The struct layout has `Node* root` as the only private member.
        // We bit-cast through `this` — both classes are POD-ish (no
        // virtual table). This is a deliberate hack to avoid editing the
        // course headers.
        return *reinterpret_cast<Node**>(this);
    }
};

void walk(Node* node, int parentIdx, int depth) {
    if (node == nullptr) return;
    NodeSnapshot snap;
    snap.key = node->key;
    snap.height = node->height;
    snap.ldesc = node->ldesc;
    snap.rdesc = node->rdesc;
    snap.parentIdx = parentIdx;
    snap.leftIdx = -1;   // filled in after recursion
    snap.rightIdx = -1;
    snap.depth = depth;
    int myIdx = (int)g_snap.size();
    g_snap.push_back(snap);

    if (node->left) {
        g_snap[myIdx].leftIdx = (int)g_snap.size();
        walk(node->left, myIdx, depth + 1);
    }
    if (node->right) {
        g_snap[myIdx].rightIdx = (int)g_snap.size();
        walk(node->right, myIdx, depth + 1);
    }
}

void rebuildSnapshot() {
    g_snap.clear();
    if (g_tree == nullptr) return;
    OpenAVL* open = reinterpret_cast<OpenAVL*>(g_tree);
    walk(open->getRoot(), -1, 0);
}

}  // namespace

extern "C" {

/**
 * Reset the AVL — drop the existing tree, allocate a fresh empty one.
 */
EMSCRIPTEN_KEEPALIVE
void avl_reset(void) {
    delete g_tree;
    g_tree = new AVL();
    g_snap.clear();
}

/**
 * Insert a word. Returns 1 on success. JS must call `avl_snapshot()`
 * afterwards to refresh the snapshot used by the rendering accessors.
 *
 * The underlying AVL prints to stdout if the word already exists; we
 * accept that as a no-op (Emscripten will swallow stdout unless the
 * runtime is configured to emit it).
 */
EMSCRIPTEN_KEEPALIVE
int avl_insert(const char* cstr) {
    if (g_tree == nullptr) avl_reset();
    g_tree->insert(std::string(cstr));
    rebuildSnapshot();
    return 1;
}

/**
 * Range query — number of words in tree with `low <= key <= high`.
 */
EMSCRIPTEN_KEEPALIVE
int avl_range(const char* low, const char* high) {
    if (g_tree == nullptr) return 0;
    return g_tree->range(std::string(low), std::string(high));
}

/**
 * After avl_insert(), JS calls these accessors to walk the snapshot.
 *
 * Indexing: g_snap is in DFS-in-construction order (root, then left
 * subtree, then right subtree). Each node carries leftIdx/rightIdx so
 * JS can navigate.
 */
EMSCRIPTEN_KEEPALIVE
int avl_count(void) { return (int)g_snap.size(); }

/**
 * Returns the snapshot index of the root, or -1 if the tree is empty.
 */
EMSCRIPTEN_KEEPALIVE
int avl_root_idx(void) { return g_snap.empty() ? -1 : 0; }

EMSCRIPTEN_KEEPALIVE
int avl_node_left(int i) {
    if (i < 0 || i >= (int)g_snap.size()) return -1;
    return g_snap[i].leftIdx;
}

EMSCRIPTEN_KEEPALIVE
int avl_node_right(int i) {
    if (i < 0 || i >= (int)g_snap.size()) return -1;
    return g_snap[i].rightIdx;
}

EMSCRIPTEN_KEEPALIVE
int avl_node_parent(int i) {
    if (i < 0 || i >= (int)g_snap.size()) return -1;
    return g_snap[i].parentIdx;
}

EMSCRIPTEN_KEEPALIVE
int avl_node_height(int i) {
    if (i < 0 || i >= (int)g_snap.size()) return 0;
    return g_snap[i].height;
}

EMSCRIPTEN_KEEPALIVE
int avl_node_ldesc(int i) {
    if (i < 0 || i >= (int)g_snap.size()) return 0;
    return g_snap[i].ldesc;
}

EMSCRIPTEN_KEEPALIVE
int avl_node_rdesc(int i) {
    if (i < 0 || i >= (int)g_snap.size()) return 0;
    return g_snap[i].rdesc;
}

EMSCRIPTEN_KEEPALIVE
int avl_node_depth(int i) {
    if (i < 0 || i >= (int)g_snap.size()) return 0;
    return g_snap[i].depth;
}

/**
 * Returns the key string for node `i`. Caller must use UTF8ToString()
 * on the returned pointer; the buffer is owned by the wasm module and
 * remains valid until the next call to avl_node_key().
 */
EMSCRIPTEN_KEEPALIVE
const char* avl_node_key(int i) {
    if (i < 0 || i >= (int)g_snap.size()) return "";
    g_outBuf = g_snap[i].key;
    return g_outBuf.c_str();
}

}  // extern "C"
