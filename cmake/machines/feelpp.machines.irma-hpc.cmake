###  TEMPLATE.txt.tpl; coding: utf-8 ---

#  Author(s): Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
#       Date: 2012-04-12
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
set(CMAKE_SYSTEM_PREFIX_PATH "/usr/local/dev;/usr")
set(Boost_LIBRARY_DIRS /usr/local/dev/lib )
set(Boost_NO_SYSTEM_PATHS  ON)
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/local/dev/lib ${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-L/usr/local/dev/lib ${CMAKE_SHARED_LINKER_FLAGS}")

set(FEELPP_ENABLE_MPI_MODE ON)