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
	set(Arabica_WITH_BOOST "--with-boost=${BOOST_ROOT}/include")
else()
	message("BOOST_ROOT isn't set. If you encounter build problems, consider setting it.")
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

find_package(ZLIB REQUIRED)

check_library_exists(-lrt clock_gettime "" HAVE_LIBRT)
if(HAVE_LIBRT)
	set(LIBRT_LIBRARIES -lrt)
else()
	set(LIBRT_LIBRARIES "")
endif()


### Build libstemmer_c
set(STEMMER_ROOT ${CMAKE_SOURCE_DIR}/external/libstemmer_c)
set(STEMMER_INSTALL_DIR ${CMAKE_BINARY_DIR}/external/libstemmer_c)

ExternalProject_add(stemmer_external
	SOURCE_DIR ${STEMMER_ROOT}
	CONFIGURE_COMMAND true
	BUILD_COMMAND ${MAKE}
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STEMMER_ROOT}/libstemmer.o ${STEMMER_INSTALL_DIR}
	INSTALL_DIR ${STEMMER_INSTALL_DIR}
	BUILD_IN_SOURCE 1
)

add_library(stemmer STATIC IMPORTED)
add_dependencies(stemmer stemmer_external)
set_target_properties(stemmer PROPERTIES
	IMPORTED_LOCATION "${STEMMER_INSTALL_DIR}/libstemmer.o"
)

set(STEMMER_INCLUDE_DIRS ${STEMMER_ROOT}/include)


### Build Arabica
set(Arabica_ROOT ${CMAKE_SOURCE_DIR}/external/arabica)
set(Arabica_LIBRARIES arabica)
set(BUILD_ARABICA_EXAMPLES OFF)

add_subdirectory(${Arabica_ROOT})
get_property(Arabica_INCLUDE_DIRS DIRECTORY external/arabica PROPERTY INCLUDE_DIRECTORIES)


### Build KenLM
add_definitions(-DKENLM_MAX_ORDER=${KENLM_MAX_ORDER})
set(KenLM_ROOT ${CMAKE_SOURCE_DIR}/external/kenlm)
set(BUILD_TESTING OFF)

include_directories(${KenLM_ROOT})
add_subdirectory(${KenLM_ROOT})


### Build ProbingPT
set(ProbingPT_ROOT ${CMAKE_SOURCE_DIR}/external/ProbingPT)

include_directories(${ProbingPT_ROOT})
add_library(probingpt STATIC
	${ProbingPT_ROOT}/hash.cpp
	${ProbingPT_ROOT}/huffmanish.cpp
	${ProbingPT_ROOT}/line_splitter.cpp
	${ProbingPT_ROOT}/probing_hash_utils.cpp
	${ProbingPT_ROOT}/quering.cpp
	${ProbingPT_ROOT}/storing.cpp
	${ProbingPT_ROOT}/vocabid.cpp
)
add_dependencies(probingpt
	kenlm
	kenlm_util
)


### Build Docent
link_directories(
	${Boost_LIBRARY_DIRS}
	${Arabica_LIBRARY_DIRS}
	${MPI_LIBRARY_DIRS}
)
include_directories(
	src
	${STEMMER_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
	${Arabica_INCLUDE_DIRS}
	${KenLM_ROOT}
	${ProbingPT_ROOT}
	${MPI_INCLUDE_DIRS}
)
add_library(decoder STATIC
	src/BeamSearchAdapter.cpp
	src/CoolingSchedule.cpp
	src/DecoderConfiguration.cpp
	src/DocumentState.cpp
	src/FeatureFunction.cpp
	src/LocalBeamSearch.cpp
	src/Logger.cpp
	src/MMAXDocument.cpp
	src/NbestStorage.cpp
	src/NistXmlDocument.cpp
	src/NistXmlRefset.cpp
	src/NistXmlTestset.cpp
	src/PhrasePair.cpp
	src/PhrasePairCollection.cpp
	src/PhraseTable.cpp
	src/Random.cpp
	src/SearchAlgorithm.cpp
	src/SearchStep.cpp
	src/SimulatedAnnealing.cpp
	src/StateGenerator.cpp
	src/StateOperation.cpp
	src/models/BleuModel.cpp
	src/models/BracketingModel.cpp
	src/models/ConsistencyQModelPhrase.cpp
	src/models/ConsistencyQModelWord.cpp
	src/models/GappyLanguageModel.cpp
	src/models/NgramModel.cpp
	src/models/OvixModel.cpp
	src/models/SemanticSimilarityModel.cpp
	src/models/SemanticSpaceLanguageModel.cpp
	src/models/SentenceParityModel.cpp
	src/models/TypeTokenRateModel.cpp
	src/models/WellFormednessModel.cpp
)
add_dependencies(decoder
	stemmer
	arabica
	kenlm
	kenlm_builder
	kenlm_filter
	kenlm_interpolate
	kenlm_util
)

set(DECODER_LIBRARIES
	decoder
	stemmer
	kenlm
	kenlm_builder
	kenlm_filter
	kenlm_interpolate
	kenlm_util
	probingpt
	${Arabica_LIBRARIES}
	${Boost_LIBRARIES}
)

add_executable(docent
	src/docent.cpp
	src/PhrasePair.StreamOperators-normal.cpp
)
target_link_libraries(docent
	${DECODER_LIBRARIES}
)

add_executable(docent-test
	src/docent-test.cpp
	src/PhrasePair.StreamOperators-normal.cpp
)
target_link_libraries(docent-test
	${DECODER_LIBRARIES}
)

add_executable(detailed-docent
	src/detailed-docent.cpp
	src/PhrasePair.StreamOperators-detailed.cpp
)
target_link_libraries(detailed-docent
	${DECODER_LIBRARIES}
)

add_executable(lcurve-docent
	src/lcurve-docent.cpp
	src/PhrasePair.StreamOperators-normal.cpp
)
target_link_libraries(lcurve-docent
	${DECODER_LIBRARIES}
)

if(MPI_FOUND)
	add_executable(mpi-docent
		src/mpi-docent.cpp
		src/PhrasePair.StreamOperators-normal.cpp
	)

	if(MPI_COMPILE_FLAGS)
		set_target_properties(mpi-docent PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
	endif()

	if(MPI_LINK_FLAGS)
		set_target_properties(mpi-docent PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
	endif()

	target_link_libraries(mpi-docent
		${DECODER_LIBRARIES}
		${MPI_LIBRARIES}
	)
endif()
