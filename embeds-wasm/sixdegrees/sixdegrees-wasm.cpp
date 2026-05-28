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
#include <vector>

namespace {

Graph* g_graph = nullptr;
std::list<std::string> g_lastPath;
std::vector<std::string> g_pathFlat;  // for indexed access from JS
int g_actorCount = 0;
std::string g_outBuf;
std::string g_lastError;

void resetState() {
    delete g_graph;
    g_graph = new Graph();
    g_lastPath.clear();
    g_pathFlat.clear();
    g_actorCount = 0;
    g_lastError.clear();
}

}  // namespace

extern "C" {

/**
 * Parse a movie list and build the graph. Format: one film per line,
 * whitespace-separated, first token is the film title and the rest
 * are actor names. Empty lines are skipped.
 *
 * Idempotent — replaces any previously-loaded graph.
 */
EMSCRIPTEN_KEEPALIVE
int sixdeg_load(const char* text) {
    resetState();
    std::istringstream in(text);
    std::string line;
    int films = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        // Skip comment lines that start with '#'.
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
        if (tokens.size() < 2) continue;  // need at least one actor
        // graph.insert expects [movie, actor1, actor2, ...]; the
        // original parser passes the same vector.
        g_graph->insert(tokens);
        ++films;
    }

    // Count distinct actors via a separate API: graph.h doesn't expose
    // a count directly, but `printMap` walks the unordered_map. We
    // approximate by re-walking with our own structure — but to keep
    // graph.h unmodified we just re-parse and count.
    {
        std::istringstream in2(text);
        std::vector<std::string> seen;
        std::string line2;
        while (std::getline(in2, line2)) {
            if (line2.empty() || line2[0] == '#') continue;
            std::istringstream ls(line2);
            std::vector<std::string> tokens;
            std::string t;
            while (ls >> t) tokens.push_back(t);
            for (size_t i = 1; i < tokens.size(); ++i) {
                bool found = false;
                for (const auto& s : seen) if (s == tokens[i]) { found = true; break; }
                if (!found) seen.push_back(tokens[i]);
            }
        }
        g_actorCount = (int)seen.size();
    }

    if (films == 0) {
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
