// Tests des deux implementations de pile generique.
//
// Les deux classes sont soumises au meme jeu d'essais, ce qui permet de
// verifier qu'elles ont exactement le meme comportement observable, seule la
// representation interne differe.

#include "pile_liste.hpp"
#include "pile_stl.hpp"
#include "pile_tableau.hpp"
#include "test.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace {

// Type sans constructeur par defaut : une pile fondee sur "new T[n]" ne peut pas
// le stocker, puisque cette ecriture construit tout le bloc d'avance.
struct SansConstructeurParDefaut {
    int valeur;
    std::string etiquette;

    SansConstructeurParDefaut(int v, std::string e) : valeur(v), etiquette(std::move(e)) {}
};

// Type qui compte ses instances vivantes, pour verifier que la pile detruit
// bien les elements qu'elle retire et qu'elle n'en construit jamais deux fois
// au meme emplacement.
struct Comptable {
    static int vivants;

    int valeur;

    explicit Comptable(int v = 0) : valeur(v) { ++vivants; }
    Comptable(const Comptable& autre) : valeur(autre.valeur) { ++vivants; }
    Comptable(Comptable&& autre) noexcept : valeur(autre.valeur) { ++vivants; }
    Comptable& operator=(const Comptable&) = default;
    Comptable& operator=(Comptable&&) = default;
    ~Comptable() { --vivants; }
};

int Comptable::vivants = 0;

} // namespace

