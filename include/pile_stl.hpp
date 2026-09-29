#ifndef PILE_STL_HPP
#define PILE_STL_HPP

// Adaptateur de reference delegant a la bibliotheque standard.
//
// Ce conteneur n'est pas utilise dans les conversions de notations : il sert
// uniquement de point de comparaison dans le banc d'essai, afin de verifier
// que l'implementation manuelle sur tableau dynamique se comporte comme la
// STL officielle (meme cout amorti, meme occupation memoire).

#include <cstddef>
#include <stack>
#include <vector>

namespace piles {

template <typename T>
class PileSTL {
public:
    using valeur_type = T;

    explicit PileSTL(std::size_t capaciteInitiale = 8) {
        elements_.reserve(capaciteInitiale);
    }

    void push(const T& valeur) {
        elements_.push_back(valeur);
    }

    void pop() {
        elements_.pop_back();
    }

    T& top() {
        return elements_.back();
    }

    const T& top() const {
        return elements_.back();
    }

    bool empty() const {
        return elements_.empty();
    }

    std::size_t size() const {
        return elements_.size();
    }

    std::size_t capacity() const {
        return elements_.capacity();
    }

    void clear() {
        elements_.clear();
    }

    void reserve(std::size_t nouvelleCapacite) {
        elements_.reserve(nouvelleCapacite);
    }

    void reduireCapacite() {
        elements_.shrink_to_fit();
    }

private:
    std::vector<T> elements_;
};

} // namespace piles

#endif // PILE_STL_HPP
