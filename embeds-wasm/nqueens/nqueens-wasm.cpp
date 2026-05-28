// nqueens-wasm.cpp
//
// Thin step-by-step wrapper around the unmodified CSE 101 HW2
// Board/Queens classes, exposed to JavaScript via Emscripten's
// extern "C" + EMSCRIPTEN_KEEPALIVE.
//
// The original `nqueens.cpp` driver was a batch program: read input
// file → solve → write output file. For an interactive web demo we
// need a step-by-step iterator: each call to `nqueens_step()` performs
// one column-placement attempt (or one backtrack) and reports what
// happened, so the UI can animate it.
//
// The actual algorithm logic lives in Board.cpp / Queens.cpp from the
// course HW. This file only adds a state-machine driver around it.

#include "Board.h"
#include "Queens.h"

#include <emscripten/emscripten.h>
#include <vector>
#include <cstring>
#include <string>

namespace {

// State of the iterative backtracking solver.
struct State {
    Board* board = nullptr;
    int N = 0;
    // For each row 1..N, the column we last attempted. Starts at 0.
    // When we backtrack out of row r, we resume on `lastCol[r-1]+1`.
    std::vector<int> lastCol;
    bool finished = false;
    bool foundSolution = false;

    void reset(int n) {
        delete board;
        board = new Board(n);
        N = n;
        lastCol.assign(n + 1, 0);  // 1-indexed; lastCol[0] unused
        finished = false;
        foundSolution = false;
    }

    ~State() { delete board; }
};

State g_state;

// Buffer for returning the board to JS. Allocated once, grown as needed.
// JS reads from this via emscripten_get_module_property / Module._malloc
// patterns; we just expose a pointer + length.
std::string g_outBuf;

}  // namespace

extern "C" {

/**
 * Initialise a fresh N-queens board. Idempotent: subsequent calls
 * blow away the previous state.
 *
 * @return 1 on success, 0 if N is out of [4, 16] (UI cap)
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_init(int n) {
    if (n < 4 || n > 16) return 0;
    g_state.reset(n);
    return 1;
}

/**
 * Step descriptor — how the most recent step changed the board.
 *
 * 0 = no step taken (already finished); call nqueens_done() / nqueens_solution()
 * 1 = placed a queen at (row, col)
 * 2 = backtracked from (row, col) — that placement was invalid or led
 *     to a dead end; the queen is no longer on the board
 * 3 = solver finished and a solution exists
 * 4 = solver finished and no solution exists (impossible — N≥4 always
 *     has at least one)
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_step(void) {
    if (g_state.finished) return 0;
    if (g_state.board == nullptr) return 0;

    Board& b = *g_state.board;

    // The current row to fill is `numQueens + 1` (1-indexed). When we
    // backtrack, we pop the last queen and resume from its column + 1.
    int row = b.numQueens + 1;
    int startCol = g_state.lastCol[row] + 1;

    // Try to place at row, startCol..N
    for (int col = startCol; col <= g_state.N; ++col) {
        if (b.isValid(row, col)) {
            b.placeQueen(row, col);
            g_state.lastCol[row] = col;
            if (b.numQueens == g_state.N) {
                g_state.finished = true;
                g_state.foundSolution = true;
                return 3;  // done, success
            }
            // Reset the lastCol for the next row, since we're entering it fresh.
            g_state.lastCol[row + 1] = 0;
            return 1;  // placed
        }
    }

    // No valid column at this row — backtrack.
    if (b.numQueens == 0) {
        // Already at the root with no options; impossible.
        g_state.finished = true;
        g_state.foundSolution = false;
        return 4;
    }

    // Pop the last queen (which is at b.board.back()).
    int lastRow = b.board.back().getRow();
    g_state.lastCol[lastRow + 1] = 0;  // clear forward state
    b.board.pop_back();
    b.numQueens--;
    return 2;  // backtracked
}

/**
 * Run nqueens_step() repeatedly until the solver terminates.
 * Returns the number of steps taken. Bounded internally to 1<<20 to
 * keep the UI responsive in pathological cases.
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_solve_all(void) {
    int steps = 0;
    while (!g_state.finished && steps < (1 << 20)) {
        nqueens_step();
        ++steps;
    }
    return steps;
}

/**
 * Number of queens currently placed on the board.
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_num_queens(void) {
    if (g_state.board == nullptr) return 0;
    return g_state.board->numQueens;
}

/**
 * Returns the row of the i-th queen on the board (1-indexed).
 * Returns -1 if i is out of range.
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_queen_row(int i) {
    if (g_state.board == nullptr) return -1;
    if (i < 0 || i >= (int)g_state.board->board.size()) return -1;
    return g_state.board->board[i].getRow();
}

/**
 * Returns the column of the i-th queen on the board (1-indexed).
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_queen_col(int i) {
    if (g_state.board == nullptr) return -1;
    if (i < 0 || i >= (int)g_state.board->board.size()) return -1;
    return g_state.board->board[i].getCol();
}

/**
 * Whether the solver is finished.
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_done(void) { return g_state.finished ? 1 : 0; }

/**
 * Whether a solution was found (only meaningful after finished).
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_has_solution(void) { return g_state.foundSolution ? 1 : 0; }

/**
 * Returns the board size N.
 */
EMSCRIPTEN_KEEPALIVE
int nqueens_size(void) { return g_state.N; }

}  // extern "C"
