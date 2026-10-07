// ============================================================================
// test_range.cpp — check range parsing, card removal and range equity.
// ============================================================================
#include "cards.h"
#include "equity.h"
#include "range.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>

static int failures = 0;

static void check(bool ok, const char* what) {
    std::printf("  [%s] %s\n", ok ? " OK " : "FAIL", what);
    if (!ok) ++failures;
}

static void test_combo_counts() {
    std::printf("Combo counts:\n");
    struct Case { const char* text; size_t combos; };
    const Case cases[] = {
        {"AA", 6}, {"AKs", 4}, {"AKo", 12}, {"AK", 16}, {"aks", 4},
        {"TT+", 30}, {"22+", 78}, {"ATs+", 16}, {"KQs+", 4},
        {"55-88", 24}, {"A2s-A5s", 16}, {"A5s-A2s", 16},
        {"AhKh", 1}, {"any", 1326}, {"AA, AA", 6}, {"TT+, AK,", 46},
    };
    for (const Case& c : cases) {
        size_t got = Range::parse(c.text).size();
        char line[96];
        std::snprintf(line, sizeof line, "%-10s -> %4zu combos (expected %zu)", c.text, got, c.combos);
        check(got == c.combos, line);
    }

    Range r = Range::parse("AK, AKs:0.5");
    check(r.size() == 16 && r.total_weight() == 14.0,
          "\"AK, AKs:0.5\": 16 combos, weight 12x1 + 4x0.5 = 14 (last mention wins)");
}

static void test_bad_input() {
    std::printf("\nBad input is rejected:\n");
    const char* bad[] = {"AKx", "XX", "A", "AAs", "AA-KQs", "AK-QJ", "AKs:abc", "AKs:2", "AhAh", ""};
    for (const char* text : bad) {
        bool threw = false;
        try {
            Range::parse(text);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        char line[64];
        std::snprintf(line, sizeof line, "\"%s\" throws", text);
        check(threw, line);
    }
}

static void test_card_removal() {
    std::printf("\nCard removal (blockers):\n");
    Range r = Range::parse("AA, KK, AK");
    std::vector<Card> board = parse_cards("Ah 7c 2d");
    check(r.size() == 28, "AA,KK,AK = 6 + 6 + 16 = 28 combos");
    check(Range::parse("AA").live_combos(board).size() == 3, "with Ah on board, AA: 6 -> 3");
    check(Range::parse("AK").live_combos(board).size() == 12, "with Ah on board, AK: 16 -> 12");
    check(r.live_combos(board).size() == 21, "so AA,KK,AK: 28 -> 21");
}

// |a - b| within `k` standard errors of a Monte Carlo result (plus `slack`
// for a published value that was rounded).
static bool close(double estimate, double truth, double se, double k = 4, double slack = 0) {
    return std::fabs(estimate - truth) <= k * se + slack;
}

static void test_equity() {
    std::printf("\nEquity vs ranges:\n");
    std::vector<Card> hero  = parse_cards("AhKh");
    std::vector<Card> board = parse_cards("7h 2h 9c");
    char line[128];

    double single = equity_exact(hero, parse_cards("QsQd"), board).equity();
    double as_range = equity_vs_range_exact(hero, Range::parse("QsQd"), board).equity();
    std::snprintf(line, sizeof line, "one-combo range = single hand (%.4f%% vs %.4f%%)",
                  100 * as_range, 100 * single);
    check(as_range == single, line);

    Range wide = Range::parse("QQ, JJ, T9s, 87s, A9");
    double exact = equity_vs_range_exact(hero, wide, board).equity();
    EquityResult mc = equity_vs_range_monte_carlo(hero, wide, board, 200000);
    std::snprintf(line, sizeof line, "flop: Monte Carlo %.2f%% within 4 SE of exact %.2f%%",
                  100 * mc.equity(), 100 * exact);
    check(close(mc.equity(), exact, mc.std_error()), line);

    std::vector<Card> no_board;
    EquityResult aa = equity_vs_range_monte_carlo(parse_cards("AsAc"), Range::parse("any"),
                                                  no_board, 1000000);
    std::snprintf(line, sizeof line, "AA vs a random hand = %.2f%% (published: 85.2%%)",
                  100 * aa.equity());
    check(close(aa.equity(), 0.852, aa.std_error(), 4, 0.0005), line);

    EquityResult mirror = range_vs_range_monte_carlo(Range::parse("TT+, AK"),
                                                     Range::parse("TT+, AK"), no_board, 300000);
    std::snprintf(line, sizeof line, "a range vs itself = %.2f%% (must be 50%%)", 100 * mirror.equity());
    check(close(mirror.equity(), 0.5, mirror.std_error()), line);

    double aa_kk = equity_exact(parse_cards("AhAs"), parse_cards("KdKc"), no_board).equity();
    EquityResult rr = range_vs_range_monte_carlo(Range::parse("AhAs"), Range::parse("KdKc"),
                                                 no_board, 500000);
    std::snprintf(line, sizeof line, "range-vs-range with single combos %.2f%% ~ exact %.2f%%",
                  100 * rr.equity(), 100 * aa_kk);
    check(close(rr.equity(), aa_kk, rr.std_error()), line);

    bool threw = false;
    try {
        range_vs_range_monte_carlo(Range::parse("AhKh"), Range::parse("AhKh"), no_board, 10);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "two ranges that always collide throw instead of looping forever");
}

int main() {
    test_combo_counts();
    test_bad_input();
    test_card_removal();
    test_equity();
    std::printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASSED" : "FAILED",
                failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
