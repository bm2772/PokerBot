#pragma once
// ============================================================================
// Step 1: Cards and deck
// ----------------------------------------------------------------------------
// This header DECLARES the types we use to talk about cards. In C++ we split
// code into headers (.h, the "what exists") and source (.cpp, the "how it
// works"). `#pragma once` tells the compiler to include this file only once,
// even if several files ask for it — it prevents duplicate-definition errors.
// ============================================================================

#include <array>      // std::array  — a fixed-size array (the 52-card deck)
#include <cstdint>    // uint8_t     — a 1-byte unsigned integer (0..255)
#include <string>     // std::string — text, for printing a card like "Ah"

// ----------------------------------------------------------------------------
// Rank and Suit as enums.
//
// An `enum class` is a named set of integer constants. Using one (instead of
// raw ints) means the compiler stops you from accidentally mixing a rank up
// with a suit. We anchor the numeric values deliberately:
//   - Two = 0 ... Ace = 12   (so rank fits in 0..12, exactly 13 values)
//   - Clubs = 0 ... Spades = 3 (suit fits in 0..3, exactly 4 values)
// ----------------------------------------------------------------------------
enum class Rank : uint8_t {
    Two = 0, Three, Four, Five, Six, Seven, Eight,
    Nine, Ten, Jack, Queen, King, Ace
};

enum class Suit : uint8_t {
    Clubs = 0, Diamonds, Hearts, Spades
};

// ----------------------------------------------------------------------------
// A Card is stored as a single integer 0..51. This is the key trick:
//   index = rank * 4 + suit
// From the index we can always recover rank (index / 4) and suit (index % 4).
// Packing it into one number makes shuffling, dealing, and (later) fast hand
// evaluation simple and cheap.
// ----------------------------------------------------------------------------
struct Card {
    uint8_t index;   // 0..51

    // Default constructor. std::array<Card,52> needs to be able to make a Card
    // with no arguments before we fill the deck in, so we give it index 0.
    Card() : index(0) {}

    // A "constructor": how to build a Card from a rank and suit.
    Card(Rank r, Suit s)
        : index(static_cast<uint8_t>(r) * 4 + static_cast<uint8_t>(s)) {}

    // Allow building straight from a raw 0..51 index too.
    explicit Card(uint8_t i) : index(i) {}

    Rank rank() const { return static_cast<Rank>(index / 4); }
    Suit suit() const { return static_cast<Suit>(index % 4); }

    // Human-readable form, e.g. "Ah" (ace of hearts), "Tc" (ten of clubs).
    std::string to_string() const;
};

// ----------------------------------------------------------------------------
// The deck: exactly 52 cards. std::array<Card, 52> is a fixed-size container
// (unlike std::vector which can grow). Fixed size is perfect here — a deck is
// always 52 cards.
// ----------------------------------------------------------------------------
class Deck {
public:
    Deck();                 // builds a fresh, ordered 52-card deck
    void shuffle();         // randomize the order
    Card deal();            // hand out the top card

    int cards_remaining() const { return 52 - dealt_; }
    void reset() { dealt_ = 0; }   // put all cards back (without reshuffling)

private:
    std::array<Card, 52> cards_;
    int dealt_ = 0;         // how many cards have been dealt off the top
};
