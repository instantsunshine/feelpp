###  TEMPLATE.txt.tpl; coding: utf-8 ---

#  Author(s): Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
#       Date: 2012-05-27
#
#  Copyright (C) 2012 Universit� Joseph Fourier (Grenoble I)
#
# Distributed under the GPL(GNU Public License):
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
# GLOG_FOUND - system has GLOG
# GLOG_INCLUDE_DIR - headers location
# GLOG_LIBRARIES - libraries

# try to find glog headers, if not found then install glog from contrib into
# build directory and set GLOG_INCLUDE_DIR and GLOG_LIBRARIES
FIND_PACKAGE(GFLAGS)
FIND_PATH(GLOG_INCLUDE_DIR glog/logging.h
  ${CMAKE_BINARY_DIR}/contrib/glog/include
  $ENV{FEELPP_DIR}/include
  $ENV{FEELPP_DIR}/include/feel
  /usr/include/feel
  /usr/local/include/feel
  /opt/local/include/feel
  NO_DEFAULT_PATH
  )
message(STATUS "Glog first pass: ${GLOG_INCLUDE_DIR}")


if (NOT GLOG_INCLUDE_DIR )
  if(${CMAKE_SOURCE_DIR}/contrib/glog/configure.ac IS_NEWER_THAN ${CMAKE_BINARY_DIR}/contrib/glog-compile/configure)
    message(STATUS "Building glog in ${CMAKE_BINARY_DIR}/contrib/glog-compile...")
    execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/contrib/glog-compile)
    execute_process(
      COMMAND ${FEELPP_HOME_DIR}/contrib/glog/configure --prefix=${CMAKE_BINARY_DIR}/contrib/glog --with-gflags=${GFLAGS_DIR}/lib/libfeelpp_gflags.so
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/contrib/glog-compile
#      OUTPUT_QUIET
      OUTPUT_FILE "glog-configure"
      )
    set(GLOG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/contrib/glog/include)
  endif()
endif()

if ( EXISTS ${CMAKE_SOURCE_DIR}/contrib/glog/ )
  if ( (${CMAKE_SOURCE_DIR}/contrib/glog/src/glog/logging.h.in IS_NEWER_THAN ${CMAKE_BINARY_DIR}/contrib/glog/include/glog/logging.h) OR
      ( ${CMAKE_SOURCE_DIR}/contrib/glog/src/logging.cc IS_NEWER_THAN ${CMAKE_BINARY_DIR}/contrib/glog/include/glog/logging.h ) )
    message(STATUS "Installing glog in ${CMAKE_BINARY_DIR}/contrib/glog...")
    execute_process(
      COMMAND make -k -j${NProcs2} install
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/contrib/glog-compile
      OUTPUT_QUIET
      )
  endif()
endif()

FIND_LIBRARY(GLOG_LIBRARY
  NAMES feelpp_glog glog
  PATHS
  ${CMAKE_BINARY_DIR}/contrib/glog/lib64/
  ${CMAKE_BINARY_DIR}/contrib/glog/lib/
  $ENV{FEELPP_DIR}/lib
  NO_DEFAULT_PATH
  )
set(GLOG_LIBRARIES ${GLOG_LIBRARY})
message(STATUS "GLog includes: ${GLOG_INCLUDE_DIR} Libraries: ${GLOG_LIBRARIES}" )


# handle the QUIETLY and REQUIRED arguments and set GLOG_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (GLOG DEFAULT_MSG GLOG_INCLUDE_DIR GLOG_LIBRARIES )

mark_as_advanced (GLOG_INCLUDE_DIR GLOG_LIBRARIES GLOG_LIBRARY)
