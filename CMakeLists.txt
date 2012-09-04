cmake_minimum_required(VERSION 2.6)
project(docent)

set(CMAKE_CXX_FLAGS "-g -Wall -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(MPI)

set(Boost_NO_SYSTEM_PATHS TRUE)
if(MPI_FOUND)
	find_package(Boost 1.44 COMPONENTS mpi thread random log serialization regex REQUIRED)
else()
	find_package(Boost 1.44 COMPONENTS random log serialization regex REQUIRED)
endif()

find_package(Moses REQUIRED)
find_package(Arabica REQUIRED)

link_directories(${Boost_LIBRARY_DIRS}
	${MOSES_LIBRARY_DIRS}
	${Arabica_LIBRARY_DIRS}
	${MPI_LIBRARY_DIRS}
)

include_directories(
	src
	kenlm
	external/libstemmer_c/include
	${Boost_INCLUDE_DIRS}
	${MOSES_INCLUDE_DIRS}
	${Arabica_INCLUDE_DIRS}
	${MPI_INCLUDE_DIRS}
)

add_library(
	decoder STATIC

	src/BeamSearchAdapter.cpp
	src/CoolingSchedule.cpp
	src/DecoderConfiguration.cpp
	src/DocumentState.cpp
	src/FeatureFunction.cpp
	#src/LexicalChainCohesionModel.cpp
	src/LocalBeamSearch.cpp
	#src/MetropolisHastingsSampler.cpp
	src/MMAXDocument.cpp
	src/NbestStorage.cpp
	src/NgramModel.cpp
	#src/NgramModelIrstlm.cpp
	src/NistXmlTestset.cpp
	src/PhrasePair.cpp
	src/PhrasePairCollection.cpp
	src/PhraseTable.cpp
	#src/PronominalAnaphoraModel.cpp
	src/Random.cpp
	src/SearchAlgorithm.cpp
	src/SearchStep.cpp
	src/SemanticSpaceLanguageModel.cpp
	src/SentenceParityModel.cpp
	src/SimulatedAnnealing.cpp
	src/StateGenerator.cpp
	#src/WordSpaceCohesionModel.cpp
)

add_library(stemmer STATIC IMPORTED)
set_target_properties(stemmer PROPERTIES
	IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/libstemmer_c/libstemmer.o")

add_executable(
	docent
	src/docent.cpp
)

target_link_libraries(
	docent
	decoder
	#kenlm
	stemmer

	${MOSES_LIBRARIES}
	${Arabica_LIBRARIES}
	${Boost_LIBRARIES}
)

add_executable(
	lcurve-docent
	src/lcurve-docent.cpp
)

target_link_libraries(
	lcurve-docent
	decoder
	#kenlm
	stemmer

	${Boost_LIBRARIES}
	${MOSES_LIBRARIES}
	${Arabica_LIBRARIES}
)

#add_executable(
	#mh-sampler
	#src/mh-sampler.cpp
#)

#target_link_libraries(
	#mh-sampler
	#decoder
	#kenlm
	#stemmer

	#${Boost_LIBRARIES}
	#${MOSES_LIBRARIES}
	#${Arabica_LIBRARIES}
#)

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
		decoder
		#kenlm
		stemmer

		${Boost_LIBRARIES}
		${MOSES_LIBRARIES}
		${Arabica_LIBRARIES}
		${MPI_LIBRARIES}
	)
endif()
