#pragma once
// ============================================================================
// Step 5: Ranges — every hand an opponent COULD have, not just one.
// ----------------------------------------------------------------------------
// A range is stored as a list of concrete two-card COMBOS, each with a weight
// in (0, 1]: "how often they'd play this exact hand this way".
//
// Combo counts (out of 1326 possible starting hands):
//   pair     "QQ"   = 6   (choose 2 of the 4 queens)
//   suited   "AKs"  = 4   (one per suit)
//   offsuit  "AKo"  = 12  (4 ace suits x 3 OTHER king suits)
//   both     "AK"   = 16
//
// Notation accepted by Range::parse (comma-separated, spaces ignored):
//   QQ  AKs  AKo  AK         single hand classes
//   TT+                      TT, JJ, QQ, KK, AA
//   ATs+   AT+               ATs, AJs, AQs, AKs  (kicker climbs up to just below A)
//   55-88   A2s-A5s          spans between two classes of the same shape
//   AhKh                     one exact combo
//   any                      all 1326 combos
//   <anything>:0.5           weight; a later mention of a combo overrides an earlier one
// ============================================================================

#include "cards.h"

#include <string>
#include <vector>

struct Combo {
    Card   a, b;          // stored with a.index > b.index, so each combo has ONE spelling
    double weight = 1.0;

    bool uses(Card c) const { return a.index == c.index || b.index == c.index; }
    bool overlaps(const Combo& o) const { return uses(o.a) || uses(o.b); }
    std::string to_string() const { return a.to_string() + b.to_string(); }
};

class Range {
public:
    Range();

    // Build a range from notation (see above). Throws std::invalid_argument
    // on bad input, naming the piece it couldn't understand.
    static Range parse(const std::string& text);

    // Add one combo, or overwrite its weight if it's already in the range.
    void add(Card a, Card b, double weight = 1.0);

    const std::vector<Combo>& combos() const { return combos_; }
    size_t size() const { return combos_.size(); }
    double total_weight() const;            // sum of weights: "effective" combo count

    // Card removal: the combos still possible when `dead` cards are known to
    // be elsewhere (in your hand, on the board). This is how blockers work.
    std::vector<Combo> live_combos(const std::vector<Card>& dead) const;

private:
    std::vector<Combo> combos_;
    // slot_[a*52 + b] = position of combo {a,b} in combos_, or -1 if absent.
    // A direct lookup table: 2704 ints, instant "is this combo already here?".
    std::vector<int> slot_;
};
