# examples/MyModule/CMakeLists.txt
#
# This file is part of NEST.
#
# Copyright (C) 2004 The NEST Initiative
#
# NEST is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# NEST is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with NEST.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required( VERSION 2.8.12 )

# This CMakeLists.txt is configured to build your external module for NEST. For
# illustrative reasons this module is called 'my' (change SHORT_NAME to your
# preferred module name). NEST requires you to extend the 'SLIModule' (see
# mymodule.h and mymodule.cpp as an example) and provide a module header
# (see MODULE_HEADER). The subsequent instructions 
#
# The configuration requires a compiled and installed NEST; if `nest-config` is 
# not in the PATH, please specify the absolute path with `-Dwith-nest=...`.
#
# For more informations on how to extend and use your module see:
#           https://nest.github.io/nest-simulator/extension_modules

# 1) Name your module here, i.e. add later with -Dexternal-modules=my:
set( SHORT_NAME lookback )

#    the complete module name is here:
set( MODULE_NAME ${SHORT_NAME}module )

# 2) Add all your sources here
set( MODULE_SOURCES
    lookback_connector_model.h lookback_connector_model_impl.h
    lookback_exceptions.h lookback_exceptions.cpp
    lookback_node.h)

# 3) We require a header name like this:
set( MODULE_HEADER ${MODULE_NAME}.h )
# containing the class description of the class extending the SLIModule

# 4) Specify your module version
set( MODULE_VERSION_MAJOR 1 )
set( MODULE_VERSION_MINOR 0 )
set( MODULE_VERSION "${MODULE_VERSION_MAJOR}.${MODULE_VERSION_MINOR}" )

# 5) Leave the rest as is. All files in `sli` will be installed to
#    `share/nest/sli/`, so that NEST will find the during initialization.

# Leave the call to "project(...)" for after the compiler is determined.

# Set the `nest-config` executable to use during configuration.
set( with-nest OFF CACHE STRING "Specify the `nest-config` executable." )

# If it is not set, look for a `nest-config` in the PATH.
if ( NOT with-nest )
  # try find the program ourselves
  find_program( NEST_CONFIG
      NAMES nest-config
      )
  if ( NEST_CONFIG STREQUAL "NEST_CONFIG-NOTFOUND" )
    message( FATAL_ERROR "Cannot find the program `nest-config`. Specify via -Dwith-nest=... ." )
  endif ()
else ()
  set( NEST_CONFIG ${with-nest} )
endif ()

# Use `nest-config` to get the compile and installation options used with the
# NEST installation.

# Get the compiler that was used for NEST.
execute_process(
    COMMAND ${NEST_CONFIG} --compiler
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_COMPILER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# One check on first execution, if `nest-config` is working.
if ( NOT RES_VAR EQUAL 0 )
  message( FATAL_ERROR "Cannot run `${NEST_CONFIG}`. Please specify correct `nest-config` via -Dwith-nest=... " )
endif ()

# Setting the compiler has to happen before the call to "project(...)" function.
set( CMAKE_CXX_COMPILER "${NEST_COMPILER}" )

project( ${MODULE_NAME} CXX )

# Get the install prefix.
execute_process(
    COMMAND ${NEST_CONFIG} --prefix
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Use the `NEST_PREFIX` as `CMAKE_INSTALL_PREFIX`.
set( CMAKE_INSTALL_PREFIX "${NEST_PREFIX}" CACHE STRING "Install path prefix, prepended onto install directories." FORCE )

# Get the CXXFLAGS.
execute_process(
    COMMAND ${NEST_CONFIG} --cflags
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the Includes.
execute_process(
    COMMAND ${NEST_CONFIG} --includes
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_INCLUDES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if ( NEST_INCLUDES )
  # make a cmake list
  string( REPLACE " " ";" NEST_INCLUDES_LIST "${NEST_INCLUDES}" )
  foreach ( inc_complete ${NEST_INCLUDES_LIST} )
    # if it is actually a -Iincludedir
    if ( "${inc_complete}" MATCHES "^-I.*" )
      # get the directory
      string( REGEX REPLACE "^-I(.*)" "\\1" inc "${inc_complete}" )
      # and check whether it is a directory
      if ( IS_DIRECTORY "${inc}" )
        include_directories( "${inc}" )
      endif ()
    endif ()
  endforeach ()
endif ()

# Get, if NEST is build as a (mostly) static application. If yes, also only build
# static library.
execute_process(
    COMMAND ${NEST_CONFIG} --static-libraries
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_STATIC_LIB
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if ( NEST_STATIC_LIB )
  set( BUILD_SHARED_LIBS OFF )
else ()
  set( BUILD_SHARED_LIBS ON )
endif ()

# Get all linked libraries.
execute_process(
    COMMAND ${NEST_CONFIG} --libs
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the data install dir.
execute_process(
    COMMAND ${NEST_CONFIG} --datadir
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_DATADIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the documentation install dir.
execute_process(
    COMMAND ${NEST_CONFIG} --docdir
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_DOCDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the library install dir.
execute_process(
    COMMAND ${NEST_CONFIG} --libdir
    RESULT_VARIABLE RES_VAR
    OUTPUT_VARIABLE NEST_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# on OS X
set( CMAKE_MACOSX_RPATH ON )

include( GNUInstallDirs )

# When building shared libraries, also create a module for loading at runtime
# with the `Install` command.
# Build dynamic/static library for standard linking from NEST.
add_library( ${MODULE_NAME} ${MODULE_SOURCES})
if ( BUILD_SHARED_LIBS )
  # Dynamic libraries are initiated by a `global` variable of the `SLIModule`,
  # which is included, when the flag `LINKED_MODULE` is set.
endif ()
set_target_properties( ${MODULE_NAME}
    PROPERTIES
    COMPILE_FLAGS "${NEST_CXXFLAGS}"
    OUTPUT_NAME ${MODULE_NAME} )
separate_arguments(NEST_LIBS)
target_link_libraries(${MODULE_NAME} ${NEST_LIBS})
target_link_libraries(${MODULE_NAME} -Wl,--no-undefined)

configure_file(enable_nest_lookback.sh.in enable_nest_lookback.sh @ONLY)
