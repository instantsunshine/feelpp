/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <prudhomme@unistra.fr>
       Date: 2013-07-05

  Copyright (C) 2013 Université de Strasbourg

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
   \file test_integration_ginac.cpp
   \author Christophe Prud'homme <prudhomme@unistra.fr>
   \date 2013-07-05
 */
#include <sstream>
#include <boost/timer.hpp>
#include <feel/feel.hpp>

int main(int argc, char**argv )
{
    using namespace Feel;
    Environment env( Feel::_argc=argc,
                     Feel::_argv=argv );

    auto mesh = loadMesh( _mesh=new Mesh<Simplex<3>> );
    //auto Xh = Pch<3>( mesh );
    auto area = integrate( boundaryfaces(mesh), cst(1.) ).evaluate()(0,0);
    auto bottom = integrate( markedfaces(mesh,"Bottom"), cst(1.) ).evaluate()(0,0);
    auto top = integrate( markedfaces(mesh,"Top"), cst(1.) ).evaluate()(0,0);
    auto right = integrate( markedfaces(mesh,"Right"), cst(1.) ).evaluate()(0,0);
    auto left = integrate( markedfaces(mesh,"Left"), cst(1.) ).evaluate()(0,0);
    auto front = integrate( markedfaces(mesh,"Front"), cst(1.) ).evaluate()(0,0);
    auto back = integrate( markedfaces(mesh,"Back"), cst(1.) ).evaluate()(0,0);

    CHECK( math::abs( top - 100 ) < 1e-10  ) << "Top = " << top << " should be 100 ";

    CHECK( math::abs( left - 100 ) < 1e-10  ) << "Left = " << left << " should be 100 ";
    CHECK( math::abs( front - 100 ) < 1e-10  ) << "Front = " << front << " should be 100 ";
    CHECK( math::abs( back - 100 ) < 1e-10  ) << "Back = " << back << " should be 100 ";

    CHECK( math::abs( right - 100 ) < 1e-10  ) << "Right = " << right << " should be 100 ";
    CHECK( math::abs( bottom - 100 ) < 1e-10  ) << "Bottom = " << bottom << " should be 100 ";

}