#ifndef ANALYSEUR_HPP
#define ANALYSEUR_HPP

// Analyse lexicale et analyse syntaxique des expressions arithmetiques.
//
// Deux analyseurs syntaxiques complementaires sont fournis :
//
//   - AnalyseurDescendantRecursif : descente recursive, qui construit un
//     arbre d'expression et produit des diagnostics tres precis (colonne,
//     nature du jeton). Sert de reference.
//
//   - methode de la dichotomie ("shunting-yard") : conversion directe
//     infixe -> postfixe a l'aide de deux piles, avec verification
//     syntaxique au passage.
//
// Le fait que les deux methodes, ecrites independamment, donnent le meme
// resultat constitue le principal argument de validation du rapport.

#include "jeton.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace expressions {

// ---------------------------------------------------------------------------
// Analyse lexicale
// ---------------------------------------------------------------------------

// Decoupe la chaine en jetons. Les espaces sont ignores. Toute suite de
// caracteres non reconnue est signalee puis ignoree, afin que l'analyse
// lexicale ne s'arrete jamais sur une entree arbitraire.
API_EXPORT std::vector<Jeton> tokeniser(const std::string& source, std::vector<Diagnostic>& diagnostics);

// ---------------------------------------------------------------------------
// Arbre d'expression
// ---------------------------------------------------------------------------

enum class Genre { Nombre, Unaire, Binaire };

struct NoeudExpression {
    Genre genre = Genre::Nombre;
    double valeur = 0.0;              // si genre == Nombre
    char operateur = '\0';            // si genre != Nombre
    std::unique_ptr<NoeudExpression> gauche;  // operande binaire de gauche
    std::unique_ptr<NoeudExpression> droite;  // operande droit (binaire)
};

using PtrNoeud = std::unique_ptr<NoeudExpression>;

// Construit un noeud feuille numerique.
API_EXPORT PtrNoeud creerNombre(double valeur);

// ---------------------------------------------------------------------------
// Analyse syntaxique par descente recursive
// ---------------------------------------------------------------------------

// Analyse l'expression et renvoie son arbre. En cas d'erreur, renvoie nullptr
// et renseigne diagnostics. Une seule erreur est rapportee par appel : le
// parcours s'arrete sur la premiere anomalie, ce qui evite des messages
// en cascade impossibles a relire.
API_EXPORT PtrNoeud analyser(const std::string& source, std::vector<Diagnostic>& diagnostics);

// Variante prenant en entree une suite de jetons deja produite.
API_EXPORT PtrNoeud analyserJetons(const std::vector<Jeton>& jetons, std::vector<Diagnostic>& diagnostics);

// ---------------------------------------------------------------------------
// Analyse syntaxique par methode de la dichotomie (deux piles)
// ---------------------------------------------------------------------------

// Convertit l'infixe en postfixe. Utilise une pile d'operateurs et une pile
// de sortie, toutes deux instanciees depuis PileTableau.
API_EXPORT bool infixeVersPostfixeParDichotomie(const std::string& source,
                                     std::vector<Jeton>& postfixe,
                                     std::vector<Diagnostic>& diagnostics);

// Variante sur une suite de jetons deja produite.
API_EXPORT bool jetonsVersPostfixeParDichotomie(const std::vector<Jeton>& jetons,
                                     std::vector<Jeton>& postfixe,
                                     std::vector<Diagnostic>& diagnostics);

} // namespace expressions

#endif // ANALYSEUR_HPP
