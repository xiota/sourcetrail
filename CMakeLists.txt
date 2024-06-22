cmake_minimum_required(VERSION 3.13)

if (CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake$")
	message("Vcpkg Build")
	set(isVcpkgBuild TRUE)
else()
	message("System Build")
	set(isVcpkgBuild FALSE)
endif()

if (isVcpkgBuild)
	# A condition in the 'CMakePresets.json' would be prefered, but seems not possible
	if (UNIX)
		set(VCPKG_TARGET_TRIPLET "custom-x64-linux-static-md" CACHE STRING "Target triplet for Linux")
	elseif(WIN32)
		set(VCPKG_TARGET_TRIPLET "custom-x64-windows-static-md" CACHE STRING "Target triplet for Windows")
	endif()
endif()

include(CMakePrintHelpers)

include(cmake/version.cmake)
include(cmake/version_setup.cmake)
include(cmake/licenses.cmake)

set(BUILD_CXX_LANGUAGE_PACKAGE OFF CACHE BOOL "Add C and C++ support to the Sourcetrail indexer.")
set(BUILD_JAVA_LANGUAGE_PACKAGE OFF CACHE BOOL "Add Java support to the Sourcetrail indexer.")

# prohibit in-source-builds
if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	message(STATUS "In-source-builds are not allowed")
	message(STATUS "Clean your source directory (e.g. delete the CMakeCache.txt file)")
	message(FATAL_ERROR "Please create a separate build directory and call CMake again")
endif()

# Make Ninja build verbose as well (https://github.com/ninja-build/ninja/issues/900):
if (CMAKE_VERBOSE_MAKEFILE)
	set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Verbose Makefile" FORCE)
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Variables --------------------------------------------------------------------

if (WIN32)
	set(PLATFORM_INCLUDE "includesWindows.h")
elseif (UNIX)
	set(PLATFORM_INCLUDE "includesLinux.h")
else()
	message(FATAL_ERROR "Unsupported platform!")
endif ()


# Project ----------------------------------------------------------------------

project(Sourcetrail)

# set Standard build type to Release
set(CMAKE_BUILD_TYPE_INIT "Release")

#[[
#RPATH
if(UNIX)
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/lib/:$$ORIGIN/lib/")
endif()
]]

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Settings ---------------------------------------------------------------------

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_compile_options(-Wno-unknown-warning-option)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_compile_options(/MP)
endif()

# For debugging the release build on linux
#if (UNIX AND "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	#add_definitions(-fno-omit-frame-pointer)
#endif ()


# Clang ------------------------------------------------------------------------

if (BUILD_CXX_LANGUAGE_PACKAGE)
	find_package(Clang REQUIRED)
	message(STATUS "Found LLVM ${LLVM_VERSION}")

	if (isVcpkgBuild)
		set(headerSourceDir "${LLVM_TOOLS_BINARY_DIR}/lib/clang/${LLVM_VERSION_MAJOR}/include")
	else()
		# V15: set(headerSourceDir "${LLVM_TOOLS_BINARY_DIR}/../lib/clang/${LLVM_VERSION}/include")
		set(headerSourceDir "${LLVM_TOOLS_BINARY_DIR}/../lib/clang/${LLVM_VERSION_MAJOR}/include")
	endif()
	set(headerTargetDir "${CMAKE_SOURCE_DIR}/bin/app/data/cxx/")
	
	message(STATUS "Copying LLVM header: ${headerSourceDir} -> ${headerTargetDir}")
	file(COPY "${headerSourceDir}" DESTINATION "${headerTargetDir}")
endif()


# Boost ------------------------------------------------------------------------

set(Boost_NO_WARN_NEW_VERSIONS ON)

if (isVcpkgBuild)
	set(Boost_NO_SYSTEM_PATHS ON)
endif()

find_package(Boost 1.74 REQUIRED COMPONENTS
	# compiled libraries:
	system program_options filesystem date_time locale

	# 'header-only' libraries which can't be found with find_package:
	# interprocess uuid asio process

	# See https://github.com/Kitware/CMake/blob/master/Modules/FindBoost.cmake#L1409
	# for a list of compiled libraries.
)
message(STATUS "Found Boost ${Boost_VERSION_STRING}")

