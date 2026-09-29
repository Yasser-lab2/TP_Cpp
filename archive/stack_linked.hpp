#ifndef STACK_LINKED_HPP
#define STACK_LINKED_HPP

#include <cstddef>
#include <stdexcept>

namespace piles {

// Pile generique implementee avec une liste chainee.
template <typename T>
class PileListe {
private:
    struct Noeud {
        T valeur;
        Noeud* suivant;
    };

    Noeud* sommet = nullptr;
    std::size_t taille = 0;

public:
    PileListe() = default;

    // Detruire tous les noeuds de la liste.
    ~PileListe() {
        while (sommet != nullptr) {
            Noeud* ancienSommet = sommet;
            sommet = sommet->suivant;
            delete ancienSommet;
        }
    }

    PileListe(const PileListe&) = delete;
    PileListe& operator=(const PileListe&) = delete;

    // Ajouter un element au sommet.
    void push(const T& valeur) {
        sommet = new Noeud{valeur, sommet};
        ++taille;
    }

    // Retirer l'element du sommet.
    void pop() {
        if (empty()) {
            throw std::out_of_range("La pile est vide, impossible de depiler.");
        }

        Noeud* ancienSommet = sommet;
        sommet = sommet->suivant;
        delete ancienSommet;
        --taille;
    }

    // Consulter l'element au sommet.
    T& top() {
        if (empty()) {
            throw std::out_of_range("La pile est vide.");
        }
        return sommet->valeur;
    }

    const T& top() const {
        if (empty()) {
            throw std::out_of_range("La pile est vide.");
        }
        return sommet->valeur;
    }

    // Verifier si la pile est vide.
    bool empty() const {
        return sommet == nullptr;
    }

    // Obtenir la taille de la pile.
    std::size_t size() const {
        return taille;
    }
};

} // namespace piles

#endif // STACK_LINKED_HPP
