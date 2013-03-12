# -*- mode: cmake -*-
#
#  This file is part of the Feel++ library
#
#  Author(s): Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
#       Date: 2010-02-10
#
#  Copyright (C) 2010 Universit� Joseph Fourier
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 3.0 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# this files defines
#  - MATHEVAL_INCLUDE_DIR
#  - MATHEVAL_LIBRARIES
#  - MATHEVAL_FOUND

INCLUDE(CheckIncludeFileCXX)

# try to find libmatheval headers, if not found then install glog from contrib into
# build directory and set MATHEVAL_INCLUDE_DIR and MATHEVAL_LIBRARIES
FIND_PATH(MATHEVAL_INCLUDE_DIR matheval/matheval.h
  ${CMAKE_BINARY_DIR}/contrib/libmatheval/include
  $ENV{FEELPP_DIR}/include/feel/
  /opt/local/include /opt/local/include/matheval
  /usr/local/include /usr/local/include/matheval
  /usr/include /usr/include/matheval
  PATH_SUFFIXES
  feel
  )
message(STATUS "Libmatheval first pass: ${MATHEVAL_INCLUDE_DIR}")

if (NOT MATHEVAL_INCLUDE_DIR )
  execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/contrib/libmatheval-compile)
  if(${CMAKE_SOURCE_DIR}/contrib/libmatheval/configure IS_NEWER_THAN ${CMAKE_BINARY_DIR}/contrib/libmatheval-compile/Makefile)
    message(STATUS "Building libmatheval in ${CMAKE_BINARY_DIR}/contrib/libmatheval-compile...")
    if ( APPLE )
      execute_process(
        COMMAND ${FEELPP_HOME_DIR}/contrib/libmatheval/configure --prefix=${CMAKE_BINARY_DIR}/contrib/libmatheval --enable-tests=no LDFLAGS=-L/opt/local/lib
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/contrib/libmatheval-compile
        OUTPUT_QUIET
        OUTPUT_FILE "libmatheval-configure.log"
        )
    else()
      execute_process(
        COMMAND ${FEELPP_HOME_DIR}/contrib/libmatheval/configure --prefix=${CMAKE_BINARY_DIR}/contrib/libmatheval --enable-tests=no
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/contrib/libmatheval-compile
        OUTPUT_QUIET
        OUTPUT_FILE "libmatheval-configure.log"
        )
    endif()
  endif()
  if(${CMAKE_SOURCE_DIR}/contrib/libmatheval/lib/matheval.h IS_NEWER_THAN ${CMAKE_BINARY_DIR}/contrib/libmatheval/include/matheval.h)
    message(STATUS "Installing libmatheval in ${CMAKE_BINARY_DIR}/contrib/libmatheval (this may take a while)...")
    execute_process(
      COMMAND make -j${NProcs2} install
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/contrib/libmatheval-compile
      OUTPUT_QUIET
      OUTPUT_FILE "libmatheval-make.log"
      )
  endif()
  set(MATHEVAL_INCLUDE_DIR ${CMAKE_BINARY_DIR}/contrib/libmatheval/include)

endif ()


FIND_LIBRARY( MATHEVAL_LIB feelpp_matheval matheval
  PATHS
  $ENV{FEELPP_DIR}/lib
  ${CMAKE_BINARY_DIR}/contrib/libmatheval/lib/
  /usr/lib /opt/local/lib  $ENV{MATHEVAL_DIR}/lib)

if (MATHEVAL_LIB )
  FIND_LIBRARY( LIBFL NAMES fl REQUIRED)
endif ()

SET(MATHEVAL_LIBRARIES ${MATHEVAL_LIB} ${LIBFL} )
message(STATUS "Libmatheval includes: ${MATHEVAL_INCLUDE_DIR} Libraries: ${MATHEVAL_LIBRARIES}" )

# handle the QUIETLY and REQUIRED arguments and set GLOG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MATHEVAL "Could not find MATHEVAL " MATHEVAL_INCLUDE_DIR MATHEVAL_LIBRARIES)
MARK_AS_ADVANCED(MATHEVAL_INCLUDE_DIR MATHEVAL_LIBRARIES )
