cmake_minimum_required(VERSION 3.10)
include(CMakeLists.local OPTIONAL)

option(UNITTESTS "Builds with unittests" ON)
option(VISUALTESTS "Builds with visual tests (src/tests)" ON)

option(GAMES "Builds with games (openworld)" ON)
option(OPENWORLD_SERVER "Builds with server (needs GAMES to be active)" ON)
option(OPENWORLD_CLIENT "Builds with client (needs GAMES to be active)" ON)

option(TOOLS "Builds with tools (voxedit, thumbnailer, voxconvert, mapview, aidebug)" ON)
option(VOXEDIT "Builds voxedit (needs TOOLS to be active)" ON)
option(THUMBNAILER "Builds thumbnailer (needs TOOLS to be active)" ON)
option(VOXCONVERT "Builds voxconvert (needs TOOLS to be active)" ON)
option(MAPVIEW "Builds mapview (needs TOOLS to be active)" ON)
option(AIDEBUG "Builds aidebug (needs TOOLS to be active)" ON)

option(USE_CCACHE "Use ccache" ON)
option(USE_GPROF "Use gprof - will become slow" OFF)
option(USE_GCOV "Use gcov - will become slow" OFF)
option(USE_DOXYGEN_CHECK "Use -Wdocumentation if available" OFF)
option(USE_CPPCHECK "Enable cppcheck" OFF)
option(USE_CLANG_TIDY "Enable Clang Tidy" OFF)
option(USE_STACKTRACES "Enable stacktraces" ON)

set(PKGDATADIR "" CACHE STRING "System directory to search for data files (must end on /)")
set(DB_PW "engine" CACHE STRING "The database password (openworld server)")
set(SERVER_PORT "11337" CACHE STRING "Port where the server is listening on (openworld server)")
set(HTTP_SERVER_PORT "8080" CACHE STRING "Port where the http server is listening on (openworld server)")
set(DOCKER_REGISTRY "" CACHE STRING "Docker registry")

set(ROOT_PROJECT_MAJOR_VERSION 0)
set(ROOT_PROJECT_MINOR_VERSION 0)
set(ROOT_PROJECT_MICRO_VERSION 22)
set(ROOT_PROJECT_PATCH_VERSION 0)
set(ROOT_PROJECT_VERSION "${ROOT_PROJECT_MAJOR_VERSION}.${ROOT_PROJECT_MINOR_VERSION}.${ROOT_PROJECT_MICRO_VERSION}.${ROOT_PROJECT_PATCH_VERSION}")
set(ROOT_PROJECT_NAME vengi)
project(${ROOT_PROJECT_NAME} VERSION ${ROOT_PROJECT_VERSION})
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Root dir")
set(SCRIPTS_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake)
set(GENERATE_DIR ${CMAKE_BINARY_DIR}/generated CACHE STRING "Generated code dir")
add_custom_target(codegen)

include(CMakeParseArguments)
include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)
include(CheckIncludeFiles)
include(GNUInstallDirs)

if (CMAKE_COMPILER_IS_GNUCC)
	set(USE_GCC TRUE)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
	set(USE_CLANG TRUE)
elseif (MSVC)
	set(USE_MSVC TRUE)
endif()

include(${SCRIPTS_CMAKE_DIR}/common.cmake)
include(${SCRIPTS_CMAKE_DIR}/util.cmake)
include(${SCRIPTS_CMAKE_DIR}/genshader.cmake)
include(${SCRIPTS_CMAKE_DIR}/gencomputeshader.cmake)
include(${SCRIPTS_CMAKE_DIR}/gendbmodel.cmake)
include(${SCRIPTS_CMAKE_DIR}/genprotocol.cmake)
include(${SCRIPTS_CMAKE_DIR}/externallib.cmake)
include(${SCRIPTS_CMAKE_DIR}/valgrind.cmake)
include(${SCRIPTS_CMAKE_DIR}/perf.cmake)
include(${SCRIPTS_CMAKE_DIR}/heaptrack.cmake)
include(${SCRIPTS_CMAKE_DIR}/apitrace.cmake)
include(${SCRIPTS_CMAKE_DIR}/debug.cmake)
include(${SCRIPTS_CMAKE_DIR}/googletest.cmake)
include(${SCRIPTS_CMAKE_DIR}/manifests.cmake)
include(${SCRIPTS_CMAKE_DIR}/docker.cmake)
include(${SCRIPTS_CMAKE_DIR}/lua.cmake)
include(${SCRIPTS_CMAKE_DIR}/macros.cmake)
include(${SCRIPTS_CMAKE_DIR}/pack.cmake)
include(CPack)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(WINDOWS 1)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	set(LINUX 1)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(DARWIN 1)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(LINUX 1)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
	set(EMSCRIPTEN 1)
