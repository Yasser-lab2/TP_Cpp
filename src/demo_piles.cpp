// Demonstration des trois piles generiques.
//
// Le programme montre la meme sequence d'operations sur les trois
// implementations, puis verifie qu'elles restent interchangeables, et enfin
// mesure leurs couts sur une sequence plus longue. Le detail des mesures est
// repris dans src/benchmark_piles.cpp ; ici on ne garde que l'ordre de
// grandeur, pour que la demonstration reste lisible.

#include "pile_liste.hpp"
#include "pile_stl.hpp"
#include "pile_tableau.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Affiche le contenu d'une pile, du fond vers le sommet, en vidant la copie
// plutot que la pile elle-meme. On travaille sur une copie pour montrer que
// les trois piles sont reellement copiables.
template <typename Pile>
std::string contenu(Pile pile) {
    std::string texte = "[";
    while (!pile.empty()) {
        texte += std::to_string(pile.top());
        pile.pop();
        if (!pile.empty()) {
            texte += ", ";
        }
    }
    return texte + "]";
}

// La meme sequence, ecrite avec l'interface de std::stack, doit produire le
// meme resultat sur les trois piles. C'est le test qui justifie l'existence
// d'une interface commune.
template <typename Pile>
void decrire(const std::string& nom) {
    Pile pile;

    pile.push(10);
    pile.push(20);
    pile.push(30);

    std::cout << nom << '\n';
    std::cout << "  apres trois push   : " << contenu(pile) << '\n';
    std::cout << "  sommet             : " << pile.top() << '\n';
    std::cout << "  taille             : " << pile.size() << '\n';

    pile.pop();
    std::cout << "  apres un pop       : sommet = " << pile.top() << ", taille = " << pile.size() << '\n';

    // Une pile a deux usages classiques : l'evaluation d'une expression
    // postfixee, et le retour arriere dans un parcours.
    pile.pop();
    pile.pop();
    std::cout << "  apres vidage       : vide = " << std::boolalpha << pile.empty() << "\n\n";
}

// Parcourt la suite de LeverCotes et montre que la pile sert de memoire
// auxiliaire pour un retour en arriere.
void retourArriere() {
    std::cout << "Usage 2 : retour en arriere dans un parcours\n";

    piles::PileTableau<std::size_t> suspects;
    suspects.push(2);
    suspects.push(4);
    suspects.push(6);

    const std::vector<std::size_t> mesures = {10, 20, 30, 40, 50, 60, 70};

    for (std::size_t i = 0; i < mesures.size(); ++i) {
        while (!suspects.empty() && suspects.top() <= i) {
            std::cout << "  on ferme la mesure " << suspects.top() << " a la date " << i << '\n';
            suspects.pop();
        }
        std::cout << "  a la date " << i << ", mesure = " << mesures[i] << '\n';
    }
    std::cout << '\n';
}

// Mesure le temps d'une sequence complete de push puis de pop. Les trois
// valeurs mesurees doivent coincider, ce qui verifie l'equivalence
// fonctionnelle du programme.
template <typename Pile>
double mesurer(std::size_t nombreElements) {
    Pile pile;
    std::size_t somme = 0;

    const auto debut = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < nombreElements; ++i) {
        pile.push(i);
    }
    while (!pile.empty()) {
        somme += pile.top();
        pile.pop();
    }

    const auto fin = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> duree = fin - debut;

    // Afficher la somme empeche le compilateur de supprimer le calcul.
    if (somme != nombreElements * (nombreElements - 1) / 2) {
        std::cerr << "Somme de controle inattendue : " << somme << '\n';
    }
    return duree.count();
}

} // namespace

int main() {
    std::cout << "Demonstration des piles generiques\n";
    std::cout << "==================================\n\n";

    std::cout << "Usage 1 : LIFO de base\n\n";
    decrire<piles::PileTableau<int>>("Pile sur tableau dynamique");
    decrire<piles::PileListe<int>>("Pile sur liste chainee");
    decrire<piles::PileSTL<int>>("Pile sur std::vector");

    retourArriere();

    std::cout << "Cout compare pour 500 000 push puis 500 000 pop\n";
    constexpr std::size_t nombreElements = 500000;
    const double tempsTableau = mesurer<piles::PileTableau<int>>(nombreElements);
    const double tempsListe = mesurer<piles::PileListe<int>>(nombreElements);
    const double tempsSTL = mesurer<piles::PileSTL<int>>(nombreElements);

    std::cout << "  tableau dynamique : " << tempsTableau << " ms\n";
    std::cout << "  liste chainee     : " << tempsListe << " ms\n";
    std::cout << "  std::vector       : " << tempsSTL << " ms\n";

    std::cout << "\nLe tableau et la liste se comportent de facon identique du point\n"
                 "de vue de l'interface ; ils different surtout par la dispersion\n"
                 "en memoire et le nombre d'allocations, que le banc d'essai detaille.\n";

    return 0;
}
