# - Try to find moses
# Once done this will define
#  SRILM_FOUND - System has moses
#  SRILM_INCLUDE_DIRS - The moses include directories
#  SRILM_LIBRARIES - The libraries needed to use moses
#  SRILM_DEFINITIONS - Compiler switches required for using moses

find_path(SRILM_INCLUDE_DIR Ngram.h)

if(${APPLE})
  set(_arch macosx)
else(${APPLE})
  if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64)
    set(_arch i686-m64)
  else(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64)
    set(_arch ${CMAKE_SYSTEM_PROCESSOR})
  endif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64)
endif(${APPLE})

find_library(DSTRUCT_LIBRARY dstruct PATH_SUFFIXES lib/${_arch})
find_library(FLM_LIBRARY flm PATH_SUFFIXES lib/${_arch})
find_library(MISC_LIBRARY misc PATH_SUFFIXES lib/${_arch})
find_library(OOLM_LIBRARY oolm PATH_SUFFIXES lib/${_arch})

set(SRILM_LIBRARIES ${DSTRUCT_LIBRARY} ${FLM_LIBRARY} ${MISC_LIBRARY} ${OOLM_LIBRARY})
set(SRILM_INCLUDE_DIRS ${SRILM_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set SRILM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(srilm  DEFAULT_MSG
                                  DSTRUCT_LIBRARY FLM_LIBRARY MISC_LIBRARY OOLM_LIBRARY
                                  SRILM_INCLUDE_DIR)

mark_as_advanced(SRILM_INCLUDE_DIR SRILM_LIBRARY)

