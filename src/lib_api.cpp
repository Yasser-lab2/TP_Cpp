// Coeur de la bibliotheque partagee.
//
// La frontiere publique est volontairement en C : le client ne transporte
// aucune classe C++, aucune std::string et aucun objet dont la propriete
// serait partagee entre le client et la bibliotheque. Le moteur interne reste
// C++ et est masque par TpSession, type opaque dans tp_api.h.

#include "tp_api.h"

#include "analyseur.hpp"
#include "convertisseur.hpp"
#include "evaluation.hpp"
#include "jeton.hpp"

#include "pile_liste.hpp"
#include "pile_stl.hpp"
#include "pile_tableau.hpp"

#include <cstddef>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

// Les instanciations explicites gardent dans le module les types de piles
// utilises par le moteur. Elles ne font pas partie du contrat C du client.
template class API_EXPORT piles::PileTableau<std::size_t>;
template class API_EXPORT piles::PileListe<std::size_t>;
template class API_EXPORT piles::PileSTL<std::size_t>;
template class API_EXPORT piles::PileTableau<std::string>;
template class API_EXPORT piles::PileListe<std::string>;

struct TpSession {
    // Les diagnostics de la derniere operation sont conserves pendant l'appel
    // courant, puis recopies dans le tampon fourni par le client.
    std::vector<expressions::Diagnostic> memoire;
};

// Copie un message en le tronquant si necessaire et en le terminant toujours
// par '\0'. La frontiere C ne retourne jamais un pointeur vers une std::string.
static void copierMessage(const std::string& texte,
                          char* destination,
                          std::size_t longueurDestination) {
    if (destination == nullptr || longueurDestination == 0) {
        return;
    }
    const std::size_t aCopier =
        texte.size() < longueurDestination - 1 ? texte.size() : longueurDestination - 1;
    std::memcpy(destination, texte.data(), aCopier);
    destination[aCopier] = '\0';
}

static void effacerTexte(char* destination, std::size_t longueurDestination) {
    if (destination != nullptr && longueurDestination > 0) {
        destination[0] = '\0';
    }
}

static std::string premierDiagnostic(const std::vector<expressions::Diagnostic>& diagnostics,
                                    const char* repli) {
    return diagnostics.empty() ? std::string(repli) : diagnostics[0].formater();
}

static int appelInvalide(char* erreur,
                         std::size_t longueurErreur,
                         const char* message) {
    copierMessage(message, erreur, longueurErreur);
    return 0;
}

static int exceptionAttrapee(char* erreur,
                             std::size_t longueurErreur,
                             const char* message) {
    copierMessage(message, erreur, longueurErreur);
    return 0;
}

static bool convertirExpression(const char* infixe,
                               std::vector<expressions::Jeton>& postfixe,
                               std::vector<expressions::Diagnostic>& diagnostics) {
    return expressions::infixeVersPostfixeParDichotomie(std::string(infixe), postfixe, diagnostics);
}

extern "C" API_EXPORT TpSession* tp_creer(void) {
    try {
        return new TpSession{};
    } catch (...) {
        return nullptr;
    }
}

extern "C" API_EXPORT void tp_liberer(TpSession* session) {
    delete session;
}

extern "C" API_EXPORT int tp_verifier(TpSession* session,
                                       const char* infixe,
                                       char* erreur,
                                       std::size_t longueurErreur) {
    effacerTexte(erreur, longueurErreur);
    if (session == nullptr || infixe == nullptr) {
        return appelInvalide(erreur, longueurErreur,
                             "appel invalide : session ou entree null");
    }

    try {
        session->memoire.clear();
        std::vector<expressions::Jeton> postfixe;
        if (convertirExpression(infixe, postfixe, session->memoire)) {
            return 1;
        }
        copierMessage(premierDiagnostic(session->memoire, "expression non valide"),
                      erreur, longueurErreur);
        return 0;
    } catch (const std::exception& exception) {
        return exceptionAttrapee(erreur, longueurErreur, exception.what());
    } catch (...) {
        return exceptionAttrapee(erreur, longueurErreur,
                                 "erreur interne de la bibliotheque");
    }
}

extern "C" API_EXPORT int tp_evaluer(TpSession* session,
                                      const char* infixe,
                                      double* resultat,
                                      char* erreur,
                                      std::size_t longueurErreur) {
    effacerTexte(erreur, longueurErreur);
    if (session == nullptr || infixe == nullptr || resultat == nullptr) {
        return appelInvalide(erreur, longueurErreur,
                             "appel invalide : session, entree ou resultat null");
    }

    try {
        session->memoire.clear();
        double valeur = 0.0;
        if (expressions::evaluer(std::string(infixe), valeur, session->memoire)) {
            *resultat = valeur;
            return 1;
        }
        copierMessage(premierDiagnostic(session->memoire, "expression non valide"),
                      erreur, longueurErreur);
        return 0;
    } catch (const std::exception& exception) {
        return exceptionAttrapee(erreur, longueurErreur, exception.what());
    } catch (...) {
        return exceptionAttrapee(erreur, longueurErreur,
                                 "erreur interne de la bibliotheque");
    }
}

extern "C" API_EXPORT int tp_postfixe(TpSession* session,
                                       const char* infixe,
                                       char* sortie,
                                       std::size_t longueurSortie,
                                       char* erreur,
                                       std::size_t longueurErreur) {
    effacerTexte(sortie, longueurSortie);
    effacerTexte(erreur, longueurErreur);
    if (session == nullptr || infixe == nullptr || sortie == nullptr || longueurSortie == 0) {
        return appelInvalide(erreur, longueurErreur,
                             "appel invalide : session, entree ou sortie null ou tampon vide");
    }

    try {
        session->memoire.clear();
        std::vector<expressions::Jeton> postfixe;
        if (convertirExpression(infixe, postfixe, session->memoire)) {
            const std::string texte = expressions::ecrireJetons(postfixe);
            if (texte.size() + 1 > longueurSortie) {
                return appelInvalide(erreur, longueurErreur,
                                     "tampon de sortie trop petit pour la notation postfixee");
            }
            copierMessage(texte, sortie, longueurSortie);
            return 1;
        }
        copierMessage(premierDiagnostic(session->memoire, "expression non valide"),
                      erreur, longueurErreur);
        return 0;
    } catch (const std::exception& exception) {
        return exceptionAttrapee(erreur, longueurErreur, exception.what());
    } catch (...) {
        return exceptionAttrapee(erreur, longueurErreur,
                                 "erreur interne de la bibliotheque");
    }
}
