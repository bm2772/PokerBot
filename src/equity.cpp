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
void showdown(Card h0, Card h1, Card v0, Card v1, const Card board[5], Tally& t) {
    Card h[7] = {h0, h1, board[0], board[1], board[2], board[3], board[4]};
    Card v[7] = {v0, v1, board[0], board[1], board[2], board[3], board[4]};
    HandScore hs = evaluate(h, 7);
    HandScore vs = evaluate(v, 7);
    if      (hs > vs) t.win++;
    else if (hs < vs) t.lose++;
    else              t.tie++;
}

// Convenience overload: two vectors of hole cards. C++ picks which version to
// call by looking at the argument types ("function overloading"; Python can't
// do this with plain functions).
void showdown(const std::vector<Card>& hero, const std::vector<Card>& villain,
              const Card board[5], Tally& t) {
    showdown(hero[0], hero[1], villain[0], villain[1], board, t);
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

// ---- Helpers for Step 5 ---------------------------------------------------

// Add `cards` to a 52-bit "used" mask, refusing duplicates.
uint64_t add_cards(uint64_t used, const std::vector<Card>& cards) {
    for (const Card& c : cards) {
        uint64_t bit = 1ULL << c.index;
        if (used & bit) throw std::invalid_argument("card " + c.to_string() + " appears twice");
        used |= bit;
    }
    return used;
}

uint64_t mask_of(const Combo& c) {
    return (1ULL << c.a.index) | (1ULL << c.b.index);
}

// Live combos of a range, or an error if there are none at all.
std::vector<Combo> live_or_throw(const Range& range, const std::vector<Card>& dead) {
    std::vector<Combo> live = range.live_combos(dead);
    if (live.empty()) throw std::invalid_argument("no combo in the range is possible with these cards");
    return live;
}

// A sampler that returns index i with probability weight_i / total_weight.
std::discrete_distribution<size_t> weighted_picker(const std::vector<Combo>& combos) {
    std::vector<double> weights;
    for (const Combo& c : combos) weights.push_back(c.weight);
    return std::discrete_distribution<size_t>(weights.begin(), weights.end());
}

// Fill board slots [known..4] with random cards not in `used`, by REJECTION
// SAMPLING: pick any of the 52 cards; if it's taken, just pick again. Each
// accepted card is equally likely to be any free card, and with at most 9 of
// 52 taken, a retry is rarely needed.
void complete_board(std::mt19937_64& rng, uint64_t used, Card board[5], int known) {
    std::uniform_int_distribution<int> any_card(0, 51);
    for (int i = known; i < 5; ++i) {
        int c;
        do {
            c = any_card(rng);
        } while ((used >> c) & 1ULL);
        used |= 1ULL << c;
        board[i] = Card(static_cast<uint8_t>(c));
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

// ============================================================================
// Step 5: equity against ranges
// ============================================================================

EquityResult equity_vs_range_exact(const std::vector<Card>& hero,
                                   const Range& villain,
                                   const std::vector<Card>& board) {
    std::vector<Card> dead = hero;
    dead.insert(dead.end(), board.begin(), board.end());
    std::vector<Combo> live = live_or_throw(villain, dead);

    // Weighted average of the exact result against each live combo.
    EquityResult total;
    double weight_sum = 0;
    for (const Combo& c : live) {
        EquityResult r = equity_exact(hero, {c.a, c.b}, board);
        total.win  += c.weight * r.win;
        total.tie  += c.weight * r.tie;
        total.lose += c.weight * r.lose;
        total.trials += r.trials;
        weight_sum += c.weight;
    }
    total.win  /= weight_sum;
    total.tie  /= weight_sum;
    total.lose /= weight_sum;
    return total;
}

EquityResult equity_vs_range_monte_carlo(const std::vector<Card>& hero,
                                         const Range& villain,
                                         const std::vector<Card>& board,
                                         long long iterations,
                                         uint64_t seed) {
    if (hero.size() != 2) throw std::invalid_argument("hero needs exactly 2 hole cards");
    if (board.size() > 5) throw std::invalid_argument("the board has at most 5 cards");
    uint64_t known = add_cards(add_cards(0, hero), board);

    std::vector<Card> dead = hero;
    dead.insert(dead.end(), board.begin(), board.end());
    std::vector<Combo> live = live_or_throw(villain, dead);
    auto pick = weighted_picker(live);   // `auto`: let the compiler fill in the long type name

    std::mt19937_64 rng(seed);
    Card full_board[5];
    const int k = static_cast<int>(board.size());
    for (int i = 0; i < k; ++i) full_board[i] = board[i];

    Tally t;
    for (long long it = 0; it < iterations; ++it) {
        const Combo& v = live[pick(rng)];                 // 1. which hand does villain have?
        complete_board(rng, known | mask_of(v), full_board, k);  // 2. deal around all known cards
        showdown(hero[0], hero[1], v.a, v.b, full_board, t);     // 3. who wins?
    }
    return to_result(t);
}

EquityResult range_vs_range_monte_carlo(const Range& a,
                                        const Range& b,
                                        const std::vector<Card>& board,
                                        long long iterations,
                                        uint64_t seed) {
    if (board.size() > 5) throw std::invalid_argument("the board has at most 5 cards");
    uint64_t board_mask = add_cards(0, board);

    std::vector<Combo> live_a = live_or_throw(a, board);
    std::vector<Combo> live_b = live_or_throw(b, board);
    auto pick_a = weighted_picker(live_a);
    auto pick_b = weighted_picker(live_b);

    std::mt19937_64 rng(seed);
    Card full_board[5];
    const int k = static_cast<int>(board.size());
    for (int i = 0; i < k; ++i) full_board[i] = board[i];

    Tally t;
    for (long long it = 0; it < iterations; ++it) {
        // Draw a PAIR of combos; if they share a card, reject BOTH and redraw.
        // Redrawing only b would over-represent a-combos that rarely collide.
        const Combo* x = nullptr;
        const Combo* y = nullptr;
        int attempts = 0;
        do {
            if (++attempts > 10000) {
                throw std::invalid_argument("the two ranges (almost) always share a card");
            }
            x = &live_a[pick_a(rng)];
            y = &live_b[pick_b(rng)];
        } while (x->overlaps(*y));

        complete_board(rng, board_mask | mask_of(*x) | mask_of(*y), full_board, k);
        showdown(x->a, x->b, y->a, y->b, full_board, t);
    }
    return to_result(t);
}
