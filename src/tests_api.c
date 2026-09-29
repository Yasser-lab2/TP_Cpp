#include "tp_api.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int verifier(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "[ECHEC] %s\n", message);
        return 1;
    }
    return 0;
}

int main(void) {
    int echecs = 0;
    TpSession* session = tp_creer();
    char erreur[256];
    char sortie[256];
    double resultat = 0.0;

    echecs += verifier(session != NULL, "tp_creer doit fournir une session");
    if (session == NULL) {
        return 1;
    }

    erreur[0] = '\0';
    echecs += verifier(tp_verifier(session, "2 + 3 * 4", erreur, sizeof erreur) == 1,
                       "une expression infixe correcte doit etre acceptee");

    erreur[0] = '\0';
    echecs += verifier(tp_verifier(session, "2(3 + 4)", erreur, sizeof erreur) == 0 &&
                           strstr(erreur, "juxtaposes") != NULL,
                       "une juxtaposition doit etre refusee avec un diagnostic");

    erreur[0] = '\0';
    echecs += verifier(tp_evaluer(session, "2 + 3 * 4", &resultat,
                                  erreur, sizeof erreur) == 1 &&
                           fabs(resultat - 14.0) < 1e-12,
                       "tp_evaluer doit renvoyer 14");

    erreur[0] = '\0';
    sortie[0] = '\0';
    echecs += verifier(tp_postfixe(session, "2 + 3 * 4", sortie, sizeof sortie,
                                   erreur, sizeof erreur) == 1 &&
                           strcmp(sortie, "2 3 4 * +") == 0,
                       "tp_postfixe doit produire la forme attendue");

    erreur[0] = '\0';
    sortie[0] = '\0';
    echecs += verifier(tp_postfixe(session, "2 + 3 * 4", sortie, 4,
                                   erreur, sizeof erreur) == 0 &&
                           strstr(erreur, "trop petit") != NULL,
                       "un tampon postfixe trop petit doit etre signale");

    tp_liberer(session);
    printf("Tests de l'API C : %d echec(s)\n", echecs);
    return echecs == 0 ? 0 : 1;
}
