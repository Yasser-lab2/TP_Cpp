#ifndef EVALUATION_HPP
#define EVALUATION_HPP

// Verification de la forme postfixee et evaluation des expressions.
//
// L'evaluation d'une notation postfixee se fait par un balayage des jetons
// avec une pile de valeurs : un nombre est empile, un operateur remplace ses
// un ou deux operandes du sommet par leur resultat. Le meme procede, avec
// une pile de sous-expressions, permet de passer du postfixe au prefixe
// (voir convertisseur.hpp).

#include "analyseur.hpp"
#include "jeton.hpp"

#include <string>
#include <vector>

namespace expressions {

// Verifie qu'une suite de jetons en notation postfixee decrit exactement une
// expression. Simule le depilement des operandes sans ever les calculer :
// un operateur doit disposer de ses operandes, et il doit rester une seule
// valeur sur la pile a la fin.
API_EXPORT bool validerPostfixe(const std::vector<Jeton>& jetons, std::vector<Diagnostic>& diagnostics);

// Evalue une suite de jetons en notation postfixee et renvoie true si
// l'evaluation est possible. Les erreurs lexicales ou arithmetiques (division
// par zero, puissance de zero negatif) sont reportees dans diagnostics.
API_EXPORT bool evaluerPostfixe(const std::vector<Jeton>& jetons, double& resultat, std::vector<Diagnostic>& diagnostics);

// Variante chaine de caracteres : analyse lexicale puis evaluation. Les memes
// regles d'arite s'appliquent qu'en version vectorielle, et notamment
// l'ambiguite du "+" et du "-", traitee ici par le nombre d'operandes
// disponibles au moment de l'operateur.
API_EXPORT bool evaluerPostfixe(const std::string& postfixe, double& resultat, std::vector<Diagnostic>& diagnostics);

// Evalue l'arbre produit par l'analyseur descendant. Sert de reference pour
// recouper l'evaluation postfixee : les deux voies doivent aboutir au meme
// resultat.
API_EXPORT bool evaluerArbre(const NoeudExpression* noeud, double& resultat, std::vector<Diagnostic>& diagnostics);

// Chaine complete : analyse lexicale, analyse syntaxique, ecriture postfixee,
// evaluation. Renvoie false des la premiere erreur rencontree.
API_EXPORT bool evaluer(const std::string& infixe, double& resultat, std::vector<Diagnostic>& diagnostics);

// Application d'un operateur, exposee pour etre testee isolement.
API_EXPORT bool appliquerOperateur(char symbole, double gauche, double droite, double& resultat, std::string& message);

} // namespace expressions

#endif // EVALUATION_HPP
