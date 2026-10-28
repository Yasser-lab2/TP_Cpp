#include <iostream>
#include <queue>

using namespace std;

struct Noeud {
    int valeur;
    Noeud* gauche = nullptr;
    Noeud* droite = nullptr;

    explicit Noeud(int v) : valeur(v) {}
};

class ArbreBinaire {
public:
    Noeud* racine;
    ArbreBinaire() : racine(nullptr) {}

    ~ArbreBinaire() {
        detruire(racine);
    }

    ArbreBinaire(const ArbreBinaire&) = delete;
    ArbreBinaire& operator=(const ArbreBinaire&) = delete;

    void inserer(int valeur);

    // Renvoie true si la valeur est presente dans l'arbre.
    bool rechercher(int valeur) const;

    // Supprime la valeur et renvoie true si elle existait.
    bool supprimer(int valeur);

    // Affiche l'arbre niveau par niveau, de gauche a droite.
    void afficher() const;

    // Dans un arbre de recherche binaire, ce parcours affiche les valeurs triees.
    void afficherInfixe() const;

private:
    static void detruire(Noeud* noeud);
    static void afficherInfixe(Noeud* noeud);
    static bool supprimer(Noeud*& noeud, int valeur);
};

void ArbreBinaire::inserer(int valeur) {
    if (racine == nullptr) {
        racine = new Noeud(valeur);
        return;
    }

    Noeud* courant = racine;
    while (true) {
        if (valeur < courant->valeur) {
            if (courant->gauche == nullptr) {
                courant->gauche = new Noeud(valeur);
                return;
            }
            courant = courant->gauche;
        } else if (valeur > courant->valeur) {
            if (courant->droite == nullptr) {
                courant->droite = new Noeud(valeur);
                return;
            }
            courant = courant->droite;
        } else {
            // Une valeur deja presente n'est pas ajoutee deux fois.
            return;
        }
    }
}

bool ArbreBinaire::rechercher(int valeur) const {
    const Noeud* courant = racine;

    while (courant != nullptr) {
        if (valeur == courant->valeur) {
            return true;
        }

        if (valeur < courant->valeur) {
            courant = courant->gauche;
        } else {
            courant = courant->droite;
        }
    }

    return false;
}

bool ArbreBinaire::supprimer(int valeur) {
    return supprimer(racine, valeur);
}

bool ArbreBinaire::supprimer(Noeud*& noeud, int valeur) {
    if (noeud == nullptr) {
        return false;
    }

    if (valeur < noeud->valeur) {
        return supprimer(noeud->gauche, valeur);
    }
    if (valeur > noeud->valeur) {
        return supprimer(noeud->droite, valeur);
    }

    // Noeud trouve : il n'a pas de fils gauche ou n'a pas de fils droit.
    if (noeud->gauche == nullptr) {
        Noeud* ancien = noeud;
        noeud = noeud->droite;
        delete ancien;
        return true;
    }
    if (noeud->droite == nullptr) {
        Noeud* ancien = noeud;
        noeud = noeud->gauche;
        delete ancien;
        return true;
    }

    // Deux fils : remplacer par le plus petit element du sous-arbre droit.
    Noeud* successeur = noeud->droite;
    while (successeur->gauche != nullptr) {
        successeur = successeur->gauche;
    }

    noeud->valeur = successeur->valeur;
    return supprimer(noeud->droite, successeur->valeur);
}

void ArbreBinaire::afficher() const {
    if (racine == nullptr) {
        cout << "(arbre vide)\n";
        return;
    }

    queue<Noeud*> file;
    file.push(racine);

    while (!file.empty()) {
        Noeud* courant = file.front();
        file.pop();

        cout << courant->valeur << ' ';

        if (courant->gauche != nullptr) {
            file.push(courant->gauche);
        }
        if (courant->droite != nullptr) {
            file.push(courant->droite);
        }
    }
    cout << '\n';
}

void ArbreBinaire::afficherInfixe() const {
    afficherInfixe(racine);
    cout << '\n';
}

void ArbreBinaire::afficherInfixe(Noeud* noeud) {
    if (noeud == nullptr) {
        return;
    }

    afficherInfixe(noeud->gauche);
    cout << noeud->valeur << ' ';
    afficherInfixe(noeud->droite);
}

void ArbreBinaire::detruire(Noeud* noeud) {
    if (noeud == nullptr) {
        return;
    }

    detruire(noeud->gauche);
    detruire(noeud->droite);
    delete noeud;
}

int main() {
    ArbreBinaire arbre;

    // Les valeurs sont inserees successivement pour construire l'arbre.
    const int valeurs[] = {50, 30, 70, 20, 40, 60, 80};
    for (int valeur : valeurs) {
        arbre.inserer(valeur);
    }

    cout << "Affichage par niveaux : ";
    arbre.afficher();

    cout << "Affichage infixe : ";
    arbre.afficherInfixe();

    cout << "Recherche de 40 : "
         << (arbre.rechercher(40) ? "trouvee" : "non trouvee") << '\n';
    cout << "Recherche de 45 : "
         << (arbre.rechercher(45) ? "trouvee" : "non trouvee") << '\n';

    cout << "Suppression de 30 : "
         << (arbre.supprimer(30) ? "effectuee" : "valeur absente") << '\n';
    cout << "Apres suppression, par niveaux : ";
    arbre.afficher();

    cout << "Apres suppression, infixe : ";
    arbre.afficherInfixe();

    return 0;
}
