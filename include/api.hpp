#ifndef API_HPP
#define API_HPP

// Macro de controle des symboles pour la construction du module en
// bibliotheque partagee (ou DLL sous Windows).
//
// Deux situations, deux sens de la macro :
//   - en compilant la bibliotheque (API_TP_CONSTRUCTION_LIB defini), la macro
//     annonce au chargeur que le symbole sera fourni par le module ;
//   - en compilant un client, elle annonce au compilateur que le symbole vient
//     d'un module externe.
//
// Sous Windows, MSVC et MinGW utilisent __declspec(dllexport/dllimport). Sur
// les autres plateformes, on applique la convention GNU : compiler la
// bibliotheque avec -fvisibility=hidden, puis ramener a "default" les seuls
// symboles du site public, grace a un attribut de visibilite. Sans cet
// attribut, la macro ne change rien au code.

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(API_TP_CONSTRUCTION_LIB)
#    define API_EXPORT __declspec(dllexport)
#  else
#    define API_EXPORT __declspec(dllimport)
#  endif
#else
#  if defined(API_TP_CONSTRUCTION_LIB)
#    define API_EXPORT __attribute__((visibility("default")))
#  else
#    define API_EXPORT
#  endif
#endif

// Encapsule un site "extern C", necessaire pour que l'interface C de la
// bibliotheque (lib_api.cpp) soit utilisable aussi bien depuis un programme
// C++ que depuis un programme ecrit en C.
#if defined(__cplusplus)
#  define API_DEBUT_EXTERN_C extern "C" {
#  define API_FIN_EXTERN_C   }
#else
#  define API_DEBUT_EXTERN_C
#  define API_FIN_EXTERN_C
#endif

#endif // API_HPP