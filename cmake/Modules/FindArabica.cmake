# - Try to find moses
# Once done this will define
#  Arabica_FOUND - System has moses
#  Arabica_INCLUDE_DIRS - The moses include directories
#  Arabica_LIBRARIES - The libraries needed to use moses
#  Arabica_DEFINITIONS - Compiler switches required for using moses

set(_depflags ${Arabica_FIND_QUIETLY} ${Arabica_FIND_REQUIRED})
#find_package(EXPAT ${_depflags})

find_path(Arabica_INCLUDE_DIR SAX/ArabicaConfig.hpp PATH_SUFFIXES arabica)

find_library(Arabica_LIBRARY arabica
             PATH_SUFFIXES src)

set(Arabica_LIBRARIES ${Arabica_LIBRARY} ${EXPAT_LIBRARIES})
set(Arabica_INCLUDE_DIRS ${Arabica_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set Arabica_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(arabica  DEFAULT_MSG
                                  Arabica_LIBRARY Arabica_INCLUDE_DIR)

mark_as_advanced(Arabica_INCLUDE_DIR Arabica_LIBRARY)

