// ============================================================================
// main.cpp — a tiny program to SEE Step 1 working.
// `main` is where every C++ program starts running.
// ============================================================================
#include "cards.h"

#include <iostream>   // std::cout — print to the screen

int main() {
    // 1) Show that rank/suit math works for a specific card.
    Card ace_of_hearts(Rank::Ace, Suit::Hearts);
    std::cout << "Built a card: " << ace_of_hearts.to_string()
              << "  (raw index " << static_cast<int>(ace_of_hearts.index) << ")\n\n";

    // 2) Build a deck, shuffle it, and deal a 2-player hand + a board.
    Deck deck;
    deck.shuffle();

    std::cout << "Dealing a Texas Hold'em hand:\n";

    Card p1a = deck.deal(), p1b = deck.deal();   // player 1's two hole cards
    Card p2a = deck.deal(), p2b = deck.deal();   // player 2's two hole cards

    std::cout << "  Player 1: " << p1a.to_string() << " " << p1b.to_string() << "\n";
    std::cout << "  Player 2: " << p2a.to_string() << " " << p2b.to_string() << "\n";

    std::cout << "  Board:    ";
    for (int i = 0; i < 5; ++i) {                // flop(3) + turn(1) + river(1)
        std::cout << deck.deal().to_string() << " ";
    }
    std::cout << "\n\n";

    std::cout << "Cards left in deck: " << deck.cards_remaining() << "\n";
    return 0;
}
