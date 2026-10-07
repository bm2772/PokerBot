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