# Qt ---------------------------------------------------------------------------

set (QT_MIN_VERSION "5.15.10")
set (QT_MIN_VERSION_HEX 0x05150A)
find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS Widgets PrintSupport Network Svg REQUIRED)

if (WIN32)
	find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS WinExtras REQUIRED)
endif()

message(STATUS "Found Qt ${Qt5Widgets_VERSION_STRING}")

# FIX: Qt was built with -reduce-relocations
if (Qt5_POSITION_INDEPENDENT_CODE)
	SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
add_definitions (-DQT_DEPRECATED_WARNINGS)

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=${QT_MIN_VERSION_HEX}    # disables all the APIs deprecated at or before the specified Qt version
add_definitions (-DQT_DISABLE_DEPRECATED_BEFORE=${QT_MIN_VERSION_HEX})

# External Libs  ------------------------------------------------------------------

add_library(External_lib_cxx)
add_subdirectory(src/external)

target_compile_options(External_lib_cxx
	PRIVATE
		-w
)

# SQLite3

if (isVcpkgBuild)
	find_package(unofficial-sqlite3 CONFIG REQUIRED)
	target_link_libraries(External_lib_cxx
		PUBLIC
			unofficial::sqlite3::sqlite3
	)
else()
	find_package(SQLite3 REQUIRED)
	message(STATUS "Found SQLite3 ${SQLite3_VERSION}")
	target_link_libraries(External_lib_cxx
		PUBLIC
			SQLite::SQLite3
	)
endif()

# TinyXML

if (UNIX)
	find_package(PkgConfig REQUIRED)
endif()

if (isVcpkgBuild)
	find_package(tinyxml CONFIG REQUIRED)
	target_link_libraries(External_lib_cxx
		PUBLIC
			unofficial-tinyxml::unofficial-tinyxml
	)
else()
	pkg_check_modules(tinyxml tinyxml IMPORTED_TARGET REQUIRED)
	message(STATUS "Found tinyxml ${tinyxml_VERSION}")
	target_link_libraries(External_lib_cxx
		PUBLIC
			PkgConfig::tinyxml
	)
endif()

# Lib Utility ------------------------------------------------------------------

add_library(Sourcetrail_lib_utility)
add_subdirectory(src/lib_utility)

target_link_libraries(Sourcetrail_lib_utility
	PUBLIC
		Boost::locale
		Qt5::Core
)

# Lib --------------------------------------------------------------------------

add_library(Sourcetrail_lib)
add_subdirectory(src/lib)

#configure language package defines
configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/language_packages.h.in"
	"${CMAKE_BINARY_DIR}/src/lib/language_packages.h"
)

target_include_directories(Sourcetrail_lib
	PUBLIC
		"${CMAKE_BINARY_DIR}/src/lib"
)

target_compile_definitions(Sourcetrail_lib
	PUBLIC
		# Fix "Boost-uuid should link against bcrypt on windows"
		# (https://github.com/microsoft/vcpkg/issues/4481)
		BOOST_UUID_FORCE_AUTO_LINK

		# Trying to add the definition to the boost target like this:
		# target_compile_definitions(Boost::uuid PUBLIC BOOST_UUID_FORCE_AUTO_LINK)
		# leads to the error:
		# "Cannot specify compile definitions for target "Boost::uuid" which is not built by this project."
)

target_link_libraries(Sourcetrail_lib
	PUBLIC
		Sourcetrail_lib_utility
		Sourcetrail_lib_gui
		External_lib_cxx
		$<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail_lib_cxx>
		$<$<BOOL:${BUILD_JAVA_LANGUAGE_PACKAGE}>:Sourcetrail_lib_java>

		Boost::headers
		Boost::system
		Boost::program_options
		Boost::filesystem
		Boost::date_time
)

if (UNIX)
	find_package(Threads REQUIRED)
	target_link_libraries(Sourcetrail_lib
		PUBLIC
			${CMAKE_DL_LIBS}
			Threads::Threads
	)