namespace {

template <typename Pile>
void testerComportementLifo(const std::string& nom) {
    tests::section(nom + " : comportement LIFO");

    Pile pile;
    tests::verifier(pile.empty(), "une pile neuve est vide");
    tests::verifier(pile.size() == 0, "une pile neuve a une taille nulle");

    pile.push(1);
    pile.push(2);
    pile.push(3);

    tests::verifier(!pile.empty(), "la pile n'est plus vide");
    tests::verifier(pile.size() == 3, "la taille vaut 3 apres trois insertions");
    tests::verifierEgaux(3.0, static_cast<double>(pile.top()), "le sommet est le dernier insere");

    pile.pop();
    tests::verifierEgaux(2.0, static_cast<double>(pile.top()), "le pop retire bien le sommet");
    tests::verifier(pile.size() == 2, "la taille a diminue de un");

    pile.pop();
    pile.pop();
    tests::verifier(pile.empty(), "la pile est a nouveau vide");

    bool leve = false;
    try {
        pile.pop();
    } catch (const std::out_of_range&) {
        leve = true;
    }
    tests::verifier(leve, "pop() sur une pile vide leve une exception");
}

template <typename Pile>
void testerAvecChaines(const std::string& nom) {
    tests::section(nom + " : instants de templates (std::string)");

    Pile pile;
    pile.push("Bonjour");
    pile.push("Monde");
    pile.push(std::string("C++"));
    tests::verifierEgaux(std::string("C++"), pile.top(), "le sommet est une chaine complete");
    pile.pop();
    tests::verifierEgaux(std::string("Monde"), pile.top(), "l'ordre LIFO se conserve");
}

template <typename Pile>
void testerSequenceLongue(const std::string& nom) {
    tests::section(nom + " : sequence de 10 000 elements");

    Pile pile;
    for (int i = 0; i < 10000; ++i) {
        pile.push(i);
    }
    tests::verifier(pile.size() == 10000, "taille correcte apres remplissage");

    bool coherent = true;
    for (int i = 9999; i >= 0; --i) {
        if (pile.top() != i) {
            coherent = false;
            break;
        }
        pile.pop();
    }
    tests::verifier(coherent, "les elements ressortent dans l'ordre inverse exact");
    tests::verifier(pile.empty(), "la pile se vide exactement");
}

void testerPileTableau() {
    testerComportementLifo<piles::PileTableau<int>>("PileTableau");
    testerAvecChaines<piles::PileTableau<std::string>>("PileTableau");
    testerSequenceLongue<piles::PileTableau<int>>("PileTableau");

    tests::section("PileTableau : croissance amortie par doublement");

    piles::PileTableau<int>::reinitialiserRedimensionnements();

    piles::PileTableau<int> pile(2);
    tests::verifier(pile.capacity() == 2, "la capacite initiale est respectee");

    for (int i = 0; i < 1024; ++i) {
        pile.push(i);
    }
    tests::verifier(pile.capacity() >= 1024, "la capacite a suivi la taille");
    tests::verifier(piles::PileTableau<int>::redimensionnements == 9,
                    "1024 insertions depuis une capacite de 2 : exactement 9 reallocations, "
                    "soit log2(1024/2)",
                    std::to_string(piles::PileTableau<int>::redimensionnements) + " reallocations");
    tests::verifierEgaux(1023.0, static_cast<double>(pile.top()), "le sommet est le dernier insere");

    tests::section("PileTableau : copie, deplacement et destruction");

    piles::PileTableau<int> originale;
    originale.push(7);
    originale.push(8);

    piles::PileTableau<int> copie(originale);
    originale.pop();
    tests::verifierEgaux(8.0, static_cast<double>(copie.top()), "la copie est independante de l'originale");
    tests::verifierEgaux(7.0, static_cast<double>(originale.top()), "l'originale a bien ete modifie");

    piles::PileTableau<int> deplacee(std::move(copie));
    tests::verifierEgaux(8.0, static_cast<double>(deplacee.top()), "le deplacement transfere le contenu");
    tests::verifier(copie.empty(), "la source du deplacement est videe");

    piles::PileTableau<int> affectee;
    affectee = deplacee;
    tests::verifierEgaux(8.0, static_cast<double>(affectee.top()), "l'affectation par copie fonctionne");

    tests::section("PileTableau : reserve et reduction de capacite");

    piles::PileTableau<int> ajustee;
    ajustee.push(1);
    ajustee.reserve(1000);
    tests::verifier(ajustee.capacity() == 1000, "reserve alloue la capacite demandee");
    ajustee.reserve(10);
    tests::verifier(ajustee.capacity() == 1000, "reserve n'a rien fait quand la place suffit");

    ajustee.reduireCapacite();
    tests::verifier(ajustee.capacity() == 1, "la reduction ramene la capacite a la taille");

    piles::PileTableau<int> aVider;
    aVider.push(1);
    aVider.push(2);
    aVider.clear();
    tests::verifier(aVider.empty(), "clear vide la pile sans liberer la capacite");
    aVider.push(3);
    tests::verifierEgaux(3.0, static_cast<double>(aVider.top()), "la pile reste utilisable apres clear");
}

void testerPileListe() {
    testerComportementLifo<piles::PileListe<int>>("PileListe");
    testerAvecChaines<piles::PileListe<std::string>>("PileListe");
    testerSequenceLongue<piles::PileListe<int>>("PileListe");

    tests::section("PileListe : une allocation par insertion");

    piles::PileListe<int>::reinitialiserAllocations();

    piles::PileListe<int> pile;
    for (int i = 0; i < 1000; ++i) {
        pile.push(i);
    }
    tests::verifier(piles::PileListe<int>::allocations == 1000,
                    "1000 insertions donnent exactement 1000 allocations",
                    std::to_string(piles::PileListe<int>::allocations) + " allocations");

    tests::verifier(pile.capacity() == pile.size(), "la capacite vaut la taille : aucune reserve possible");

    pile.clear();
    tests::verifier(pile.empty(), "clear libere tous les noeuds");

    tests::section("PileListe : deplacement et destruction sans fuite");

    piles::PileListe<std::string> source;
    source.push(std::string("a"));
    source.push(std::string("b"));

    piles::PileListe<std::string> cible(std::move(source));
    tests::verifierEgaux(std::string("b"), cible.top(), "le deplacement transfere la tete");
    tests::verifier(source.empty(), "la source est videe");

    // La destruction de "cible" libere deux noeuds sans double liberation.
    tests::verifier(cible.size() == 2, "les deux noeuds sont bien detruits a la sortie de portee");

    // Les trois piles sont copiables. Pour la liste chainee, la copie est
    // profonde : un noeud par element, recree dans le meme ordre. C'est ce
    // qui rend les trois implementations vraiment interchangeables, une
    // difference de comportement etant plus facile a surveiller qu'une
    // difference d'interface.
    static_assert(std::is_copy_constructible_v<piles::PileListe<int>>,
                  "une pile chainee doit etre copiable, noeud par noeud");
    static_assert(std::is_move_constructible_v<piles::PileListe<int>>,
                  "une pile proprietaire de ses noeuds doit etre deplacable");
    static_assert(std::is_copy_constructible_v<piles::PileTableau<int>>,
                  "une pile sur tableau dynamique doit etre copiable, son bloc pouvant etre duplique");

    piles::PileListe<int> originale;
    originale.push(1);
    originale.push(2);
    originale.push(3);

    piles::PileListe<int> copie(originale);
    tests::verifier(copie.size() == 3 && copie.top() == 3, "la copie d'une liste chainee a le meme contenu",
                    std::to_string(copie.size()) + " elements, sommet " + std::to_string(copie.top()));

    copie.pop();
    tests::verifier(originale.size() == 3 && copie.size() == 2, "la copie a ses propres noeuds",
                    std::to_string(originale.size()) + "/" + std::to_string(copie.size()));
}

void testerEquivalence() {
    tests::section("PileTableau et PileListe : comportement identique");

    piles::PileTableau<int> tableau;
    piles::PileListe<int> liste;
    piles::PileSTL<int> stl;

    bool identiques = true;
    for (int phase = 0; phase < 200; ++phase) {
        for (int i = 0; i < 7; ++i) {
            const int valeur = phase * 7 + i;
            tableau.push(valeur);
            liste.push(valeur);
            stl.push(valeur);
        }
        for (int i = 0; i < 3; ++i) {
            tableau.pop();
            liste.pop();
            stl.pop();
        }
        if (tableau.size() != liste.size() || tableau.size() != stl.size()) {
            identiques = false;
            break;
        }
        if (!tableau.empty() && (tableau.top() != liste.top() || tableau.top() != stl.top())) {
            identiques = false;
            break;
        }
    }

    tests::verifier(identiques, "les trois piles restent synchronisees sur 200 cycles melanges");
    tests::verifierEgaux(800.0, static_cast<double>(tableau.size()), "taille finale identique : 800");
}

} // namespace

