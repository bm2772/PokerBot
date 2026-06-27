// ============================================================================
// cards.cpp — the IMPLEMENTATION of what cards.h declared.
// ============================================================================
#include "cards.h"

#include <random>     // std::mt19937 — a good random number generator
#include <algorithm>  // std::shuffle

// Lookup strings for printing. The index into these matches our enum values,
// because we defined Two=0..Ace=12 and Clubs=0..Spades=3.
static const char RANK_CHARS[13] = {
    '2','3','4','5','6','7','8','9','T','J','Q','K','A'
};
static const char SUIT_CHARS[4]  = { 'c','d','h','s' };

std::string Card::to_string() const {
    std::string s;
    s += RANK_CHARS[index / 4];   // rank char
    s += SUIT_CHARS[index % 4];   // suit char
    return s;
}

// Build an ordered deck: card 0, 1, 2, ... 51.
Deck::Deck() {
    for (uint8_t i = 0; i < 52; ++i) {
        cards_[i] = Card(i);
    }
}

void Deck::shuffle() {
    // A random engine. We seed it from std::random_device so each program run
    // shuffles differently. mt19937 is the Mersenne Twister — fast and standard.
    static std::random_device rd;
    static std::mt19937 rng(rd());

    std::shuffle(cards_.begin(), cards_.end(), rng);
    dealt_ = 0;   // a fresh shuffle means nothing has been dealt yet
}

// Deal the next card off the "top" of the deck.
Card Deck::deal() {
    // (We trust the caller not to deal past 52 for now; we'll add safety later.)
    return cards_[dealt_++];
}
