#include "stack_linked.hpp"
#include "stack_vector.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>


template <typename Pile>
void tester(const std::string& nom) {
    Pile pile;

    pile.push("Bonjour");
    pile.push("Monde");

    std::cout << nom << " :\n";
    std::cout << "Sommet : " << pile.top() << '\n';
    pile.pop();
    std::cout << "Apres pop : " << pile.top() << '\n';
    std::cout << "Taille : " << pile.size() << "\n\n";
}


template <typename Pile>
double mesurer(std::size_t nombreElements) {
    Pile pile;
    long long somme = 0;

    const auto debut = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < nombreElements; ++i) {
        pile.push(static_cast<int>(i));
    }

    while (!pile.empty()) {
        somme += pile.top();
        pile.pop();
    }

    const auto fin = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> duree = fin - debut;

    // Afficher la somme empeche le compilateur de supprimer le calcul.
    std::cout << "Somme de controle : " << somme << '\n';
    return duree.count();
}

int main() {
    tester<piles::PileListe<std::string>>("Pile par liste chainee");
    tester<piles::PileVector<std::string>>("Pile par vector");

    constexpr std::size_t nombreElements = 500'000;
    const double tempsListe = mesurer<piles::PileListe<int>>(nombreElements);
    const double tempsVector = mesurer<piles::PileVector<int>>(nombreElements);

    std::cout << "\nComparaison pour " << nombreElements << " elements :\n";
    std::cout << "Liste chainee : " << tempsListe << " ms\n";
    std::cout << "Vector         : " << tempsVector << " ms\n";

    return 0;
}
