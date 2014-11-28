cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(Ensishell)
enable_testing()
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Werror -std=gnu99")

#########
# Gestion des variantes
#########
# Vous devez editer les lignes suivantes pour y mettre vos logins
# et le numéro de la variante que vous avez choisi
#########
set(VARIANTE_LOGINS vanbeurl  jeanr)
set(VARIANTE_SUJET 4)
###
list(SORT VARIANTE_LOGINS)
if (VARIANTE_LOGINS MATCHES "login[123]")
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre vos logins")
endif()
if (VARIANTE_SUJET EQUAL -1)
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre le numéro de votre variante du sujet")
endif()

configure_file (
  src/variante.h.in
  ${CMAKE_SOURCE_DIR}/src/variante.h
)

#####
# Détection de la bibliothèque Readline
#####
find_library(READLINE_LIBRARY NAMES readline libreadline history libhistory)


#########
# Fin de gestion des variantes
#########

##
# Si vous utilisé plusieurs fichiers, en plus de ensishell.c, pour votre
# shell il faut les ajouter ici
##
add_executable(ensishell src/readcmd.c src/ensishell.c src/interpretor.c)
target_link_libraries(ensishell readline history)

##
# Programme de test
##
add_test(UnitShellTests ../tests/allShellTests.rb)

##
# Ajout d'une cible pour lancer les tests de manière verbeuse
##
add_custom_target(check ../tests/allShellTests.rb)

##
# Construction de l'archive
##
string(REPLACE ";" "-" LOGINS_SANS_POINTVIRGULE "${VARIANTE_LOGINS}")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH ${LOGINS_SANS_POINTVIRGULE})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  "~$"
  "\\\\.o$"
  "^${PROJECT_SOURCE_DIR}/build/"
  "^${PROJECT_SOURCE_DIR}/.git/"
)
include(CPack)
