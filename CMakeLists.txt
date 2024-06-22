cmake_minimum_required(VERSION 3.27)

# Detect vcpkg build scenario here, so we don't depend on CMakePresets settings:
if (CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake$")
	message(STATUS "Vcpkg Build")
	set(isVcpkgBuild TRUE)
else()
	message(STATUS "System Build")
	set(isVcpkgBuild FALSE)
endif()

if (isVcpkgBuild)
	# A condition in the 'CMakePresets.json' would be prefered, but seems not possible
	if (UNIX)
		set(VCPKG_TARGET_TRIPLET "custom-x64-linux-static-md" CACHE STRING "Target triplet for Linux" FORCE)
	elseif(WIN32)
		set(VCPKG_TARGET_TRIPLET "custom-x64-windows-static-md" CACHE STRING "Target triplet for Windows" FORCE)
	endif()
endif()

include(CMakePrintHelpers)

# Configure which language(s)/package(s) should be supported:
option(BUILD_CXX_LANGUAGE_PACKAGE    "Add C/C++ support to the Sourcetrail indexer." OFF)
option(BUILD_JAVA_LANGUAGE_PACKAGE   "Add Java support to the Sourcetrail indexer." OFF)
option(BUILD_PYTHON_LANGUAGE_PACKAGE "Add Python support to the Sourcetrail indexer." OFF)
option(BUILD_UNIT_TESTS_PACKAGE      "Build the corresponding language unit tests." OFF)

configure_file(cmake/language_packages.h.in src/lib/language_packages.h)

# Create license information:
include(cmake/licenses.cmake)
configureLicenseFile(src/lib_gui/licenses.h)

# prohibit in-source-builds
if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	message(FATAL_ERROR "In-source-builds are strongly discouraged and not supported!")
endif()

# Make Ninja build verbose as well (https://github.com/ninja-build/ninja/issues/900):
if (CMAKE_VERBOSE_MAKEFILE)
	set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Verbose Makefile" FORCE)
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Project ----------------------------------------------------------------------

project(Sourcetrail VERSION 2024.8.2)
configure_file(cmake/productVersion.h.in src/lib_gui/productVersion.h)

include(cmake/Sourcetrail.cmake)

# set Standard build type to Release
set(CMAKE_BUILD_TYPE_INIT "Release")

#[[
#RPATH
if (UNIX)
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/lib/:$$ORIGIN/lib/")
endif()
]]

# Specify the requested standards:
# https://cmake.org/cmake/help/latest/prop_tgt/C_STANDARD.html
# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Boost ------------------------------------------------------------------------

set(Boost_NO_WARN_NEW_VERSIONS ON)

if (isVcpkgBuild)
	# TODO: Investigate bug entry: https://gitlab.kitware.com/cmake/cmake/-/issues/21200 under Linux.
	set(Boost_NO_SYSTEM_PATHS ON)
endif()

find_package(Boost 1.83 REQUIRED COMPONENTS
	# compiled libraries:
	system program_options filesystem date_time locale

	# 'header-only' libraries which can't be found with find_package:
	# interprocess uuid asio process predef dll

	# See https://github.com/Kitware/CMake/blob/master/Modules/FindBoost.cmake#L1409
	# for a list of compiled libraries.
)
message(STATUS "Found Boost ${Boost_VERSION}")

add_library(External_lib_boost INTERFACE)

target_compile_definitions(External_lib_boost
	INTERFACE
		# Fix "Boost-uuid should link against bcrypt on windows"
		# (https://github.com/microsoft/vcpkg/issues/4481)
		BOOST_UUID_FORCE_AUTO_LINK

		# If boost::filesystem gets replaced with std::filesystem, then this symbol must be defined:
		#BOOST_DLL_USE_STD_FS

		# Trying to add the definition to the boost target like this:
		# target_compile_definitions(Boost::uuid PUBLIC BOOST_UUID_FORCE_AUTO_LINK)
		# leads to the error:
		# "Cannot specify compile definitions for target "Boost::uuid" which is not built by this project."
)

# Qt ---------------------------------------------------------------------------

set (QT_MIN_VERSION "6.4.2")
set (QT_MIN_VERSION_HEX 0x060402)
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
	Widgets PrintSupport Network Svg Core5Compat
)
message(STATUS "Found Qt ${Qt6_VERSION}")

# SQLite3 ----------------------------------------------------------------------

if (isVcpkgBuild)
	find_package(unofficial-sqlite3 CONFIG REQUIRED)

	add_library(External_lib_sqlite3 ALIAS unofficial::sqlite3::sqlite3)
else()
	find_package(SQLite3 REQUIRED)
	message(STATUS "Found SQLite3 ${SQLite3_VERSION}")

	add_library(External_lib_sqlite3 ALIAS SQLite::SQLite3)
endif()

# TinyXML ----------------------------------------------------------------------

if (isVcpkgBuild)
	find_package(tinyxml CONFIG REQUIRED)

	add_library(External_lib_tinyxml ALIAS unofficial-tinyxml::unofficial-tinyxml)
else()
	if (UNIX)
		find_package(PkgConfig REQUIRED)
	endif()

	pkg_check_modules(tinyxml tinyxml IMPORTED_TARGET REQUIRED)
	message(STATUS "Found tinyxml ${tinyxml_VERSION}")

	add_library(External_lib_tinyxml ALIAS PkgConfig::tinyxml)
endif()

# External Lib CppSQlite3  ------------------------------------------------------------------

add_subdirectory(src/external)

# Lib Utility ------------------------------------------------------------------

