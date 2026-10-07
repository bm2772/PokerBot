// ============================================================================
// test_evaluator.cpp — prove the evaluator correct.
//
// Strategy:
//   1. Hand-picked edge cases where we know which hand should win.
//   2. Evaluate EVERY possible 5-card hand (2.6 million) and EVERY possible
//      7-card hand (133.8 million), count each category, and compare with the
//      known published totals. If every count matches, we can be confident
//      in the evaluator.
// ============================================================================
#include "cards.h"
#include "evaluator.h"

#include <chrono>    // timing
#include <cstdio>    // printf: C's print function, works fine in C++

static int failures = 0;

// A minimal "assert" that reports instead of crashing, so we see every failure.
static void check(bool ok, const char* what) {
    std::printf("  [%s] %s\n", ok ? " OK " : "FAIL", what);
    if (!ok) ++failures;
}

static HandScore eval_str(const char* text) {
    std::vector<Card> c = parse_cards(text);
    return evaluate(c.data(), static_cast<int>(c.size()));
}

static void test_edge_cases() {
    std::printf("Edge cases:\n");

    check(category_of(eval_str("Ah 2c 3d 4s 5h")) == HandCategory::Straight,
          "A-2-3-4-5 (the wheel) is a straight");
    check(eval_str("2h 3c 4d 5s 6h") > eval_str("Ah 2c 3d 4s 5h"),
          "6-high straight beats the wheel (Ace plays LOW there)");
    check(category_of(eval_str("Qh Kc Ad 2s 3h")) == HandCategory::HighCard,
          "Q-K-A-2-3 is NOT a straight (no wrapping around)");
    check(describe(eval_str("Th Jh Qh Kh Ah")) == "Royal Flush",
          "T-J-Q-K-A suited is a royal flush");
    check(eval_str("2h 4h 6h 8h Th") > eval_str("Ac Kd Qh Js Tc"),
          "Any flush beats any straight");
    check(eval_str("Ah Ad Kc Qs 9h") > eval_str("Ah Ad Kc Qs 8h"),
          "Pair with better 3rd kicker wins");
    check(eval_str("Kh Kd 5c 5s 2h 2d Ac") == eval_str("Kc Ks 5h 5d 3h 3d Ah"),
          "Three pairs: only the top two count + best kicker (both K-K-5-5-A)");
    check(category_of(eval_str("7h 7d 7c 4s 4h 4d 2c")) == HandCategory::FullHouse,
          "Two sets of trips make a full house");
    check(describe(eval_str("7h 7d 7c 4s 4h 4d 2c")) == "Full House, Sevens full of Fours",
          "...and it's Sevens full of Fours, not Fours full of Sevens");
    check(eval_str("9h 9d 9c 9s 2h") < eval_str("9h 9d 9c 9s Ah"),
          "Quads: the kicker matters");
}

// Known totals for each category, HighCard first, StraightFlush last.
// (Royal flushes are counted as straight flushes.)
static const long long FIVE_CARD_COUNTS[9] = {
    1302540, 1098240, 123552, 54912, 10200, 5108, 3744, 624, 40
};
static const long long SEVEN_CARD_COUNTS[9] = {
    23294460, 58627800, 31433400, 6461620, 6180020, 4047644, 3473184, 224848, 41584
};

static void compare_counts(const long long* got, const long long* expected) {
    for (int c = 0; c < 9; ++c) {
        char line[128];
        std::snprintf(line, sizeof line, "%-16s %10lld  (expected %10lld)",
                      category_name(static_cast<HandCategory>(c)), got[c], expected[c]);
        check(got[c] == expected[c], line);
    }
}

static void test_all_5_card_hands() {
    std::printf("\nAll 2,598,960 five-card hands:\n");
    long long counts[9] = {0};
    Card h[5];
    // Five nested loops with a < b < c < d < e visit every combination exactly
    // once (order doesn't matter in a hand, so we never revisit a set).
    for (int a = 0; a < 52; ++a) { h[0] = Card(static_cast<uint8_t>(a));
    for (int b = a + 1; b < 52; ++b) { h[1] = Card(static_cast<uint8_t>(b));
    for (int c = b + 1; c < 52; ++c) { h[2] = Card(static_cast<uint8_t>(c));
    for (int d = c + 1; d < 52; ++d) { h[3] = Card(static_cast<uint8_t>(d));
    for (int e = d + 1; e < 52; ++e) { h[4] = Card(static_cast<uint8_t>(e));
        counts[static_cast<int>(category_of(evaluate(h, 5)))]++;
    }}}}}
    compare_counts(counts, FIVE_CARD_COUNTS);
}

static void test_all_7_card_hands() {
    std::printf("\nAll 133,784,560 seven-card hands (takes a few seconds):\n");
    auto start = std::chrono::steady_clock::now();

    long long counts[9] = {0};
    Card h[7];
    for (int a = 0; a < 52; ++a) { h[0] = Card(static_cast<uint8_t>(a));
    for (int b = a + 1; b < 52; ++b) { h[1] = Card(static_cast<uint8_t>(b));
    for (int c = b + 1; c < 52; ++c) { h[2] = Card(static_cast<uint8_t>(c));
    for (int d = c + 1; d < 52; ++d) { h[3] = Card(static_cast<uint8_t>(d));
    for (int e = d + 1; e < 52; ++e) { h[4] = Card(static_cast<uint8_t>(e));
    for (int f = e + 1; f < 52; ++f) { h[5] = Card(static_cast<uint8_t>(f));
    for (int g = f + 1; g < 52; ++g) { h[6] = Card(static_cast<uint8_t>(g));
        counts[static_cast<int>(category_of(evaluate(h, 7)))]++;
    }}}}}}}

    double secs = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    compare_counts(counts, SEVEN_CARD_COUNTS);
    std::printf("  Speed: %.2f s total, %.1f million hands/second\n",
                secs, 133784560.0 / secs / 1e6);
}

int main() {
    test_edge_cases();
    test_all_5_card_hands();
    test_all_7_card_hands();

    std::printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASSED" : "FAILED",
                failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;   // nonzero exit code = failure, by convention
}
