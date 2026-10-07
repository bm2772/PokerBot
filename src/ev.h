#pragma once
// ============================================================================
// Step 4: Expected value (EV) — turning equity into decisions.
// ----------------------------------------------------------------------------
// EV = sum over outcomes of  P(outcome) * (chips won or lost in that outcome)
//
// Conventions used by every function here:
//   * All amounts are in chips (or big blinds; the units don't matter).
//   * `pot` = everything ALREADY in the middle, INCLUDING any bet you face.
//   * Chips already in the pot are no longer yours ("sunk"), so FOLDING IS
//     ALWAYS WORTH 0, and every other option is measured against that.
//   * No further betting: the hand goes straight to showdown. True for
//     all-ins and river decisions; multi-street play needs a game tree (Step 6).
// ============================================================================

// EV of folding. Always zero by our convention; it exists so code reads as
// "compare ev_call(...) against ev_fold()".
double ev_fold();

// EV of calling `to_call` into `pot` with `equity` (0..1).
//   win  (prob equity):    you gain everything in the pot -> +pot
//   lose (prob 1-equity):  you lose your call             -> -to_call
//   EV = equity * pot - (1 - equity) * to_call
//      = equity * (pot + to_call) - to_call        (same thing, rearranged)
double ev_call(double equity, double pot, double to_call);

// "Pot odds": the minimum equity needed for a call to break even.
// Solve ev_call = 0 for equity:   to_call / (pot + to_call)
double required_equity(double pot, double to_call);

// EV of betting `bet` into `pot` when the opponent folds with probability
// `fold_prob`, and you have `equity_when_called` against the hands that call.
//   they fold:  you win the pot                   -> +pot
//   they call:  final pot is pot + 2*bet, you put in `bet`
//               -> equity_when_called * (pot + 2*bet) - bet
double ev_bet(double pot, double bet, double fold_prob, double equity_when_called);

// How often a pure bluff (0% equity) must make them fold to break even.
// Solve ev_bet(fold_prob, equity=0) = 0:   bet / (pot + bet)
double breakeven_fold_frequency(double pot, double bet);
