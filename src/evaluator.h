#pragma once
// ============================================================================
// Step 2: Hand evaluator
// ----------------------------------------------------------------------------
// evaluate() takes 5, 6, or 7 cards, finds the best 5-card poker hand among
// them, and returns a HandScore: a single integer where BIGGER = BETTER.
// Comparing two hands is then just `a > b`.
//
// Score layout (like the card index in Step 1, a number built from "digits"):
//
//   bits:  23..20     19..16   15..12   11..8    7..4     3..0
//          category | k1     | k2     | k3     | k4     | k5
//
// The category (pair, flush, ...) sits in the highest bits, so it always
// decides first. Within a category, k1..k5 are the ranks that break ties,
// most important first (e.g. for "Pair of Kings, A-9-4 kickers":
// k1=King, k2=Ace, k3=Nine, k4=Four). Each rank is 0..12, which fits in 4 bits.
// ============================================================================

#include "cards.h"

#include <cstdint>
#include <string>

enum class HandCategory : uint8_t {
    HighCard = 0,
    OnePair,
    TwoPair,
    ThreeOfAKind,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush,
};

// `using` creates a type alias, like a nickname. HandScore IS a uint32_t, but
// the name tells the reader what the number means.
using HandScore = uint32_t;

// Evaluate `n` cards (5..7) starting at `cards`. Passing a pointer + count is
// the C way of passing an array, which you already know. It works with plain
// arrays, std::array, and std::vector (via .data()).
HandScore evaluate(const Card* cards, int n);

// Pull the category back out of a score (the top "digit").
HandCategory category_of(HandScore score);

// "Full House", "Flush", ...
const char* category_name(HandCategory category);

// A full human-readable description: "Full House, Kings full of Fives".
std::string describe(HandScore score);
