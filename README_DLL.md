# Utilisation du projet sous forme de DLL Windows

Ce document rassemble tout ce qu'il faut savoir pour transformer l'outillage
du projet (`libtp` : le moteur d'expressions + les piles) en une vraie **DLL
Windows**, et pour qu'un client externe l'utilise. Tout est donné **étape par
étape**, pour deux chaînes de compilation : **MinGW-w64** (g++) et **MSVC**
(cl).

Le code est portable : la même macro `API_EXPORT` produit `dllexport`
sous Windows et la visibilité GNU ailleurs. La frontière C expose explicitement
la vérification (`tp_verifier`), l'évaluation et la conversion ; le client ne
recompile pas le moteur.

---

## 1. Ce que le projet livre déjà

| Ce que le client reçoit | Fichier |
| --- | --- |
| L'interface C : `tp_creer`, `tp_liberer`, `tp_verifier`, `tp_evaluer`, `tp_postfixe` | `include/tp_api.h` |
| La macro d'export (`dllexport` / `dllimport` automatiques) | `include/api.hpp` |
| La bibliothèque à produire | `lib/libtp.dll` |
| Le programme de test | `src/client_dll.cpp` |

Le client n'a besoin **que de** `tp_api.h`, de l'import library (`libtp.dll.a`
ou `libtp.lib`) et de la DLL : il n'inclut aucun en-tête du moteur et ne
possède aucun fichier source.

Les **5 sources** à compiler dans la DLL :

```
src\jeton.cpp
src\analyseur.cpp
src\evaluation.cpp
src\convertisseur.cpp
src\lib_api.cpp
```

`src/lib_api.cpp` contient les instanciations explicites des piles et la
frontière C (les fonctions `tp_*`).

## 2. Prérequis et choix d'outil

Il faut une machine Windows et **une seule** des deux chaînes suivantes.

- **Route A — MinGW-w64** : recommandée si tu connais `g++` (c'est l'outil
  du projet sous Linux). Compilation : `g++`. Mémoire : des `obj` et un
  import-lib `libtp.dll.a`.
- **Route B — MSVC** : la chaîne officielle Windows (Visual Studio Build
  Tools). Compilation : `cl` + `link`. Mémoire : des `obj`, un import-lib
  `libtp.lib` et un fichier `libtp.exp`.

Aucune bibliothèque externe : le moteur n'utilise que la bibliothèque
standard, fournie avec l'outil.

> **Si tu es sous WSL** — voir la section 9 avant de commencer. Quoi qu'il
> arrive, on ne compile **jamais** depuis cmd sur le chemin
> `\\wsl.localhost\Ubuntu\home\yasser\TP_Bouziri` : les outils Windows y
> sont lents, refusent souvent les chemins UNC, et les permissions WSL
> cassent les chaînes de build Windows. La cmd ne doit voir **que** du
> disque NTFS.

## 3. Étape 0 — Mettre le projet sur Windows

Depuis WSL, duplique le projet sur le disque Windows (le pont `/mnt/c`) :

```sh
cp -r /home/yasser/TP_Bouziri /mnt/c/Users/<tonUtilisateur>/TP_Bouziri
```

Vérifie depuis Windows que le projet est visible :

```bat
dir C:\Users\<tonUtilisateur>\TP_Bouziri
```

Toutes les commandes ci-dessous se tapent dans un **invite de commandes
(cmd)** situé dans ce dossier :

```bat
cd C:\Users\<tonUtilisateur>\TP_Bouziri
```

---

## 4. Route A — MinGW-w64, pas à pas

### A.1 Installer MinGW-w64 (une seule fois)

Deux façons :

**Option 1 — MSYS2 (recommandée)**. MSYS2 est un dépôt de paquets pour
Windows ; il fournit la version officielle de MinGW-w64.

```bat
winget install -e --id MSYS2.MSYS2
```

Puis, dans un terminal **MSYS2** :

```sh
pacman -Syu
pacman -S --needed mingw-w64-x86_64-gcc
```

Le compilateur est alors dans `C:\msys64\mingw64\bin\g++.exe`.

**Option 2 — w64devkit (sans installation)**. Télécharge
« *w64devkit-x.x.x.zip* » et décompresse-le dans `C:\w64devkit`.
Le compilateur est alors `C:\w64devkit\bin\g++.exe`, sans rien installer.

### A.2 Rendre `g++` disponible dans cmd

Ajoute le dossier `bin` à la variable `PATH` **de l'utilisateur** (une seule
fois, puis **ferme et rouvre la fenêtre cmd**) :

