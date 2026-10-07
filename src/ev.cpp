// ============================================================================
// ev.cpp — each function is the formula documented in ev.h, nothing more.
// Keeping them this small makes each one easy to check by hand.
// ============================================================================
#include "ev.h"

double ev_fold() {
    return 0.0;
}

double ev_call(double equity, double pot, double to_call) {
    return equity * (pot + to_call) - to_call;
}

double required_equity(double pot, double to_call) {
    return to_call / (pot + to_call);
}

double ev_bet(double pot, double bet, double fold_prob, double equity_when_called) {
    double ev_if_fold   = pot;
    double ev_if_called = equity_when_called * (pot + 2.0 * bet) - bet;
    return fold_prob * ev_if_fold + (1.0 - fold_prob) * ev_if_called;
}

double breakeven_fold_frequency(double pot, double bet) {
    return bet / (pot + bet);
}