endif()

# Lib Cxx ----------------------------------------------------------------------

if (BUILD_CXX_LANGUAGE_PACKAGE)
	add_library(Sourcetrail_lib_cxx)
	add_subdirectory(src/lib_cxx)

	target_include_directories(Sourcetrail_lib_cxx SYSTEM
		PUBLIC
			${LLVM_INCLUDE_DIRS}
			${CLANG_INCLUDE_DIRS}
	)

	if (WIN32)
		target_compile_definitions(Sourcetrail_lib_cxx
			PRIVATE
				_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS # Due to Clang
		)
		target_link_libraries(Sourcetrail_lib_cxx
			PUBLIC
				version
		)
	endif()

	if(LLVM_LINK_LLVM_DYLIB)
		set(REQ_LLVM_LIBS LLVM)
	else()
		llvm_map_components_to_libnames(REQ_LLVM_LIBS
			support core libdriver passes option
		)
		foreach(LLVM_TARGET ${LLVM_TARGETS_TO_BUILD})
			get_property(lib_deps GLOBAL PROPERTY "LLVMBUILD_LIB_DEPS_LLVM${LLVM_TARGET}CodeGen")
			list(APPEND REQ_LLVM_LIBS "LLVM${LLVM_TARGET}CodeGen")
			list(APPEND REQ_LLVM_LIBS "${lib_deps}")
			get_property(lib_deps GLOBAL PROPERTY "LLVMBUILD_LIB_DEPS_LLVM${LLVM_TARGET}AsmParser")
			if (NOT "${lib_deps}" STREQUAL "")
				list(APPEND REQ_LLVM_LIBS "LLVM${LLVM_TARGET}AsmParser")
				list(APPEND REQ_LLVM_LIBS "${lib_deps}")
			endif()
		endforeach()
	endif()

	if(LLVM_LINK_LLVM_DYLIB)  # Should be CLANG_LINK_CLANG_DYLIB in future LLVM release
		set(CLANG_LIBRARIES clang-cpp)
	else()
		set(CLANG_LIBRARIES
			clangASTMatchers
			clangFrontend
			clangSerialization
			clangDriver
			clangTooling
			clangParse
			clangSema
			clangStaticAnalyzerFrontend
			clangStaticAnalyzerCheckers
			clangStaticAnalyzerCore
			clangAnalysis
			clangRewriteFrontend
			clangEdit
			clangAST
			clangLex
			clangBasic
		)
	endif()

	target_link_libraries(Sourcetrail_lib_cxx
		PUBLIC
			Sourcetrail_lib_utility
			Sourcetrail_lib
			External_lib_cxx
			${CLANG_LIBRARIES}
			${REQ_LLVM_LIBS}
	)

	set(CMAKE_CXX_STANDARD ${CAPTURED_CMAKE_CXX_STANDARD})
else()
	message(STATUS "Building the Cxx indexer will be skipped. You can enable building this target by setting 'BUILD_CXX_LANGUAGE_PACKAGE' to 'ON'.")
endif()


# Lib Java ---------------------------------------------------------------------

if (BUILD_JAVA_LANGUAGE_PACKAGE)

	find_package(Java 1.8 REQUIRED)
	message(STATUS "Found Java ${Java_VERSION}")
	
	find_package(JNI REQUIRED)
	message(STATUS "Found JNI ${JNI_VERSION}")

	find_program(MVN_COMMAND NAMES "mvn" REQUIRED)
	message(STATUS "Found Maven ${MVN_COMMAND}")

	add_library(Sourcetrail_lib_java)
	add_subdirectory(src/lib_java)

	target_link_libraries(Sourcetrail_lib_java
		PUBLIC
			Sourcetrail_lib_utility
			Sourcetrail_lib
			JNI::JNI
	)

	# Maven build (java-indexer.jar)

	add_custom_command(
		TARGET Sourcetrail_lib_java
		PRE_BUILD
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/java_indexer/"
		COMMAND ${MVN_COMMAND} package
		BYPRODUCTS "./target/java-indexer.jar"
		COMMENT "Building java indexer jars"
	)
	# TODO: This also copies unneeded maven directories (classes, generated-sources, ...)
	add_custom_command(
		TARGET Sourcetrail_lib_java
		POST_BUILD
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory "java_indexer/target/"  "bin/app/data/java/lib/"
		COMMENT "Copying java indexer jars"
	)
	# Workaround for above copy problem
	add_custom_command(
		TARGET Sourcetrail_lib_java
		POST_BUILD
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/app/data/java/lib/"
		COMMAND ${CMAKE_COMMAND} -E rm -rf "classes" "generated-sources" "maven-archiver" "maven-status" "test-classes"
		COMMENT "Deleting unneeded maven directories"
	)

