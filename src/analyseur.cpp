#include "analyseur.hpp"

#include "pile_tableau.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace expressions {

// ===========================================================================
// Analyse lexicale
// ===========================================================================

std::vector<Jeton> tokeniser(const std::string& source, std::vector<Diagnostic>& diagnostics) {
    std::vector<Jeton> jetons;
    std::size_t i = 0;

    while (i < source.size()) {
        const char c = source[i];

        if (estEspace(c)) {
            ++i;
            continue;
        }

        if (estChiffre(c) || (c == '.' && i + 1 < source.size() && estChiffre(source[i + 1]))) {
            const std::size_t debut = i;
            bool pointVu = false;

            while (i < source.size() && (estChiffre(source[i]) || source[i] == '.')) {
                if (source[i] == '.') {
                    if (pointVu) {
                        diagnostics.push_back({debut, source.substr(debut, i - debut + 1),
                                               "nombre mal forme : deux separateurs decimaux."});
                        break;
                    }
                    pointVu = true;
                }
                ++i;
            }

            const std::string texte = source.substr(debut, i - debut);
            jetons.push_back({TypeJeton::Nombre, texte, std::strtod(texte.c_str(), nullptr), debut});
            continue;
        }

        if (c == '(') {
            jetons.push_back({TypeJeton::ParentheseOuvrante, "(", 0.0, i});
            ++i;
            continue;
        }

        if (c == ')') {
            jetons.push_back({TypeJeton::ParentheseFermante, ")", 0.0, i});
            ++i;
            continue;
        }

        if (estOperateur(c)) {
            jetons.push_back({TypeJeton::Operateur, std::string(1, c), 0.0, i});
            ++i;
            continue;
        }

        // Caractere non reconnu : on le signale et on passe au suivant, afin
        // que l'utilisateur obtienne un diagnostic par caractere fautif.
        const std::size_t debut = i;
        std::string lexeme(1, c);
        ++i;
        while (i < source.size() && !estEspace(source[i]) && !estOperateur(source[i]) &&
               source[i] != '(' && source[i] != ')' && !estChiffre(source[i]) && source[i] != '.') {
            lexeme.push_back(source[i]);
            ++i;
        }
        diagnostics.push_back(
            {debut, lexeme, "caractere(s) non reconnu(s) dans l'expression."});
    }

    return jetons;
}

PtrNoeud creerNombre(double valeur) {
    auto noeud = std::make_unique<NoeudExpression>();
    noeud->genre = Genre::Nombre;
    noeud->valeur = valeur;
    return noeud;
}

// ===========================================================================
// Analyse syntaxique par descente recursive
// ===========================================================================
//
// La grammaire retenue est la suivante. Elle encode la precedence et
// l'associativite, ce qui evite d'ecrire une table de priorites et rend les
// diagnostics naturally explicites.
//
//   Expression  ::= Terme { ('+' | '-') Terme }
//   Terme       ::= Unaire { ('*' | '/' | '%') Unaire }
//   Unaire      ::= ('+' | '-') Unaire | Puissance
//   Puissance   ::= Atom ['^' Unaire]        // associativite a droite
//   Atom        ::= Nombre | '(' Expression ')'
//
// Cette grammaire donne -2^2 = -(2^2) = -4, 2^-3 = 1/8, 2^3^2 = 2^9 = 512 et
// -2*3 = (-2)*3, conformement aux conventions mathematicians usuelles.

namespace {

class AnalyseurDescendantRecursif {
public:
    AnalyseurDescendantRecursif(const std::vector<Jeton>& jetons, std::vector<Diagnostic>& diagnostics)
        : jetons_(jetons), diagnostics_(diagnostics) {}

    PtrNoeud analyser() {
        if (jetons_.empty()) {
            echec(0, "", "expression vide.");
            return nullptr;
        }

        PtrNoeud racine = expression();
        if (!racine) {
            return nullptr;
        }

        if (position_ < jetons_.size()) {
            const Jeton& enTrop = jetons_[position_];
            echec(enTrop.position, enTrop.texte,
                  enTrop.type == TypeJeton::ParentheseFermante
                      ? "parenthese fermante ) en trop."
                      : "jeton inattendu apres une expression complete.");
            return nullptr;
        }

        return racine;
    }

private:
    const std::vector<Jeton>& jetons_;
    std::vector<Diagnostic>& diagnostics_;
    std::size_t position_ = 0;

