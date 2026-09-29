# Fichiers sources
CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS := -Iinclude

# Versionnement automatique des dependances, si make le permet
DEPFLAGS := -MMD -MP

BIN     := bin
BUILD   := build
PIC     := $(BUILD)/pic
LIB     := lib
RAPPORT := rapport

# Cibles
PILES        := $(BIN)/demo_piles
EXPRESSIONS  := $(BIN)/demo_expressions
BENCHMARK    := $(BIN)/benchmark_piles
TEST_PILES   := $(BIN)/tests_piles
TEST_EXPRESSIONS := $(BIN)/tests_expressions

EXECUTABLES := $(PILES) $(EXPRESSIONS) $(BENCHMARK) $(TEST_PILES) $(TEST_EXPRESSIONS)

# Les modules de l'outillage sur les expressions
MODULES := $(BUILD)/jeton.o $(BUILD)/analyseur.o $(BUILD)/evaluation.o $(BUILD)/convertisseur.o

# --- Bibliotheque partagee ---------------------------------------------------

# Nom du module partage. Le fichier reel porte un numero de version (soname),
# et dev libtp.so est un lien vers lui, pour pouvoir ecrire -ltp au lien.
LIBTP_SO  := $(LIB)/libtp.so.1
LIBTP_DEV := $(LIB)/libtp.so
SONAME    := libtp.so.1

# Version statique du meme objet, pour la comparaison statique / dynamique.
LIBTP_A   := $(LIB)/libtp.a

# Le pickle de la bibliotheque : les modules du moteur, lib_api.cpp, et les
# instanciations explicites des piles, tous compiles en code independant de la
# position (-fPIC) et avec la visibilite cachee par defaut : seuls les symboles
# marques API_EXPORT sortent du module.
PIC_MODULES := $(PIC)/jeton.o $(PIC)/analyseur.o $(PIC)/evaluation.o $(PIC)/convertisseur.o $(PIC)/lib_api.o

# Programmes de la plateforme "dll" : un client externe a l'interface C, et
# les memes binaires de test/demonstration lies statiquement puis dynamiquement.
CLIENT           := $(BIN)/client_dll
TESTS_EXPR_STAT  := $(BIN)/tests_expressions_stat
TESTS_EXPR_DYN   := $(BIN)/tests_expressions_dyn
DEMO_EXPR_DYN    := $(BIN)/demo_expressions_dyn
TESTS_API        := $(BIN)/tests_api

# Construction Windows avec MinGW-w64. Le compilateur peut etre surcharge,
# par exemple : make dll-windows WINCXX=x86_64-w64-mingw32-g++
WINCXX       ?= x86_64-w64-mingw32-g++
WINBUILD     := $(BUILD)/windows
WINLIB       := $(LIB)/libtp.dll
WINIMPORT    := $(LIB)/libtp.dll.a
WINPIC_MODULES := $(WINBUILD)/jeton.o $(WINBUILD)/analyseur.o \
                  $(WINBUILD)/evaluation.o $(WINBUILD)/convertisseur.o \
                  $(WINBUILD)/lib_api.o

.PHONY: all tests demos benches check rapport bibliotheque lib-client lib-tests \
        lib-check rapport-dll tout tout-dll nettoyer nettoyer-tout clean \
        dll-windows windows-client

all: $(EXECUTABLES)

# --- Compilation ------------------------------------------------------------

demos: $(PILES) $(EXPRESSIONS)

benches: $(BENCHMARK)

tests: $(TEST_PILES) $(TEST_EXPRESSIONS)

$(BIN)/demo_piles: $(BUILD)/demo_piles.o | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN)/demo_expressions: $(BUILD)/demo_expressions.o $(MODULES) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN)/benchmark_piles: $(BUILD)/benchmark_piles.o | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN)/tests_piles: $(BUILD)/tests_piles.o | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN)/tests_expressions: $(BUILD)/tests_expressions.o $(MODULES) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD)/%.o: src/%.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN) $(BUILD) $(PIC) $(LIB) $(WINBUILD):
	@mkdir -p $@

# --- Bibliotheque : compilations --------------------------------------------------

