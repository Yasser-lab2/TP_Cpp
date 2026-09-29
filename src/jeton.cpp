#include "jeton.hpp"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace expressions {

namespace {

const std::vector<InfoOperateur> tableOperateurs = {
    {'+', 0, true, "addition"},
    {'-', 0, true, "soustraction"},
    {'*', 1, true, "multiplication"},
    {'/', 1, true, "division"},
    {'%', 1, true, "reste"},
    {'^', 3, false, "puissance"},
};

// Priorite du signe unaire : il doit lier plus fort que * et /, et moins fort
// que ^, afin que -2*3 = (-2)*3 et que -2^2 = -(2^2).
constexpr int precedenceOperateurUnaire = 2;

} // namespace

const std::vector<InfoOperateur>& operateurs() {
    return tableOperateurs;
}

const InfoOperateur* rechercherOperateur(char symbole) {
    for (const InfoOperateur& info : tableOperateurs) {
        if (info.symbole == symbole) {
            return &info;
        }
    }
    return nullptr;
}

int precedenceDe(char symbole) {
    // Rang BINAIRE du symbole, tel qu'il figure dans la table des operateurs.
    // L'ambiguite +/- ne concerne pas cette fonction : elle sert uniquement la
    // reecriture d'un arbre, ou le genre du noeud (Unaire ou Binaire) est deja
    // connu et a ete traite separement. Pour un jeton dont l'arite n'est pas
    // etablie, c'est precedenceDuJeton qu'il faut appeler.
    const InfoOperateur* info = rechercherOperateur(symbole);
    return info == nullptr ? -1 : info->precedence;
}

int precedenceDuJeton(const Jeton& jeton) {
    if (jeton.type != TypeJeton::Operateur || jeton.texte.empty()) {
        return -1;
    }
    if (jeton.arite == Aritete::Unaire) {
        return precedenceOperateurUnaire;
    }
    const InfoOperateur* info = rechercherOperateur(jeton.texte[0]);
    return info == nullptr ? -1 : info->precedence;
}

bool estOperateur(char symbole) {
    return rechercherOperateur(symbole) != nullptr;
}

bool estOperateurUnaire(char symbole) {
    return symbole == '+' || symbole == '-';
}

bool estChiffre(char c) {
    return c >= '0' && c <= '9';
}

bool estEspace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

std::string formaterNombre(double valeur) {
    if (!std::isfinite(valeur)) {
        return std::isnan(valeur) ? "NaN" : (valeur > 0.0 ? "inf" : "-inf");
    }

    if (valeur == std::floor(valeur) && std::abs(valeur) < 1e15) {
        return std::to_string(static_cast<long long>(valeur));
    }

    // On retient la plus petite precision decimale qui restitue exactement le
    // meme double : 0.1 rend "0.1" et non "0.10000000000000001".
    for (int precision = 1; precision <= 17; ++precision) {
        std::ostringstream flux;
        flux << std::setprecision(precision) << valeur;
        if (std::strtod(flux.str().c_str(), nullptr) == valeur) {
            return flux.str();
        }
    }

    std::ostringstream flux;
    flux << std::setprecision(17) << valeur;
    return flux.str();
}


// Charge l'arite des operateurs d'une suite de jetons notation postfixee.
//
// Le postfixe, comme le prefixe, ne dit pas explicitement si un "+" ou un "-"
// est unaire ou binaire. On tranche ici par le nombre d'operandes disponibles
// au moment ou l'operateur est rencontre : un operateur binaire en consomme
// deux, un operateur unaire un seul.
//
// Ce n'est qu'une heuristique, mais c'est la meilleure possible, car la
// notation postfixee est reellement ambigue sur ce point : "2 3 - +" se lit
// 2 - 3 + (soit -1) ou bien 2 + (-3) (soit 5), et rien dans la suite de
// jetons ne permet de choisir. En usage interne, l'analyseur connait l'arite
// exacte et la fournit dans le jeton ; cette fonction ne sert que pour du
// postfixe saisi directement en toutes lettres.
std::vector<Jeton> avecAritesPostfixeInferes(const std::vector<Jeton>& jetons) {
    std::vector<Jeton> resultat = jetons;

    // Seule compte le nombre d'operandes disponibles, pas leur valeur : un
    // simple compteur suffit, la pile n'intervient qu'a l'evaluation.
    std::size_t operandes = 0;

    for (Jeton& jeton : resultat) {
        if (jeton.type != TypeJeton::Operateur) {
            ++operandes;
            continue;
        }

        // L'arite n'est incertaine que pour "+" et "-" : "*", "/", "%" et "^"
        // sont toujours binaires. Leur conserver une arite eventuellement
        // erronee ferait passer des suites manifestement mal formees, comme
        // "2 3 * *".
        if (estOperateurUnaire(jeton.texte[0])) {
            jeton.arite = (operandes >= 2) ? Aritete::Binaire : Aritete::Unaire;
        } else {
            jeton.arite = Aritete::Binaire;
        }

        const int arite = (jeton.arite == Aritete::Unaire) ? 1 : 2;
        operandes = (operandes >= static_cast<std::size_t>(arite)) ? operandes - arite + 1 : 0;
    }

    return resultat;
}

std::string Diagnostic::formater() const {
    std::string texte = "colonne " + std::to_string(position + 1);
    if (!jeton.empty()) {
        texte += " (jeton \"" + jeton + "\")";
    }
    return texte + " : " + message;
}

} // namespace expressions
