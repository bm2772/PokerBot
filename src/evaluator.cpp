// ============================================================================
// evaluator.cpp — how we score a poker hand.
// ============================================================================
#include "evaluator.h"

// An "anonymous namespace" makes everything inside it private to this file
// (the same job `static` did in cards.cpp). These are helpers nobody outside
// needs to see, and hiding them prevents name clashes with other files.
namespace {

// Pack a category plus up to 5 tie-break ranks into one score (see the layout
// in evaluator.h). Parameters with `= 0` are optional: callers only pass as
// many kickers as their category uses.
HandScore make_score(HandCategory cat,
                     int k1 = 0, int k2 = 0, int k3 = 0, int k4 = 0, int k5 = 0) {
    return (static_cast<uint32_t>(cat) << 20)
         | (static_cast<uint32_t>(k1)  << 16)
         | (static_cast<uint32_t>(k2)  << 12)
         | (static_cast<uint32_t>(k3)  << 8)
         | (static_cast<uint32_t>(k4)  << 4)
         |  static_cast<uint32_t>(k5);
}

// Given a 13-bit mask of ranks present (bit r set = rank r present), return
// the top rank of the best straight, or -1 if there is no straight.
//
// Why a bitmask? Five consecutive ranks are just five consecutive 1-bits, so
// "is there a straight ending at X?" is a single AND, with no sorting needed.
//
// The one wrinkle is the "wheel" (A-2-3-4-5), where the Ace plays low. We
// handle it by building a 14-bit mask with an extra "ace-as-one" bit at the
// bottom:
//      ext bit 0      = Ace (low)
//      ext bit r + 1  = rank r     (Two at bit 1 ... Ace at bit 13)
int straight_top(uint32_t rank_mask) {
    uint32_t ext = (rank_mask << 1) | ((rank_mask >> 12) & 1u);

    // Slide a 5-bit window (0b11111 = 0x1F) from the highest position down,
    // so the FIRST hit is the best straight.
    for (int top = 13; top >= 4; --top) {
        uint32_t window = 0x1Fu << (top - 4);
        if ((ext & window) == window) {
            return top - 1;   // convert ext position back to a rank (wheel -> Five)
        }
    }
    return -1;
}

// Write the `count` highest ranks present in `mask` into `out`, best first.
// Used for kickers and for flushes. Returns how many were written.
int top_ranks(uint32_t mask, int count, int* out) {
    int written = 0;
    for (int r = 12; r >= 0 && written < count; --r) {
        if (mask & (1u << r)) out[written++] = r;
    }
    return written;
}

} // namespace