```bat
setx PATH "%PATH%;C:\msys64\mingw64\bin"
```

(pour w64devkit : `%PATH%;C:\w64devkit\bin`).

Vérifie :

```bat
g++ --version
```

Tu dois voir une ligne commençant par `g++ (MinGW-W64 ...)` ou
`g++ (Rev6, Built by MSYS2 project ...)`.

### A.3 Compiler la DLL (une seule commande)

Toujours dans `C:\Users\<tonUtilisateur>\TP_Bouziri` :

```bat
g++ -std=c++17 -O2 -Iinclude -DAPI_TP_CONSTRUCTION_LIB -fvisibility=hidden -shared -Wl,--out-implib,lib\libtp.dll.a src\jeton.cpp src\analyseur.cpp src\evaluation.cpp src\convertisseur.cpp src\lib_api.cpp -o lib\libtp.dll
```

Ce que chaque morceau signifie :

| Morceau | Rôle |
| --- | --- |
| `g++` | l'appelle le compilateur (MinGW) |
| `-std=c++17` | le standard du projet (requis : le code utilise C++17) |
| `-O2` | optimisation, niveau du Makefile Linux |
| `-Iinclude` | **indispensable** : les en-têtes (`jeton.hpp`, `analyseur.hpp`, `tp_api.h`, …) vivent dans `include/`, pas à côté des `.cpp` |
| `-DAPI_TP_CONSTRUCTION_LIB` | **le plus important** : fait prendre à `API_EXPORT` le sens `__declspec(dllexport)`, donc exporte les symboles dans la DLL. Sans lui, une DLL vide ! |
| `-fvisibility=hidden` | (conseillé) cache le reste des symboles, surface contrôlée comme sous Linux |
| `-shared` | fabrique une **bibliothèque partagée** (une DLL), pas un exécutable |
| `-Wl,--out-implib,lib\libtp.dll.a` | demande l'**import-lib** explicite `lib\libtp.dll.a` (sert à lier le client, voir A.4) |
| `src\jeton.cpp ... src\lib_api.cpp` | les 5 sources du moteur |
| `-o lib\libtp.dll` | nom du fichier de sortie |

Il faut bien vérifier le dossier `lib\` après : il doit contenir

```bat
dir lib\
```

```
libtp.dll        <- la bibliothèque à distribuer
libtp.dll.a      <- l'import-lib (uniquement pour lier le client)
```

### A.4 Compiler le client (une seule commande)

```bat
g++ -std=c++17 -O2 -Iinclude src\client_dll.cpp lib\libtp.dll.a -o bin\client_dll.exe
```

- `-Iinclude` : où trouver `tp_api.h` et `api.hpp`.
- `lib\libtp.dll.a` : **le** import-lib, donné par son **chemin direct**.
  C'est le plus sûr ; la notation `-llibtp` est piégeuse (ambiguïté de
  préfixe `lib` dans le nom), on l'évite.
- **Remarque** : aucune option `-DAPI_TP_CONSTRUCTION_LIB` ici ! Le client
  doit voir `API_EXPORT` comme `dllimport`, c'est-à-dire « ces symboles
  viennent d'ailleurs ». Si le client échoue au lien en disant
  `undefined reference to tp_creer`, la cause est presque toujours là.

Vérifie qu'il existe bien :

```bat
dir bin\client_dll.exe
```

### A.5 Copier la DLL à côté de l'exe et lancer

```bat
copy lib\libtp.dll bin\
bin\client_dll.exe
```

Pourquoi le `copy` : Windows ne connaît pas le `rpath` ; l'exécutable ne
cherche la DLL que dans (1) son propre dossier, (2) `%PATH%`, (3)
System32. La copier à côté de l'exe est donc **obligatoire**.

C'est terminé pour MinGW : la sortie doit correspondre à la section 8.

---

## 5. Route B — MSVC, pas à pas

### B.1 Installer Visual Studio Build Tools (une seule fois)

```bat
winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