add_subdirectory(src/lib_utility)

# Lib Gui ----------------------------------------------------------------------

add_subdirectory(src/lib_gui)

# Lib --------------------------------------------------------------------------

if (UNIX)
	find_package(Threads REQUIRED)
endif()

add_subdirectory(src/lib)

#
# Resolve cyclic dependencies between Sourcetrail_lib and Sourcetrail_lib_gui:
#

if (NOT isVcpkgBuild)
	set_target_properties(Sourcetrail_lib
		PROPERTIES
			LINK_INTERFACE_MULTIPLICITY 3
	)

	set_target_properties(Sourcetrail_lib_gui
		PROPERTIES
			LINK_INTERFACE_MULTIPLICITY 3
	)
endif()

# Lib Cxx ----------------------------------------------------------------------

if (BUILD_CXX_LANGUAGE_PACKAGE)

	# Clang:

	find_package(Clang 18.1...<19.0 REQUIRED)
	message(STATUS "Found LLVM ${LLVM_VERSION}")

	if (isVcpkgBuild)
		set(headerSourceDir "${LLVM_TOOLS_BINARY_DIR}/lib/clang/${LLVM_VERSION_MAJOR}/include/")
	else()
		set(headerSourceDir "${LLVM_TOOLS_BINARY_DIR}/../lib/clang/${LLVM_VERSION_MAJOR}/include/")
	endif()
	set(headerTargetDir "app/data/cxx/include/")

	message(STATUS "Copying LLVM header: ${headerSourceDir} -> ${headerTargetDir}")
	file(COPY "${headerSourceDir}" DESTINATION "${headerTargetDir}")

	add_library(External_lib_clang INTERFACE)
	target_compile_definitions(External_lib_clang
		INTERFACE
			${LLVM_DEFINITIONS}
	)
	target_include_directories(External_lib_clang SYSTEM
		INTERFACE
			${LLVM_INCLUDE_DIRS}
			${CLANG_INCLUDE_DIRS}
	)

	if (WIN32)
		target_compile_definitions(External_lib_clang
			PRIVATE
				_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS # Due to Clang
		)
		target_link_libraries(External_lib_clang
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

	target_link_libraries(External_lib_clang
		INTERFACE
			Sourcetrail_lib_utility
			Sourcetrail_lib
			${CLANG_LIBRARIES}
			${REQ_LLVM_LIBS}
	)

	add_subdirectory(src/lib_cxx)
else()
	message(STATUS "Building the Cxx indexer will be skipped. You can enable building this target by setting 'BUILD_CXX_LANGUAGE_PACKAGE' to 'ON'.")
endif()

# Lib Java ---------------------------------------------------------------------

if (BUILD_JAVA_LANGUAGE_PACKAGE)
	# Java:
	find_package(Java 1.8 REQUIRED)
	message(STATUS "Found Java ${Java_VERSION}")

	# JNI:
	find_package(JNI REQUIRED)
	message(STATUS "Found JNI ${JNI_VERSION}")

	# Maven:
	find_program(MVN_COMMAND NAMES "mvn" REQUIRED)
	message(STATUS "Found Maven ${MVN_COMMAND}")

	add_subdirectory(src/lib_java)
else()
	message(STATUS "Building the Java indexer will be skipped. You can enable building this target by setting 'BUILD_JAVA_LANGUAGE_PACKAGE' to 'ON'.")
endif()

# Lib Python -------------------------------------------------------------------

if (BUILD_PYTHON_LANGUAGE_PACKAGE)
	# Bash:
	find_program(BASH_COMMAND NAMES "bash" REQUIRED)
	message(STATUS "Found Bash ${BASH_COMMAND}")

	add_subdirectory(src/lib_python)
else()
	message(STATUS "Building the Python indexer will be skipped. You can enable building this target by setting 'BUILD_PYTHON_LANGUAGE_PACKAGE' to 'ON'.")
endif()

# Indexer App ------------------------------------------------------------------

add_subdirectory(src/indexer)

# App --------------------------------------------------------------------------

add_subdirectory(src/app)

# Test ----------------------------------------------------------------------

if (BUILD_UNIT_TESTS_PACKAGE)
	find_package(Catch2 3.4 CONFIG REQUIRED)
	message(STATUS "Found Catch2 ${Catch2_VERSION}")

	include(CTest)
	include(Catch)

	add_subdirectory(src/test)
else()
	message(STATUS "Building the unit tests will be skipped. You can enable building this target by setting 'BUILD_UNIT_TESTS_PACKAGE' to 'ON'.")
endif()

# Installing/Packaging ---------------------------------------------------------

install(DIRECTORY "${CMAKE_BINARY_DIR}/app/" DESTINATION "${PROJECT_NAME}/app/" USE_SOURCE_PERMISSIONS)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Source code explorer")
set(CPACK_PACKAGE_DESCRIPTION "Free and open-source cross-platform source explorer that helps you get productive on unfamiliar source code.")

set(CPACK_PACKAGE_CONTACT "P. Most <pmost@pera-software.com>")
set(CPACK_PACKAGE_VENDOR "PERA Software Solutions GmbH")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/petermost/Sourcetrail")
set(CPACK_PACKAGE_ICON "${CMAKE_BINARY_DIR}/app/Sourcetrail.ico")

if (isVcpkgBuild)
	set(CPACK_GENERATOR ZIP)
else()
	if (UNIX)
		set(CPACK_GENERATOR DEB)
		set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
		set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/")
	else()
		set(CPACK_GENERATOR ZIP)
	endif()
endif()

include(CPack)
