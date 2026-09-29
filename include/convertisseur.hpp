#ifndef CONVERTISSEUR_HPP
#define CONVERTISSEUR_HPP

// Conversions entre les trois notations usuelles.
//
//   infixe    (2 + 3) * 4     les operandes encadrent l'operateur
//   prefixe   * + 2 3 4       l'operateur precede ses operandes
//   postfixe  2 3 + 4 *       l'operateur suit ses operandes
//
// Deux niveaux de conversion coexistent.
//
//  1. A partir de l'arbre d'expression : simple parcours de l'arbre, le
//     plus sur comme le plus rapide. Sert de reference.
//
//  2. A partir d'une simple suite de jetons, en n'utilisant qu'une pile de
//     sous-expressions. C'est l'exercice attendu : il ne dispose d'aucune
//     information sur la structure de l'expression.
//
//   - postfixe -> prefixe : lecture de la suite de droite a gauche. Chaque
//     operande est empilee telle quelle ; chaque operateur remplace les une
//     ou deux sous-expressions du sommet par leur combinaison, et l'arite
//     est relue sur le jeton pour distinguer "-3" de "7-3".
//
//   - prefixe -> postfixe : lecture de gauche a droite, le meme procede.
//
// Les deux niveaux sont verifies l'un par l'autre par les tests : une
// conversion reexecutee doit rendre l'original.

#include "analyseur.hpp"
#include "jeton.hpp"

#include <string>
#include <vector>

namespace expressions {

// ---------------------------------------------------------------------------
// Conversions par parcours de l'arbre (niveau 1)
// ---------------------------------------------------------------------------

// Ecrit l'arbre en infixe. Si parenthesesMinimales vaut false, chaque
// operateur est entoure de parentheses, ce qui garantit la reprise exacte de
// l'arbre. Sinon seules les parentheses necessaires a la precedence sont
// conservees.
API_EXPORT std::string ecrireInfixe(const NoeudExpression* noeud, bool parenthesesMinimales = true);

API_EXPORT std::string ecrirePrefixe(const NoeudExpression* noeud);
API_EXPORT std::string ecrirePostfixe(const NoeudExpression* noeud);

// Meme ecriture, mais sous forme de jetons portant chacun l'arite exacte de
// l'operateur. Utilise pour recoupement, ou l'analyseur dispose de
// l'information d'arite que la notation textuelle, elle, ne donne pas.
API_EXPORT std::vector<Jeton> jetonsPostfixeDepuisArbre(const NoeudExpression* noeud);

// ---------------------------------------------------------------------------
// Conversions par pile de sous-expressions (niveau 2)
// ---------------------------------------------------------------------------

// Met bout en bout l'infixe vers le prefixe puis le postfixe, en passant
// par l'arbre. Les etapes intermediaires sont conservees pour l'affichage.
struct Conversion {
    bool valide = false;
    std::string infixe;
    std::string prefixe;
    std::string postfixe;
    std::vector<Diagnostic> diagnostics;
};

// Convertit une expression infixe vers le prefixe et le postfixe. Renvoie
// false si l'expression n'est pas valide, sans rien produire.
API_EXPORT bool convertirDepuisInfixe(const std::string& infixe, Conversion& conversion);

// Convertit une suite de jetons postfixee en notation prefixee, a l'aide
// d'une pile. Verifie au passage que le postfixe est bien forme.
API_EXPORT bool postfixeVersPrefixe(const std::vector<Jeton>& jetons, std::vector<Jeton>& prefixe,
                         std::vector<Diagnostic>& diagnostics);

// Variante chaine de caracteres, pour un usage direct depuis le main.
API_EXPORT bool postfixeVersPrefixe(const std::string& postfixe, std::string& prefixe,
                         std::vector<Diagnostic>& diagnostics);

// Convertit une suite de jetons prefixee en notation postfixee, a l'aide
// d'une pile. Verifie au passage que le prefixe est bien forme.
API_EXPORT bool prefixeVersPostfixe(const std::vector<Jeton>& jetons, std::vector<Jeton>& postfixe,
                         std::vector<Diagnostic>& diagnostics);

API_EXPORT bool prefixeVersPostfixe(const std::string& prefixe, std::string& postfixe,
                         std::vector<Diagnostic>& diagnostics);

// ---------------------------------------------------------------------------
// Rendu textuel d'une suite de jetons
// ---------------------------------------------------------------------------

API_EXPORT std::string ecrireJetons(const std::vector<Jeton>& jetons);

} // namespace expressions

#endif // CONVERTISSEUR_HPP
