#include "convertisseur.hpp"

#include "evaluation.hpp"
#include "pile_tableau.hpp"

#include <algorithm>
#include <sstream>

namespace expressions {

std::string ecrireJetons(const std::vector<Jeton>& jetons) {
    std::ostringstream flux;
    for (std::size_t i = 0; i < jetons.size(); ++i) {
        if (i > 0) {
            flux << ' ';
        }
        flux << jetons[i].texte;
    }
    return flux.str();
}

// ===========================================================================
// Niveau 1 : parcours de l'arbre d'expression
// ===========================================================================

namespace {

// Faut-il entourer cette sous-expression de parentheses pour qu'elle soit
// relue comme elle l'est ici ?
//
// La regle se formule en fonction du rang du parent et du rang de l'enfant :
//
//   rang(enfant) < rang(parent)  ->  parentheses obligatoires
//   rang(enfant) > rang(parent)  ->  parentheses inutiles
//   rangs egaux                  ->  depend du cote et du sens d'association
//
// Le troisieme cas merite d'etre explicite. Pour un operateur associatif a
// gauche comme "-" :
//   a - (b - c)  doit garder ses parentheses, car a - b - c se lirait (a-b)-c ;
//   (a - b) - c  n'en a pas besoin, car a - b - c se lit deja ainsi.
// Pour un operateur associatif a droite comme "^", c'est l'inverse :
//   2 ^ (3 ^ 2)  se lit 2 ^ 3 ^ 2, parentheses inutiles ;
//   (2 ^ 3) ^ 2  doit les garder, car 2 ^ 3 ^ 2 se lirait 2 ^ (3 ^ 2).
//
// C'est donc le rang de l'enfant qui commande, son sens d'association
// determining si l'egalite de rang est tolerable du cote considere.
bool parenthesesNecessaires(const NoeudExpression* noeud, int rangParent, bool estOperandeGauche) {
    if (noeud == nullptr || noeud->genre == Genre::Nombre) {
        return false;
    }

    if (noeud->genre == Genre::Unaire) {
        // Un signe unaire ne demande jamais de parentheses. L'ecriture
        // minimale de -2^2 est bien "-2 ^ 2", qui se relit comme -(2^2) :
        // la grammaire place l'operateur unaire en amont de la puissance.
        return false;
    }

    const int rangEnfant = precedenceDe(noeud->operateur);
    if (rangEnfant != rangParent) {
        return rangEnfant < rangParent;
    }

    const InfoOperateur* info = rechercherOperateur(noeud->operateur);
    return info->associatifGauche ? !estOperandeGauche : estOperandeGauche;
}

std::string infixeAvecParentheses(const NoeudExpression* noeud) {
    if (noeud == nullptr) {
        return {};
    }
    if (noeud->genre == Genre::Nombre) {
        return formaterNombre(noeud->valeur);
    }
    if (noeud->genre == Genre::Unaire) {
        return std::string(1, noeud->operateur) + infixeAvecParentheses(noeud->droite.get());
    }
    return "(" + infixeAvecParentheses(noeud->gauche.get()) + " " + std::string(1, noeud->operateur) + " " +
           infixeAvecParentheses(noeud->droite.get()) + ")";
}

std::string prefixeSansArbre(const NoeudExpression* noeud) {
    if (noeud == nullptr) {
        return {};
    }
    if (noeud->genre == Genre::Nombre) {
        return formaterNombre(noeud->valeur);
    }
    if (noeud->genre == Genre::Unaire) {
        return std::string(1, noeud->operateur) + " " + prefixeSansArbre(noeud->droite.get());
    }
    return std::string(1, noeud->operateur) + " " + prefixeSansArbre(noeud->gauche.get()) + " " +
           prefixeSansArbre(noeud->droite.get());
}

std::string postfixeSansArbre(const NoeudExpression* noeud) {
    if (noeud == nullptr) {
        return {};
    }
    if (noeud->genre == Genre::Nombre) {
        return formaterNombre(noeud->valeur);
    }
    if (noeud->genre == Genre::Unaire) {
        return postfixeSansArbre(noeud->droite.get()) + " " + std::string(1, noeud->operateur);
    }
    return postfixeSansArbre(noeud->gauche.get()) + " " + postfixeSansArbre(noeud->droite.get()) + " " +
           std::string(1, noeud->operateur);
}

} // namespace

std::string ecrireInfixe(const NoeudExpression* noeud, bool parenthesesMinimales) {
    if (noeud == nullptr) {
        return {};
    }
    if (!parenthesesMinimales) {
        return infixeAvecParentheses(noeud);
    }
    if (noeud->genre == Genre::Nombre) {
        return formaterNombre(noeud->valeur);
    }
    if (noeud->genre == Genre::Unaire) {
        return std::string(1, noeud->operateur) + ecrireInfixe(noeud->droite.get(), true);
    }

    const int rang = precedenceDe(noeud->operateur);
    const std::string gauche = ecrireInfixe(noeud->gauche.get(), true);
    const std::string droite = ecrireInfixe(noeud->droite.get(), true);

    return (parenthesesNecessaires(noeud->gauche.get(), rang, true) ? "(" + gauche + ")" : gauche) + " " +
           std::string(1, noeud->operateur) + " " +
           (parenthesesNecessaires(noeud->droite.get(), rang, false) ? "(" + droite + ")" : droite);
}

std::string ecrirePrefixe(const NoeudExpression* noeud) {
    return prefixeSansArbre(noeud);
}

std::string ecrirePostfixe(const NoeudExpression* noeud) {
    return postfixeSansArbre(noeud);
}

namespace {

// Meme parcours, mais en conservant l'arite exacte de chaque operateur dans le
// jeton. C'est la forme utilisee pour recoupement : l'analyseur sait si un "-"
// est unaire ou binaire, alors qu'une suite de jetons saisie en toutes
// lettres ne le dit pas.
void jetonsPostfixeDepuisArbre(const NoeudExpression* noeud, std::vector<Jeton>& sortie) {
    if (noeud == nullptr) {
        return;
    }
    if (noeud->genre == Genre::Nombre) {
        sortie.push_back({TypeJeton::Nombre, formaterNombre(noeud->valeur), noeud->valeur, 0});
        return;
    }
    if (noeud->genre == Genre::Unaire) {
        jetonsPostfixeDepuisArbre(noeud->droite.get(), sortie);
        sortie.push_back({TypeJeton::Operateur, std::string(1, noeud->operateur), 0.0, 0, Aritete::Unaire});
        return;
    }
    jetonsPostfixeDepuisArbre(noeud->gauche.get(), sortie);
    jetonsPostfixeDepuisArbre(noeud->droite.get(), sortie);
    sortie.push_back({TypeJeton::Operateur, std::string(1, noeud->operateur), 0.0, 0, Aritete::Binaire});
}

} // namespace

std::vector<Jeton> jetonsPostfixeDepuisArbre(const NoeudExpression* noeud) {
    std::vector<Jeton> sortie;
    jetonsPostfixeDepuisArbre(noeud, sortie);
    return sortie;
}

// ===========================================================================
// Niveau 2 : pile de sous-expressions
// ===========================================================================

namespace {

// ---------------------------------------------------------------------------
// Resolution de l'arite des operateurs d'une notation prefixee
// ---------------------------------------------------------------------------
//
// Une suite de jetons notation prefixee ne precise pas si un "+" ou un "-"
// est unaire ou binaire : "1 2 + -" se lit (1 + 2) - ou bien 1 - (2 +), et
// rien dans la suite de jetons ne permet de choisir. Aucun predicat portant
// sur un seul jeton ne peut trancher.
//
// On procede par un modele d'emplacements. Une expression prefixee se decoupe
// en niveaux : le niveau 0 est l'expression entiere et attend une seule
// chose, son operateur. Des que cet operateur est lu, il occupe
// l'emplacement du niveau courant et ouvre un nouveau niveau attendant
// autant d'operandes que son arite. Un nombre occupe un emplacement du
// niveau courant. Un niveau dont tous les emplacements sont pris se ferme, et
// son resultat occupe un emplacement du niveau parent.
//
// Ce modele ne demande qu'une comparaison par jeton, et il tranche
// l'ambiguite : on tente l'arite unaire, et des que le modele se retrouve
// sans emplacement disponible, on retente avec l'arite binaire. La recherche
// n'explore que les jetons "+" et "-", et un echec se propage vite, ce qui la
// rend lineaire sur les expressions usuelles. Un budget la borne malgre tout
// contre une entree volontairement pathologique.
constexpr std::size_t budgetRecherche = 100000;

// arites[i] vaut 0 pour un operateur unaire, 1 pour un operateur binaire.
struct ResolutionArites {
    const std::vector<Jeton>& jetons;
    std::vector<int>& arites;
    std::size_t budget = budgetRecherche;
    std::vector<Diagnostic>* diagnostics = nullptr;
};

// Verifie que le modele d'emplacements reste tenable jusqu'au bout et
// qu'il aboutit a une expression complete. C'est la fonction de decision :
// vrai signifie que les arites tentees jusqu'ici sont viables.
bool modeleTient(const std::vector<Jeton>& jetons, const std::vector<int>& arites) {
    // niveaux[k] = nombre d'emplacements encore libres au niveau k.
    std::vector<int> niveaux;
    niveaux.reserve(jetons.size() + 1);
    niveaux.push_back(1); // l'expression entiere attend son operateur

    auto fermerNiveauxComplets = [&]() {
        while (niveaux.size() > 1 && niveaux.back() == 0) {
            niveaux.pop_back();
        }
    };

    for (std::size_t i = 0; i < jetons.size(); ++i) {
        fermerNiveauxComplets();
        if (niveaux.back() == 0) {
            return false;
        }

        const Jeton& jeton = jetons[i];
        if (jeton.type != TypeJeton::Operateur) {
            --niveaux.back();
            continue;
        }

        --niveaux.back(); // l'operateur occupe un emplacement du niveau courant
        niveaux.push_back((arites[i] == 0) ? 1 : 2);
    }

    fermerNiveauxComplets();
    return niveaux.size() == 1 && niveaux[0] == 0;
}

// Parcours recursif : a chaque signe, on essaie l'arite unaire puis, si le
// modele ne tient plus, l'arite binaire.
bool resoudreArites(ResolutionArites& etat, std::size_t index) {
    if (index == etat.jetons.size()) {
        return modeleTient(etat.jetons, etat.arites);
    }
    if (etat.budget == 0) {
        return false;
    }
    --etat.budget;

    const Jeton& jeton = etat.jetons[index];

    if (jeton.type != TypeJeton::Operateur) {
        etat.arites[index] = 0;
        return resoudreArites(etat, index + 1);
    }

    if (!estOperateurUnaire(jeton.texte[0])) {
        etat.arites[index] = 1; // "*", "/", "%" et "^" sont toujours binaires
        return resoudreArites(etat, index + 1);
    }

    for (int tentative = 0; tentative < 2; ++tentative) {
        etat.arites[index] = tentative;
        if (resoudreArites(etat, index + 1)) {
            return true;
        }
    }
    return false;
}


} // namespace

bool prefixeVersPostfixe(const std::vector<Jeton>& jetons, std::vector<Jeton>& postfixe,
                         std::vector<Diagnostic>& diagnostics) {
    postfixe.clear();

    if (jetons.empty()) {
        diagnostics.push_back({0, "", "expression vide."});
        return false;
    }

    // Etape 1 : lever l'ambiguite d'arite des signes, par le modele
    // d'emplacements decrit plus haut.
    std::vector<int> arites(jetons.size(), 1);
    ResolutionArites resolution{jetons, arites, budgetRecherche, &diagnostics};

    if (!resoudreArites(resolution, 0)) {
        const bool signesSatures = std::any_of(arites.begin(), arites.end(), [](int a) { return a == 0; });
        if (!signesSatures && resolution.budget == 0) {
            diagnostics.push_back({0, "", "expression prefixee trop ambigue : depassement du budget de recherche."});
        } else {
            diagnostics.push_back(
                {jetons[0].position, jetons[0].texte,
                 "notation prefixee incorrecte : le nombre d'operandes ne correspond pas a celui des operateurs."});
        }
        return false;
    }

    // Etape 2 : une fois les arites connues, la conversion est un simple
    // parcours de droite a gauche. En prefixe, les operandes suivent
    // l'operateur qui les consomme : parcourant la suite a l'envers, on
    // rencontre donc d'abord les operandes, puis l'operateur qui les englobe.
    piles::PileTableau<std::vector<Jeton>> sousExpressions;

    for (std::size_t i = jetons.size(); i-- > 0;) {
        Jeton jeton = jetons[i];

        if (jeton.type != TypeJeton::Operateur) {
            sousExpressions.push({jeton});
            continue;
        }

        jeton.arite = (arites[i] == 0) ? Aritete::Unaire : Aritete::Binaire;
        const int nombreOperandes = (arites[i] == 0) ? 1 : 2;

        if (sousExpressions.size() < static_cast<std::size_t>(nombreOperandes)) {
            diagnostics.push_back({jeton.position, jeton.texte,
                                   "operande manquant pour l'operateur \"" + jeton.texte + "\"."});
            return false;
        }

        // L'operande de gauche est celle qui precede l'operateur ; c'est donc
        // elle que l'on a empilee en dernier, donc celle que l'on retire en
        // premier.
        std::vector<std::vector<Jeton>> operandes;
        for (int k = 0; k < nombreOperandes; ++k) {
            operandes.push_back(std::move(sousExpressions.top()));
            sousExpressions.pop();
        }

        std::vector<Jeton> combinaison;
        for (const std::vector<Jeton>& operande : operandes) {
            combinaison.insert(combinaison.end(), operande.begin(), operande.end());
        }
        combinaison.push_back(jeton);

        sousExpressions.push(std::move(combinaison));
    }

    postfixe = std::move(sousExpressions.top());
    return true;
}

bool postfixeVersPrefixe(const std::vector<Jeton>& jetons, std::vector<Jeton>& prefixe,
                         std::vector<Diagnostic>& diagnostics) {
    prefixe.clear();

    if (!validerPostfixe(jetons, diagnostics)) {
        return false;
    }

    piles::PileTableau<std::vector<Jeton>> sousExpressions;

    // Lecture de gauche a droite. En postfixe, les operandes precedent
    // l'operateur qui les consomme : un operande empile tel quel forme une
    // sous-expression complete, et un operateur englobe les une ou deux
    // sous-expressions sitting au sommet de la pile, qu'il remplace par une
    // sous-expression plus large. A la fin, la pile ne contient qu'un element :
    // l'expression entiere, ecrite en prefixe.
    for (const Jeton& jeton : jetons) {
        if (jeton.type != TypeJeton::Operateur) {
            sousExpressions.push({jeton});
            continue;
        }

        const int arite = (jeton.arite == Aritete::Unaire) ? 1 : 2;
        if (sousExpressions.size() < static_cast<std::size_t>(arite)) {
            diagnostics.push_back({jeton.position, jeton.texte,
                                   "operande manquant pour l'operateur \"" + jeton.texte + "\"."});
            return false;
        }

        // En postfixe, l'operande de droite a ete empilee en dernier : c'est
        // donc elle que l'on retire en premier.
        std::vector<std::vector<Jeton>> operandes;
        operandes.reserve(static_cast<std::size_t>(arite));
        for (int k = 0; k < arite; ++k) {
            operandes.push_back(std::move(sousExpressions.top()));
            sousExpressions.pop();
        }

        std::vector<Jeton> combinaison;
        combinaison.push_back(jeton);
        for (int k = arite - 1; k >= 0; --k) {
            combinaison.insert(combinaison.end(), operandes[static_cast<std::size_t>(k)].begin(),
                               operandes[static_cast<std::size_t>(k)].end());
        }

        sousExpressions.push(std::move(combinaison));
    }

    if (sousExpressions.size() != 1) {
        diagnostics.push_back({jetons.back().position, jetons.back().texte,
                               "notation postfixee incorrecte : plus d'une expression resultante."});
        return false;
    }

    prefixe = std::move(sousExpressions.top());
    return true;
}

bool postfixeVersPrefixe(const std::string& postfixe, std::string& prefixe, std::vector<Diagnostic>& diagnostics) {
    const std::vector<Jeton> jetons = tokeniser(postfixe, diagnostics);
    if (!diagnostics.empty()) {
        return false;
    }
    std::vector<Jeton> resultat;
    // Le postfixe saisi en toutes lettres ne porte pas l'arite de ses
    // operateurs : elle est deduite du nombre d'operandes disponibles.
    if (!postfixeVersPrefixe(avecAritesPostfixeInferes(jetons), resultat, diagnostics)) {
        return false;
    }
    prefixe = ecrireJetons(resultat);
    return true;
}

bool prefixeVersPostfixe(const std::string& prefixe, std::string& postfixe, std::vector<Diagnostic>& diagnostics) {
    const std::vector<Jeton> jetons = tokeniser(prefixe, diagnostics);
    if (!diagnostics.empty()) {
        return false;
    }
    std::vector<Jeton> resultat;
    if (!prefixeVersPostfixe(jetons, resultat, diagnostics)) {
        return false;
    }
    postfixe = ecrireJetons(resultat);
    return true;
}

// ===========================================================================
// Chaine complete, avec recoupement des deux niveaux
// ===========================================================================

bool convertirDepuisInfixe(const std::string& infixe, Conversion& conversion) {
    conversion = Conversion{};
    conversion.infixe = infixe;

    PtrNoeud racine = analyser(infixe, conversion.diagnostics);
    if (racine == nullptr) {
        return false;
    }

    conversion.prefixe = ecrirePrefixe(racine.get());
    conversion.postfixe = ecrirePostfixe(racine.get());

    // Recoupement 1 : la conversion postfixe vers prefixe, faite a partir de
    // la seule notation et a l'aide d'une pile, doit restituer exactement la
    // notation issue de l'arbre. Une divergence signalerait que l'ecriture par
    // arbre a perdu de l'information, ou que l'algorithme sur pile est fautif.
    // Le sens postfixe vers prefixe a ete choisi parce qu'il ne souffre pas de
    // l'ambiguite d'arite propre a la notation prefixee (cf. essayerConversionPrefixe).
    std::vector<Diagnostic> diagnostics;

    const std::vector<Jeton> jetonsPostfixe = jetonsPostfixeDepuisArbre(racine.get());

    std::vector<Jeton> jetonsPrefixe;
    if (!postfixeVersPrefixe(jetonsPostfixe, jetonsPrefixe, diagnostics)) {
        conversion.diagnostics.insert(conversion.diagnostics.end(), diagnostics.begin(), diagnostics.end());
        return false;
    }
    if (ecrireJetons(jetonsPrefixe) != conversion.prefixe) {
        conversion.diagnostics.push_back({0, "",
                                           "postfixe vers prefixe : \"" + ecrireJetons(jetonsPrefixe) +
                                               "\" au lieu de \"" + conversion.prefixe + "\"."});
        return false;
    }

    // Recoupement 2 : la notation postfixee obtenue doit fournir la meme valeur
    // que l'evaluation de l'arbre.
    double valeurParPile = 0.0;
    if (!evaluerPostfixe(jetonsPostfixe, valeurParPile, diagnostics)) {
        conversion.diagnostics.insert(conversion.diagnostics.end(), diagnostics.begin(), diagnostics.end());
        return false;
    }

    double valeurParArbre = 0.0;
    std::vector<Diagnostic> diagnosticsArbre;
    if (!evaluerArbre(racine.get(), valeurParArbre, diagnosticsArbre)) {
        conversion.diagnostics.insert(conversion.diagnostics.end(), diagnosticsArbre.begin(), diagnosticsArbre.end());
        return false;
    }
    if (valeurParPile != valeurParArbre) {
        conversion.diagnostics.push_back({0, "",
                                           "divergence d'evaluation : " + std::to_string(valeurParPile) + " par pile, " +
                                               std::to_string(valeurParArbre) + " par arbre."});
        return false;
    }

    // Aller-retour sur l'infixe : reanalyser l'ecriture doit redonner une
    // expression de meme valeur. La comparaison porte sur la valeur et non sur
    // la structure, car l'ecriture minimale supprime les parentheses
    // superflues : "(2 + 3) * 4" et "2 + 3 * 4" ont des arbres differents et la
    // meme valeur, ce qui est tout ce que l'on demande a un rendu lisible.
    const std::string canonique = ecrireInfixe(racine.get(), true);
    PtrNoeud reconstitution = analyser(canonique, diagnostics);
    if (reconstitution == nullptr) {
        conversion.diagnostics.push_back({0, "", "l'ecriture \"" + canonique + "\" n'est pas relisible."});
        return false;
    }

    double valeurRecrite = 0.0;
    std::vector<Diagnostic> diagnosticsRecriture;
    if (!evaluerArbre(reconstitution.get(), valeurRecrite, diagnosticsRecriture) || valeurRecrite != valeurParPile) {
        conversion.diagnostics.push_back({0, "",
                                           "l'ecriture \"" + canonique + "\" ne redonne pas la valeur d'origine (" +
                                               std::to_string(valeurRecrite) + " au lieu de " +
                                               std::to_string(valeurParPile) + ")."});
        return false;
    }

    conversion.valide = true;
    return true;
}

} // namespace expressions
