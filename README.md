# Piles génériques et expressions arithmétiques

Projet C++17 en deux parties :

1. **Trois implémentations d'une pile générique** — un tableau dynamique, une
   liste chaînée, et un adaptateur de référence sur `std::vector` — avec
   gestion correcte de la durée de vie des éléments.
2. **Analyse et évaluation d'expressions arithmétiques** — analyse lexicale,
   deux analyseurs indépendants, évaluation par notation postfixée, et
   conversions entre notations infixe, préfixée et postfixée.

L'outillage devient aussi un objet distribuable : une bibliothèque statique
et une bibliothèque partagée (`lib/libtp.a` et `lib/libtp.so.1`), exposant
une interface C consumable par un client externe sans source du moteur
(`include/tp_api.h`, rapport complémentaire `rapport/rapport_dll.pdf`).

## Construction

```sh
make            # tout compile dans bin/
make check      # exécute les deux suites de tests
make run        # démonstrations et banc d'essai
make rapport    # compile rapport/rapport.pdf
make tout       # les trois ci-dessus
```

`make` ne produit rien d'utile tout seul : les cibles utiles sont `check`,
`run` et `rapport`.

## Bibliothèque partagée

```sh
make bibliotheque   # construit lib/libtp.a et lib/libtp.so.1
make lib-tests      # binaires de comparaison statistique/dynamique
make lib-client     # client externe (interface C seule)
make lib-check      # clients C/C++ + tests statique/dynamique + symboles
make dll-windows    # DLL Windows MinGW-w64 + import library
make windows-client # client Windows et copie de la DLL à côté de l'exe
make rapport-dll    # compile rapport/rapport_dll.pdf
make tout-dll       # le tout, y compris le rapport complémentaire
```

Le module est compilé en `-fPIC -fvisibility=hidden` ; seuls les symboles
marqués `API_EXPORT` (ou fixés par une instanciation explicite des piles)
en sortent. Le programme `bin/client_dll` ne connaît que `include/tp_api.h`
et le module ; le `rpath` relatif (`$ORIGIN`), gravé dans le binaire, lui
permet de tourner sans `LD_LIBRARY_PATH`. Sous Windows, le code est portable
(`API_EXPORT` prend la forme `__declspec(dllexport/dllimport)`) mais la DLL
doit être construite sur une machine Windows.

## Organisation

```
include/
  pile_tableau.hpp   pile sur tableau dynamique, mémoire brute
  pile_liste.hpp     pile sur liste chaînée
  pile_stl.hpp       adaptateur de référence sur std::vector
  jeton.hpp/.cpp     jetons, priorités, diagnostics, arité postfixée
  analyseur.hpp/.cpp descente récursive et méthode de la dichotomie
  evaluation.hpp/.cpp validation et évaluation postfixée
  convertisseur.hpp/.cpp conversions entre notations
  api.hpp            macro API_EXPORT (visibilité/dllexport)
  tp_api.h           interface C publique de la bibliothèque
  test.hpp           micro-cadre de tests, sans dépendance
src/
  demo_piles.cpp        démonstration des trois piles
  demo_expressions.cpp  démonstration des expressions
  benchmark_piles.cpp   banc d'essai comparatif
  tests_piles.cpp       62 tests
  tests_expressions.cpp 55 tests
  tests_api.c             client de test C de l'interface publique
  lib_api.cpp             instanciations explicites des piles + frontière C
  client_dll.cpp          client externe de démonstration
lib/                    libtp.a et libtp.so.1 générés
rapport/                sources LaTeX et PDF (principal + complément)
archive/                anciennes versions, conservées pour référence
```

## Les trois piles

Elles exposent la même interface : `push`, `pop`, `top`, `empty`, `size`,
`clear`, `capacity`, `reserve` et `reduireCapacite`, plus le constructeur de
copie, l'affectation et le déplacement.

`PileTableau` n'utilise jamais `new T[n]`, qui construit les objets avant de
les déplacer. Elle alloue de la mémoire brute par `::operator new`, puis construit
et détruit chaque élément explicitement. Elle accepte donc des types sans
constructeur par défaut, et une exception pendant un redimensionnement ne
provoque ni fuite ni destruction double.

`PileListe` alloue un nœud par élément. Les deux implémentations sont
entièrement interchangeables, ce que les tests vérifient en leur appliquant
exactement la même suite d'opérations.

## L'ambiguïté de l'arité

Un `+` ou un `-` en notation préfixée ou postfixée ne dit pas s'il est unaire
ou binaire, et l'information est perdue au moment de la conversion. Aucune
suite de jetons ne permet de trancher.

Le projet applique donc deux conventions explicites :

- **postfixée** : un opérateur consomme deux opérandes si deux sont
  disponibles, un sinon ;
- **préfixée** : l'arité unaire est essayée en premier, avec retour arrière sur
  l'ensemble des jetons.

Ce sont des choix, pas des déductions, et le rapport les présente comme tels.

## Tests

```sh
make check
```

62 tests pour les piles, 55 pour les expressions, et un test C du contrat DLL. Les deux binaires
renvoient un code de sortie non nul en cas d'échec, ce qui permet de les
enchaîner. La compilation avec `-fsanitize=address,undefined` ne signale
aucune erreur sur l'ensemble des deux suites.

## Banc d'essai

```sh
make run
```

Sur un million d'éléments, cinq répétitions, meilleur temps retenu. Le
protocole et les résultats sont discutés en section 10 du rapport.
