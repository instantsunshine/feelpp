/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2012-09-18

  Copyright (C) 2012 Feel++ Consortium

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file feel.hpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2012-09-18
 */
#if !defined(FEELPP_FEEL_HPP)
#define FEELPP_FEEL_HPP 1

#include <boost/math/constants/constants.hpp>

#include <feel/options.hpp>


#include <feel/feelcore/environment.hpp>

#include <feel/feelcore/application.hpp>

#include <feel/feelalg/backend.hpp>

#include <feel/feeldiscr/functionspace.hpp>
#include <feel/feelpoly/lagrange.hpp>
#include <feel/feelpoly/crouzeixraviart.hpp>

#include <feel/feelvf/vf.hpp>

#include <feel/feeldiscr/operatorlinear.hpp>

#include <feel/feeldiscr/bdf2.hpp>

#include <feel/feeldiscr/projector.hpp>

#include <feel/feeldiscr/operatorinterpolation.hpp>
#include <feel/feeldiscr/operatorlagrangep1.hpp>

#include <ginac/ginac.h>
namespace Feel
{
using GiNaC::symbol;
using GiNaC::ex;
}

#include <feel/feelfilters/gmsh.hpp>

#include <feel/feelfilters/exporter.hpp>
#include <feel/feelfilters/geotool.hpp>

#endif /* FEELPP_FEEL_HPP */
