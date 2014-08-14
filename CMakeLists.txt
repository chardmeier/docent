cmake_minimum_required(VERSION 2.8.1)
cmake_policy(SET CMP0015 NEW)
project(docent)

include(CheckLibraryExists)
include(ExternalProject)
set_directory_properties(PROPERTY EP_BASE external)

### Configurable section

set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(KENLM_MAX_ORDER 7)

# Add -march=native if the compiler supports it

if(CMAKE_COMPILER_IS_GNUCXX)
	execute_process(
		COMMAND ${CMAKE_CXX_COMPILER} -E -x c++ -march=native /dev/null
		RESULT_VARIABLE MARCH_NATIVE
		OUTPUT_QUIET
		ERROR_QUIET)

	if(MARCH_NATIVE EQUAL 0)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
	endif()
endif()

### Portability hacks

if(APPLE)
	# Darwin ld doesn't support, and doesn't seem to need, start-/end-group
	set(LD_START_GROUP "")
	set(LD_END_GROUP "")
else()
	set(LD_START_GROUP -Wl,--start-group)
	set(LD_END_GROUP -Wl,--end-group)
endif()

### Find out if we have MPI support

find_package(MPI)

### Find Boost

if(DEFINED ENV{BOOST_ROOT} AND NOT DEFINED BOOST_ROOT)
	set(BOOST_ROOT $ENV{BOOST_ROOT})
endif()

if(DEFINED BOOST_ROOT)
	set(MOSES_WITH_BOOST "--with-boost=${BOOST_ROOT}")
	set(Arabica_WITH_BOOST "--with-boost=${BOOST_ROOT}/include")
else()
	message("BOOST_ROOT isn't set. If you encounter build problems, consider setting it.")
	set(MOSES_WITH_BOOST "")
	set(Arabica_WITH_BOOST "")
endif()

set(Boost_NO_SYSTEM_PATHS TRUE)

if(MPI_FOUND)
	find_package(Boost 1.44 COMPONENTS mpi)
	if(NOT Boost_MPI_FOUND)
		message(WARNING "Found MPI but not boost::mpi. Building without MPI support.")
		set(MPI_FOUND FALSE)
	endif()
else()
	message("You don't seem to have MPI installed, so mpi-docent won't be built. "
		"The other binaries won't be affected.")
endif()

find_package(Boost 1.44 COMPONENTS thread random serialization regex system filesystem REQUIRED)

### Build libstemmer_c

set(STEMMER_ROOT ${CMAKE_SOURCE_DIR}/external/libstemmer_c)
set(STEMMER_INSTALL_DIR ${CMAKE_BINARY_DIR}/external/Install/stemmer_external)

ExternalProject_add(stemmer_external
	SOURCE_DIR ${STEMMER_ROOT}
	CONFIGURE_COMMAND true
	BUILD_COMMAND ${MAKE}
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STEMMER_ROOT}/libstemmer.o ${STEMMER_INSTALL_DIR}
	INSTALL_DIR ${STEMMER_INSTALL_DIR}
	BUILD_IN_SOURCE 1)

add_library(stemmer STATIC IMPORTED)
add_dependencies(stemmer stemmer_external)
set_target_properties(stemmer PROPERTIES
	IMPORTED_LOCATION "${STEMMER_INSTALL_DIR}/libstemmer.o")

set(STEMMER_INCLUDE_DIRS ${STEMMER_ROOT}/include)

### Build Arabica

set(Arabica_ROOT ${CMAKE_SOURCE_DIR}/external/arabica)

set(BUILD_ARABICA_EXAMPLES OFF)
add_subdirectory(${Arabica_ROOT})

get_property(Arabica_INCLUDE_DIRS DIRECTORY external/arabica PROPERTY INCLUDE_DIRECTORIES)
set(Arabica_LIBRARIES arabica)

### Build Moses

set(MOSES_ROOT ${CMAKE_SOURCE_DIR}/external/mosesdecoder)
set(MOSES_INSTALL_DIR ${CMAKE_BINARY_DIR}/external/Install/moses_external)

ExternalProject_add(moses_external
	SOURCE_DIR ${MOSES_ROOT}
	CONFIGURE_COMMAND true
	BUILD_COMMAND ./bjam --max-kenlm-order=${KENLM_MAX_ORDER} ${MOSES_WITH_BOOST}
	INSTALL_COMMAND ./bjam --prefix=${MOSES_INSTALL_DIR} ${MOSES_WITH_BOOST} install
	INSTALL_DIR ${MOSES_INSTALL_DIR}
	BUILD_IN_SOURCE 1)

add_library(moses STATIC IMPORTED)
add_dependencies(moses moses_external)