# Modules du moteur et frontiere C, en code position-independant, visibilite
# cachee par defaut, et macros d'export activees (API_TP_CONSTRUCTION_LIB) :
# seuls les symboles marques API_EXPORT perceront la visibilite.
$(PIC)/%.o: src/%.cpp | $(PIC)
	$(CXX) $(CPPFLAGS) -DAPI_TP_CONSTRUCTION_LIB $(CXXFLAGS) -fPIC -fvisibility=hidden $(DEPFLAGS) -c $< -o $@

# --- Bibliotheque : edition de liens ---------------------------------------

# Module partage. -Wl,-soname fige le nom de reference : le lien des clients se
# fait contre libtp.so (lien), mais l'execution ne cherche que libtp.so.1.
$(LIBTP_SO): $(PIC_MODULES) | $(LIB)
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,$(SONAME) $^ -o $@
	@echo "Bibliotheque partagee $(LIBTP_SO) construite."

$(LIBTP_DEV): $(LIBTP_SO)
	@ln -sf $(SONAME) $@

# Version statique du meme objet, pour la comparaison statique / dynamique.
$(LIBTP_A): $(MODULES) $(BUILD)/lib_api.o | $(LIB)
	ar rcs $@ $^
	@echo "Bibliotheque statique $(LIBTP_A) construite."

# --- Bibliotheque : programmes -----------------------------------------------------

bibliotheque: $(LIBTP_SO) $(LIBTP_DEV) $(LIBTP_A)

lib-client: $(CLIENT)

lib-tests: $(TESTS_EXPR_STAT) $(TESTS_EXPR_DYN) $(DEMO_EXPR_DYN) $(TESTS_API)

# Client externe : ne voit que tp_api.h et lie la bibliotheque en dynamique.
# Le rpath relatif a l'emplacement du programme ($$ORIGIN) rend le binaire
# executable sans aucune variable d'environnement.
$(CLIENT): src/client_dll.cpp $(LIBTP_DEV) | $(BIN)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -L$(LIB) -ltp -Wl,-rpath,'$$ORIGIN/../$(LIB)' -o $@

# Client minimal en C : il ne connait que tp_api.h et verifie le contrat de
# la frontiere C (verification, evaluation, conversion et buffers).
$(BUILD)/tests_api.o: src/tests_api.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TESTS_API): $(BUILD)/tests_api.o $(LIBTP_DEV) | $(BIN)
	$(CC) $(CFLAGS) $< -L$(LIB) -ltp -Wl,-rpath,'$$ORIGIN/../$(LIB)' -o $@

# Les memes suites liennees statiquement puis dynamiquement contre le moteur.
$(TESTS_EXPR_STAT): $(BUILD)/tests_expressions.o $(LIBTP_A) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TESTS_EXPR_DYN): $(BUILD)/tests_expressions.o $(LIBTP_DEV) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -L$(LIB) -ltp -Wl,-rpath,'$$ORIGIN/../$(LIB)' -o $@

$(DEMO_EXPR_DYN): $(BUILD)/demo_expressions.o $(LIBTP_DEV) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -L$(LIB) -ltp -Wl,-rpath,'$$ORIGIN/../$(LIB)' -o $@

# Checks de la bibliotheque : les trois configurations produisent le meme
# resultat (tests_expressions : 55 reussis ; demo : le meme texte), et la
# frontiere du module partage ne laisse passer que les symboles voulus.
lib-check: bibliotheque lib-client lib-tests
	@echo "== Client externe (interface C) =="
	@./$(CLIENT)
	@echo
	@echo "== Moteur lie statiquement (libtp.a) =="
	@./$(TESTS_EXPR_STAT)
	@echo
	@echo "== Moteur lie dynamiquement (libtp.so.1) =="
	@./$(TESTS_EXPR_DYN)
	@echo
	@echo "== Demonstration liee dynamiquement =="
	@./$(DEMO_EXPR_DYN)
	@echo
	@echo "== Client C (contrat de l'API) =="
	@./$(TESTS_API)
	@echo
	@echo "== Symboles exportes par la bibliotheque partagee =="
	@nm -D --defined-only $(LIBTP_SO) | awk '$$3 !~ /^(_ZN.*(PileTableau|PileListe|PileSTL)|_ZTh)/ { print }' | head -30
	@echo "Total symboles exportes :" $(shell nm -D --defined-only $(LIBTP_SO) | wc -l)
	@echo "Export rares (frontiere C) :" $(shell nm -D --defined-only $(LIBTP_SO) | grep -cE 'T? tp_(creer|liberer|verifier|evaluer|postfixe)')

