// ============================================================================
// range.cpp — parsing range notation into weighted combos.
// ============================================================================
#include "range.h"

#include <cctype>
#include <stdexcept>
#include <utility>   // std::swap

namespace {

Card make_card(int rank, int suit) {
    return Card(static_cast<Rank>(rank), static_cast<Suit>(suit));
}

// Does this text spell a card ("Ah")? `catch (...)` catches any error at all;
// here an error simply means "no".
bool is_card(const std::string& s) {
    try {
        parse_card(s);
        return true;
    } catch (...) {
        return false;
    }
}

// One "hand class" like QQ, AKs, AKo or AK, before it's expanded into combos.
//   kind: 'p' = pair, 's' = suited, 'o' = offsuit, 'b' = both suited and offsuit
struct HandClass {
    int  hi, lo;   // ranks, hi >= lo
    char kind;
};

std::invalid_argument bad(const std::string& token, const std::string& why) {
    return std::invalid_argument("can't parse range piece '" + token + "': " + why);
}

// "AKs" -> {Ace, King, 's'}.  "KA" is accepted too (we put the higher rank first).
HandClass parse_class(const std::string& s, const std::string& token) {
    if (s.size() != 2 && s.size() != 3) throw bad(token, "expected something like QQ, AKs, AKo or AK");

    int r1 = rank_from_char(s[0]);
    int r2 = rank_from_char(s[1]);
    if (r1 < 0 || r2 < 0) throw bad(token, "unknown rank");

    if (r1 == r2) {
        if (s.size() == 3) throw bad(token, "a pair can't be suited or offsuit");
        return {r1, r1, 'p'};
    }

    char kind = 'b';
    if (s.size() == 3) {
        kind = static_cast<char>(std::tolower(s[2]));
        if (kind != 's' && kind != 'o') throw bad(token, "third letter must be 's' or 'o'");
    }
    if (r1 < r2) std::swap(r1, r2);
    return {r1, r2, kind};
}

// Turn one hand class into its 4, 6, 12 or 16 combos.
void expand(Range& range, const HandClass& c, double weight) {
    for (int s1 = 0; s1 < 4; ++s1) {
        for (int s2 = 0; s2 < 4; ++s2) {
            if (c.kind == 'p') {
                // s1 < s2 so each pair of suits is used once: 6 combos.
                if (s1 < s2) range.add(make_card(c.hi, s1), make_card(c.hi, s2), weight);
            } else {
                bool suited = (s1 == s2);
                bool wanted = c.kind == 'b' || (c.kind == 's' && suited) || (c.kind == 'o' && !suited);
                if (wanted) range.add(make_card(c.hi, s1), make_card(c.lo, s2), weight);
            }
        }
    }
}

// Expand every class from `from` up to `to`, varying the lower card:
//   pairs:      55 -> 66 -> ... -> 88
//   non-pairs:  A2s -> A3s -> ... -> A5s   (top card fixed)
void expand_span(Range& range, HandClass from, HandClass to, double weight) {
    if (from.lo > to.lo) std::swap(from, to);
    for (int r = from.lo; r <= to.lo; ++r) {
        HandClass c = from;
        if (c.kind == 'p') c.hi = r;   // pairs: both cards climb
        c.lo = r;
        expand(range, c, weight);
    }
}

void parse_token(Range& range, const std::string& token) {
    // 1. Peel off an optional ":weight".
    std::string body = token;
    double weight = 1.0;
    size_t colon = token.find(':');
    if (colon != std::string::npos) {
        body = token.substr(0, colon);
        try {
            weight = std::stod(token.substr(colon + 1));   // string -> double
        } catch (const std::exception&) {
            throw bad(token, "weight after ':' must be a number");
        }
        if (weight <= 0 || weight > 1) throw bad(token, "weight must be in (0, 1]");
    }
    if (body.empty()) throw bad(token, "empty");

    // 2. "any": the whole deck of starting hands.
    std::string lower;
    for (char ch : body) lower += static_cast<char>(std::tolower(ch));
    if (lower == "any" || lower == "random") {
        for (int i = 0; i < 52; ++i)
            for (int j = 0; j < i; ++j)
                range.add(Card(static_cast<uint8_t>(i)), Card(static_cast<uint8_t>(j)), weight);
        return;
    }

    // 3. An exact combo like "AhKh": 4 characters that read as two cards.
    //    ("AKs+" is also 4 characters, but "AK" isn't a card, so it falls through.)
    if (body.size() == 4 && is_card(body.substr(0, 2)) && is_card(body.substr(2, 2))) {
        Card a = parse_card(body.substr(0, 2));
        Card b = parse_card(body.substr(2, 2));
        if (a.index == b.index) throw bad(token, "same card twice");
        range.add(a, b, weight);
        return;
    }

    // 4. A span "55-88" / "A2s-A5s".
    size_t dash = body.find('-');
    if (dash != std::string::npos) {
        HandClass from = parse_class(body.substr(0, dash), token);
        HandClass to   = parse_class(body.substr(dash + 1), token);
        bool same_shape = from.kind == to.kind && (from.kind == 'p' || from.hi == to.hi);
        if (!same_shape) throw bad(token, "both ends must be the same shape (e.g. 55-88, A2s-A5s)");
        expand_span(range, from, to, weight);
        return;
    }

    // 5. "TT+" / "ATs+": from this class up to the strongest of its shape.
    if (body.back() == '+') {
        HandClass from = parse_class(body.substr(0, body.size() - 1), token);
        HandClass to = from;
        to.lo = (from.kind == 'p') ? 12 : from.hi - 1;   // up to AA, or up to A-K / K-Q / ...
        expand_span(range, from, to, weight);
        return;
    }

    // 6. A single class "QQ" / "AKs".
    expand(range, parse_class(body, token), weight);
}

} // namespace

Range::Range() : slot_(52 * 52, -1) {}

void Range::add(Card a, Card b, double weight) {
    if (a.index == b.index) throw std::invalid_argument("a combo needs two different cards");
    if (a.index < b.index) std::swap(a, b);   // canonical order: one spelling per combo

    int& slot = slot_[a.index * 52 + b.index];   // a REFERENCE: writing `slot` writes the table
    if (slot >= 0) {
        combos_[slot].weight = weight;           // already present: last mention wins
    } else {
        slot = static_cast<int>(combos_.size());
        combos_.push_back(Combo{a, b, weight});
    }
}

Range Range::parse(const std::string& text) {
    Range range;
    std::string token;
    // Walk the text, collecting characters until a comma ends each token.
    // Whitespace is dropped, so "TT+, AK" and "TT+,AK" are the same.
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == ',') {
            if (!token.empty()) parse_token(range, token);
            token.clear();
        } else if (!std::isspace(static_cast<unsigned char>(text[i]))) {
            token += text[i];
        }
    }
    if (range.size() == 0) throw std::invalid_argument("range is empty");
    return range;
}

double Range::total_weight() const {
    double total = 0;
    for (const Combo& c : combos_) total += c.weight;
    return total;
}

std::vector<Combo> Range::live_combos(const std::vector<Card>& dead) const {
    // A 64-bit mask with bit i set if card i is dead. Same bitmask idea as in
    // the evaluator: one AND tells us whether a combo touches a dead card.
    uint64_t dead_mask = 0;
    for (const Card& c : dead) dead_mask |= 1ULL << c.index;

    std::vector<Combo> live;
    for (const Combo& c : combos_) {
        uint64_t combo_mask = (1ULL << c.a.index) | (1ULL << c.b.index);
        if ((combo_mask & dead_mask) == 0) live.push_back(c);
    }
    return live;
}