add_definitions(-DKENLM_MAX_ORDER=${KENLM_MAX_ORDER})

find_package(ZLIB REQUIRED)
check_library_exists(-lrt clock_gettime "" HAVE_LIBRT)
if(HAVE_LIBRT)
	set(LIBRT_LIBRARIES -lrt)
else()
	set(LIBRT_LIBRARIES "")
endif()

set(MOSES_INCLUDE_DIRS
	${MOSES_INSTALL_DIR}/include
	${ZLIB_INCLUDE_DIRS})
set(MOSES_LIBRARIES
	${LD_START_GROUP}
	${MOSES_INSTALL_DIR}/lib/libCompactPT.a
	${MOSES_INSTALL_DIR}/lib/libCYKPlusParser.a
	${MOSES_INSTALL_DIR}/lib/libdynsa.a
	${MOSES_INSTALL_DIR}/lib/libfuzzy-match.a
	${MOSES_INSTALL_DIR}/lib/libkenlm.a
	${MOSES_INSTALL_DIR}/lib/libkenutil.a
	${MOSES_INSTALL_DIR}/lib/libLM.a
	${MOSES_INSTALL_DIR}/lib/libmoses.a
	${MOSES_INSTALL_DIR}/lib/libmoses_internal.a
	${MOSES_INSTALL_DIR}/lib/libOnDiskPt.a
	${MOSES_INSTALL_DIR}/lib/libpcfg_common.a
	${MOSES_INSTALL_DIR}/lib/libRuleTable.a
	${MOSES_INSTALL_DIR}/lib/libScope3Parser.a
	${LD_END_GROUP}
	${LIBRT_LIBRARIES}
	${ZLIB_LIBRARIES})

### Build Docent

link_directories(${Boost_LIBRARY_DIRS}
	${MOSES_LIBRARY_DIRS}
	${Arabica_LIBRARY_DIRS}
	${MPI_LIBRARY_DIRS}
)

include_directories(
	src
	${STEMMER_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
	${MOSES_INCLUDE_DIRS}
	${Arabica_INCLUDE_DIRS}
	${MPI_INCLUDE_DIRS}
)

add_library(
	decoder STATIC

	src/BeamSearchAdapter.cpp
	src/BleuModel.cpp
	src/ConsistencyQModelPhrase.cpp
	src/ConsistencyQModelWord.cpp
	src/CoolingSchedule.cpp
	src/DecoderConfiguration.cpp
	src/DocumentState.cpp
	src/FeatureFunction.cpp
	src/FinalWordRhymeModel.cpp
##	src/InitialCharModel.cpp
	src/LocalBeamSearch.cpp
	src/Logger.cpp
	src/MMAXDocument.cpp
	src/NbestStorage.cpp
	src/NgramModel.cpp
	src/NistXmlRefset.cpp
	src/NistXmlTestset.cpp
	src/OvixModel.cpp	
	src/PhrasePair.cpp
	src/PhrasePairCollection.cpp
	src/PhraseTable.cpp
	src/Random.cpp
##	src/ReverseNgramModel.cpp
	src/SearchAlgorithm.cpp
	src/SearchStep.cpp
	src/SemanticSpaceLanguageModel.cpp
	src/SentenceParityModel.cpp
##	src/SentenceInitialCharModel.cpp
##	src/SentenceFinalCharModel.cpp
##	src/SentenceFinalWordModel.cpp
	src/SimulatedAnnealing.cpp
	src/StateGenerator.cpp
##	src/TypeTokenRateModel.cpp
)

add_dependencies(decoder
	stemmer arabica moses)

set(DECODER_LIBRARIES
	decoder
	stemmer
	${Arabica_LIBRARIES}
	${MOSES_LIBRARIES}
	${Boost_LIBRARIES})

#add_executable(
#	docent
#	src/docent.cpp
#)

#target_link_libraries(
#	docent
#	${DECODER_LIBRARIES}
#)

#add_executable(
#	detailed-docent
#	src/detailed-docent.cpp
#)

#target_link_libraries(
#	detailed-docent
#	${DECODER_LIBRARIES}
#)

add_executable(
	lcurve-docent
	src/lcurve-docent.cpp
)

target_link_libraries(
	lcurve-docent
	${DECODER_LIBRARIES}
)

if(MPI_FOUND)
	add_executable(
		mpi-docent
		src/mpi-docent.cpp
	)

	if(MPI_COMPILE_FLAGS)
	  set_target_properties(mpi-docent PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
	endif()

	if(MPI_LINK_FLAGS)
	  set_target_properties(mpi-docent PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
	endif()

	target_link_libraries(
		mpi-docent
		${DECODER_LIBRARIES}
		${MPI_LIBRARIES}
	)
endif()
