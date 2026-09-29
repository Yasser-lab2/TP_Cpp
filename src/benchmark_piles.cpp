// Banc d'essai des trois piles generiques.
//
// Le programme mesure trois grandeurs, choisies pour caracteriser les
// implementations autant que possible plutot que la machine :
//
//   1. le temps d'une sequence de push, puis de la meme sequence de push
//      suivie de pop (le cout du pop inclut donc celui du push) ;
//   2. le nombre d'allocations memoire, deduit de la croissance de la
//      capacite, qui vaut exactement une allocation pour les trois
//      implementations ; il est recoupe par les compteurs internes du
//      tableau et de la liste ;
//   3. le cout d'une sequence mixte, plus proche d'un usage reel : la pile
//      oscille entre zero et une profondeur constante.
//
// Chaque mesure est repetee plusieurs fois et l'on ne garde que la meilleure,
// ce qui est la facon la plus simple de limiter l'influence du bruit de
// l'ordonnanceur et des autres processus. Le nombre d'iterations est ajuste
// pour que chaque mesure dure assez longtemps pour etre fiable, mais assez
// peu pour que le programme entier reste rapide a executer.

#include "pile_liste.hpp"
#include "pile_stl.hpp"
#include "pile_tableau.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using piles::PileListe;
using piles::PileSTL;
using piles::PileTableau;

constexpr std::size_t nombreElements = 1000000;
constexpr int nombreRepetitions = 5;

// Hauteur atteinte par la sequence mixte, choisie pour rester dans le cache de
// premier niveau tout en exercant la deallocation dans l'ordre inverse.
constexpr std::size_t profondeurMelange = 1024;

// Une sequence de valeurs non triviales, pour que l'optimiseur ne puisse pas
// se contenter de reaffecter un entier deja connu.
std::vector<std::size_t> construirePatron() {
    std::vector<std::size_t> resultat;
    resultat.reserve(nombreElements);
    for (std::size_t i = 0; i < nombreElements; ++i) {
        resultat.push_back(i * 2654435761u);
    }
    return resultat;
}

// Le patron d'elements, partage par toutes les implementations.
const std::vector<std::size_t>& patron() {
    static const std::vector<std::size_t> valeurs = construirePatron();
    return valeurs;
}

// Mesure repetitive. On rend le meilleur temps, en millisecondes.
template <typename Mesure>
double meilleure(int repetitions, Mesure&& mesure) {
    double meilleur = 1e300;
    for (int i = 0; i < repetitions; ++i) {
        const auto debut = std::chrono::steady_clock::now();
        const std::size_t somme = mesure();
        const auto fin = std::chrono::steady_clock::now();
        // La somme sert a empecher le compilateur de supprimer le calcul.
        if (somme == nombreElements + 1) {
            std::cerr << "Somme de controle inattendue\n";
        }
        const double ms = std::chrono::duration<double, std::milli>(fin - debut).count();
        meilleur = std::min(meilleur, ms);
    }
    return meilleur;
}

struct Resultat {
    double pushMs = 0.0;
    double popMs = 0.0;
    double mixteMs = 0.0;
    std::size_t allocations = 0;
};

