# - Try to find moses
# Once done this will define
#  MOSES_FOUND - System has moses
#  MOSES_INCLUDE_DIRS - The moses include directories
#  MOSES_LIBRARIES - The libraries needed to use moses
#  MOSES_DEFINITIONS - Compiler switches required for using moses

#find_package(IRSTLM REQUIRED)
#find_package(SRILM REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost 1.44 COMPONENTS system thread filesystem REQUIRED)

find_path(MOSES_INCLUDE_DIR StaticData.h
          PATH_SUFFIXES moses/src)

find_path(MOSES_UTIL_DIR util/check.hh)

find_library(MOSES_LIBRARY moses)
find_library(MOSES_INTERNAL_LIBRARY moses_internal)
find_library(ONDISKPT_LIBRARY OnDiskPt)
find_library(COMPACTPT_LIBRARY CompactPT)
find_library(RULETABLE_LIBRARY RuleTable)
find_library(SCOPE3PARSER_LIBRARY Scope3Parser)
find_library(CYKPLUSPARSER_LIBRARY CYKPlusParser)
find_library(LM_LIBRARY LM)
find_library(FUZZYMATCH_LIBRARY fuzzy-match)
find_library(KENLM_LIBRARY kenlm)
find_library(KENUTIL_LIBRARY kenutil)

set(MOSES_LIBRARIES
	-Wl,--start-group
	${MOSES_LIBRARY} ${KENLM_LIBRARY} ${KENUTIL_LIBRARY}
	${ONDISKPT_LIBRARY} ${COMPACTPT_LIBRARY} ${RULETABLE_LIBRARY} ${SCOPE3PARSER_LIBRARY}
	${CYKPLUSPARSER_LIBRARY} ${FUZZYMATCH_LIBRARY}
	${MOSES_INTERNAL_LIBRARY} ${LM_LIBRARY} ${Boost_LIBRARIES} ${ZLIB_LIBRARIES}
	-Wl,--end-group)

set(MOSES_INCLUDE_DIRS ${MOSES_INCLUDE_DIR} ${MOSES_UTIL_DIR} ${ZLIB_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set MOSES_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(moses  DEFAULT_MSG
                                  MOSES_LIBRARY MOSES_INCLUDE_DIR)

mark_as_advanced(MOSES_INCLUDE_DIR MOSES_LIBRARY)

