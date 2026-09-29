#ifndef TP_API_H
#define TP_API_H

// Interface C de la bibliotheque partagee.
//
// Ce fichier est le seul en-tete qu'un client externe doit inclure pour
// utiliser le moteur d'expressions : il ne depend pas des en-tetes C++
// du projet (jeton.hpp, analyseur.hpp, ...) et peut etre inclus depuis un
// programme ecrit en C pur. Toute la frontiere avec le C++ est encapsulee
// dans la bibliotheque elle-meme.
//
// Un TypePile "session" opaque regroupe l'etat d'une utilisation :
//   tp_creer()   -> tp_verifier()/tp_evaluer()/tp_postfixe() ... -> tp_liberer()
// L'ordre des appels est impose par le contrat : on ne peut pas evaluer
// une expression sans avoir cree une session, et toute session creee doit
// etre liberee exactement une fois.

#include "api.hpp"

#include <stddef.h>

API_DEBUT_EXTERN_C

/* Session opaque : contexte d'evaluation valable entre tp_creer et tp_liberer. */
typedef struct TpSession TpSession;

/* Cree une session. Renvoie NULL si l'allocation echoue. */
API_EXPORT TpSession* tp_creer(void);

/* Detruit la session et libere tout ce qu'elle contenait. */
API_EXPORT void tp_liberer(TpSession* session);

/* Verifie la syntaxe d'une expression infixe.
   Renvoie 1 si l'expression est valide, 0 sinon. En cas d'echec, le premier
   diagnostic est copie dans erreur lorsqu'un tampon est fourni. Cette
   fonction ne calcule pas la valeur : une erreur arithmetique (par exemple
   une division par zero) est detectee par tp_evaluer. */
API_EXPORT int tp_verifier(TpSession* session,
                           const char* infixe,
                           char* erreur,
                           size_t longueurErreur);

/* Evalue une expression infixe. Renvoie 1 et ecrit la valeur dans *resultat.
   Renvoie 0 et ecrit le message du premier diagnostic dans erreur, tronque
   a taille erreur, sinon. */
API_EXPORT int tp_evaluer(TpSession* session,
                          const char* infixe,
                          double* resultat,
                          char* erreur,
                          size_t longueurErreur);

/* Convertit une expression infixe en notation postfixee, apres avoir
   verifie sa validite. Renvoie 1 en cas de succes ; 0 sinon, avec le message
   dans erreur. La forme postfixee est ecrite dans sortie, terminee par '\0'. */
API_EXPORT int tp_postfixe(TpSession* session,
                           const char* infixe,
                           char* sortie,
                           size_t longueurSortie,
                           char* erreur,
                           size_t longueurErreur);

API_FIN_EXTERN_C

#endif // TP_API_H