    const Jeton* courant() const {
        return position_ < jetons_.size() ? &jetons_[position_] : nullptr;
    }

    bool echec(std::size_t position, const std::string& jeton, const std::string& message) {
        diagnostics_.push_back({position, jeton, message});
        return false;
    }

    // attendUnOperande(position) : gere les messages d'erreur quand un
    // operande est attendu. Le message est different selon que l'on a fini
    // par "(" ou non, ce qui oriente l'utilisateur.
    PtrNoeud atom() {
        const Jeton* jeton = courant();
        if (jeton == nullptr) {
            echec(dernierePosition(), "", "operande manquant : l'expression se termine prematurement.");
            return nullptr;
        }

        if (jeton->type == TypeJeton::Nombre) {
            PtrNoeud feuille = creerNombre(jeton->valeur);
            ++position_;
            return feuille;
        }

        if (jeton->type == TypeJeton::ParentheseOuvrante) {
            const std::size_t positionParenthese = jeton->position;
            ++position_;
            PtrNoeud interne = expression();
            if (!interne) {
                return nullptr;
            }
            const Jeton* fermeture = courant();
            if (fermeture == nullptr || fermeture->type != TypeJeton::ParentheseFermante) {
                echec(positionParenthese, "(",
                      "parenthese ouvrante ( jamais fermee : nombre de ( et de ) non equilibre.");
                return nullptr;
            }
            ++position_;
            return interne;
        }

        if (jeton->type == TypeJeton::Operateur) {
            echec(jeton->position, jeton->texte,
                  "operande manquant avant l'operateur \"" + jeton->texte + "\".");
            return nullptr;
        }

        echec(jeton->position, jeton->texte, "operande manquant.");
        return nullptr;
    }

    PtrNoeud puissance() {
        PtrNoeud base = atom();
        if (!base) {
            return nullptr;
        }

        const Jeton* jeton = courant();
        if (jeton != nullptr && jeton->type == TypeJeton::Operateur && jeton->texte == "^") {
            ++position_;
            // L'exposant est un Unaire et non une Puissance : c'est ce qui
            // rend 2^3^2 associatif a droite et autorise 2^-3.
            PtrNoeud exposant = unaire();
            if (!exposant) {
                return nullptr;
            }
            return creerBinaire('^', std::move(base), std::move(exposant));
        }

        return base;
    }

    PtrNoeud unaire() {
        const Jeton* jeton = courant();
        if (jeton != nullptr && jeton->type == TypeJeton::Operateur &&
            estOperateurUnaire(jeton->texte[0])) {
            const std::size_t positionSigne = jeton->position;
            const char symbole = jeton->texte[0];
            ++position_;
            PtrNoeud operande = unaire();
            if (!operande) {
                return nullptr;
            }
            if (symbole == '+') {
                return operande;
            }
            return creerUnaire('-', std::move(operande), positionSigne);
        }
        return puissance();
    }

    PtrNoeud terme() {
        PtrNoeud gauche = unaire();
        if (!gauche) {
            return nullptr;
        }

        while (const Jeton* jeton = courant()) {
            if (jeton->type != TypeJeton::Operateur) {
                break;
            }
            const char symbole = jeton->texte[0];
            if (symbole != '*' && symbole != '/' && symbole != '%') {
                break;
            }
            ++position_;
            PtrNoeud droite = unaire();
            if (!droite) {
                return nullptr;
            }
            gauche = creerBinaire(symbole, std::move(gauche), std::move(droite));
        }

        return gauche;
    }

    PtrNoeud expression() {
        PtrNoeud gauche = terme();
        if (!gauche) {
            return nullptr;
        }

        while (const Jeton* jeton = courant()) {
            if (jeton->type != TypeJeton::Operateur) {
                break;
            }
            const char symbole = jeton->texte[0];
            if (symbole != '+' && symbole != '-') {
                break;
            }
            ++position_;
            PtrNoeud droite = terme();
            if (!droite) {
                return nullptr;
            }
            gauche = creerBinaire(symbole, std::move(gauche), std::move(droite));
        }

        return gauche;
    }

    std::size_t dernierePosition() const {
        return jetons_.empty() ? 0 : jetons_.back().position;
    }

    static PtrNoeud creerBinaire(char symbole, PtrNoeud gauche, PtrNoeud droite) {
        auto noeud = std::make_unique<NoeudExpression>();
        noeud->genre = Genre::Binaire;
        noeud->operateur = symbole;
        noeud->gauche = std::move(gauche);
        noeud->droite = std::move(droite);
        return noeud;
    }

