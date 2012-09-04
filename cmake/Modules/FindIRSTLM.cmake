# - Try to find moses
# Once done this will define
#  IRSTLM_FOUND - System has moses
#  IRSTLM_INCLUDE_DIRS - The moses include directories
#  IRSTLM_LIBRARIES - The libraries needed to use moses
#  IRSTLM_DEFINITIONS - Compiler switches required for using moses

find_package(ZLIB REQUIRED)

find_path(IRSTLM_INCLUDE_DIR n_gram.h)

find_library(IRSTLM_LIBRARY irstlm)

set(IRSTLM_LIBRARIES ${ZLIB_LIBRARIES} ${IRSTLM_LIBRARY})
set(IRSTLM_INCLUDE_DIRS ${IRSTLM_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set IRSTLM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(irstlm  DEFAULT_MSG
                                  IRSTLM_LIBRARY IRSTLM_INCLUDE_DIR)

mark_as_advanced(IRSTLM_INCLUDE_DIR IRSTLM_LIBRARY)

