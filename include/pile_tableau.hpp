#ifndef PILE_TABLEAU_HPP
#define PILE_TABLEAU_HPP

// Pile generique implementee sur un tableau dynamique alloue manuellement.
//
// Representation : un bloc contigu de T suivi d'un indice "sommet" qui designe
// le nombre d'elements empiles. Le sommet de la pile (element le plus recent)
// est donc toujours a l'indice sommet_ - 1.
//
// Le stockage est gere a la main : le bloc est de la memoire brute, obtenue
// avec ::operator new, et chaque element est construit ou detruit
// explicitement aux emplacements utilises. On ne passe donc jamais par
// "new T[n]", qui construit d'emblee tous les elements du bloc et imposerait
// en outre que T soit constructible par defaut. C'est ce qui permet a cette
// pile d'accepter des types qui ne le sont pas, comme une pile de structures
// sans constructeur par defaut.
//
// Complexite :
//   push   O(1) amorti (doublement de la capacite) mais O(n) au pire,
//   pop    O(1), top O(1), empty O(1), size O(1).

#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace piles {

template <typename T>
class PileTableau {
public:
    using valeur_type = T;

    // Compteur partage par toutes les instances : sert au banc d'essai pour
    // mesurer le nombre de reallocations du tableau.
    static std::size_t redimensionnements;

    explicit PileTableau(std::size_t capaciteInitiale = 8)
        : elements_(capaciteInitiale == 0 ? nullptr : allouer(capaciteInitiale)),
          sommet_(0),
          capacite_(capaciteInitiale) {}

    PileTableau(const PileTableau& autre)
        : elements_(nullptr),
          sommet_(0),
          capacite_(0) {
        if (this != &autre) {
            elements_ = allouer(autre.capacite_);
            capacite_ = autre.capacite_;
            try {
                for (std::size_t i = 0; i < autre.sommet_; ++i) {
                    construire(elements_ + i, autre.elements_[i]);
                    ++sommet_;
                }
            } catch (...) {
                liberer();
                throw;
            }
        }
    }

    PileTableau& operator=(const PileTableau& autre) {
        if (this != &autre) {
            PileTableau copie(autre);
            echangeAvec(copie);
        }
        return *this;
    }

    PileTableau(PileTableau&& autre) noexcept
        : elements_(autre.elements_),
          sommet_(autre.sommet_),
          capacite_(autre.capacite_) {
        autre.elements_ = nullptr;
        autre.sommet_ = 0;
        autre.capacite_ = 0;
    }

    PileTableau& operator=(PileTableau&& autre) noexcept {
        if (this != &autre) {
            liberer();
            elements_ = autre.elements_;
            sommet_ = autre.sommet_;
            capacite_ = autre.capacite_;
            autre.elements_ = nullptr;
            autre.sommet_ = 0;
            autre.capacite_ = 0;
        }
        return *this;
    }

    ~PileTableau() {
        liberer();
    }

    void push(const T& valeur) {
        pushInterne(valeur);
    }

    void push(T&& valeur) {
        pushInterne(std::move(valeur));
    }

    void pop() {
        if (empty()) {
            throw std::out_of_range("PileTableau : pop() sur une pile vide.");
        }
        --sommet_;
        detruire(elements_ + sommet_);
    }

    T& top() {
        if (empty()) {
            throw std::out_of_range("PileTableau : top() sur une pile vide.");
        }
        return elements_[sommet_ - 1];
    }

    const T& top() const {
        if (empty()) {
            throw std::out_of_range("PileTableau : top() sur une pile vide.");
        }
        return elements_[sommet_ - 1];
    }

    bool empty() const {
        return sommet_ == 0;
    }

    std::size_t size() const {
        return sommet_;
    }

    std::size_t capacity() const {
        return capacite_;
    }

    void clear() {
        for (std::size_t i = sommet_; i-- > 0;) {
            detruire(elements_ + i);
        }
        sommet_ = 0;
    }

    // Reserve assez de place pour nouvelleCapacite elements sans
    // reallouer. Si la place existe deja, l'appel n'a aucun effet.
    void reserve(std::size_t nouvelleCapacite) {
        if (nouvelleCapacite > capacite_) {
            redimensionner(nouvelleCapacite);
        }
    }