    static PtrNoeud creerUnaire(char symbole, PtrNoeud operande, std::size_t) {
        auto noeud = std::make_unique<NoeudExpression>();
        noeud->genre = Genre::Unaire;
        noeud->operateur = symbole;
        noeud->droite = std::move(operande);
        return noeud;
    }
};

} // namespace

PtrNoeud analyserJetons(const std::vector<Jeton>& jetons, std::vector<Diagnostic>& diagnostics) {
    AnalyseurDescendantRecursif analyseur(jetons, diagnostics);
    return analyseur.analyser();
}

PtrNoeud analyser(const std::string& source, std::vector<Diagnostic>& diagnostics) {
    const std::vector<Jeton> jetons = tokeniser(source, diagnostics);
    if (!diagnostics.empty()) {
        return nullptr;
    }
    return analyserJetons(jetons, diagnostics);
}

// ===========================================================================
// Methode de la dichotomie (shunting-yard)
// ===========================================================================
//
// Les jetons sont parcours de gauche a droite. Deux piles sont utilisees :
// la pile des operateurs et la pile de sortie, qui accumule les jetons du
// resultat.
//
// Regle de depilement a l'arrivee d'un operateur binaire `o` :
//
//   - operateur binaire : depiler tant que la precedence du sommet est
//     superieure a celle de `o`, ou egale lorsque les deux sont associatifs
//     a gauche. C'est cette regle unique qui produit 3+4*5 = 23 et non 35,
//     ainsi que 7-3-2 = 2 et non 6 ;
//
//   - operateur unaire  : empiler directement, sans rien depiler.
//
// Le second point merite justification, car c'est le point delicat de
// l'algorithme. Un operateur unaire (le signe moins) doit pouvoir attendre
// son operande sans depiler un "^" qui n'a encore qu'un seul operande : c'est
// exactement le cas de 2^-3, ou un depilement declenche par l'operateur
// unaire detruirait le "^" alors que son operande de droite n'est pas encore
// lu. Comme le signe unaire est plus prioritaire que *, / et %, il sera de
// lui-meme depile par l'operateur binaire de ces familles lorsqu'il se
// presentera : le cas -2*3 se resout alors correctement en (-2)*3.

namespace {

// Depose dans la pile de sortie le resultat de l'operateur sommet de la pile
// des operateurs. Le nombre d'operandes a recuperer est lu dans le jeton
// lui-meme : l'analyseur a tranche l'arite a la lecture, ce qui evite toute
// nouvelle interrogation du contexte ici.
bool appliquerOperateur(piles::PileTableau<Jeton>& operateurs,
                        piles::PileTableau<Jeton>& sortie,
                        std::vector<Diagnostic>& diagnostics) {
    const Jeton operateur = operateurs.top();
    operateurs.pop();

    std::vector<Jeton> operandes;

    if (operateur.arite == Aritete::Unaire) {
        if (sortie.empty()) {
            diagnostics.push_back({operateur.position, operateur.texte,
                                   "operande manquant pour l'operateur unaire \"" + operateur.texte + "\"."});
            return false;
        }
        operandes.push_back(sortie.top());
        sortie.pop();
    } else {
        if (sortie.size() < 2) {
            diagnostics.push_back({operateur.position, operateur.texte,
                                   "operande manquant pour l'operateur \"" + operateur.texte + "\"."});
            return false;
        }
        // Les operandes sont depiles dans l'ordre droite puis gauche. On les
        // remet en pile dans l'ordre inverse, gauche puis droite, car c'est
        // l'ordre d'apparition en notation postfixee.
        operandes.push_back(sortie.top());
        sortie.pop();
        operandes.push_back(sortie.top());
        sortie.pop();
        std::swap(operandes[0], operandes[1]);
    }

    for (const Jeton& operande : operandes) {
        sortie.push(operande);
    }
    sortie.push(operateur);
    return true;
}

} // namespace

