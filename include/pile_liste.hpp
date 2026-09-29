#ifndef PILE_LISTE_HPP
#define PILE_LISTE_HPP

// Pile generique implementee sur une liste chainee.
//
// Representation : chaque noeud est alloue separement sur le tas et contient
// la valeur ainsi que le pointeur vers le noeud precedent. Le sommet de la
// pile est la tete de la liste.
//
// Complexite :
//   push   O(1), pop O(1), top O(1), empty O(1), size O(1).
// Toutes les operations sont exactes : aucune n'est amortie.

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace piles {

template <typename T>
class PileListe {
public:
    using valeur_type = T;

    // Nombre total d'allocations de noeuds effectuees par la classe.
    static std::size_t allocations;

    PileListe() = default;

    // Les noeuds sont liberes un par un : la destruction recopie d'abord la
    // tete courante avant de detruire le noeud, sinon le pointeur suivant
    // serait deja invalide.
    ~PileListe() {
        detruireNoeuds();
    }

    // La copie est profonde : elle recree un noeud par element, dans le meme
    // ordre. C'est ce qui rend la pile vraiment interchangeable avec la pile
    // sur tableau, dont la copie l'est deja. Le cout est en O(taille), ce qui
    // reste acceptable pour une pile, dont les elements sont Ajoutes et
    // retires un par un.
    PileListe(const PileListe& autre)
        : sommet_(nullptr),
          taille_(0) {
        // On parcourt la source du fond vers le sommet et on empile chaque
        // valeur sur la tete de la copie : les noeuds se retrouvent ainsi
        // dans le meme ordre que ceux de la source.
        Noeud** curseur = &sommet_;
        for (const Noeud* source = autre.sommet_; source != nullptr; source = source->suivant) {
            ++allocations;
            *curseur = new Noeud{source->valeur, nullptr};
            curseur = &(*curseur)->suivant;
            ++taille_;
        }
    }

    PileListe& operator=(const PileListe& autre) {
        if (this != &autre) {
            PileListe copie(autre);
            detruireNoeuds();
            sommet_ = copie.sommet_;
            taille_ = copie.taille_;
            copie.sommet_ = nullptr;
            copie.taille_ = 0;
        }
        return *this;
    }

    PileListe(PileListe&& autre) noexcept
        : sommet_(autre.sommet_),
          taille_(autre.taille_) {
        autre.sommet_ = nullptr;
        autre.taille_ = 0;
    }

    PileListe& operator=(PileListe&& autre) noexcept {
        if (this != &autre) {
            detruireNoeuds();
            sommet_ = autre.sommet_;
            taille_ = autre.taille_;
            autre.sommet_ = nullptr;
            autre.taille_ = 0;
        }
        return *this;
    }

    void push(const T& valeur) {
        ++allocations;
        sommet_ = new Noeud{valeur, sommet_};
        ++taille_;
    }

    void push(T&& valeur) {
        ++allocations;
        sommet_ = new Noeud{std::move(valeur), sommet_};
        ++taille_;
    }

    void pop() {
        if (empty()) {
            throw std::out_of_range("PileListe : pop() sur une pile vide.");
        }
        Noeud* aDetruire = sommet_;
        sommet_ = sommet_->suivant;
        delete aDetruire;
        --taille_;
    }

    T& top() {
        if (empty()) {
            throw std::out_of_range("PileListe : top() sur une pile vide.");
        }
        return sommet_->valeur;
    }

    const T& top() const {
        if (empty()) {
            throw std::out_of_range("PileListe : top() sur une pile vide.");
        }
        return sommet_->valeur;
    }

    bool empty() const {
        return sommet_ == nullptr;
    }

    std::size_t size() const {
        return taille_;
    }

    // La liste chainee n'a pas de capacite reservee : sa taille courante
    // est la seule information comparable a celle d'un tableau dynamique.
    std::size_t capacity() const {
        return taille_;
    }

    void clear() {
        detruireNoeuds();
    }

    void reserve(std::size_t) {
        // Operation sans effet : c'est precisement la disadvantage de la
        // liste chainee, mis en evidence par le banc d'essai.
    }

    void reduireCapacite() {
        // Operation sans effet : aucune place reservee a liberer.
    }

    static void reinitialiserAllocations() {
        allocations = 0;
    }

private:
    struct Noeud {
        T valeur;
        Noeud* suivant;
    };

    Noeud* sommet_ = nullptr;
    std::size_t taille_ = 0;

    void detruireNoeuds() {
        while (sommet_ != nullptr) {
            Noeud* aDetruire = sommet_;
            sommet_ = sommet_->suivant;
            delete aDetruire;
        }
        taille_ = 0;
    }
};

template <typename T>
std::size_t PileListe<T>::allocations = 0;

} // namespace piles

#endif // PILE_LISTE_HPP
