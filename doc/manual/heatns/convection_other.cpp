/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
       Date: 2009-03-04

  Copyright (C) 2009 Universite Joseph Fourier (Grenoble I)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3.0 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file convection_other.cpp
   \author Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
   \date 2009-03-04
 */
#include <boost/lexical_cast.hpp>

#include "convection.hpp"

// Gmsh geometry/mesh generator
#include <feel/feelfilters/gmsh.hpp>

// gmsh importer
#include <feel/feelfilters/gmsh.hpp>





// ****** CONSTRUCTEURS ****** //
// <int Order_s, int Order_p, int Order_t>
Convection::Convection( int argc,
        char **argv,
        AboutData const& ad,
        po::options_description const& od )
    :
    super( argc,argv,ad,od ),
    M_backend( backend_type::build( this->vm() ) ),
    exporter( Exporter<mesh_type>::New( this->vm(), this->about().appName() ) )
{


    double meshSize = this->vm()["hsize"]. as<double>() ;
    double timeStep = this->vm()["dt"]. as<double>() ;
    double simu = this->vm()["tf"]. as<double>() ;
    int state = this->vm()["steady"]. as<int>() ;
    int weakdir( this->vm()["weakdir"]. as<int>() );

    this->changeRepository( boost::format( "cavity/%1%/steady=%2%/weakdir=%3%/time=%4%/dt=%5%/meshsize=%6%/procs_%7%/" )
                            % this->about().appName()
                            % state
                            % weakdir
                            % simu
                            % timeStep
                            % meshSize
                            % Environment::numberOfProcessors() );

}
// <int Order_s, int Order_p, int Order_t>
Feel::gmsh_ptrtype
Convection::createMesh()
{
    double h = this->vm()["hsize"]. as<double>();
    double l = this->vm()["length"]. as<double>();

    timers["mesh"].first.restart();
    gmsh_ptrtype gmshp( new gmsh_type );
    gmshp->setWorldComm( Environment::worldComm() );
    std::ostringstream ostr;
    ostr << gmshp->preamble()
         << "a=" << 0 << ";\n"
         << "b=" << l << ";\n"
         << "c=" << 0 << ";\n"
         << "d=" << 1 << ";\n"
         << "hBis=" << h << ";\n"
         << "Point(1) = {a,c,0.0,hBis};\n"
         << "Point(2) = {b,c,0.0,hBis};\n"
         << "Point(3) = {b,d,0.0,hBis};\n"
         << "Point(4) = {a,d,0.0,hBis};\n"
         << "Point(5) = {b/2,d,0.0,hBis};\n"
         << "Point(6) = {b/2,d/2,0.0,hBis};\n"
         << "Line(1) = {1,2};\n"
         << "Line(2) = {2,3};\n"
         << "Line(3) = {3,5};\n"
         << "Line(4) = {5,4};\n"
         << "Line(5) = {4,1};\n"
         << "Line(6) = {5,6};\n"
         << "Line Loop(7) = {1,2,3,4,5,6};\n"
         //<< "Line Loop(7) = {1,2,3,4,5};\n"
         << "Plane Surface(8) = {7};\n"
         << "Physical Line(\"Tinsulated\") = {1,3,4};\n"
         << "Physical Line(\"Tfixed\") = {5};\n"
         << "Physical Line(\"Tflux\") = {2};\n"
         << "Physical Line(\"Fflux\") = {6};\n"
         << "Physical Surface(\"domain\") = {8};\n";

    std::ostringstream fname;
    fname << "domain";

    gmshp->setPrefix( fname.str() );
    gmshp->setDescription( ostr.str() );
    Log() << "[timer] createMesh(): " << timers["mesh"].second << "\n";
    return gmshp;



}


// <int Order_s, int Order_p, int Order_t>
void Convection ::solve( sparse_matrix_ptrtype & J ,
        element_type & u,
        vector_ptrtype& F )
{
    M_backend->nlSolve( _solution= u );
};

// <int Order_s, int Order_p, int Order_t>
void Convection ::exportResults( boost::format fmt, element_type& U, double t )
{
    exporter->addPath( fmt );
    exporter->step( t )->setMesh( U.functionSpace()->mesh() );
    exporter->step( t )->add( "u", U. element<0>() );
    exporter->step( t )->add( "p", U. element<1>() );
    exporter->step( t )->add( "T", U. element<2>() );
    exporter->save();
};

// instantiation
// class Convection<2,1,2>;