bool jetonsVersPostfixeParDichotomie(const std::vector<Jeton>& jetons,
                                     std::vector<Jeton>& postfixe,
                                     std::vector<Diagnostic>& diagnostics) {
    postfixe.clear();

    if (jetons.empty()) {
        diagnostics.push_back({0, "", "expression vide."});
        return false;
    }

    piles::PileTableau<Jeton> operateurs;
    piles::PileTableau<Jeton> sortie;

    // Vrai si le prochain jeton doit etre un operande, c'est-a-dire un nombre
    // ou une parenthese ouvrante. Ce seul indicateur permet de distinguer le
    // "-" unaire de "-3" du "-" binaire de "7-3", de reperer deux operandes
    // juxtaposes, et de signaler un operande manquant avant ")".
    bool attendOperande = true;

    for (const Jeton& jeton : jetons) {
        switch (jeton.type) {
            case TypeJeton::Nombre: {
                if (!attendOperande) {
                    diagnostics.push_back({jeton.position, jeton.texte,
                                           "deux operandes juxtaposes : l'operateur entre les deux est manquant."});
                    return false;
                }
                sortie.push(jeton);
                attendOperande = false;
                break;
            }

            case TypeJeton::ParentheseOuvrante: {
                if (!attendOperande) {
                    diagnostics.push_back({jeton.position, jeton.texte,
                                           "deux operandes juxtaposes : l'operateur entre les deux est manquant."});
                    return false;
                }
                operateurs.push(jeton);
                attendOperande = true;
                break;
            }

            case TypeJeton::ParentheseFermante: {
                if (attendOperande) {
                    diagnostics.push_back({jeton.position, jeton.texte,
                                           "operande manquant avant la parenthese fermante."});
                    return false;
                }

                bool ouvertureTrouvee = false;
                while (!operateurs.empty()) {
                    if (operateurs.top().type == TypeJeton::ParentheseOuvrante) {
                        ouvertureTrouvee = true;
                        break;
                    }
                    if (!appliquerOperateur(operateurs, sortie, diagnostics)) {
                        return false;
                    }
                }

                if (!ouvertureTrouvee) {
                    diagnostics.push_back({jeton.position, ")",
                                           "parenthese fermante sans ouverture correspondante."});
                    return false;
                }

                operateurs.pop();
                attendOperande = false;
                break;
            }

            case TypeJeton::Operateur: {
                const char symbole = jeton.texte[0];

                if (estOperateurUnaire(symbole) && attendOperande) {
                    Jeton unaire = jeton;
                    unaire.arite = Aritete::Unaire;
                    operateurs.push(unaire); // empile sans rien depiler
                    attendOperande = true;     // le signe attend son operande
                    break;
                }

                Jeton binaire = jeton;
                binaire.arite = Aritete::Binaire;

                const InfoOperateur* info = rechercherOperateur(symbole);
                if (info == nullptr) {
                    diagnostics.push_back({jeton.position, jeton.texte, "operateur inconnu."});
                    return false;
                }

                while (!operateurs.empty() && operateurs.top().type == TypeJeton::Operateur) {
                    const int precedenceSommet = precedenceDuJeton(operateurs.top());
                    const bool aDepiler = precedenceSommet > info->precedence ||
                                          (precedenceSommet == info->precedence && info->associatifGauche);
                    if (!aDepiler) {
                        break;
                    }
                    if (!appliquerOperateur(operateurs, sortie, diagnostics)) {
                        return false;
                    }
                }

                operateurs.push(binaire);
                attendOperande = true;
                break;
            }

            case TypeJeton::Fin:
                break;
        }
    }

    while (!operateurs.empty()) {
        if (operateurs.top().type == TypeJeton::ParentheseOuvrante) {
            diagnostics.push_back({operateurs.top().position, "(",
                                   "parenthese ouvrante jamais fermee : le nombre de ( et de ) n'est pas equilibre."});
            return false;
        }
        if (!appliquerOperateur(operateurs, sortie, diagnostics)) {
            return false;
        }
    }

    // La pile de sortie contient la notation postfixee, du premier au dernier
    // jeton. Elle se lit donc de bas en haut.
    postfixe.resize(sortie.size());
    for (std::size_t i = sortie.size(); i-- > 0;) {
        postfixe[i] = sortie.top();
        sortie.pop();
    }
    return true;
}

bool infixeVersPostfixeParDichotomie(const std::string& source,
                                     std::vector<Jeton>& postfixe,
                                     std::vector<Diagnostic>& diagnostics) {
    const std::vector<Jeton> jetons = tokeniser(source, diagnostics);
    if (!diagnostics.empty()) {
        return false;
    }
    return jetonsVersPostfixeParDichotomie(jetons, postfixe, diagnostics);
}

} // namespace expressions
