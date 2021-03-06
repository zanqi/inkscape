# CMake TODO
# - Remove hard coded defines (see bwelow)
# - Test on MinGW and OSX
# - Add configurable options for Python/Perl/Whiteboard, see configure --help
#
# ideasman42

cmake_minimum_required(VERSION 2.8.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeScripts/Modules")

# avoid having empty buildtype
set(CMAKE_BUILD_TYPE_INIT "Release")

project(inkscape)

set(INKSCAPE_VERSION 0.48+devel)
set(PROJECT_NAME inkscape)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
	cmake_policy(SET CMP0005 NEW) # propper define quoting
endif()

# workaround for omission in cmake 2.8.4's GNU.cmake, fixed in 2.8.5
if(CMAKE_COMPILER_IS_GNUCC)
	if(NOT DARWIN)
		set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem ")
	endif()
endif()

#-----------------------------------------------------------------------------
# Redirect output files

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "" FORCE )
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "" FORCE )

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(WITH_DBUS "Compile with support for DBus interface" OFF)
option(ENABLE_LCMS "Compile with LCMS support" OFF)
option(WITH_GNOME_VFS "Compile with support for Gnome VFS" OFF)
#option(WITH_INKJAR "Enable support for openoffice files (SVG jars)" ON)

option(WITH_PROFILING "Turn on profiling" OFF) # Set to true if compiler/linker should enable profiling

option(WITH_GTKSPELL "Compile with support for GTK spelling widget" ON)
option(WITH_LIBWPG "Compile with support of libpoppler-cairo for WordPrefect Graphics" ON)
option(ENABLE_POPPLER "Compile with support of libpoppler" ON)
option(ENABLE_POPPLER_CAIRO "Compile with support of libpoppler-cairo for rendering PDF preview (depends on ENABLE_POPPLER)" ON)

include(CMakeScripts/ConfigPaths.cmake) # Installation Paths
include(CMakeScripts/DefineDependsandFlags.cmake) # Includes, Compiler Flags, and Link Libraries
include(CMakeScripts/HelperMacros.cmake) # Misc Utility Macros



# -----------------------------------------------------------------------------
# BAD HACKS, NEED TO INVESTIGATE MAKING THESE LESS BAD
add_definitions(-D_FORTIFY_SOURCE=2)
add_definitions(-DPOTRACE="potrace")
add_definitions(-DORBIT2=1)
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DHAVE_CAIRO_PDF=1)  # needed for src/libnrtype/Layout-TNG.h
add_definitions(-DHAVE_TR1_UNORDERED_SET) # XXX make an option!
#
# end badness
# -----------------------------------------------------------------------------


# make dist target
set(INKSCAPE_DIST_PREFIX "${PROJECT_NAME}-${INKSCAPE_VERSION}")
add_custom_target(dist svn export --force -q "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}" 
		COMMAND tar -czf "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.tar.gz" -C "${CMAKE_BINARY_DIR}" --exclude=".hidden" ${INKSCAPE_DIST_PREFIX}
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# make unistall target
configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE @ONLY)

configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/inkscape.desktop.in"
		"${CMAKE_BINARY_DIR}/inkscape.desktop"
		IMMEDIATE @ONLY)

add_custom_target(uninstall
		"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


# -----------------------------------------------------------------------------
# Subdirs (on only 1 atm), but a lot happens here
# -----------------------------------------------------------------------------

add_subdirectory(src)


# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------

if(UNIX)
	# TODO: man, locale, icons

	# message after building.
	add_custom_command(
		TARGET blender POST_BUILD MAIN_DEPENDENCY blender
		COMMAND ${CMAKE_COMMAND} -E echo 'now run: \"make install\" to copy runtime files & scripts to ${CMAKE_INSTALL_PREFIX}'
	)

	install(
		PROGRAMS ${EXECUTABLE_OUTPUT_PATH}/inkscape
		DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	)

	install(
		FILES ${CMAKE_BINARY_DIR}/inkscape.desktop
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications
	)

	install(
		DIRECTORY
			${CMAKE_SOURCE_DIR}/share/attributes
			${CMAKE_SOURCE_DIR}/share/branding
			${CMAKE_SOURCE_DIR}/share/examples
			${CMAKE_SOURCE_DIR}/share/extensions
			${CMAKE_SOURCE_DIR}/share/filters
			${CMAKE_SOURCE_DIR}/share/fonts
			${CMAKE_SOURCE_DIR}/share/gradients
			${CMAKE_SOURCE_DIR}/share/icons
			${CMAKE_SOURCE_DIR}/share/keys
			${CMAKE_SOURCE_DIR}/share/markers
			${CMAKE_SOURCE_DIR}/share/palettes
			${CMAKE_SOURCE_DIR}/share/patterns
			${CMAKE_SOURCE_DIR}/share/screens
			${CMAKE_SOURCE_DIR}/share/symbols
			${CMAKE_SOURCE_DIR}/share/templates
			${CMAKE_SOURCE_DIR}/share/tutorials
			${CMAKE_SOURCE_DIR}/share/ui
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/inkscape
	)

else()
	# TODO, WIN32/APPLE
endif()
