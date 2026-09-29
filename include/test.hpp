#ifndef TEST_HPP
#define TEST_HPP

// Micro-cadre de tests, sans dependance externe.
//
// L'objectif est de pouvoir afficher un compte rendu lisible dans la console
// et de renvoyer un code de sortie different de zero au systeme, afin que
// "make test" echoue si une regression est introduite.

#include <cmath>
#include <iostream>
#include <string>

namespace tests {

struct Compteur {
    int reussis = 0;
    int echecs = 0;
    int sectionCourante = 0;
    std::string section;
};

inline Compteur& compteur() {
    static Compteur instance;
    return instance;
}

inline void section(const std::string& titre) {
    compteur().section = titre;
    std::cout << "\n== " << titre << " ==\n";
}

inline void succes(const std::string& libelle) {
    ++compteur().reussis;
    std::cout << "  [ok]   " << libelle << '\n';
}

inline void echec(const std::string& libelle, const std::string& detail) {
    ++compteur().echecs;
    std::cout << "  [ECHEC] " << libelle << '\n';
    std::cout << "          " << detail << '\n';
}

inline void verifier(bool condition, const std::string& libelle, const std::string& detail = "") {
    if (condition) {
        succes(libelle);
    } else {
        echec(libelle, detail);
    }
}

inline void verifierEgaux(double attendu, double obtenu, const std::string& libelle, double tolerance = 1e-9) {
    if (std::abs(attendu - obtenu) <= tolerance) {
        succes(libelle);
    } else {
        echec(libelle, "attendu " + std::to_string(attendu) + ", obtenu " + std::to_string(obtenu));
    }
}

inline void verifierEgaux(const std::string& attendu, const std::string& obtenu, const std::string& libelle) {
    if (attendu == obtenu) {
        succes(libelle);
    } else {
        echec(libelle, "attendu \"" + attendu + "\", obtenu \"" + obtenu + "\"");
    }
}

inline int bilan(const std::string& nomSuite) {
    const Compteur& c = compteur();
    std::cout << "\n---------------------------------------------\n";
    std::cout << nomSuite << " : " << c.reussis << " reussis, " << c.echecs << " echecs\n";
    return c.echecs == 0 ? 0 : 1;
}

} // namespace tests

#endif // TEST_HPP
