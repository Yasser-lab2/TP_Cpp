// Demonstration de l'outillage sur les expressions arithmetiques.
//
// Le programme enchaine les trois etapes de la problematique :
//
//   1. dire si une expression infixe est bien formee, et signaler précisément
//      pourquoi elle ne l'est pas ;
//   2. l'evaluer, par une pile sur notation postfixee, et par l'arbre
//      d'expression, les deux chemins devant concordancer ;
//   3. la convertir entre notations infixe, prefixee et postfixee.
//
// Chaque etape est illustree sur des cas qui ont du sens pedagogique,
// notamment les priorites, l'egalite de priorite a droite, et les signes
// unaires.

#include "analyseur.hpp"
#include "convertisseur.hpp"
#include "evaluation.hpp"
#include "jeton.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Affiche les diagnostics d'une suite d'erreurs, ou un tiret s'il n'y en a pas.
void afficherDiagnostics(const std::vector<expressions::Diagnostic>& diagnostics) {
    if (diagnostics.empty()) {
        std::cout << "        aucun diagnostic\n";
        return;
    }
    for (const expressions::Diagnostic& diagnostic : diagnostics) {
        std::cout << "        " << diagnostic.formater() << "\n";
    }
}

void analyserEtEvaluer(const std::string& infixe) {
    std::cout << "  \"" << infixe << "\"\n";

    std::vector<expressions::Diagnostic> diagnostics;
    const expressions::PtrNoeud racine = expressions::analyser(infixe, diagnostics);

    if (racine == nullptr) {
        std::cout << "    -> expression refusee\n";
        afficherDiagnostics(diagnostics);
        std::cout << '\n';
        return;
    }

    expressions::Conversion resultat;
    if (!expressions::convertirDepuisInfixe(infixe, resultat)) {
        std::cout << "    -> expression refusee\n";
        afficherDiagnostics(resultat.diagnostics);
        std::cout << '\n';
        return;
    }

    // L'evaluation par l'arbre est lancee a part, pour montrer que les deux
    // chemins independants aboutissent au meme resultat.
    double valeur = 0.0;
    std::vector<expressions::Diagnostic> diagnosticsArbre;
    expressions::evaluerArbre(racine.get(), valeur, diagnosticsArbre);

    std::cout << std::fixed << std::setprecision(6) << "    -> valeur          : " << valeur << '\n';
    std::cout << "    -> infixe minimal  : " << resultat.infixe << '\n';
    std::cout << "    -> prefixe         : " << resultat.prefixe << '\n';
    std::cout << "    -> postfixe        : " << resultat.postfixe << '\n';
    std::cout << '\n';
}

void analyserInfixe() {
    std::cout << "1. Analyse d'une expression infixe\n";
    std::cout << "----------------------------------\n";
    for (const char* expression : {"2 + 3 * 4", "(2 + 3) * 4", "2 ^ 3 ^ 2", "-2 ^ 2", "2 + 3)"}) {
        analyserEtEvaluer(expression);
    }
}

void evaluer() {
    std::cout << "2. Evaluation par notation postfixee\n";
    std::cout << "------------------------------------\n";

    struct Cas {
        const char* postfixe;
        const char* commentaire;
    };

    const std::vector<Cas> cas = {
        {"3 4 + 2 *", "addition puis multiplication, par priorite"},
        {"2 3 4 + *", "parentheses explicites autour de 3 + 4"},
        {"2 3 + 4 *", "priorite par defaut, sans parentheses"},
        {"2 3 2 ^ ^", "l'exposant est associatif a droite"},
        {"2 3 -", "le sommet de la pile est l'operande de droite"},
        {"5 -", "un signe unaire retire une seule valeur"},
    };

    std::cout << std::fixed << std::setprecision(6);
    for (const Cas& c : cas) {
        std::vector<expressions::Diagnostic> diagnostics;
        double valeur = 0.0;
        const bool ok = expressions::evaluerPostfixe(c.postfixe, valeur, diagnostics);
        std::cout << "  " << std::setw(14) << std::left << c.postfixe << std::right << "  ->  ";
        if (ok) {
            std::cout << std::setw(12) << valeur << "   " << c.commentaire << '\n';
        } else {
            std::cout << "refuse\n";
            afficherDiagnostics(diagnostics);
        }
    }
    std::cout << '\n';
}

void convertir() {
    std::cout << "3. Conversions de notations\n";
    std::cout << "---------------------------\n";
    std::cout << std::fixed << std::setprecision(6);

    // Conversions dans les deux sens, en passant par la notation intermediaire
    // qui est la plus simple a verifier.
    const std::vector<const char*> paires = {
        "3 4 + 2 *",   // postfixe -> prefixe
        "* + 2 3 4",   // prefixe  -> postfixe, soit (2 + 3) * 4
        "2 3 4 * +",   // postfixe -> prefixe
        "+ - 3 4",     // prefixe  -> postfixe, signe unaire : (-3) + 4
        "2 3 2 ^ ^",   // postfixe -> prefixe, associativite a droite
        "- ^ 2 2",     // prefixe  -> postfixe, soit -(2 ^ 2)
    };

    for (const char* notation : paires) {
        std::vector<expressions::Diagnostic> diagnostics;
        std::string resultat;
        const bool departPrefixe =
            std::string(notation).find_first_of("+-*^") == 0;

        const bool ok = departPrefixe ? expressions::prefixeVersPostfixe(notation, resultat, diagnostics)
                                      : expressions::postfixeVersPrefixe(notation, resultat, diagnostics);

        std::cout << "  " << std::setw(14) << std::left << notation << std::right << "  ->  ";
        if (ok) {
            std::cout << std::setw(14) << resultat << '\n';
        } else {
            std::cout << "refuse\n";
            afficherDiagnostics(diagnostics);
        }
    }
    std::cout << '\n';
}

void ambiguite() {
    std::cout << "4. Le cas difficile : l'arite des signes\n";
    std::cout << "--------------------------------------\n";
    std::cout << "En notation prefixee comme en notation postfixee, un \"+\" ou un \"-\"\n"
              << "ne precise pas s'il est unaire ou binaire. Deux suites franchement\n"
              << "ambigues :\n\n";

    const std::vector<const char*> ambigues = {"2 3 - +", "1 2 + 3 +"};

    for (const char* notation : ambigues) {
        std::vector<expressions::Diagnostic> diagnostics;
        std::string prefixe;
        const bool ok = expressions::postfixeVersPrefixe(notation, prefixe, diagnostics);

        std::cout << "  \"" << notation << "\" se lit ";
        if (!ok) {
            std::cout << " : refus\n";
            afficherDiagnostics(diagnostics);
        } else {
            std::cout << "\"" << prefixe << "\"\n";
        }
    }

    std::cout << "\nLa convention retenue est d'essayer l'arite unaire en premier, ce qui\n"
              << "privilegie la lecture naturelle. Ce n'est pas une deduction : c'est un\n"
              << "choix, et aucune suite de jetons ne permettrait de le prouver. Le cours\n"
              << "evite d'ailleurs la question en imposant des parentheses autour du signe\n"
              << "unaire lorsqu'il se combine a d'autres operateurs.\n\n";
}

} // namespace

int main() {
    std::cout << "Demonstration de l'outillage sur les expressions\n";
    std::cout << "===============================================\n\n";

    analyserInfixe();
    evaluer();
    convertir();
    ambiguite();

    return 0;
}
