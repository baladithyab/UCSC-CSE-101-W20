// sixdegrees-wasm.cpp
//
// Emscripten shim around the unmodified CSE 101 HW4 Sixdegrees solver
// — graph.cpp + graph.h from hw4/Sixdegrees/. Implements
// "six-degrees-of-Kevin-Bacon"-style BFS shortest path through an
// actor/movie graph: actors are nodes, edges connect any two actors
// who appeared together in a film.
//
// JS interface:
//   sixdeg_load(movieList)     — parse a movie list (one film per
//                                line: "Title Actor1 Actor2 ...");
//                                builds the graph
//   sixdeg_query(src, dest)    — find the shortest path; returns 1
//                                if found, 0 otherwise
//   sixdeg_path_length()       — number of nodes in the path (0 if
//                                no path or query not run)
//   sixdeg_path_get(i)         — get the i-th element of the path;
//                                actors and movies alternate
//   sixdeg_actor_count()       — total distinct actors loaded
//   sixdeg_last_error()        — diagnostic string

#include "graph.h"

#include <emscripten/emscripten.h>
#include <list>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

Graph* g_graph = nullptr;
std::list<std::string> g_lastPath;
std::vector<std::string> g_pathFlat;  // for indexed access from JS
int g_actorCount = 0;
std::string g_outBuf;
std::string g_lastError;
std::unordered_set<std::string> g_distinctActors;
int g_filmsTotal = 0;

void resetState() {
    // NOTE: we deliberately leak the previous Graph rather than `delete`-ing it.
    // The graph's nested unordered_map<string, list<Coactor>> holds millions of
    // small allocations; emscripten's wasm runtime traps with
    // "memory access out of bounds" partway through teardown when the graph
    // gets large (~50K+ films). Leaking is safe here — each demo session is
    // a tab lifetime, the OS reclaims the wasm linear memory on tab close,
    // and load is rare (user clicks at most a few times).
    g_graph = new Graph();
    g_lastPath.clear();
    g_pathFlat.clear();
    g_actorCount = 0;
    g_lastError.clear();
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int sixdeg_begin(void);
EMSCRIPTEN_KEEPALIVE int sixdeg_append(const char* text);
EMSCRIPTEN_KEEPALIVE int sixdeg_finish(void);

/**
 * Parse a movie list and build the graph. Format: one film per line,
 * whitespace-separated, first token is the film title and the rest
 * are actor names. Empty lines are skipped.
 *
 * Idempotent — replaces any previously-loaded graph.
 */
EMSCRIPTEN_KEEPALIVE
int sixdeg_load(const char* text) {
    sixdeg_begin();
    int ok = sixdeg_append(text);
    sixdeg_finish();
    return ok;
}

/**
 * Streaming version of sixdeg_load: parse and add films to the EXISTING graph
 * without resetting. Lets large datasets be loaded in chunks from JS so the
 * UTF-8 string copy doesn't have to allocate the whole text at once. Call
 * sixdeg_begin() before the first sixdeg_append().
 */
EMSCRIPTEN_KEEPALIVE
int sixdeg_begin(void) {
    resetState();
    g_distinctActors.clear();
    g_distinctActors.reserve(80000);
    g_filmsTotal = 0;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int sixdeg_append(const char* text) {
    if (g_graph == nullptr) {
        g_graph = new Graph();
    }
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        bool allSpace = true;
        for (char c : line) {
            if (c == '#') { allSpace = true; break; }
            if (!isspace((unsigned char)c)) { allSpace = false; break; }
        }
        if (allSpace) continue;
        if (line[0] == '#') continue;

        std::istringstream ls(line);
        std::vector<std::string> tokens;
        std::string tok;
        while (ls >> tok) tokens.push_back(tok);
        if (tokens.size() < 2) continue;

        g_graph->insert(tokens);
        for (size_t i = 1; i < tokens.size(); ++i) g_distinctActors.insert(tokens[i]);
        ++g_filmsTotal;
    }
    g_actorCount = (int)g_distinctActors.size();
    return g_filmsTotal > 0 ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int sixdeg_finish(void) {
    g_actorCount = (int)g_distinctActors.size();
    if (g_filmsTotal == 0) {
        g_lastError = "no films parsed";
        return 0;
    }
    return 1;
}

/**
 * Run BFS shortest path between two actors. Returns 1 if a path was
 * found, 0 otherwise (call sixdeg_last_error()).
 */
EMSCRIPTEN_KEEPALIVE
int sixdeg_query(const char* src, const char* dest) {
    g_lastError.clear();
    g_lastPath.clear();
    g_pathFlat.clear();
    if (g_graph == nullptr) {
        g_lastError = "no graph loaded; call sixdeg_load() first";
        return 0;
    }
    std::string s(src), d(dest);
    if (!g_graph->exists(s, d)) {
        g_lastError = "one or both actors not in graph";
        return 0;
    }
    if (s == d) {
        g_lastPath.push_back(s);
        g_pathFlat.push_back(s);
        return 1;
    }
    g_lastPath = g_graph->shortestPath(s, d);
    if (g_lastPath.empty() || (g_lastPath.size() == 1 && g_lastPath.front().empty())) {
        g_lastError = "no path found between actors";
        g_lastPath.clear();
        return 0;
    }
    for (const auto& s : g_lastPath) g_pathFlat.push_back(s);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int sixdeg_path_length(void) { return (int)g_pathFlat.size(); }

EMSCRIPTEN_KEEPALIVE
const char* sixdeg_path_get(int i) {
    if (i < 0 || i >= (int)g_pathFlat.size()) { g_outBuf.clear(); return g_outBuf.c_str(); }
    g_outBuf = g_pathFlat[i];
    return g_outBuf.c_str();
}

EMSCRIPTEN_KEEPALIVE
int sixdeg_actor_count(void) { return g_actorCount; }

EMSCRIPTEN_KEEPALIVE
const char* sixdeg_last_error(void) { return g_lastError.c_str(); }

}  // extern "C"
