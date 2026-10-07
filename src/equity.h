#pragma once
// ============================================================================
// Step 3: Equity — "if all the cards were dealt out, how often do I win?"
// ----------------------------------------------------------------------------
// Heads-up (2 players): hero vs villain, each with 2 hole cards, plus 0..5
// known board cards. We deal the rest of the board and count wins/ties/losses.
//
//   equity = P(win) + P(tie) / 2        (a tie splits the pot)
//
// Two ways to compute it:
//   equity_exact        — try EVERY possible remaining board. Perfect answer.
//   equity_monte_carlo  — try N RANDOM boards. Approximate, error ~ 1/sqrt(N),
//                         but works even when "every board" is far too many.
// ============================================================================

#include "cards.h"
#include "range.h"

#include <cstdint>
#include <vector>

struct EquityResult {
    double    win  = 0;   // fraction of boards hero wins outright
    double    tie  = 0;   // fraction that split the pot
    double    lose = 0;   // fraction hero loses
    long long trials = 0; // how many boards were evaluated

    double equity() const { return win + tie / 2.0; }

    // Monte Carlo only: the "standard error", roughly how far the estimate is
    // likely to be from the truth. About 2/3 of the time the true equity lies
    // within +/- 1 standard error, and about 95% within +/- 2.
    double std_error() const;
};

// Both throw std::invalid_argument if a hand isn't exactly 2 cards, the board
// has more than 5 cards, or the same card appears twice.
EquityResult equity_exact(const std::vector<Card>& hero,
                          const std::vector<Card>& villain,
                          const std::vector<Card>& board);

EquityResult equity_monte_carlo(const std::vector<Card>& hero,
                                const std::vector<Card>& villain,
                                const std::vector<Card>& board,
                                long long iterations,
                                uint64_t seed = 12345);   // fixed seed = repeatable results

// ============================================================================
// Step 5: equity against RANGES.
// ----------------------------------------------------------------------------
// Only "live" combos count: any combo using a card in hero's hand or on the
// board is impossible and gets skipped (card removal). Each live combo counts
// in proportion to its weight. All three throw std::invalid_argument if no
// combo is live.
// ============================================================================

// Hero's exact hand vs a range: the weighted average of equity_exact against
// every live combo. Fast on the flop or later; preflop each combo costs ~1.7M
// boards, so prefer Monte Carlo there for wide ranges.
EquityResult equity_vs_range_exact(const std::vector<Card>& hero,
                                   const Range& villain,
                                   const std::vector<Card>& board);

// Same question by sampling: each trial draws a villain combo (weighted),
// then a random board.
EquityResult equity_vs_range_monte_carlo(const std::vector<Card>& hero,
                                         const Range& villain,
                                         const std::vector<Card>& board,
                                         long long iterations,
                                         uint64_t seed = 12345);

// Range vs range, from range `a`'s point of view. Each trial draws one combo
// from each range; if they share a card, both are redrawn.
EquityResult range_vs_range_monte_carlo(const Range& a,
                                        const Range& b,
                                        const std::vector<Card>& board,
                                        long long iterations,
                                        uint64_t seed = 12345);