    // Reduit la capacite au strict necessaire. Cout O(n), a n'appeler
    // qu'apres une sequence de push/pop connue a l'avance.
    void reduireCapacite() {
        if (sommet_ < capacite_) {
            redimensionner(sommet_ == 0 ? 1 : sommet_);
        }
    }

    static void reinitialiserRedimensionnements() {
        redimensionnements = 0;
    }

private:
    T* elements_;
    std::size_t sommet_;
    std::size_t capacite_;

    // Construit et detruit un seul element a un emplacement donne. Ces deux
    // fonctions intermediaires evitent d'ecrire "~T" et "new T(...)" avec un T
    // dependent du modele, ce que le langage n'accepte pas tel quel.
    template <typename... Args>
    static void construire(T* emplacement, Args&&... args) {
        new (static_cast<void*>(emplacement)) T(std::forward<Args>(args)...);
    }

    static void detruire(T* element) {
        element->~T();
    }

    static T* allouer(std::size_t nombreElements) {
        return static_cast<T*>(::operator new(nombreElements * sizeof(T)));
    }

    // Detruit les elements encore construits, puis rend la memoire. L'etat est
    // laisse vide plutot que seulement libere, pour que l'on puisse rappeler
    // liberer sans risque depuis un constructeur ou un destructeur.
    void liberer() {
        for (std::size_t i = sommet_; i-- > 0;) {
            detruire(elements_ + i);
        }
        ::operator delete(static_cast<void*>(elements_));
        elements_ = nullptr;
        sommet_ = 0;
        capacite_ = 0;
    }

    void echangeAvec(PileTableau& autre) {
        std::swap(elements_, autre.elements_);
        std::swap(sommet_, autre.sommet_);
        std::swap(capacite_, autre.capacite_);
    }

    template <typename U>
    void pushInterne(U&& valeur) {
        if (sommet_ == capacite_) {
            redimensionner(capacite_ == 0 ? 8 : capacite_ * 2);
        }
        // Si la construction echoue, la pile doit rester dans son etat
        // anterieur : l'element n'est compte qu'apres sa construction.
        construire(elements_ + sommet_, std::forward<U>(valeur));
        ++sommet_;
    }

    // Alloue un nouveau bloc, y transfere les elements existants puis libere
    // l'ancien bloc. Le nouveau bloc est alloue avant tout le reste, et les
    // elements y sont construits l'un apres l'autre, afin que l'operation reste
    // sans effet sur la pile si une allocation ou une copie echoue.
    void redimensionner(std::size_t nouvelleCapacite) {
        if (nouvelleCapacite < 1) {
            nouvelleCapacite = 1;
        }

        T* nouveauBloc = allouer(nouvelleCapacite);
        std::size_t construits = 0;

        try {
            if constexpr (std::is_nothrow_move_constructible_v<T>) {
                for (std::size_t i = 0; i < sommet_; ++i) {
                    construire(nouveauBloc + i, std::move(elements_[i]));
                    ++construits;
                }
            } else {
                for (std::size_t i = 0; i < sommet_; ++i) {
                    construire(nouveauBloc + i, elements_[i]);
                    ++construits;
                }
            }
        } catch (...) {
            for (std::size_t i = construits; i-- > 0;) {
                detruire(nouveauBloc + i);
            }
            ::operator delete(static_cast<void*>(nouveauBloc));
            throw;
        }

        // Le transfert est termine : l'ancien bloc peut etre rendu. Les
        // elements qui le Composaient restent des objets vivants, meme
        // apres avoir ete deplaces, et doivent donc etre detruits.
        for (std::size_t i = sommet_; i-- > 0;) {
            detruire(elements_ + i);
        }
        ::operator delete(static_cast<void*>(elements_));

        elements_ = nouveauBloc;
        capacite_ = nouvelleCapacite;
        ++redimensionnements;
    }
};

template <typename T>
std::size_t PileTableau<T>::redimensionnements = 0;

} // namespace piles

#endif // PILE_TABLEAU_HPP
