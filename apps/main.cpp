// ============================================================================
// main.cpp — demo of Steps 2-4, plus a small command-line tool.
//
//   ./poker                                  run the guided demo
//   ./poker equity <hero> <villain> [board]  exact equity for your own spot
//        e.g. ./poker equity AhKh QsQd 7h2h9c
//
// `argc` = how many words were typed (including the program name), and
// `argv` = the words themselves. Same as in C.
// ============================================================================
#include "cards.h"
#include "equity.h"
#include "evaluator.h"
#include "ev.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

static void print_equity(const char* label, const EquityResult& r) {
    std::printf("  %-10s equity %6.2f%%   (win %6.2f%%  tie %5.2f%%  lose %6.2f%%)\n",
                label, 100 * r.equity(), 100 * r.win, 100 * r.tie, 100 * r.lose);
}

// ---------------------------------------------------------------------------
static void demo_evaluator() {
    std::printf("=== Step 2: Hand evaluator =====================================\n");
    std::vector<Card> hero    = parse_cards("AhKh");
    std::vector<Card> villain = parse_cards("QsQd");
    std::vector<Card> board   = parse_cards("7h 2h 9c Kd Th");

    // Combine hole cards + board into one 7-card hand for each player.
    // `insert` appends a range of elements to the end of a vector.
    std::vector<Card> h = hero;    h.insert(h.end(), board.begin(), board.end());
    std::vector<Card> v = villain; v.insert(v.end(), board.begin(), board.end());

    HandScore hs = evaluate(h.data(), 7);
    HandScore vs = evaluate(v.data(), 7);

    std::printf("  Board:   %s\n", cards_to_string(board).c_str());
    std::printf("  Hero     %s -> %-28s (score 0x%06X)\n",
                cards_to_string(hero).c_str(), describe(hs).c_str(), hs);
    std::printf("  Villain  %s -> %-28s (score 0x%06X)\n",
                cards_to_string(villain).c_str(), describe(vs).c_str(), vs);
    std::printf("  Winner: %s\n\n", hs > vs ? "Hero" : hs < vs ? "Villain" : "Split pot");
    // Printing the score in hex (0x...) shows the packed "digits" directly:
    // first hex digit = category, then one hex digit per tie-break rank.
}

// ---------------------------------------------------------------------------
static void demo_equity() {
    std::printf("=== Step 3: Equity ==============================================\n");
    std::vector<Card> aces  = parse_cards("AhAs");
    std::vector<Card> kings = parse_cards("KdKc");
    std::vector<Card> no_board;

    auto t0 = std::chrono::steady_clock::now();
    EquityResult exact = equity_exact(aces, kings, no_board);
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    std::printf("  AhAs vs KdKc, preflop, EXACT over all %lld boards (%.2f s):\n",
                exact.trials, secs);
    print_equity("AhAs", exact);

    std::printf("\n  Monte Carlo: how fast does a random sample approach the truth?\n");
    std::printf("  %10s  %9s  %9s  %14s\n", "trials", "estimate", "error", "predicted 1 SE");
    for (long long n : {100LL, 1000LL, 10000LL, 100000LL, 1000000LL}) {
        EquityResult mc = equity_monte_carlo(aces, kings, no_board, n);
        std::printf("  %10lld  %8.2f%%  %8.2f%%  %13.2f%%\n",
                    n, 100 * mc.equity(), 100 * std::fabs(mc.equity() - exact.equity()),
                    100 * mc.std_error());
    }
    std::printf("  -> each 100x more trials gives ~10x less error (error ~ 1/sqrt(N)).\n\n");
}

// ---------------------------------------------------------------------------
static void demo_ev() {
    std::printf("=== Step 4: EV decisions ========================================\n");

    // Spot 1: facing an all-in on the flop with a flush draw + overcards.
    std::vector<Card> hero    = parse_cards("AhKh");
    std::vector<Card> villain = parse_cards("QsQd");
    std::vector<Card> board   = parse_cards("7h 2h 9c");
    double pot_before = 150, shove = 100;
    double pot = pot_before + shove;          // the pot INCLUDES the bet we face

    EquityResult r = equity_exact(hero, villain, board);
    double need = required_equity(pot, shove);
    double ev   = ev_call(r.equity(), pot, shove);

    std::printf("  Spot 1: Hero %s vs Villain %s on %s\n",
                cards_to_string(hero).c_str(), cards_to_string(villain).c_str(),
                cards_to_string(board).c_str());
    std::printf("  Pot was %.0f, villain shoves %.0f -> pot %.0f, hero must call %.0f.\n",
                pot_before, shove, pot, shove);
    print_equity("Hero", r);
    std::printf("  Pot odds: need %.1f%% equity to call (%.0f / (%.0f + %.0f)).\n",
                100 * need, shove, pot, shove);
    std::printf("  EV(call) = %.3f x %.0f - %.0f = %+.1f chips   vs   EV(fold) = %.1f\n",
                r.equity(), pot + shove, shove, ev, ev_fold());
    std::printf("  Decision: %s\n\n", ev > ev_fold() ? "CALL" : "FOLD");

    // Spot 2: a river bluff. Hero has nothing (0% equity if called).
    double river_pot = 100, bet = 75;
    double be = breakeven_fold_frequency(river_pot, bet);
    std::printf("  Spot 2: River bluff of %.0f into %.0f with a hand that never wins.\n",
                bet, river_pot);
    std::printf("  Break-even: villain must fold %.1f%% of the time (%.0f / (%.0f + %.0f)).\n",
                100 * be, bet, river_pot, bet);
    for (double f : {0.30, be, 0.60}) {
        std::printf("    if villain folds %5.1f%%:  EV(bluff) = %+6.1f chips\n",
                    100 * f, ev_bet(river_pot, bet, f, 0.0));
    }
    std::printf("\n");
}

// ---------------------------------------------------------------------------
static int run_equity_command(int argc, char** argv) {
    if (argc < 4) {
        std::printf("usage: %s equity <hero> <villain> [board]\n", argv[0]);
        return 1;
    }
    std::vector<Card> hero    = parse_cards(argv[2]);
    std::vector<Card> villain = parse_cards(argv[3]);
    std::vector<Card> board   = argc >= 5 ? parse_cards(argv[4]) : std::vector<Card>{};

    EquityResult r = equity_exact(hero, villain, board);
    std::printf("Board: %s  (%lld possible run-outs)\n",
                board.empty() ? "(preflop)" : cards_to_string(board).c_str(), r.trials);
    print_equity(cards_to_string(hero).c_str(), r);
    // Villain's numbers are hero's mirrored: villain wins when hero loses.
    EquityResult v = r;
    v.win = r.lose;
    v.lose = r.win;
    print_equity(cards_to_string(villain).c_str(), v);
    return 0;
}

int main(int argc, char** argv) {
    // try/catch: if anything inside `throw`s (e.g. a typo like "Zx" in a card),
    // control jumps to `catch`, and we print the message instead of crashing.
    try {
        if (argc >= 2 && std::string(argv[1]) == "equity") {
            return run_equity_command(argc, argv);
        }
        demo_evaluator();
        demo_equity();
        demo_ev();
    } catch (const std::exception& e) {
        std::printf("error: %s\n", e.what());
        return 1;
    }
    return 0;
}