template <typename Pile>
Resultat mesurer(const std::string& nom) {
    Resultat resultat;
    const std::vector<std::size_t>& elements = patron();

    resultat.pushMs = meilleure(nombreRepetitions, [&] {
        Pile pile;
        for (std::size_t valeur : elements) {
            pile.push(valeur);
        }
        return pile.size();
    });

    resultat.popMs = meilleure(nombreRepetitions, [&] {
        Pile pile;
        for (std::size_t valeur : elements) {
            pile.push(valeur);
        }
        std::size_t somme = 0;
        while (!pile.empty()) {
            somme += pile.top();
            pile.pop();
        }
        return somme;
    });

    // Sequence mixte : on empile par blocs de profondeur constante, puis on
    // retire ce bloc dans l'ordre inverse. La pile oscille donc entre zero et
    // profondeurMelange, ce qui correspond au usage reel d'une pile : un
    // automate qui empile quelques valeurs, puis les consomme.
    //
    // Une alternance push/pop element par element ne conviendrait pas : la
    // pile resterait de hauteur un, ne deplacerait jamais la moindre case et
    // ne mesurerait que le cout d'un aller-retour sur le sommet.
    resultat.mixteMs = meilleure(nombreRepetitions, [&] {
        Pile pile;
        std::size_t somme = 0;
        for (std::size_t i = 0; i < elements.size(); i += profondeurMelange) {
            const std::size_t fin = std::min(i + profondeurMelange, elements.size());
            for (std::size_t j = i; j < fin; ++j) {
                pile.push(elements[j]);
            }
            for (std::size_t j = fin; j > i; --j) {
                somme += pile.top();
                pile.pop();
            }
        }
        return somme;
    });

    // Comptage des allocations. On observe la croissance de la capacite, qui
    // correspond exactement a une allocation memoire pour les trois
    // implementations, ce qui rend la mesure homogene. On repart de zero
    // pour n'observer que les allocations dues a cette sequence.
    PileTableau<std::size_t>::reinitialiserRedimensionnements();
    PileListe<std::size_t>::reinitialiserAllocations();
    {
        Pile pile;
        std::size_t capacite = pile.capacity();
        // Le bloc initial n'existe que si le constructeur a reserve de la
        // place : la liste chainee demarre a la capacite nulle.
        if (capacite > 0) {
            ++resultat.allocations;
        }
        for (std::size_t valeur : elements) {
            pile.push(valeur);
            if (pile.capacity() != capacite) {
                capacite = pile.capacity();
                ++resultat.allocations;
            }
        }
        while (!pile.empty()) {
            pile.pop();
        }
    }
    // Les compteurs internes doivent confirmer la mesure par capacite, sans
    // quoi celle-ci ne voudrait rien dire.
    if constexpr (std::is_same_v<Pile, PileTableau<std::size_t>>) {
        if (resultat.allocations != PileTableau<std::size_t>::redimensionnements + 1) {
            std::cerr << "Compteur de redimensionnements incoherent\n";
        }
    } else if constexpr (std::is_same_v<Pile, PileListe<std::size_t>>) {
        if (resultat.allocations != PileListe<std::size_t>::allocations) {
            std::cerr << "Compteur de noeuds incoherent\n";
        }
    }

    std::cout << "  " << std::left << std::setw(22) << nom << std::right << std::fixed << std::setprecision(2)
              << std::setw(9) << resultat.pushMs << std::setw(11) << resultat.popMs << std::setw(10)
              << resultat.mixteMs << std::setw(14) << resultat.allocations << '\n';

    return resultat;
}

} // namespace

int main() {
    std::cout << "Banc d'essai des piles generiques\n";
    std::cout << "================================\n\n";
    std::cout << "Chaque mesure est repetee " << nombreRepetitions << " fois sur " << nombreElements
              << " elements ;\n"
              << "on affiche le meilleur temps obtenu, en millisecondes.\n\n";

    std::cout << std::left << std::setw(24) << "implementation" << std::right << std::setw(9) << "push" << std::setw(11)
              << "push+pop" << std::setw(10) << "mixte" << std::setw(14) << "allocations" << "\n";
    std::cout << std::string(70, '-') << '\n';

    const Resultat tableau = mesurer<PileTableau<std::size_t>>("tableau dynamique");
    const Resultat liste = mesurer<PileListe<std::size_t>>("liste chainee");
    const Resultat stl = mesurer<PileSTL<std::size_t>>("std::vector");

    std::cout << '\n' << std::string(70, '-') << '\n';
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "rapport liste / tableau : " << std::setw(6) << (liste.pushMs / tableau.pushMs)
              << " en push, " << std::setw(6) << (liste.popMs / tableau.popMs) << " en pop\n";
    std::cout << "rapport STL / tableau   : " << std::setw(6) << (stl.pushMs / tableau.pushMs)
              << " en push, " << std::setw(6) << (stl.popMs / tableau.popMs) << " en pop\n";

    // Les paragraphes sont donnes ligne a ligne, ce qui evite d'ecrire des
    // chaines trop longues dans le code.
    std::cout << '\n' << std::string(70, '-') << '\n';

    std::cout << "\nLe tableau double sa capacite a chaque reallocation. Il lui faut " << tableau.allocations
              << " allocations pour " << nombreElements << " elements, dont une reserve initiale et "
              << (tableau.allocations - 1) << " reallocations. Le cout total des copies vaut\n"
              << "de l'ordre de deux fois le nombre d'elements, quel que soit ce nombre : c'est le\n"
              << "paiement d'une dette d'amortissement, pas d'un cout par insertion.\n\n"
              << "La liste chainee alloue un noeud par element, soit " << liste.allocations
              << " allocations. Son cout est donc lineaire en nombre d'elements,\n"
              << "sans part amortie, mais il n'y a aucune copie a acquitter : chaque element n'est\n"
              << "deplace qu'une fois, a son propre emplacement.\n\n"
              << "std::vector fait " << stl.allocations << " allocations, soit exactement le meme nombre que notre\n"
              << "tableau. L'ecart de temps ne vient donc pas du nombre d'allocations, mais de leur\n"
              << "cout : la bibliotheque standard recoit la taille finale a chaque doublement, de\n"
              << "sorte que sa capacite finit exactement egale a la taille, et elle rend le bloc en\n"
              << "une seule operation la ou notre tableau restitue successivement les anciens.\n\n"
              << "Ces mesures recoupent le cours : le cout d'un ajout depend de ce qui se passe\n"
              << "autour, la dispersion en memoire de la liste pese sur les caches, et une structure\n"
              << "dont la croissance est geometrique ne paie qu'un cout amorti constant.\n";

    return 0;
}