(L'installation dure quelques minutes.)

### B.2 Ouvrir le bon invite de commandes

Ne pas ouvrir « cmd », mais, dans le menu Démarrer :

```
x64 Native Tools Command Prompt for VS 2022
```

C'est lui qui place `cl` et `link` dans le chemin. Vérifie :

```bat
cl
```

Tu dois voir le bandeau « Microsoft (R) C/C++ Optimizing Compiler … ».
Place-toi dans le projet :

```bat
cd C:\Users\<tonUtilisateur>\TP_Bouziri
```

### B.3 Compiler les 5 sources en objets (sans lier)

```bat
cl /nologo /std:c++17 /O2 /EHsc /DAPI_TP_CONSTRUCTION_LIB /Iinclude /c src\jeton.cpp src\analyseur.cpp src\evaluation.cpp src\convertisseur.cpp src\lib_api.cpp
```

- `/std:c++17` : le standard du projet.
- `/O2` : optimisation.
- `/EHsc` : **obligatoire** — active le modèle d'exceptions C++ ; sans lui,
  MSVC refuse le `try/catch` de `src/lib_api.cpp`.
- `/DAPI_TP_CONSTRUCTION_LIB` : fait prendre à `API_EXPORT` le sens
  `dllexport` (d'où viennent les exports) — l'équivalent du `-D` de MinGW.
- `/Iinclude` : dossier des en-têtes.
- `/c` : compile **sans** lier (on ne veut que des `.obj`).

Résultat : `jeton.obj`, `analyseur.obj`, `evaluation.obj`,
`convertisseur.obj`, `lib_api.obj` dans le dossier courant.

### B.4 Assembler la DLL

```bat
link /nologo /DLL jeton.obj analyseur.obj evaluation.obj convertisseur.obj lib_api.obj /OUT:lib\libtp.dll /IMPLIB:lib\libtp.lib
```

`/DLL` produit une bibliothèque partagée. Il naît **trois** fichiers dans
`lib\` :

```
libtp.dll   <- la bibliothèque à distribuer (et à copier à côté de l'exe)
libtp.lib   <- l'import-lib, pour lier le client
libtp.exp   <- fichier d'export, inutile ici, peut être supprimé
```

Vérifie avec :

```bat
dir lib\
```

### B.5 Compiler le client

```bat
cl /nologo /std:c++17 /O2 /EHsc /Iinclude /c src\client_dll.cpp
```

**Surtout pas de `/DAPI_TP_CONSTRUCTION_LIB`** ici : ce programme est le
client, il doit voir `API_EXPORT` comme `dllimport`. Il produit
`client_dll.obj`.

### B.6 Lier le client contre l'import-lib

```bat
link /nologo client_dll.obj lib\libtp.lib /OUT:bin\client_dll.exe
```

L'exécutable de sortie est `bin\client_dll.exe`.

### B.7 Copier la DLL à côté de l'exe et lancer

```bat
copy lib\libtp.dll bin\
bin\client_dll.exe
```

C'est terminé pour MSVC : la sortie doit correspondre à la section 8.

---

## 6. Utilisation du Makefile pour Windows

Dans une console MSYS2 MinGW ou avec un cross-compilateur MinGW-w64 disponible,
les cibles du Makefile automatisent les mêmes étapes :

```sh
make dll-windows WINCXX=x86_64-w64-mingw32-g++  # DLL + import library
make windows-client WINCXX=x86_64-w64-mingw32-g++  # client + copie de la DLL
```

Les fichiers produits sont `lib/libtp.dll`, `lib/libtp.dll.a` et
`bin/client_dll.exe`. La cible `windows-client` copie la DLL à côté de
l'exécutable, condition nécessaire au chargement Windows. Avec MSVC, suivre
les commandes détaillées de la section 5, car MSVC utilise `libtp.lib` au lieu
de `libtp.dll.a`.

## 7. Règles d'exécution (à lire avant toute erreur)

En résumé des deux routes : **la DLL doit toujours être à côté de
l'exécutable** (`copy lib\libtp.dll bin\`). L'ordre de recherche du chargeur
Windows est :

1. le dossier de l'exécutable ;
2. les dossiers de `%PATH%` ;
3. `C:\Windows\System32`, etc.

## 8. Vérifications

### 8.1 Vérifier que l'interface est bien exportée

MinGW :

```bat
objdump -p lib\libtp.dll | findstr tp_
```

MSVC :

```bat
dumpbin /exports lib\libtp.dll
```

Tu dois voir, **sans décoration `?`** (ce sont des fonctions C `extern "C"`) :

```
tp_creer
tp_liberer
tp_verifier
tp_evaluer
tp_postfixe
```

Plus toutes les fonctions `expressions::...` (surface C++) et les cinq
instanciations de piles. Référence mesurée sous Linux : **161 symboles** =
5 (interface C) + 34 (moteur `expressions`) + 112 (instanciations des piles)
+ 10 (intimes de la STL, exposés par la bibliothèque standard elle-même).

### 8.2 Sortie attendue du client

Le client doit afficher exactement ceci (c'est la valeur de référence du
Linux) :

```
Evaluation via la bibliotheque libtp (session opaque) :
-------------------------------------------------------
  2 + 3 * 4          -> 14
  -2 ^ 2             -> -4
  (1 + 2) * (3 - 4)  -> -3
  0.1 + 0.2          -> 0.3
  2 3 *              -> [invalide] colonne 3 (jeton "3") : deux operandes juxtaposes : l'operateur entre les deux est manquant.
  1 / 0              -> [invalide] colonne 3 (jeton "/") : division par zero.
  2 +                -> [invalide] colonne 3 (jeton "+") : operande manquant pour l'operateur "+".

Verification syntaxique directe :
--------------------------------
  2(3 + 4)           -> invalide : colonne 2 (jeton "(") : deux operandes juxtaposes : l'operateur entre les deux est manquant.
  2 + 3 * 4          -> valide

Forme postfixee (validite verifiee par la bibliotheque) :
----------------------------------------------------------
  2 + 3 * 4 -> 2 3 4 * +
  2 2 + +  -> [invalide] colonne 3 (jeton "2") : deux operandes juxtaposes : l'operateur entre les deux est manquant.

Session liberee : aucun appel n'est possible apres ce point.
```

---

## 9. Erreurs typiques sous Windows

| Message / symptôme | Cause | Correction |
| --- | --- | --- |
| `g++ n'est pas reconnu` | `g++` pas dans `PATH` | refaire A.2 puis **rouvrir** cmd |
| `cl n'est pas reconnu` | cmd simple au lieu du « x64 Native Tools » | rouvrir le bon invite (B.2) |
| `libtp.dll introuvable` (ou `0xC0000135`) à l'exécution | la DLL n'est pas dans le dossier de l'exe | `copy lib\libtp.dll bin\` |
| `The procedure entry point tp_verifier could not be located` | une **ancienne** `libtp.dll` (sans `tp_verifier`) traîne à côté de l'exe | remplacer la DLL par celle fraîchement compilée |
| `jeton.hpp: No such file or directory` (ou un autre `*.hpp`) en compilant la DLL | la commande n'a pas reçu `-Iinclude` : les en-têtes sont dans `include/`, pas à côté des `src\*.cpp` | ajouter `-Iinclude` à la ligne `g++` (voir A.3 / 10.1) |
| Lien : `undefined reference to tp_creer` (MinGW) | le client a été compilé avec `-DAPI_TP_CONSTRUCTION_LIB`, ou l'import-lib n'est pas donné | relire A.4 (pas de define côté client) |
| Lien : `unresolved external symbol tp_creer` (MSVC) | `client_dll.obj` lié sans `lib\libtp.lib` | refaire B.6 |
| Compilation MSVC refusée dans `lib_api.cpp` | `/EHsc` absent | ajouter `/EHsc` |
| `unrecognized command line option '-D...'` | commande MinGW tapée dans le mauvais terminal, et inversement | MinGW : cmd/ou MSYS2 ; MSVC : x64 Native Tools |

Aucune de ces erreurs n'indique un bug du code : c'est la mécanique normale
du couple « DLL trouvable / DLL de la bonne version ».

---

## 10. Ce qui ne change pas

- Les cibles `make` du projet (`check`, `tests`, `rapport`,
  `bibliotheque`, `lib-check`) et les suites 62/55 restent **Linux/ELF** :
  elles ne se lancent que sous Linux/WSL.
- La DLL est le *même* code, compilé pour un autre système : aucun
  changement de comportement n'est attendu, et c'est pourquoi la référence
  de la section 8 est reproductible.
- Les sanitiseurs (`-fsanitize=address,undefined`) du rapport ne
  s'appliquent qu'à la vérification Linux : ne pas essayer de les compiler
  avec MSVC (non supportés par défaut).

---

## 11. Travailler depuis WSL (le cas typique du TP)

Deux stratégies, une règle :

> **Une seule arborescence par rôle.** WSL : développer, tester, rédiger le
> rapport (`make`). NTFS (`/mnt/c/...`) : build et exécution Windows. Un
> seul pont entre les deux : `/mnt/c/`. Jamais le chemin
> `\\wsl.localhost\...` côté Windows.

### 11.1 Reste dans WSL : cross-compiler puis copier les binaires

Installe le cross-compilateur (une seule fois) :

```sh
sudo apt install g++-mingw-w64-x86-64
```

Compile la DLL et le client. `-static-libgcc -static-libstdc++` incorpore
les trois DLL d'exécution de MinGW (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`,
`libwinpthread-1.dll`) : sans ces options, Windows les réclamera à côté de
l'exe.

```sh
x86_64-w64-mingw32-g++ -std=c++17 -O2 -Iinclude -DAPI_TP_CONSTRUCTION_LIB -shared \
    -static-libgcc -static-libstdc++ -Wl,--out-implib,lib/libtp.dll.a \
    src/jeton.cpp src/analyseur.cpp src/evaluation.cpp src/convertisseur.cpp src/lib_api.cpp \
    -o lib/libtp.dll

x86_64-w64-mingw32-g++ -std=c++17 -O2 -Iinclude src/client_dll.cpp \
    -static-libgcc -static-libstdc++ lib/libtp.dll.a \
    -o bin/client_dll.exe

cp lib/libtp.dll bin/
```

Copie les **deux** fichiers vers le disque Windows :

```sh
cp bin/client_dll.exe bin/libtp.dll /mnt/c/Users/<tonUtilisateur>/
```

Puis, dans cmd (ou en double-cliquant) :

```bat
C:\Users\<tonUtilisateur>\client_dll.exe
```

### 11.2 Compile sur Windows (routes A ou B)

Depuis WSL, recopie le projet sur NTFS avant chaque build Windows :

```sh
cp -r /home/yasser/TP_Bouziri /mnt/c/Users/<tonUtilisateur>/TP_Bouziri
```

Puis suis les sections 4 (MinGW) ou 5 (MSVC) à partir du `cd ...\TP_Bouziri`.
Le projet tient en moins d'un mégaoctet : la recopie est triviale, et elle
reprend tes modifications faites côté WSL.