else()
	message(STATUS "Building the Java indexer will be skipped. You can enable building this target by setting 'BUILD_JAVA_LANGUAGE_PACKAGE' to 'ON'.")
endif()

# Lib Gui ----------------------------------------------------------------------

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# target for running versionnumber script
# workaround for running customcommand (ninja dependency cycle)
add_custom_target(
	versionnumber ALL
)

add_library(Sourcetrail_lib_gui ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h)
add_subdirectory(src/lib_gui)

# configure platform specific include file
configure_file(
	"${PROJECT_SOURCE_DIR}/src/lib_gui/platform_includes/includes.h.in"
	"${PROJECT_BINARY_DIR}/src/lib_gui/includes.h"
)

#configure the versioning file
configure_file(
	${CMAKE_SOURCE_DIR}/cmake/version.txt.in
	${CMAKE_BINARY_DIR}/version.txt
)

configure_file(
	${CMAKE_SOURCE_DIR}/cmake/productVersion.h.in
	${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h
)

set_property(SOURCE ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h
	PROPERTY
		SKIP_AUTOMOC ON
)

target_include_directories(Sourcetrail_lib_gui
	PUBLIC
		"${CMAKE_BINARY_DIR}/src/lib_gui"
)

target_link_libraries(Sourcetrail_lib_gui
	PUBLIC
		Sourcetrail_lib_utility
		Sourcetrail_lib

		Qt5::Widgets
		Qt5::Network
		Qt5::Svg
)
if (WIN32)
	target_link_libraries(Sourcetrail_lib_gui
		PUBLIC
			Qt5::WinExtras
	)
endif()

# command for versioning script
add_custom_command(
	TARGET versionnumber
	PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${CMAKE_BINARY_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/version.cmake
	BYPRODUCTS ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h
	DEPENDS Sourcetrail_lib_gui
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "check/update version number"
)
add_dependencies(Sourcetrail_lib_gui versionnumber)

set(CMAKE_AUTOMOC OFF)

# Indexer App ------------------------------------------------------------------

add_executable(Sourcetrail_indexer)
add_subdirectory(src/indexer)

set_target_properties(Sourcetrail_indexer
	PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app/"
		PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/app/"
		OUTPUT_NAME              sourcetrail_indexer
)

if (WIN32)
	# hide the console when running a release build.
	set_target_properties(Sourcetrail_indexer PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE /DEBUG:FASTLINK")
	set_target_properties(Sourcetrail_indexer PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
	set_target_properties(Sourcetrail_indexer PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
	set_target_properties(Sourcetrail_indexer PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
	set_target_properties(Sourcetrail_indexer PROPERTIES LINK_FLAGS_RELEASE "/ENTRY:\"mainCRTStartup\" /SUBSYSTEM:WINDOWS /DEBUG")
	set_target_properties(Sourcetrail_indexer PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")

	# generate pdb for release build
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
endif ()

target_link_libraries(Sourcetrail_indexer
	PRIVATE
		Sourcetrail_lib_gui
		Sourcetrail_lib
		$<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail_lib_cxx>
		$<$<BOOL:${BUILD_JAVA_LANGUAGE_PACKAGE}>:Sourcetrail_lib_java>
)

# App --------------------------------------------------------------------------

add_executable(Sourcetrail)
add_subdirectory(src/app)

set_target_properties(Sourcetrail
	PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app/"
		PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/app/"
)
configure_file("${CMAKE_SOURCE_DIR}/setup/icon/windows/sourcetrail.ico" "${CMAKE_BINARY_DIR}/sourcetrail.ico" COPYONLY)

file(WRITE ${CMAKE_BINARY_DIR}/Sourcetrail.rc
	"// Icon with lowest ID value placed first to ensure application icon\n"
	"// remains consistent on all systems.\n"
	"IDI_ICON1               ICON                    \"${CMAKE_BINARY_DIR}/Sourcetrail.ico\"\n"
)

target_sources(Sourcetrail
	PRIVATE
		${CMAKE_BINARY_DIR}/Sourcetrail.rc
)

if (WIN32)
	# also show the console when running a release build.
	set_target_properties(Sourcetrail PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE /DEBUG:FASTLINK")
	set_target_properties(Sourcetrail PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
	set_target_properties(Sourcetrail PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
	set_target_properties(Sourcetrail PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
	set_target_properties(Sourcetrail PROPERTIES LINK_FLAGS_RELEASE "/ENTRY:\"mainCRTStartup\" /SUBSYSTEM:CONSOLE /DEBUG")
	set_target_properties(Sourcetrail PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")

	# generate pdb for release build
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")

	set_target_properties(Sourcetrail
		PROPERTIES
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/app"
	)
endif ()

target_link_libraries(Sourcetrail
	PRIVATE
		Sourcetrail_lib_gui
		Sourcetrail_lib
		$<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail_lib_cxx>
		$<$<BOOL:${BUILD_JAVA_LANGUAGE_PACKAGE}>:Sourcetrail_lib_java>
)

add_dependencies(Sourcetrail Sourcetrail_indexer)


# Test ----------------------------------------------------------------------

find_package(Catch2 CONFIG REQUIRED)
message(STATUS "Found Catch2 ${Catch2_VERSION}")

include(CTest)
include(Catch)

add_executable(Sourcetrail_test)
add_subdirectory(src/test)

set_target_properties(Sourcetrail_test
	PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
		PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/test/"
)

target_link_libraries(Sourcetrail_test
	PRIVATE
		Sourcetrail_lib
		Sourcetrail_lib_gui
		$<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail_lib_cxx>
		$<$<BOOL:${BUILD_JAVA_LANGUAGE_PACKAGE}>:Sourcetrail_lib_java>
		Catch2::Catch2
)

if (WIN32)
	target_compile_options(Sourcetrail_test
		PRIVATE
			/bigobj
	)
	set_target_properties(Sourcetrail_test
		PROPERTIES
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/test"
	)
endif ()

catch_discover_tests(Sourcetrail_test)

# symlinks for data
message(STATUS "Creating symlink: ${CMAKE_SOURCE_DIR}/bin/app/data -> ${CMAKE_BINARY_DIR}/app/data")

get_property(isMultiConfigGenerator GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(isMultiConfigGenerator)
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/app")
		file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/test")

		file(CREATE_LINK "${CMAKE_SOURCE_DIR}/bin/app/data" "${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/app/data" SYMBOLIC)
		file(CREATE_LINK "${CMAKE_SOURCE_DIR}/bin/app/user" "${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/app/user" SYMBOLIC)
		file(CREATE_LINK "${CMAKE_SOURCE_DIR}/bin/test/data" "${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/test/data" SYMBOLIC)
	endforeach()
else()
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/app")
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test")

	file(CREATE_LINK "${CMAKE_SOURCE_DIR}/bin/app/data" "${CMAKE_BINARY_DIR}/app/data" SYMBOLIC)
	file(CREATE_LINK "${CMAKE_SOURCE_DIR}/bin/app/user" "${CMAKE_BINARY_DIR}/app/user" SYMBOLIC)
	file(CREATE_LINK "${CMAKE_SOURCE_DIR}/bin/test/data" "${CMAKE_BINARY_DIR}/test/data" SYMBOLIC)
endif()
