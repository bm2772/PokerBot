// ============================================================================
// equity.cpp — exact enumeration and Monte Carlo simulation.
// ============================================================================
#include "equity.h"
#include "evaluator.h"

#include <cmath>      // std::sqrt
#include <random>     // std::mt19937_64, std::uniform_int_distribution
#include <stdexcept>
#include <utility>    // std::swap

double EquityResult::std_error() const {
    if (trials == 0) return 0;
    double p = equity();
    return std::sqrt(p * (1.0 - p) / static_cast<double>(trials));
}

namespace {

// Raw counts while we work; converted to fractions at the end. Counting with
// integers (not adding up doubles) keeps the tallies exact.
struct Tally {
    long long win = 0, tie = 0, lose = 0;
};

// Check the inputs and return every card NOT already in use: the "stub" that
// the rest of the board gets dealt from.
std::vector<Card> remaining_deck(const std::vector<Card>& hero,
                                 const std::vector<Card>& villain,
                                 const std::vector<Card>& board) {
    if (hero.size() != 2 || villain.size() != 2) {
        throw std::invalid_argument("each player needs exactly 2 hole cards");
    }
    if (board.size() > 5) {
        throw std::invalid_argument("the board has at most 5 cards");
    }

    bool used[52] = {false};   // used[i] = card with index i is already out
    auto mark = [&](const std::vector<Card>& cards) {
        // A "lambda": a small function defined inline. [&] lets it see and
        // modify `used` from the surrounding function.
        for (const Card& c : cards) {
            if (used[c.index]) {
                throw std::invalid_argument("card " + c.to_string() + " appears twice");
            }
            used[c.index] = true;
        }
    };
    mark(hero);
    mark(villain);
    mark(board);

    std::vector<Card> rest;
    for (int i = 0; i < 52; ++i) {
        if (!used[i]) rest.push_back(Card(static_cast<uint8_t>(i)));
    }
    return rest;
}

// Both players see the same 5-card board; whoever's best 7-card hand scores
// higher wins. This is the one place a "showdown" happens.
void showdown(const std::vector<Card>& hero, const std::vector<Card>& villain,
              const Card board[5], Tally& t) {
    Card h[7] = {hero[0],    hero[1],    board[0], board[1], board[2], board[3], board[4]};
    Card v[7] = {villain[0], villain[1], board[0], board[1], board[2], board[3], board[4]};
    HandScore hs = evaluate(h, 7);
    HandScore vs = evaluate(v, 7);
    if      (hs > vs) t.win++;
    else if (hs < vs) t.lose++;
    else              t.tie++;
}

// Recursively fill board slots [filled..4] with every combination of cards
// from rest[start..]. Choosing only indices > the previous one means each set
// of cards is visited exactly once (the board {Kh,2c} is the same as {2c,Kh}).
// It does the same job as the nested loops in the test, but works for any
// number of missing cards.
void enumerate_boards(const std::vector<Card>& rest, size_t start,
                      Card board[5], int filled,
                      const std::vector<Card>& hero, const std::vector<Card>& villain,
                      Tally& t) {
    if (filled == 5) {
        showdown(hero, villain, board, t);
        return;
    }
    for (size_t i = start; i < rest.size(); ++i) {
        board[filled] = rest[i];
        enumerate_boards(rest, i + 1, board, filled + 1, hero, villain, t);
    }
}

EquityResult to_result(const Tally& t) {
    EquityResult r;
    r.trials = t.win + t.tie + t.lose;
    if (r.trials > 0) {
        double n = static_cast<double>(r.trials);
        r.win  = t.win  / n;
        r.tie  = t.tie  / n;
        r.lose = t.lose / n;
    }
    return r;
}

} // namespace

EquityResult equity_exact(const std::vector<Card>& hero,
                          const std::vector<Card>& villain,
                          const std::vector<Card>& board) {
    std::vector<Card> rest = remaining_deck(hero, villain, board);

    Card full_board[5];
    for (size_t i = 0; i < board.size(); ++i) full_board[i] = board[i];

    Tally t;
    enumerate_boards(rest, 0, full_board, static_cast<int>(board.size()), hero, villain, t);
    return to_result(t);
}

EquityResult equity_monte_carlo(const std::vector<Card>& hero,
                                const std::vector<Card>& villain,
                                const std::vector<Card>& board,
                                long long iterations,
                                uint64_t seed) {
    std::vector<Card> rest = remaining_deck(hero, villain, board);
    const int known = static_cast<int>(board.size());
    const int need  = 5 - known;                     // board cards still to come
    const int n     = static_cast<int>(rest.size());

    // mt19937_64 = 64-bit Mersenne Twister. Taking a seed instead of
    // random_device makes runs repeatable: same seed -> same "random" boards,
    // which makes results easy to check and debug.
    std::mt19937_64 rng(seed);

    Card full_board[5];
    for (int i = 0; i < known; ++i) full_board[i] = board[i];

    Tally t;
    for (long long it = 0; it < iterations; ++it) {
        // Partial Fisher-Yates: for each slot k, pick a random card from the
        // not-yet-chosen part rest[k..n-1] and swap it into position k. After
        // `need` steps, rest[0..need-1] is a uniformly random draw without
        // replacement, and we only did `need` swaps, not a full 48-card shuffle.
        for (int k = 0; k < need; ++k) {
            std::uniform_int_distribution<int> pick(k, n - 1);
            std::swap(rest[k], rest[pick(rng)]);
            full_board[known + k] = rest[k];
        }
        showdown(hero, villain, full_board, t);
    }
    return to_result(t);
}