HandScore evaluate(const Card* cards, int n) {
    // ---- Pass 1: count everything in a single loop ------------------------
    int      rank_count[13] = {0};  // how many of each rank (for pairs/trips/quads)
    int      suit_count[4]  = {0};  // how many of each suit (for flushes)
    uint32_t suit_mask[4]   = {0};  // which ranks appear in each suit (straight flush)
    uint32_t rank_mask      = 0;    // which ranks appear at all (straights, kickers)

    for (int i = 0; i < n; ++i) {
        int r = static_cast<int>(cards[i].rank());
        int s = static_cast<int>(cards[i].suit());
        rank_count[r]++;
        suit_count[s]++;
        suit_mask[s] |= 1u << r;
        rank_mask    |= 1u << r;
    }

    // ---- Straight flush ----------------------------------------------------
    // A flush needs 5+ cards of one suit. With at most 7 cards, at most one
    // suit can have 5, so there's no need to compare several flush suits.
    int flush_suit = -1;
    for (int s = 0; s < 4; ++s) {
        if (suit_count[s] >= 5) flush_suit = s;
    }
    if (flush_suit >= 0) {
        int sf = straight_top(suit_mask[flush_suit]);
        if (sf >= 0) return make_score(HandCategory::StraightFlush, sf);
    }

    // ---- Group ranks by how many times they appear, highest rank first ----
    // Walking from Ace down means each list comes out sorted best-first.
    int quads = -1;
    int trips[4], n_trips = 0;
    int pairs[4], n_pairs = 0;
    for (int r = 12; r >= 0; --r) {
        if      (rank_count[r] == 4) quads = r;
        else if (rank_count[r] == 3) trips[n_trips++] = r;
        else if (rank_count[r] == 2) pairs[n_pairs++] = r;
    }

    // ---- Four of a kind ----------------------------------------------------
    if (quads >= 0) {
        int kicker = 0;
        top_ranks(rank_mask & ~(1u << quads), 1, &kicker);  // best card that isn't the quad
        return make_score(HandCategory::FourOfAKind, quads, kicker);
    }

    // ---- Full house: trips + (a pair OR a second set of trips) -------------
    if (n_trips >= 1 && (n_trips >= 2 || n_pairs >= 1)) {
        int t = trips[0];
        int p = -1;
        if (n_trips >= 2)                  p = trips[1];   // use 2 of the lower trips
        if (n_pairs >= 1 && pairs[0] > p)  p = pairs[0];
        return make_score(HandCategory::FullHouse, t, p);
    }

    // ---- Flush: the 5 highest cards of the flush suit ----------------------
    if (flush_suit >= 0) {
        int k[5] = {0};
        top_ranks(suit_mask[flush_suit], 5, k);
        return make_score(HandCategory::Flush, k[0], k[1], k[2], k[3], k[4]);
    }

    // ---- Straight ----------------------------------------------------------
    int st = straight_top(rank_mask);
    if (st >= 0) return make_score(HandCategory::Straight, st);

    // ---- Three of a kind + 2 kickers ---------------------------------------
    if (n_trips >= 1) {
        int k[2] = {0};
        top_ranks(rank_mask & ~(1u << trips[0]), 2, k);
        return make_score(HandCategory::ThreeOfAKind, trips[0], k[0], k[1]);
    }

    // ---- Two pair + 1 kicker ----------------------------------------------
    // With 7 cards you can have THREE pairs. Only the top two count, and the
    // third pair's rank is still available as the kicker. Removing just the
    // top two pairs from the mask handles that automatically.
    if (n_pairs >= 2) {
        int kicker = 0;
        top_ranks(rank_mask & ~(1u << pairs[0]) & ~(1u << pairs[1]), 1, &kicker);
        return make_score(HandCategory::TwoPair, pairs[0], pairs[1], kicker);
    }

    // ---- One pair + 3 kickers ----------------------------------------------
    if (n_pairs == 1) {
        int k[3] = {0};
        top_ranks(rank_mask & ~(1u << pairs[0]), 3, k);
        return make_score(HandCategory::OnePair, pairs[0], k[0], k[1], k[2]);
    }

    // ---- High card: best 5 ranks -------------------------------------------
    int k[5] = {0};
    top_ranks(rank_mask, 5, k);
    return make_score(HandCategory::HighCard, k[0], k[1], k[2], k[3], k[4]);
}

// ============================================================================
// Turning scores back into words
// ============================================================================

HandCategory category_of(HandScore score) {
    return static_cast<HandCategory>(score >> 20);   // the top "digit"
}

const char* category_name(HandCategory category) {
    switch (category) {
        case HandCategory::HighCard:      return "High Card";
        case HandCategory::OnePair:       return "One Pair";
        case HandCategory::TwoPair:       return "Two Pair";
        case HandCategory::ThreeOfAKind:  return "Three of a Kind";
        case HandCategory::Straight:      return "Straight";
        case HandCategory::Flush:         return "Flush";
        case HandCategory::FullHouse:     return "Full House";
        case HandCategory::FourOfAKind:   return "Four of a Kind";
        case HandCategory::StraightFlush: return "Straight Flush";
    }
    return "?";
}

std::string describe(HandScore score) {
    static const char* NAME[13] = {
        "Two", "Three", "Four", "Five", "Six", "Seven", "Eight",
        "Nine", "Ten", "Jack", "Queen", "King", "Ace"
    };
    static const char* PLURAL[13] = {
        "Twos", "Threes", "Fours", "Fives", "Sixes", "Sevens", "Eights",
        "Nines", "Tens", "Jacks", "Queens", "Kings", "Aces"
    };

    // Unpack the first two "digits" (the most informative ranks).
    int k1 = (score >> 16) & 0xF;
    int k2 = (score >> 12) & 0xF;
    std::string a = NAME[k1], as = PLURAL[k1], bs = PLURAL[k2];

    switch (category_of(score)) {
        case HandCategory::StraightFlush:
            return k1 == 12 ? "Royal Flush" : "Straight Flush, " + a + " high";
        case HandCategory::FourOfAKind:  return "Four of a Kind, " + as;
        case HandCategory::FullHouse:    return "Full House, " + as + " full of " + bs;
        case HandCategory::Flush:        return "Flush, " + a + " high";
        case HandCategory::Straight:     return "Straight, " + a + " high";
        case HandCategory::ThreeOfAKind: return "Three of a Kind, " + as;
        case HandCategory::TwoPair:      return "Two Pair, " + as + " and " + bs;
        case HandCategory::OnePair:      return "Pair of " + as;
        case HandCategory::HighCard:     return "High Card, " + a;
    }
    return "?";
}
