// ============================================================================
// cards.cpp — the IMPLEMENTATION of what cards.h declared.
// ============================================================================
#include "cards.h"

#include <random>     // std::mt19937 — a good random number generator
#include <algorithm>  // std::shuffle
#include <cctype>     // std::toupper, std::tolower, std::isspace
#include <stdexcept>  // std::invalid_argument

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

// Find `c` in a lookup table; return its position, or -1 if it's not there.
// This is the reverse of printing: printing goes number -> char, parsing goes
// char -> number, using the SAME table so the two can never disagree.
static int find_char(const char* table, int size, char c) {
    for (int i = 0; i < size; ++i) {
        if (table[i] == c) return i;
    }
    return -1;
}

Card parse_card(const std::string& text) {
    if (text.size() != 2) {
        throw std::invalid_argument("card must be 2 characters, got '" + text + "'");
    }
    // Normalize case so "ah", "AH", "Ah" all work: ranks upper, suits lower.
    int r = find_char(RANK_CHARS, 13, static_cast<char>(std::toupper(text[0])));
    int s = find_char(SUIT_CHARS, 4,  static_cast<char>(std::tolower(text[1])));
    if (r < 0 || s < 0) {
        throw std::invalid_argument("not a valid card: '" + text + "'");
    }
    return Card(static_cast<Rank>(r), static_cast<Suit>(s));
}

std::vector<Card> parse_cards(const std::string& text) {
    std::vector<Card> out;
    size_t i = 0;
    while (i < text.size()) {
        // Skip separators.
        if (std::isspace(static_cast<unsigned char>(text[i])) || text[i] == ',') {
            ++i;
            continue;
        }
        // Every card is exactly 2 characters, so take the next two.
        out.push_back(parse_card(text.substr(i, 2)));
        i += 2;
    }
    return out;
}

std::string cards_to_string(const std::vector<Card>& cards) {
    std::string s;
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) s += ' ';
        s += cards[i].to_string();
    }
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
