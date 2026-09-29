// Client externe de la bibliotheque partagee.
//
// Ce programme ne voit que l'en-tete public tp_api.h et la bibliotheque
// libtp : il n'inclut aucun en-tete C++ du moteur et n'a acces a aucun
// fichier source interne. C'est exactement la situation d'un programme qui
// reutiliserait le module sans le recomplier.
//
// L'ordre des appels est celui impose par le contrat de la session :
//   tp_creer() -> tp_verifier() / tp_evaluer() / tp_postfixe() -> ... -> tp_liberer()

#include "tp_api.h"

#include <cstdio>
#include <initializer_list>

int main() {
    TpSession* session = tp_creer();
    if (session == nullptr) {
        std::fprintf(stderr, "tp_creer a echoue : impossible de continuer.\n");
        return 1;
    }

    const char* exemples[] = {
        "2 + 3 * 4",
        "-2 ^ 2",
        "(1 + 2) * (3 - 4)",
        "0.1 + 0.2",
        "2 3 *",
        "1 / 0",
        "2 +",
    };

    std::printf("Evaluation via la bibliotheque libtp (session opaque) :\n");
    std::printf("-------------------------------------------------------\n");
    for (const char* expression : exemples) {
        char erreur[256];
        erreur[0] = '\0';
        double resultat = 0.0;
        const int ok = tp_evaluer(session, expression, &resultat, erreur, sizeof erreur);
        if (ok) {
            std::printf("  %-18s -> %.10g\n", expression, resultat);
        } else {
            std::printf("  %-18s -> [invalide] %s\n", expression, erreur);
        }
    }

    std::printf("\nVerification syntaxique directe :\n");
    std::printf("--------------------------------\n");
    for (const char* expression : {"2(3 + 4)", "2 + 3 * 4"}) {
        char erreur[256];
        const int ok = tp_verifier(session, expression, erreur, sizeof erreur);
        std::printf("  %-18s -> %s%s\n", expression,
                    ok ? "valide" : "invalide : ", ok ? "" : erreur);
    }

    std::printf("\nForme postfixee (validite verifiee par la bibliotheque) :\n");
    std::printf("----------------------------------------------------------\n");
    char postfixe[256];
    char erreur[256];
    postfixe[0] = '\0';
    erreur[0] = '\0';
    if (tp_postfixe(session, "2 + 3 * 4", postfixe, sizeof postfixe,
                    erreur, sizeof erreur)) {
        std::printf("  2 + 3 * 4 -> %s\n", postfixe);
    } else {
        std::printf("  2 + 3 * 4 -> [invalide] %s\n", erreur);
    }
    postfixe[0] = '\0';
    erreur[0] = '\0';
    if (tp_postfixe(session, "2 2 + +", postfixe, sizeof postfixe,
                    erreur, sizeof erreur)) {
        std::printf("  2 2 + +  -> ?\n");
    } else {
        std::printf("  2 2 + +  -> [invalide] %s\n", erreur);
    }

    tp_liberer(session);
    std::printf("\nSession liberee : aucun appel n'est possible apres ce point.\n");
    return 0;
}