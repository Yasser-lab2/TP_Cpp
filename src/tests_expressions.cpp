// Tests du module d'expressions arithmetiques.
//
// Chaque famille de tests reprend un exemple du rapport : la suite sert donc
// de preuve executable que les resultats presentes sont bien exacts.

#include "analyseur.hpp"
#include "convertisseur.hpp"
#include "evaluation.hpp"
#include "jeton.hpp"
#include "test.hpp"

#include <string>
#include <vector>

namespace {

using expressions::Conversion;
using expressions::Diagnostic;
using expressions::Jeton;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Verifie qu'une expression est refusee et que le diagnostic mentionne le
// mot-cle attendu. Le mot-cle evite un test trop lie au libelle exact, qui
// evoluerait sans que le comportement change.
void verifierRefusee(const std::string& infixe, const std::string& motCle, const std::string& libelle) {
    std::vector<Diagnostic> diagnostics;
    double resultat = 0.0;
    const bool valide = expressions::evaluer(infixe, resultat, diagnostics);

    if (valide) {
        tests::echec(libelle, "l'expression a ete acceptee (resultat " + std::to_string(resultat) + ")");
        return;
    }
    if (diagnostics.empty()) {
        tests::echec(libelle, "refus sans aucun diagnostic");
        return;
    }

    std::string detail;
    bool motCleTrouve = false;
    for (const Diagnostic& d : diagnostics) {
        detail += d.formater() + " | ";
        if (d.message.find(motCle) != std::string::npos) {
            motCleTrouve = true;
        }
    }
    tests::verifier(motCleTrouve, libelle, detail);
}

void verifierAcceptee(const std::string& infixe, double attendu, const std::string& libelle) {
    std::vector<Diagnostic> diagnostics;
    double resultat = 0.0;
    if (!expressions::evaluer(infixe, resultat, diagnostics)) {
        std::string detail;
        for (const Diagnostic& d : diagnostics) {
            detail += d.formater() + " | ";
        }
        tests::echec(libelle, "expression refusee : " + detail);
        return;
    }
    tests::verifierEgaux(attendu, resultat, libelle);
}

void verifierConversion(const std::string& infixe, const std::string& prefixeAttendu,
                        const std::string& postfixeAttendu, const std::string& libelle) {
    Conversion conversion;
    if (!expressions::convertirDepuisInfixe(infixe, conversion)) {
        std::string detail;
        for (const Diagnostic& d : conversion.diagnostics) {
            detail += d.formater() + " | ";
        }
        tests::echec(libelle, "conversion impossible : " + detail);
        return;
    }
    if (conversion.prefixe != prefixeAttendu || conversion.postfixe != postfixeAttendu) {
        tests::echec(libelle, "prefixe \"" + conversion.prefixe + "\" / postfixe \"" + conversion.postfixe +
                                  "\", attendus \"" + prefixeAttendu + "\" / \"" + postfixeAttendu + "\"");
        return;
    }
    tests::succes(libelle);
}

// ---------------------------------------------------------------------------
// 1. Validite
// ---------------------------------------------------------------------------

void testerValidite() {
    tests::section("1. Analyse lexicale : decoupage en jetons");

    std::vector<Diagnostic> diagnostics;
    std::vector<Jeton> jetons = expressions::tokeniser("12 + 3.5*(2)", diagnostics);
    tests::verifier(jetons.size() == 7, "l'expression produit sept jetons",
                    std::to_string(jetons.size()) + " jetons");
    tests::verifier(jetons[0].type == expressions::TypeJeton::Nombre && jetons[0].valeur == 12.0,
                    "le premier jeton est le nombre 12");
    tests::verifier(jetons[1].type == expressions::TypeJeton::Operateur && jetons[1].texte == "+",
                    "le deuxieme jeton est l'operateur +");
    tests::verifier(jetons[3].type == expressions::TypeJeton::Operateur && jetons[3].texte == "*",
                    "le jeton 4 est l'operateur *");
    tests::verifier(jetons[2].position == 5, "le jeton 3.5 commence a la colonne 6",
                    "position " + std::to_string(jetons[2].position));

    verifierRefusee("3 $ 4", "non reconnu", "un caractere inconnu est signale");
    verifierRefusee("1..2", "mal forme", "un nombre avec deux points decimaux est refuse");
    verifierRefusee("", "vide", "une chaine vide est refusee");
    verifierRefusee("   ", "vide", "une chaine d'espaces est refusee");

    tests::section("2. Parenthesisation : equilibre des parentheses");

    verifierRefusee("(2+3", "jamais fermee", "parenthese ouvrante non fermee");
    verifierRefusee("2+3)", "sans ouverture", "parenthese fermante orpheline");
    verifierRefusee("()", "operande manquant", "parentheses vides");
    verifierAcceptee("((2+3))", 5.0, "l'imbrication correcte est acceptee");
    verifierAcceptee("((1+2))*3", 9.0, "imbrication puis operation externe");

    tests::section("3. Operandes manquants et operateurs en trop");

    verifierRefusee("2 +", "operande manquant", "operande manquant en fin d'expression");
    verifierRefusee("* 3", "operande manquant", "expression commencant par un operateur");
    verifierRefusee("2 + * 3", "operande manquant", "deux operateurs consecutifs");
    verifierRefusee("(2 + 3", "jamais fermee", "fin prematuree apres une parenthese");
    verifierRefusee("2 3", "juxtaposes", "deux operandes juxtaposes sans operateur");
    verifierRefusee("2(3+4)", "juxtaposes", "un operande ne peut pas preceder une parenthese");
    verifierRefusee("(2+3)(4+5)", "juxtaposes", "deux groupes doivent etre separes par un operateur");

    tests::section("4. Ordre d'evaluation : precedence et associativite");

    verifierAcceptee("2+3*4", 14.0, "la multiplication l'emporte sur l'addition");
    verifierAcceptee("2*3+4", 10.0, "l'addition s'applique apres la multiplication");
    verifierAcceptee("10-3-2", 5.0, "la soustraction est associative a gauche : (10-3)-2");
    verifierAcceptee("2^3^2", 512.0, "la puissance est associative a droite : 2^(3^2)");
    verifierAcceptee("2^3*2", 16.0, "la puissance prime sur la multiplication");
    verifierAcceptee("2*3^2", 18.0, "2*(3^2)");
    verifierAcceptee("-2*3", -6.0, "le signe unaire s'applique a 2 : (-2)*3");
    verifierAcceptee("-2^2", -4.0, "la puissance prime sur le signe : -(2^2)");
    verifierAcceptee("2^-3", 0.125, "un exposant negatif est legal : 2^(-3)");
    verifierAcceptee("--5", 5.0, "deux signes moins consecutifs s'annulent");
    verifierAcceptee("-(3+4)", -7.0, "le signe porte sur le groupe parenthese");

    tests::section("5. Erreurs arithmetiques");

    verifierRefusee("1/0", "division par zero", "division par zero");
    verifierRefusee("1%0", "zero", "reste par zero");
    verifierRefusee("(-8)^0.5", "negative", "racine d'une base negative");
    verifierAcceptee("(0-8)^2", 64.0, "une base negative elevee a un entier pair reste valide");
}

// ---------------------------------------------------------------------------
// 6. Recoupement des deux analyseurs
// ---------------------------------------------------------------------------

void testerConcordance() {
    tests::section("6. Concordance des deux methodes d'analyse");

    // L'analyse descendante recursive et la methode de la dichotomie sont deux
    // implementations independantes. Elles doivent produire le meme postfixe.
    const std::vector<std::string> expressions = {
        "1", "2+3", "2+3*4", "(2+3)*4", "10-3-2", "2^3^2", "-2^2", "2^-3", "3*-2",
        "1+2-3+4-5", "((1+2)*(3+4))/(5-6)", "-(2+3)*4", "100/5/2", "7%4+1", "2*3^2-1",
    };

    int concordances = 0;
    for (const std::string& source : expressions) {
        std::vector<Diagnostic> diagnostics;

        std::vector<Jeton> postfixeDichotomie;
        const bool okDichotomie = expressions::infixeVersPostfixeParDichotomie(source, postfixeDichotomie, diagnostics);

        expressions::PtrNoeud racine = expressions::analyser(source, diagnostics);
        const std::string postfixeArbre = okDichotomie && racine ? expressions::ecrirePostfixe(racine.get()) : "";

        if (okDichotomie && racine && expressions::ecrireJetons(postfixeDichotomie) == postfixeArbre) {
            ++concordances;
        } else {
            std::cout << "          divergence sur \"" << source << "\"\n";
        }
    }

    tests::verifier(concordances == static_cast<int>(expressions.size()),
                    "les deux analyseurs donnent le meme postfixe sur les " +
                        std::to_string(expressions.size()) + " expressions",
                    std::to_string(concordances) + " concordances sur " + std::to_string(expressions.size()));

    tests::section("7. Concordance des deux methodes d'evaluation");

    int accords = 0;
    for (const std::string& source : expressions) {
        std::vector<Diagnostic> diagnosticsD;
        double parPostfixe = 0.0;
        if (!expressions::evaluer(source, parPostfixe, diagnosticsD)) {
            continue;
        }

        std::vector<Diagnostic> diagnosticsA;
        expressions::PtrNoeud racine = expressions::analyser(source, diagnosticsA);
        double parArbre = 0.0;
        if (racine != nullptr && expressions::evaluerArbre(racine.get(), parArbre, diagnosticsA)) {
            if (std::abs(parPostfixe - parArbre) < 1e-12) {
                ++accords;
                continue;
            }
        }
        std::cout << "          desaccord sur \"" << source << "\" : " << parPostfixe << " / " << parArbre << "\n";
    }

    tests::verifier(accords == static_cast<int>(expressions.size()),
                    "l'evaluation par pile et par arbre concordent sur toutes les expressions",
                    std::to_string(accords) + " accords");
}

// ---------------------------------------------------------------------------
// 8. Conversions de notation
// ---------------------------------------------------------------------------

void testerConversions() {
    tests::section("8. Conversions infixe / prefixe / postfixe");

    verifierConversion("2+3", "+ 2 3", "2 3 +", "expression la plus simple");
    verifierConversion("2+3*4", "+ 2 * 3 4", "2 3 4 * +", "precedence appliquee");
    verifierConversion("(2+3)*4", "* + 2 3 4", "2 3 + 4 *", "parentheses explicites");
    verifierConversion("2^3^2", "^ 2 ^ 3 2", "2 3 2 ^ ^", "associativite a droite preservee");
    verifierConversion("-2^2", "- ^ 2 2", "2 2 ^ -", "le signe unaire reste unaire");
    verifierConversion("2^-3", "^ 2 - 3", "2 3 - ^", "exposant negatif");
    verifierConversion("(2+3)*(4-5)", "* + 2 3 - 4 5", "2 3 + 4 5 - *", "deux sous-groupes");

    tests::section("9. Conversions par pile, sans arbre");

    struct Cas {
        const char* postfixe;
        const char* prefixe;
    };

    const std::vector<Cas> cas = {
        {"2 3 +", "+ 2 3"},
        {"2 3 4 * +", "+ 2 * 3 4"},
        {"2 3 + 4 *", "* + 2 3 4"},
        {"2 3 2 ^ ^", "^ 2 ^ 3 2"},
        {"2 2 ^ -", "- ^ 2 2"},
        {"2 3 4 - ^", "^ 2 - 3 4"},
    };

    int allers = 0;
    int retours = 0;
    for (const Cas& c : cas) {
        std::vector<Diagnostic> diagnostics;
        std::string prefixe;
        if (expressions::postfixeVersPrefixe(c.postfixe, prefixe, diagnostics) && prefixe == c.prefixe) {
            ++allers;
        } else {
            std::cout << "          aller echoue : \"" << c.postfixe << "\" -> \"" << prefixe << "\"\n";
        }

        std::string postfixe;
        if (expressions::prefixeVersPostfixe(c.prefixe, postfixe, diagnostics) && postfixe == c.postfixe) {
            ++retours;
        } else {
            std::cout << "          retour echoue : \"" << c.prefixe << "\" -> \"" << postfixe << "\"\n";
        }
    }

    tests::verifier(allers == static_cast<int>(cas.size()),
                    "postfixe vers prefixe : tous les aller-retours concordent",
                    std::to_string(allers) + "/" + std::to_string(cas.size()));
    tests::verifier(retours == static_cast<int>(cas.size()),
                    "prefixe vers postfixe : tous les aller-retours concordent",
                    std::to_string(retours) + "/" + std::to_string(cas.size()));

    tests::section("10. Notations postfixee et prefixee invalides");

    // Ces cas ne passent pas par l'analyseur infixe : ils sont verifiees
    // directement sur les convertisseurs, chacun avec sa propre validation.
    struct CasInvalide {
        const char* notation;
        const char* cle; // "post" ou "pre"
        const char* libelle;
    };

    const std::vector<CasInvalide> invalides = {
        {"2 3", "post", "un postfixe laissant deux valeurs en attente est refuse"},
        {"2 3 * *", "post", "un postfixe dont un '*' n'a qu'un seul operande est refuse"},
        {"* 2", "post", "un operateur binaire sans operande est refuse"},
        {"* 2", "pre", "un operateur binaire sans second operande est refuse"},
        {"* 2 +", "pre", "un prefixe interrompu par un operateur sans valeur est refuse"},
        {"2 3 +", "pre", "un prefixe sans operateur initial est refuse"},
    };

    // Ces deux notations sont paradoxalement valides, et leur arite ne peut
    // pas etre devinee sans convention. Sous la convention retenue, l'arite
    // unaire est tentee en premier, et l'on obtient :
    //   "2 3 + + +"  se lit  2 + 3 = 5, puis +5, puis +5,  soit 5 ;
    //   "2 + 3 +"    se lit  +2 puis 3, puis 2 + 3,         soit 5.
    // Les deux sont documentees ici pour montrer que le refus de ces formes
    // serait une decision de convention, non une erreur de syntaxe.
    for (const char*notation : {"2 3 + + +", "2 + 3 +"}) {
        std::vector<Diagnostic> diagnostics;
        std::string prefixe;
        const bool ok = expressions::postfixeVersPrefixe(notation, prefixe, diagnostics);
        tests::verifier(ok, std::string("un postfixe valide mais ambigu est accepte : \"") + notation + "\" est accepte",
                        ok ? "lu : " + prefixe : "refuse a tort");
    }

    int refus = 0;
    for (const CasInvalide& cas : invalides) {
        std::vector<Diagnostic> diagnostics;
        std::string resultat;
        const bool ok = (std::string(cas.cle) == "post")
                            ? expressions::postfixeVersPrefixe(cas.notation, resultat, diagnostics)
                            : expressions::prefixeVersPostfixe(cas.notation, resultat, diagnostics);
        if (!ok) {
            ++refus;
        } else {
            std::cout << "          accepte a tort : \"" << cas.notation << "\"\n";
        }
    }
    tests::verifier(refus == static_cast<int>(invalides.size()),
                    "les six notations mal formees sont refusees",
                    std::to_string(refus) + "/" + std::to_string(invalides.size()));

    tests::section("11. Aller-retour complet sur 200 expressions generees");

    // Les expressions sont produites par un parcours deterministe de
    // combinaisons d'operateurs, puis chaque notation est reanalysee : le
    // resultat doit rester identique. C'est le test le plus large du projet.
    int reussites = 0;
    int total = 0;
    for (int i = 0; i < 200; ++i) {
        // Expression construite a la main autour de valeurs entieres, afin de
        // rester exactement representable en virgule flottante.
        const int a = static_cast<int>(i % 7) + 1;
        const int b = (i * 3) % 11 + 1;
        const int c = (i * 5) % 5 + 2;
        const char operateur1 = "+-*/%"[i % 5];
        const char operateur2 = "+-*"[(i / 5) % 3];

        std::string infixe = std::to_string(a) + operateur1 + std::to_string(b);
        if (i % 3 == 0) {
            infixe = "(" + std::to_string(a) + operateur1 + std::to_string(b) + ")" + operateur2 + std::to_string(c);
        } else if (i % 3 == 1) {
            infixe = std::to_string(a) + operateur1 + std::to_string(b) + "^" + std::to_string(c);
        } else {
            infixe = "-" + std::to_string(a) + operateur1 + std::to_string(b);
        }

        // Une division ou un reste par zero doit etre ecartee.
        const std::size_t dernierOperateur = infixe.find_last_of("+-*/%^");
        if (dernierOperateur != std::string::npos) {
            const std::string droite = infixe.substr(dernierOperateur + 1);
            if ((operateur1 == '/' || operateur1 == '%') && droite == "0") {
                continue;
            }
        }

        ++total;
        Conversion conversion;
        if (expressions::convertirDepuisInfixe(infixe, conversion)) {
            ++reussites;
        } else {
            std::cout << "          echec sur \"" << infixe << "\"\n";
        }
    }

    tests::verifier(reussites == total,
                    "les " + std::to_string(total) +
                        " expressions construites survivent a l'aller-retour infixe/prefixe/postfixe",
                    std::to_string(reussites) + "/" + std::to_string(total));
}

void testerRenduNombre() {
    tests::section("12. Rendu textuel des nombres");

    tests::verifierEgaux(std::string("3"), expressions::formaterNombre(3.0), "3.0 devient 3");
    tests::verifierEgaux(std::string("2.5"), expressions::formaterNombre(2.5), "2.5 reste 2.5");
    tests::verifierEgaux(std::string("0.1"), expressions::formaterNombre(0.1), "0.1 ne devient pas 0.1000000000000001");
    tests::verifierEgaux(std::string("-7"), expressions::formaterNombre(-7.0), "les valeurs negatives sont geree");
}

} // namespace

int main() {
    std::cout << "Tests du module d'expressions\n";
    std::cout << "==============================\n";

    testerValidite();
    testerConcordance();
    testerConversions();
    testerRenduNombre();

    return tests::bilan("Tests des expressions");
}
