/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2007-07-15

  Copyright (C) 2007-2012 Universite Joseph Fourier (Grenoble I)

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
   \file matrixgmm.cpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2007-07-15
 */
#include <feel/feelalg/matrixeigendense.hpp>

namespace Feel
{

template <typename T>
MatrixEigenDense<T>::MatrixEigenDense()
    :
    super(),
    _M_is_initialized( false ),
    _M_is_closed( false ),
    _M_mat()
{}
template <typename T>
MatrixEigenDense<T>::MatrixEigenDense( size_type r, size_type c )
    :
    super(),
    _M_is_initialized( false ),
    _M_is_closed( false ),
    _M_mat( r, c )
{}

template <typename T>
MatrixEigenDense<T>::MatrixEigenDense( MatrixEigenDense const & m )
    :
    super( m ),
    _M_is_initialized( m._M_is_initialized ),
    _M_is_closed( m._M_is_closed ),
    _M_mat( m._M_mat )

{}

template <typename T>
MatrixEigenDense<T>::~MatrixEigenDense()
{}

template <typename T>
void
MatrixEigenDense<T>::init ( const size_type m,
                                const size_type n,
                                const size_type /*m_l*/,
                                const size_type /*n_l*/,
                                const size_type /*nnz*/,
                                const size_type /*noz*/ )
{
    if ( ( m==0 ) || ( n==0 ) )
        return;

    _M_mat.resize( m,n );
    this->zero ();
}
template <typename T>
void
MatrixEigenDense<T>::init ( const size_type m,
                                const size_type n,
                                const size_type m_l,
                                const size_type n_l,
                                graph_ptrtype const& graph )
{
    Feel::detail::ignore_unused_variable_warning( m );
    Feel::detail::ignore_unused_variable_warning( n );
    Feel::detail::ignore_unused_variable_warning( m_l );
    Feel::detail::ignore_unused_variable_warning( n_l );
    Feel::detail::ignore_unused_variable_warning( graph );

    if ( ( m==0 ) || ( n==0 ) )
        return;

    _M_mat.resize( m,n );
    this->zero ();
}


template<typename T>
void
MatrixEigenDense<T>::addMatrix ( int* rows, int nrows,
                                 int* cols, int ncols,
                                 value_type* data )
{
    for( int i=0; i < nrows; ++i )
        for( int j=0; j < ncols; ++j )
        {
            _M_mat( rows[i], cols[j] ) += data[i*ncols+j];
        }

}
template<typename T>
void
MatrixEigenDense<T>::scale( const T a )
{
    _M_mat *= a;
}

template<typename T>
void
MatrixEigenDense<T>::resize( size_type nr, size_type nc, bool /*preserve*/ )
{
    _M_mat.resize( nr, nc );
}

template<typename T>
void
MatrixEigenDense<T>::close() const
{
    _M_is_closed = true;
}

template<typename T>
void
MatrixEigenDense<T>::transpose( MatrixSparse<value_type>& Mt ) const
{
    FEELPP_ASSERT( 0 ).warn( "not implemented yet" );
}


template<typename T>
void
MatrixEigenDense<T>::diagonal ( Vector<T>& dest ) const
{
#if 0
    VectorEigen<T>& _dest( dynamic_cast<VectorEigen<T>&>( dest ) );
    _dest = _M_mat.diagonal();
#endif
}
template<typename T>
typename MatrixEigenDense<T>::value_type
MatrixEigenDense<T>::energy( Vector<value_type> const& __v,
                             Vector<value_type> const& __u,
                             bool tranpose ) const
{
#if 0
    VectorUblas<T> const& v( dynamic_cast<VectorUblas<T> const&>( __v ) );
    VectorUblas<T> const& u( dynamic_cast<VectorUblas<T> const&>( __u ) );

    std::vector<value_type> __v1( __u.size() ), __u1( __u.size() ), __t( __u.size() );
    std::copy( v.vec().begin(), v.vec().end(), __v1.begin() );
    std::copy( u.vec().begin(), u.vec().end(), __u1.begin() );

    if ( !tranpose )
        gmm::mult( _M_mat, __u1, __t );

    else
        gmm::mult( gmm::transposed( _M_mat ), __u1, __t );

    return gmm::vect_sp( __v1, __t );
#endif
    return 0;
}

template<typename T>
void
MatrixEigenDense<T>::addMatrix( value_type v, MatrixSparse<value_type>& _m )
{
    MatrixEigenDense<value_type>* m = dynamic_cast<MatrixEigenDense<value_type>*>( &_m );
    FEELPP_ASSERT( m != 0 ).error( "invalid sparse matrix type, should be MatrixEigenDense" );
    FEELPP_ASSERT( m->closed() ).error( "invalid sparse matrix type, should be closed" );

    if ( !m )
        throw std::invalid_argument( "m" );

    if ( !this->closed() )
    {
        _M_mat += v * m->_M_mat;
    }
}

template<typename T>
void
MatrixEigenDense<T>::updateBlockMat( boost::shared_ptr<MatrixSparse<value_type> > m,
                                     std::vector<size_type> start_i,
                                     std::vector<size_type> start_j )
{
    LOG(ERROR) << "invalid call to updateBlockMat(), not yet implemented\n";

}

template<typename T>
void
MatrixEigenDense<T>::printMatlab( const std::string filename ) const
{
    std::string name = filename;
    std::string separator = " , ";

    // check on the file name
    int i = filename.find( "." );

    if ( i <= 0 )
        name = filename + ".m";

    else
    {
        if ( ( size_type ) i != filename.size() - 2 ||
                filename[ i + 1 ] != 'm' )
        {
            std::cerr << "Wrong file name ";
            name = filename + ".m";
        }
    }

    std::ofstream file_out( name.c_str() );

    FEELPP_ASSERT( file_out )( filename ).error( "[Feel::spy] ERROR: File cannot be opened for writing." );

    file_out << "S = [ ";
    file_out.precision( 16 );
    file_out.setf( std::ios::scientific );

    for ( size_type i = 0; i < _M_mat.rows(); ++i )
    {
        for ( size_type j = 0; j < _M_mat.cols(); ++j )
        {
            value_type v = _M_mat( i,j );

            file_out << i + 1 << separator
                     << j + 1 << separator
                     << v  << std::endl;
        }
    }

    file_out << "];" << std::endl;
    file_out << "I=S(:,1); J=S(:,2); S=S(:,3);" << std::endl;
    file_out << "spy(S);" << std::endl;
}


//
// Explicit instantiations
//
template class MatrixEigenDense<double>;
//template class MatrixEigenDense<double,gmm::col_major>;
}