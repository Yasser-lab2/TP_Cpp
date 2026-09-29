#ifndef JETON_HPP
#define JETON_HPP

// Modele lexical et metadonnees des operateurs pour les expressions
// arithmetiques.
//
// Le traitement se decompose en trois etapes independantes :
//   1. l'analyse lexicale   : decoupage de la chaine en jetons ;
//   2. l'analyse syntaxique : verification de la validite et de l'ordre des
//                             jetons, construction d'un arbre d'expression ;
//   3. l'evaluation ou la conversion de notation.

#include "api.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace expressions {

// Le signe "+" et le signe "-" peuvent etre soit unaires (devant un operande,
// comme dans "-3"), soit binaires (entre deux operandes, comme dans "7-3").
// C'est le contexte, c'est-a-dire le jeton precedent, qui tranche.
enum class Aritete { Unaire, Binaire };

// ---------------------------------------------------------------------------
// Jetons
// ---------------------------------------------------------------------------

enum class TypeJeton {
    Nombre,
    Operateur,
    ParentheseOuvrante,
    ParentheseFermante,
    Fin
};

struct Jeton {
    TypeJeton type = TypeJeton::Fin;
    std::string texte;        // lexeme tel qu'il apparait dans la source
    double valeur = 0.0;      // significatif si type == Nombre
    std::size_t position = 0; // index du premier caractere dans la source

    // Significatif si type == Operateur. L'analyseur tranche l'arite au
    // moment de la lecture et la memorise ici, ce qui evite de la
    // redeterminer plus tard, au moment ou l'operateur est applique.
    Aritete arite = Aritete::Binaire;
};

// ---------------------------------------------------------------------------
// Erreurs
// ---------------------------------------------------------------------------

struct Diagnostic {
    std::size_t position = 0;
    std::string jeton;   // lexeme incrimine, vide si l'erreur est globale
    std::string message;

    // Rend le diagnostic sous la forme "colonne 7 (jeton ") : message".
    API_EXPORT std::string formater() const;
};

// ---------------------------------------------------------------------------
// Operateurs
// ---------------------------------------------------------------------------

// Precedence et associativite, calculees sans ambiguite par cette table :
//
//   symbole  precedence  associativite
//   + -      0           gauche
//   * / %    1           gauche
//   (unaire) 2           droite
//   ^        3           droite
//
// L'ecart entre la precedence 0 du "+" binaire et la precedence 2 du "-"
// unaire est volontaire : il encode le fait que -2*3 vaut (-2)*3, que
// -2^2 vaut -(2^2) et que 2^-3 est legal.
struct InfoOperateur {
    char symbole;
    int precedence;
    bool associatifGauche;
    const char* nom;
};

API_EXPORT const std::vector<InfoOperateur>& operateurs();

// Recherche la table pour un symbole donne ; nullptr si le symbole n'est pas
// un operateur arithmetique.
API_EXPORT const InfoOperateur* rechercherOperateur(char symbole);

// Rang binaire d'un symbole, tel qu'il figure dans la table des operateurs :
// 0 pour +/- , 1 pour */% , 3 pour ^ ; -1 si le symbole n'est pas un
// operateur arithmetique. Cette fonction sert a la reecriture d'un arbre, ou
// le genre du noeud est deja connu et traite separement.
API_EXPORT int precedenceDe(char symbole);

// Complete une suite de jetons notation postfixee en y portant l'arite de
// chaque operateur.
//
// Le postfixe, comme le prefixe, ne dit pas explicitement si un "+" ou un "-"
// est unaire ou binaire. On tranche par le nombre d'operandes disponibles au
// moment ou l'operateur est rencontre : un operateur binaire en consomme
// deux, un operateur unaire un seul. Les autres operateurs, toujours
// binaires, gardent cette arite.
//
// Ce n'est qu'une heuristique, mais c'est la meilleure possible, car la
// notation postfixee est reellement ambigue sur ce point : "2 3 - +" se lit
// 2 - 3 + (soit -1) ou bien 2 + (-3) (soit 5), et rien dans la suite de
// jetons ne permet de choisir. En usage interne, l'analyseur connait l'arite
// exacte et la fournit deja dans le jeton ; cette fonction ne sert que pour
// du postfixe saisi directement en toutes lettres.
API_EXPORT std::vector<Jeton> avecAritesPostfixeInferes(const std::vector<Jeton>& jetons);

// Precedence effective d'un jeton, arite comprise. C'est cette fonction qu'il
// faut interroger quand l'arite n'est pas encore etablie, car un meme symbole
// "+" donne 0 lorsqu'il est binaire et 2 lorsqu'il est unaire.
API_EXPORT int precedenceDuJeton(const Jeton& jeton);

API_EXPORT bool estOperateur(char symbole);
API_EXPORT bool estOperateurUnaire(char symbole);
API_EXPORT bool estChiffre(char c);
API_EXPORT bool estEspace(char c);

// Rend un double sous la forme la plus courte qui se relit identiquement :
// 3.0 devient "3", 2.5 reste "2.5". Indispensable pour produire une notation
// postfixee ou prefixee qui puisse etre re-analysee telle quelle.
API_EXPORT std::string formaterNombre(double valeur);

} // namespace expressions

#endif // JETON_HPP