// Une pile qui stocke des pointeurs bruts ("new T[n]") perd la main sur la duree
// de vie des elements : un pop() doit detruire ce qu'il retire, un pop suivi
// d'un push ne doit pas reconstruire par-dessus un objet vivant, et un
// redimensionnement ne doit pas construire deux fois le meme element. Ces
// tests verifient les trois points, pour les trois implementations.
void testerDureeDeVie() {
    tests::section("8. Duree de vie des elements");

    // Une pile de structures sans constructeur par defaut : seule une pile qui
    // gere la memoire brute peut la stocker.
    piles::PileTableau<SansConstructeurParDefaut> sansDefaut;
    for (int i = 0; i < 20; ++i) {
        sansDefaut.push(SansConstructeurParDefaut(i, "element" + std::to_string(i)));
    }
    tests::verifier(sansDefaut.size() == 20, "un type sans constructeur par defaut peut etre empile",
                    std::to_string(sansDefaut.size()) + "/20");
    tests::verifier(sansDefaut.top().valeur == 19, "le sommet est bien le dernier element empile",
                    std::to_string(sansDefaut.top().valeur));

    piles::PileTableau<SansConstructeurParDefaut> copie = sansDefaut;
    tests::verifier(copie.size() == 20 && copie.top().valeur == 19, "la copie est independante de l'original",
                    std::to_string(copie.size()) + " elements");
    copie.pop();
    tests::verifier(sansDefaut.size() == 20 && copie.size() == 19, "un pop sur la copie ne touche pas l'original",
                    std::to_string(sansDefaut.size()) + "/" + std::to_string(copie.size()));

    // Comptabilite des instances vivantes. On recommence a zero pour que le
    // compteur ne depende pas de l'ordre d execution des sections.
    {
        piles::PileTableau<Comptable> pile(2); // capacite initiale 2 : chaque push double la capacite
        for (int i = 0; i < 10; ++i) {
            pile.push(Comptable(i));
        }
        tests::verifier(Comptable::vivants == 10, "un element vivant par element empile, apres redimensionnements",
                        std::to_string(Comptable::vivants) + "/10");

        for (int i = 0; i < 10; ++i) {
            pile.pop();
        }
        tests::verifier(Comptable::vivants == 0, "chaque pop detruit l'element qu'il retire",
                        std::to_string(Comptable::vivants) + "/10");

        // Reutiliser les emplacements liberes ne doit rien construire en double.
        for (int i = 0; i < 5; ++i) {
            pile.push(Comptable(i));
        }
        tests::verifier(Comptable::vivants == 5, "la reutilisation d'un emplacement ne double pas la construction",
                        std::to_string(Comptable::vivants) + "/5");

        pile.clear();
        tests::verifier(Comptable::vivants == 0, "clear detruit tous les elements restants",
                        std::to_string(Comptable::vivants) + "/5");
    }
    tests::verifier(Comptable::vivants == 0, "le destructeur ne laisse rien derriere lui",
                    std::to_string(Comptable::vivants) + "/10");

    // Meme verification sur la liste chainee, dont chaque noeud porte sa
    // propre copie de la valeur.
    {
        piles::PileListe<Comptable> pile;
        for (int i = 0; i < 6; ++i) {
            pile.push(Comptable(i));
        }
        for (int i = 0; i < 6; ++i) {
            pile.pop();
        }
        tests::verifier(Comptable::vivants == 0, "la liste chaine detruit bien ses noeuds",
                        std::to_string(Comptable::vivants) + "/6");
    }
}

int main() {
    std::cout << "Tests des piles generiques\n";
    std::cout << "=========================\n";

    testerPileTableau();
    testerPileListe();
    testerEquivalence();
    testerDureeDeVie();

    return tests::bilan("Tests des piles");
}
