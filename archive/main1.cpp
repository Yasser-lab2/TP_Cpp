#include <iostream>
#include <chrono> // Pour mesurer le temps avec précision

// Algorithme récursif nais : volontairement lourd pour le CPU
long long fibonacciRecursif(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacciRecursif(n - 1) + fibonacciRecursif(n - 2);
}

int main() {
    // 42 est un bon chiffre test. Si votre PC est très récent, 
    // vous pouvez monter à 43 ou 44. S'il est ancien, restez à 40.
    int n = 42; 

    std::cout << "Test de performance en cours (Calcul de Fibonacci(" << n << "))...\n";
    std::cout << "Veuillez patienter quelques secondes...\n\n";

    // Démarrage du chronomètre
    auto debut = std::chrono::high_resolution_clock::now();

    long long resultat = fibonacciRecursif(n);

    // Arrêt du chronomètre
    auto fin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tempsEcoule = fin - debut;

    std::cout << "Résultat : " << resultat << "\n";
    std::cout << "Temps d'exécution : " << tempsEcoule.count() << " secondes.\n";

    return 0;
}