else()
	message(WARNING "Unknown host system: ${CMAKE_SYSTEM_NAME}. Default to linux")
	set(LINUX 1)
endif()

if (NOT CMAKE_TOOLCHAIN_FILE)
	if (MSVC)
		include(${SCRIPTS_CMAKE_DIR}/toolchains/msvc-toolchain.cmake)
	elseif (WINDOWS)
		include(${SCRIPTS_CMAKE_DIR}/toolchains/windows-toolchain.cmake)
	elseif (DARWIN)
		include(${SCRIPTS_CMAKE_DIR}/toolchains/darwin-toolchain.cmake)
	else()
		include(${SCRIPTS_CMAKE_DIR}/toolchains/linux-toolchain.cmake)
	endif()
else()
	if (MSVC)
		# 4456: declaration of 'q' hides previous local declaration
		# 4244: possible loss of data (e.g. float/double)
		# 4201: nonstandard extension used
		# 4245: signed/unsigned mismatch
		# 4100: unreferenced formal parameter
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT /wd4244 /wd4245 /wd4201 /wd4100 /wd4456 /wd4267")
	endif()
endif()

if (MSVC)
	foreach(flag_var
		CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		if (${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endif()
	endforeach()

	foreach(flag_var
		CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		string(REGEX REPLACE "/RTC(su|[1su])" "" ${flag_var} "${${flag_var}}")
	endforeach(flag_var)
endif()

# First for the generic no-config case (e.g. with mingw)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)

# Second, for multi-config builds (e.g. msvc)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
endforeach()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${SCRIPTS_CMAKE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_host_program(CCACHE "ccache")
if (CCACHE)
	if (USE_CCACHE)
		message(STATUS "Using ccache")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
		if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
			set(ENV{CCACHE_CPP2} "yes")
			add_compile_options(-Qunused-arguments)
		endif()
	endif()
else()
	message(STATUS "ccache not found")
endif()

find_host_program(GDB_EXECUTABLE NAMES ${GDB_BINARY} gdb)
find_host_program(LLDB_EXECUTABLE NAMES ${LLDB_BINARY} lldb)
check_include_files("uuid/uuid.h" HAVE_UUID_H)

set(CURSES_NEED_NCURSES True)
find_package(Curses)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(HOT_RELOAD_ANIM 0)

# These includes are needed to let the include for IMPLICIT_DEPENDS for shaders work
include_directories(src/modules/video/shaders)
include_directories(src/modules/compute/shaders)

include_directories(src/modules)

add_subdirectory(contrib/libs)
add_subdirectory(tools)

if (UNITTESTS)
	enable_testing()
	set(CTEST_TEST_TIMEOUT 1800)
	gtest_suite_begin(tests TEMPLATE src/modules/core/tests/main.cpp.in)
endif()
add_subdirectory(src)
if (UNITTESTS)
	gtest_suite_end(tests)
endif()

configure_file(src/engine-config.h.in engine-config.h @ONLY)
message(STATUS "Generate engine-config.h in ${CMAKE_CURRENT_BINARY_DIR}")

find_package(Doxygen)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile-mcss.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile-mcss @ONLY)
if (DOXYGEN_FOUND)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
		DEPENDS codegen
	)
endif(DOXYGEN_FOUND)
