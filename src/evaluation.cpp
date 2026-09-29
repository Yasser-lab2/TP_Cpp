#include "evaluation.hpp"

#include "analyseur.hpp"
#include "pile_tableau.hpp"

#include <cmath>

namespace expressions {

bool appliquerOperateur(char symbole, double gauche, double droite, double& resultat, std::string& message) {
    switch (symbole) {
        case '+':
            resultat = gauche + droite;
            return true;
        case '-':
            resultat = gauche - droite;
            return true;
        case '*':
            resultat = gauche * droite;
            return true;
        case '/':
            if (droite == 0.0) {
                message = "division par zero.";
                return false;
            }
            resultat = gauche / droite;
            return true;
        case '%':
            if (droite == 0.0) {
                message = "reste par zero.";
                return false;
            }
            resultat = std::fmod(gauche, droite);
            return true;
        case '^':
            if (gauche < 0.0 && droite != std::floor(droite)) {
                message = "puissance d'une base negative par un exposant non entier : resultat non reel.";
                return false;
            }
            resultat = std::pow(gauche, droite);
            return true;
        default:
            message = std::string("operateur inconnu : '") + symbole + "'.";
            return false;
    }
}

bool validerPostfixe(const std::vector<Jeton>& jetons, std::vector<Diagnostic>& diagnostics) {
    if (jetons.empty()) {
        diagnostics.push_back({0, "", "expression vide."});
        return false;
    }

    piles::PileTableau<int> operandes; // seuls les nombres d'operandes comptent

    for (const Jeton& jeton : jetons) {
        switch (jeton.type) {
            case TypeJeton::Nombre:
                operandes.push(0);
                break;

            case TypeJeton::Operateur: {
                const int arite = (jeton.arite == Aritete::Unaire) ? 1 : 2;
                if (operandes.size() < static_cast<std::size_t>(arite)) {
                    diagnostics.push_back({jeton.position, jeton.texte,
                                           std::string("operande manquant pour l'operateur \"") + jeton.texte +
                                               "\" en notation postfixee."});
                    return false;
                }
                for (int i = 0; i < arite; ++i) {
                    operandes.pop();
                }
                operandes.push(0);
                break;
            }

            case TypeJeton::ParentheseOuvrante:
                diagnostics.push_back({jeton.position, "(",
                                       "parenthese forbiden en notation postfixee."});
                return false;

            case TypeJeton::ParentheseFermante:
                diagnostics.push_back({jeton.position, ")",
                                       "parenthese fermante forbiden en notation postfixee."});
                return false;

            case TypeJeton::Fin:
                break;
        }
    }

    if (operandes.size() != 1) {
        diagnostics.push_back({jetons.back().position, jetons.back().texte,
                               "notation postfixee incorrecte : il reste " + std::to_string(operandes.size()) +
                                   " valeurs en attente, une seule est attendue."});
        return false;
    }

    return true;
}

bool evaluerPostfixe(const std::vector<Jeton>& jetons, double& resultat, std::vector<Diagnostic>& diagnostics) {
    if (!validerPostfixe(jetons, diagnostics)) {
        return false;
    }

    piles::PileTableau<double> valeurs;

    for (const Jeton& jeton : jetons) {
        if (jeton.type == TypeJeton::Nombre) {
            valeurs.push(jeton.valeur);
            continue;
        }

        if (jeton.arite == Aritete::Unaire) {
            if (jeton.texte == "+") {
                continue; // le plus unaire ne change rien
            }
            const double operande = valeurs.top();
            valeurs.pop();
            valeurs.push(-operande);
            continue;
        }

        const double droite = valeurs.top();
        valeurs.pop();
        const double gauche = valeurs.top();
        valeurs.pop();

        double obtenu = 0.0;
        std::string message;
        if (!appliquerOperateur(jeton.texte[0], gauche, droite, obtenu, message)) {
            diagnostics.push_back({jeton.position, jeton.texte, message});
            return false;
        }
        valeurs.push(obtenu);
    }

    resultat = valeurs.top();
    return true;
}

bool evaluerPostfixe(const std::string& postfixe, double& resultat, std::vector<Diagnostic>& diagnostics) {
    // L'analyse lexicale est celle de l'analyseur : la suite de jetons obtenue
    // est ensuite completee par l'inference d'arite, et l'evaluation se fait
    // exactement comme dans la version vectorielle.
    std::vector<Jeton> jetons = tokeniser(postfixe, diagnostics);
    if (jetons.empty()) {
        if (diagnostics.empty()) {
            diagnostics.push_back({0, "", "notation postfixee vide."});
        }
        return false;
    }

    jetons = avecAritesPostfixeInferes(jetons);
    return evaluerPostfixe(jetons, resultat, diagnostics);
}

bool evaluerArbre(const NoeudExpression* noeud, double& resultat, std::vector<Diagnostic>& diagnostics) {
    if (noeud == nullptr) {
        diagnostics.push_back({0, "", "arbre d'expression vide."});
        return false;
    }

    switch (noeud->genre) {
        case Genre::Nombre:
            resultat = noeud->valeur;
            return true;

        case Genre::Unaire: {
            double operande = 0.0;
            if (!evaluerArbre(noeud->droite.get(), operande, diagnostics)) {
                return false;
            }
            resultat = noeud->operateur == '-' ? -operande : operande;
            return true;
        }

        case Genre::Binaire: {
            double gauche = 0.0;
            double droite = 0.0;
            if (!evaluerArbre(noeud->gauche.get(), gauche, diagnostics)) {
                return false;
            }
            if (!evaluerArbre(noeud->droite.get(), droite, diagnostics)) {
                return false;
            }
            std::string message;
            if (!appliquerOperateur(noeud->operateur, gauche, droite, resultat, message)) {
                diagnostics.push_back({0, std::string(1, noeud->operateur), message});
                return false;
            }
            return true;
        }
    }

    return false;
}

bool evaluer(const std::string& infixe, double& resultat, std::vector<Diagnostic>& diagnostics) {
    // Voie 1 : la methode de la dichotomie, c'est-a-dire les deux piles.
    std::vector<Jeton> postfixe;
    if (!infixeVersPostfixeParDichotomie(infixe, postfixe, diagnostics)) {
        return false;
    }
    if (!evaluerPostfixe(postfixe, resultat, diagnostics)) {
        return false;
    }

    // Voie 2 : l'analyseur descendant. Un ecart entre les deux voies signalerait
    // une faute dans l'une des deux implementations.
    std::vector<Diagnostic> diagnosticsArbre;
    PtrNoeud racine = analyser(infixe, diagnosticsArbre);
    if (racine == nullptr) {
        diagnostics.insert(diagnostics.end(), diagnosticsArbre.begin(), diagnosticsArbre.end());
        return false;
    }

    double resultatArbre = 0.0;
    if (!evaluerArbre(racine.get(), resultatArbre, diagnosticsArbre)) {
        diagnostics.insert(diagnostics.end(), diagnosticsArbre.begin(), diagnosticsArbre.end());
        return false;
    }

    if (resultatArbre != resultat) {
        diagnostics.push_back({0, "",
                               "divergence entre les deux methodes d'evaluation : " +
                                   std::to_string(resultat) + " contre " + std::to_string(resultatArbre) + "."});
        return false;
    }

    return true;
}

} // namespace expressions