# --- DLL Windows (MinGW-w64) ----------------------------------------------

# Les cinq objets sont compiles separement, puis g++ -shared les rassemble en
# libtp.dll. Le linker produit aussi libtp.dll.a, import-lib utilisee par le
# programme client au moment de son edition de liens.
$(WINBUILD)/%.o: src/%.cpp | $(WINBUILD)
	$(WINCXX) $(CPPFLAGS) $(CXXFLAGS) -DAPI_TP_CONSTRUCTION_LIB -c $< -o $@

$(WINLIB): $(WINPIC_MODULES) | $(LIB)
	$(WINCXX) $(CXXFLAGS) -shared -Wl,--out-implib,$(WINIMPORT) $^ -o $@
	@echo "DLL Windows construite : $(WINLIB)"

$(WINIMPORT): $(WINLIB)
	@test -f $@

# Le client est lie a l'import-lib et la DLL est copiee a cote de l'executable,
# emplacement recherche automatiquement par Windows au lancement.
dll-windows: $(WINLIB) $(WINIMPORT)

windows-client: $(WINLIB) $(WINIMPORT) src/client_dll.cpp | $(BIN)
	$(WINCXX) $(CPPFLAGS) $(CXXFLAGS) src/client_dll.cpp $(WINIMPORT) -o $(BIN)/client_dll.exe
	cp $(WINLIB) $(BIN)/libtp.dll

# --- Verification -----------------------------------------------------------

# Lance les deux suites de tests et echoue si l'une des deux echoue.
check: tests
	@./$(TEST_PILES)
	@./$(TEST_EXPRESSIONS)

# --- Execution --------------------------------------------------------------

run: $(PILES) $(EXPRESSIONS) $(BENCHMARK)
	@./$(PILES)
	@echo
	@./$(EXPRESSIONS)
	@echo
	@./$(BENCHMARK)

# --- Rapport ----------------------------------------------------------------

# Genere le PDF du rapport a partir de la source LaTeX. Le PDF est produit
# directement par pdflatex : aucune conversion intermediaire n'est necessaire.
rapport: $(RAPPORT)/rapport.pdf

# Le temoin porte un nom distinct de celui de la cible « rapport », sans quoi
# make le prendrait pour une dependance circulaire.
$(RAPPORT)/rapport.pdf: $(RAPPORT)/rapport.tex | $(RAPPORT)/.dossier
	@cd $(RAPPORT) && pdflatex -interaction=nonstopmode rapport.tex >/dev/null
	@cd $(RAPPORT) && pdflatex -interaction=nonstopmode rapport.tex >/dev/null
	@echo "rapport/rapport.pdf est a jour."

$(RAPPORT)/.dossier:
	@mkdir -p $(RAPPORT)
	@touch $@

# --- Rapport complementaire --------------------------------------------------

# Rapport consacre a la creation de la bibliotheque partagee, meme structure
# et meme regle de construction que le rapport principal.
rapport-dll: $(RAPPORT)/rapport_dll.pdf

$(RAPPORT)/rapport_dll.pdf: $(RAPPORT)/rapport_dll.tex | $(RAPPORT)/.dossier
	@cd $(RAPPORT) && pdflatex -interaction=nonstopmode rapport_dll.tex >/dev/null
	@cd $(RAPPORT) && pdflatex -interaction=nonstopmode rapport_dll.tex >/dev/null
	@echo "rapport/rapport_dll.pdf est a jour."

# --- Nettoyage --------------------------------------------------------------

tout: check run rapport

# Tout le circuit de la bibliotheque : verification des trois configurations,
# puis rapport complementaire.
tout-dll: lib-check rapport-dll

nettoyer:
	rm -rf $(BUILD) $(BIN) $(LIB)

clean: nettoyer

nettoyer-tout: nettoyer
	rm -f $(RAPPORT)/*.aux $(RAPPORT)/*.log $(RAPPORT)/*.out $(RAPPORT)/*.toc $(RAPPORT)/.dossier

# --- Dependances ------------------------------------------------------------

-include $(BUILD)/*.d
