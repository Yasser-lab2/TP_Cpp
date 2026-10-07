#include <iostream>
#include <vector>
#include <string>
#include <chrono>

using namespace std;

int main() {
    vector<string> mots(2);

    cout << "Entrez le premier mot : ";
    cin >> mots[0];

    cout << "Entrez le deuxieme mot : ";
    cin >> mots[1];

    auto debut = chrono::high_resolution_clock::now();

    bool trouve = false;

    int longueur1 = mots[0].size();
    int longueur2 = mots[1].size();

    for (int i = 0; i <= longueur1 - longueur2 && !trouve; i++) {
        bool identique = true;

        for (int j = 0; j < longueur2; j++) {
            if (mots[0][i + j] != mots[1][j]) {
                identique = false;
                break;
            }
        }

        if (identique) {
            trouve = true;
        }
    }

    auto fin = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duree = fin - debut;

    if (trouve) {
        cout << "Le premier mot contient le deuxieme mot." << endl;
    } else {
        cout << "Le deuxieme mot n'existe pas dans le premier." << endl;
    }

    cout << "Temps d'execution de la recherche : "
         << duree.count() << " millisecondes" << endl;

    return 0;
}
