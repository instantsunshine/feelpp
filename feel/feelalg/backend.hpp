/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2007-12-23

  Copyright (C) 2007-2012 Université Joseph Fourier (Grenoble I)

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
   \file backend.hpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2007-12-23
 */
#ifndef Backend_H
#define Backend_H 1

#include <boost/timer.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/fusion/include/fold.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>

#include <feel/feelcore/feel.hpp>
#include <feel/feelcore/environment.hpp>
#include <feel/feelcore/singleton.hpp>
#include <feel/feelcore/parameter.hpp>
#include <feel/feelalg/enums.hpp>
#include <feel/feelalg/vector.hpp>
#include <feel/feelalg/matrixsparse.hpp>
#include <feel/feelalg/matrixblock.hpp>
#include <feel/feelalg/vectorblock.hpp>
#include <feel/feelalg/datamap.hpp>

#include <feel/feelalg/solvernonlinear.hpp>
#include <feel/feelalg/preconditioner.hpp>
#include <feel/feeldiscr/functionspacebase.hpp>

#include <feel/feelalg/matrixshell.hpp>
#include <feel/feelalg/matrixshellsparse.hpp>
//#include <feel/feelvf/vf.hpp>
//#include <boost/fusion/support/pair.hpp>
//#include <boost/fusion/container.hpp>
//#include <boost/fusion/sequence.hpp>
//#include <boost/fusion/algorithm.hpp>

//namespace fusion = boost::fusion;

//#include <feel/feelvf/bilinearform.hpp>
#include <feel/feelvf/pattern.hpp>
#include <feel/feelvf/block.hpp>

namespace Feel
{
/*enum  Pattern
{
DEFAULT   = 1 << 0,
EXTENDED  = 1 << 1,
COUPLED   = 1 << 2,
SYMMETRIC = 1 << 3
};
*/
///! \cond detail
namespace detail
{
template<typename T>
boost::shared_ptr<DataMap> datamap( T const& t, mpl::true_ )
{
    return t->mapPtr();
}
template<typename T>
boost::shared_ptr<DataMap> datamap( T const& t, mpl::false_ )
{
    return t.mapPtr();
}
template<typename T>
boost::shared_ptr<DataMap> datamap( T const& t )
{
    return datamap( t, detail::is_shared_ptr<T>() );
}

template<typename T>
#if BOOST_VERSION >= 105300
typename boost::detail::sp_dereference< typename T::element_type >::type
#else
typename T::reference
#endif
ref( T t, mpl::true_ )
{
    return *t;
}
template<typename T>
T& ref( T& t, mpl::false_ )
{
    return t;
}
template<typename T>
auto ref( T& t ) -> decltype( ref( t, detail::is_shared_ptr<T>() ) )
{
    return ref( t, detail::is_shared_ptr<T>() );
}


}
///! \endcond detail

template<typename T> class MatrixBlockBase;
template<int NR, int NC, typename T> class MatrixBlock;
template<typename T> class VectorBlockBase;
template<int NR, typename T> class VectorBlock;

/**
 * \class Backend
 * \brief base class for all linear algebra backends
 *
 * @author Christophe Prud'homme
 * @see
 */
template<typename T>
class Backend:
    public boost::enable_shared_from_this<Backend<T> >
{
public:


    /** @name Typedefs
     */
    //@{
    typedef T value_type;
    typedef typename type_traits<T>::real_type real_type;

    typedef Vector<value_type> vector_type;
    typedef boost::shared_ptr<vector_type> vector_ptrtype;
    typedef MatrixSparse<value_type> sparse_matrix_type;
    typedef boost::shared_ptr<sparse_matrix_type> sparse_matrix_ptrtype;

    typedef MatrixShell<value_type> shell_matrix_type;
    typedef boost::shared_ptr<shell_matrix_type> shell_matrix_ptrtype;

    typedef typename sparse_matrix_type::graph_type graph_type;
    typedef typename sparse_matrix_type::graph_ptrtype graph_ptrtype;

    typedef Backend<value_type> backend_type;
    typedef boost::shared_ptr<backend_type> backend_ptrtype;

    typedef SolverNonLinear<value_type> solvernonlinear_type;
    typedef boost::shared_ptr<solvernonlinear_type> solvernonlinear_ptrtype;

    typedef boost::tuple<bool, size_type, value_type> solve_return_type;
    typedef boost::tuple<bool, size_type, value_type> nl_solve_return_type;

    typedef DataMap datamap_type;
    typedef boost::shared_ptr<datamap_type> datamap_ptrtype;

    //@}

    /** @name Constructors, destructor
     */
    //@{

    Backend( WorldComm const& worldComm=Environment::worldComm() );
    Backend( po::variables_map const& vm, std::string const& prefix = "", WorldComm const& worldComm=Environment::worldComm() );
    Backend( Backend const & );
    virtual ~Backend();


    /**
     * Builds a \p Backend, if Petsc is available, use Petsc by
     * default, otherwise use GMM which is distributed with feel
     */
    static backend_ptrtype build(
#if defined( FEELPP_HAS_PETSC_H )
        BackendType = BACKEND_PETSC
#else
        BackendType = BACKEND_GMM
#endif
        , WorldComm const& worldComm=Environment::worldComm()
    );

    /**
     * Builds a \p Backend
     */
    static backend_ptrtype build( po::variables_map const& vm, std::string const& prefix = "", WorldComm const& worldComm=Environment::worldComm() );

    /**
     * instantiate a new sparse vector
     */
    virtual sparse_matrix_ptrtype newMatrix( const size_type m,
            const size_type n,
            const size_type m_l,
            const size_type n_l,
            const size_type nnz=30,
            const size_type noz=10,
            size_type prop = NON_HERMITIAN ) = 0;

    /**
     * instantiate a new sparse vector
     */
    virtual sparse_matrix_ptrtype newMatrix( const size_type m,
            const size_type n,
            const size_type m_l,
            const size_type n_l,
            graph_ptrtype const & graph,
            size_type matrix_properties = NON_HERMITIAN ) = 0;

    /**
     * instantiate a new sparse vector
     */
    sparse_matrix_ptrtype newMatrix( const size_type m,
                                     const size_type n,
                                     const size_type m_l,
                                     const size_type n_l,
                                     graph_ptrtype const & graph,
                                     std::vector < std::vector<size_type> > indexSplit,
                                     size_type matrix_properties = NON_HERMITIAN )
    {
        auto mat = this->newMatrix( m,n,m_l,n_l,graph,matrix_properties );
        mat->setIndexSplit( indexSplit );
        return mat;
    }


    /**
     * instantiate a new sparse vector
     */
    virtual sparse_matrix_ptrtype newMatrix( datamap_ptrtype const& dm1,
                                             datamap_ptrtype const& dm2,
                                             size_type prop = NON_HERMITIAN,
                                             bool init = true ) = 0;

    /**
     * instantiate a new sparse vector
     */
    sparse_matrix_ptrtype newMatrix( datamap_ptrtype const& domainmap,
                                     datamap_ptrtype const& imagemap,
                                     graph_ptrtype const & graph,
                                     size_type matrix_properties = NON_HERMITIAN,
                                     bool init = true )
    {
        auto mat = this->newMatrix( domainmap,imagemap, matrix_properties, false );

        if ( init ) mat->init( imagemap->nDof(), domainmap->nDof(),
                                   imagemap->nLocalDofWithoutGhost(), domainmap->nLocalDofWithoutGhost(),
                                   graph );

        mat->zero();
        // todo!
        //mat->setIndexSplit( trial->dofIndexSplit() );
        return mat;
    }


    /**
     * instantiate a new sparse vector
     */
    virtual
    sparse_matrix_ptrtype
    newZeroMatrix( const size_type m,
                   const size_type n,
                   const size_type m_l,
                   const size_type n_l ) =0;

    virtual sparse_matrix_ptrtype newZeroMatrix( datamap_ptrtype const& dm1, datamap_ptrtype const& dm2 ) = 0;

    /**
     * helper function
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( sparse_matrix_ptrtype ),
                                     newMatrix,
                                     tag,
                                     ( required
                                       ( trial,*( boost::is_convertible<mpl::_,boost::shared_ptr<FunctionSpaceBase> > ) )
                                       ( test,*( boost::is_convertible<mpl::_,boost::shared_ptr<FunctionSpaceBase> > ) ) )
                                     ( optional
                                       ( pattern,( size_type ),Pattern::COUPLED )
                                       ( properties,( size_type ),NON_HERMITIAN )
                                       ( buildGraphWithTranspose, ( bool ),false )
                                       ( pattern_block,    *, ( BlocksStencilPattern(1,1,size_type( Pattern::HAS_NO_BLOCK_PATTERN ) ) ) )
                                       ( diag_is_nonzero,  *( boost::is_integral<mpl::_> ), true )
                                       ( verbose,   ( bool ), option(_prefix=this->prefix(),_name="backend.verbose").template as<bool>() )
                                       ( collect_garbage, *( boost::is_integral<mpl::_> ), true )
                                     ) )
    {
        if ( verbose )
        {
            Environment::logMemoryUsage( "backend::newMatrix begin" );
        }

        if ( !this->comm().isActive() ) return sparse_matrix_ptrtype();

        //auto mat = this->newMatrix( trial->map(), test->map(), properties, false );
        auto mat = this->newMatrix( trial->dofOnOff(), test->dofOn(), properties, false );

        if ( !buildGraphWithTranspose )
        {
            auto s = stencil( _test=test,
                              _trial=trial,
                              _pattern=pattern,
                              _pattern_block=pattern_block,
                              _diag_is_nonzero=diag_is_nonzero,
                              _collect_garbage=collect_garbage);

            mat->init( test->nDof(), trial->nDof(),
                       test->nLocalDofWithoutGhost(), trial->nLocalDofWithoutGhost(),
                       s->graph() );
        }
        else
        {
            auto s = stencil( _test=trial,
                              _trial=test,
                              _pattern=pattern,
                              _pattern_block=pattern_block.transpose(),
                              _diag_is_nonzero=false,// because transpose(do just after)
                              _close=false,
                              _collect_garbage=collect_garbage );
            // get the good graph
            auto graph = s->graph()->transpose(false);
            if ( diag_is_nonzero )
                graph->addMissingZeroEntriesDiagonal();
            graph->close();

            //maybe do that
            //stencilManagerGarbage(boost::make_tuple( trial, test, pattern, pattern_block.transpose().getSetOfBlocks(), false/*diag_is_nonzero*/));
            //now save the good graph in the StencilManager(only if entry is empty)
            stencilManagerAdd(boost::make_tuple( test, trial, pattern, pattern_block.getSetOfBlocks(), diag_is_nonzero), graph);

            mat->init( test->nDof(), trial->nDof(),
                       test->nLocalDofWithoutGhost(), trial->nLocalDofWithoutGhost(),
                       graph );
        }

        mat->zero();
        mat->setIndexSplit( trial->dofIndexSplit() );
        if ( verbose )
        {
            Environment::logMemoryUsage( "backend::newMatrix end" );
        }
        return mat;
    }

    template<typename DomainSpace, typename ImageSpace>
    sparse_matrix_ptrtype newMatrix( DomainSpace const& dm, ImageSpace const& im, sparse_matrix_ptrtype const& M, size_type prop = NON_HERMITIAN  )
    {
        sparse_matrix_ptrtype m = newMatrix( dm, im, prop  );
        m->init( im->nDof(), dm->nDof(), im->nLocalDof(), dm->nLocalDof(), M->graph() );
        return m;
    }

    /**
     * instantiate a new block matrix sparse
     */
    sparse_matrix_ptrtype newBlockMatrixImpl( vf::BlocksBase<sparse_matrix_ptrtype> const & b,
                                              bool copy_values=true,
                                              bool diag_is_nonzero=true )
    {
        typedef MatrixBlockBase<typename sparse_matrix_ptrtype::element_type::value_type> matrix_block_type;
        boost::shared_ptr<matrix_block_type> mb( new matrix_block_type( b, *this, copy_values, diag_is_nonzero ) );
        return mb->getSparseMatrix();
    }

    sparse_matrix_ptrtype newBlockMatrixImpl( vf::BlocksBase<boost::shared_ptr<GraphCSR> > const & b,
                                              bool copy_values=true,
                                              bool diag_is_nonzero=true )
    {
        typedef MatrixBlockBase<value_type> matrix_block_type;
        boost::shared_ptr<matrix_block_type> mb( new matrix_block_type( b, *this, diag_is_nonzero ) );
        return mb->getSparseMatrix();
    }

    /**
     * instantiate a new block matrix sparse
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( sparse_matrix_ptrtype ),
                                     newBlockMatrix,
                                     tag,
                                     ( required
                                       ( block,* )
                                     )
                                     ( optional
                                       ( copy_values,*( boost::is_integral<mpl::_> ),true )
                                       ( diag_is_nonzero,  *( boost::is_integral<mpl::_> ), true )
                                     )
                                   )
    {
        return newBlockMatrixImpl( block,copy_values,diag_is_nonzero );
    }

    /**
     * instantiate a new block matrix sparse
     */
    template < typename BlockType=vector_ptrtype >
    vector_ptrtype newBlockVectorImpl( vf::BlocksBase<BlockType> const & b,
                                       bool copy_values=true )
    {
        typedef VectorBlockBase<typename BlockType::element_type::value_type> vector_block_type;
        boost::shared_ptr<vector_block_type> mb( new vector_block_type( b, *this, copy_values ) );
        return mb->getVector();
    }

    /**
     * instantiate a new block matrix sparse
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( vector_ptrtype ),
                                     newBlockVector,
                                     tag,
                                     ( required
                                       ( block,* )
                                     )
                                     ( optional
                                       ( copy_values,*( boost::is_integral<mpl::_> ),true )
                                     )
                                   )
    {
        return newBlockVectorImpl( block,copy_values );
    }


    /**
     * instantiate a new zero matrix
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( sparse_matrix_ptrtype ),
                                     newZeroMatrix,
                                     tag,
                                     ( required
                                       ( test,*( boost::is_convertible<mpl::_,boost::shared_ptr<FunctionSpaceBase> >) )
                                       ( trial,*( boost::is_convertible<mpl::_,boost::shared_ptr<FunctionSpaceBase> >) )
                                     )
                                   )
    {
        return this->newZeroMatrix( trial->dofOnOff(), test->dofOn() );
    }

    /**
     * instantiate a new vector
     */
    virtual vector_ptrtype newVector( datamap_ptrtype const& dm ) = 0;

    /**
     * instantiate a new vector
     */
    virtual vector_ptrtype newVector( const size_type n, const size_type n_local ) = 0;

    /**
     * helper function
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( vector_ptrtype ),
                                     newVector,
                                     tag,
                                     ( required
                                       ( test,*( boost::is_convertible<mpl::_,boost::shared_ptr<FunctionSpaceBase> >) )
                                     )
                                   )
    {
        if ( !this->comm().isActive() ) return vector_ptrtype();

        return this->newVector( test->dof() );
    }

    //@}

    /** @name Operator overloads
     */
    //@{


    //@}

    /** @name Accessors
     */
    //@{

    /**
     * \return prefix of backend
     */
    std::string prefix() const
    {
        return M_prefix;
    }

    /**
     * \return the type of linear solver
     */
    std::string kspType() const
    {
        return M_ksp;
    }

    /**
     * \return the type of preconditioner
     */
    std::string pcType() const
    {
        return M_pc;
    }

    /**
     * return true if the null space is the constant values, false otherwise
     */
    bool hasConstantNullSpace() const
    {
        return M_constant_null_space;
    }

    /**
     * \return the type of fieldsplitType
     */
    std::string fieldsplitType() const
    {
        return M_fieldSplit;
    }

    /**
     * \return enum pc type from options
     **/
    PreconditionerType pcEnumType() const;

    /**
     * \return enum solver type from options
     **/
    SolverType kspEnumType() const;

    /**
     * \return enum fieldsplit type from options
     **/
    FieldSplitType fieldSplitEnumType() const;

    /**
     * \return the type of pcFactorMatSolverPackageType
     */
    std::string pcFactorMatSolverPackageType() const
    {
        return M_pcFactorMatSolverPackage;
    }

    /**
     * \return enum MatSolverPackage type from options
     **/
    MatSolverPackageType matSolverPackageEnumType() const;

    /**
     * \return the type of preconditioner associated to the matrix
     */
    MatrixStructure precMatrixStructure() const
    {
        return M_prec_matrix_structure;
    }

    /**
     * \return the relative tolerance
     */
    value_type rTolerance() const
    {
        return M_rtolerance;
    }

    /**
     * \return the relative tolerance SNES
     */
    value_type rToleranceSNES() const
    {
        return M_rtoleranceSNES;
    }

    /**
     * \return the divergence tolerance
     */
    value_type dTolerance() const
    {
        return M_dtolerance;
    }

    /**
     * \return the SNES step length tolerance
     */
    value_type sToleranceSNES() const
    {
        return M_stoleranceSNES;
    }

    /**
     * \return the absolute tolerance
     */
    value_type aTolerance() const
    {
        return M_atolerance;
    }

    /**
     * \return the SNES absolute tolerance
     */
    value_type aToleranceSNES() const
    {
        return M_atoleranceSNES;
    }

    /**
     * \return the maximum number of iterations
     */
    size_type maxIterations() const
    {
        return M_maxitKSP;
    }
    size_type maxIterationsKSP() const
    {
        return M_maxitKSP;
    }
    size_type maxIterationsKSPinSNES() const
    {
        return M_maxitKSPinSNES;
    }
    size_type maxIterationsSNES() const
    {
        return M_maxitSNESReuse;
    }
    size_type maxIterationsKSPReuse() const
    {
        return M_maxitKSPReuse;
    }
    size_type maxIterationsKSPinSNESReuse() const
    {
        return M_maxitKSPinSNESReuse;
    }
    size_type maxIterationsSNESReuse() const
    {
        return M_maxitSNESReuse;
    }


    /**
     * \return the KSP relative tolerance in SNES
     */
    value_type rtoleranceKSPinSNES() const
    {
        return M_rtoleranceKSPinSNES;
    }


    bool converged() const
    {
        return M_converged;
    }

    size_type nIterations() const
    {
        return M_iteration;
    }

    bool transpose() const
    {
        return M_transpose;
    }

    /**
     * \return the communicator
     */
    WorldComm const& comm() const
    {
        return M_worldComm;
    }

    bool showKSPMonitor() const { return M_showKSPMonitor; }
    bool showSNESMonitor() const { return M_showSNESMonitor; }
    bool showKSPConvergedReason() const { return M_showKSPConvergedReason; }
    bool showSNESConvergedReason() const { return M_showSNESConvergedReason; }

    bool reusePrec() const { return M_reuse_prec; }
    bool reuseJac() const { return M_reuse_jac; }

    bool reusePrecRebuildAtFirstNewtonStep() const { return M_reusePrecRebuildAtFirstNewtonStep; }
    bool reuseJacRebuildAtFirstNewtonStep() const { return M_reuseJacRebuildAtFirstNewtonStep; }

    BackendType type() const { return M_backend; }

    //@}

    /** @name  Mutators
     */
    //@{

    /**
     * set tolerances: relative tolerance \p rtol, divergence tolerance \p dtol
     * and absolute tolerance \p atol
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( void ),
                                     setTolerances,
                                     tag,
                                     ( required
                                       ( rtolerance, ( double ) )
                                     )
                                     ( optional
                                       ( maxit,      ( size_type ), 1000 )
                                       ( atolerance, ( double ),    1e-50 )
                                       ( dtolerance, ( double ),    1e5 )
                                     ) )
    {
        M_rtolerance = rtolerance;
        M_dtolerance = dtolerance;
        M_atolerance = atolerance;
        M_maxitKSP = maxit;
    }

    BOOST_PARAMETER_MEMBER_FUNCTION( ( void ),
                                     setTolerancesSNES,
                                     tag,
                                     ( required
                                       ( rtolerance, ( double ) )
                                     )
                                     ( optional
                                       ( maxit,      ( size_type ), 50 )
                                       ( atolerance, ( double ),    1e-50 )
                                       ( stolerance, ( double ),    1e-8 )
                                     ) )
    {
        M_rtoleranceSNES = rtolerance;
        M_stoleranceSNES = stolerance;
        M_atoleranceSNES = atolerance;
        M_maxitSNES = maxit;
    }

    /**
     * set solver: krylov subspace method and preconditioners
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( void ),
                                     setSolverType,
                                     tag,
                                     ( required
                                       ( ksp, ( std::string ) )
                                     )
                                     ( optional
                                       ( pc,      ( std::string ), "lu" )
                                       ( constant_null_space,      ( bool ), false )
                                       ( pcfactormatsolverpackage,  ( std::string ), "petsc" )
                                     ) )
    {
        M_ksp = ksp;
        M_pc = pc;
        M_pcFactorMatSolverPackage = pcfactormatsolverpackage;
        M_constant_null_space = constant_null_space;
    }

    /**
     * set the type of preconditioner associated to the matrix
     */
    void setPrecMatrixStructure( MatrixStructure mstruct )
    {
        M_prec_matrix_structure = mstruct;
    }

    /**
     * \return the non linear solver
     */
    solvernonlinear_ptrtype nlSolver()
    {
        return M_nlsolver;
    }

    void setTranspose( bool transpose )
    {
        M_transpose = transpose;
    }

    void setShowKSPMonitor( bool b ) { M_showKSPMonitor=b; }
    void setShowSNESMonitor( bool b ) { M_showSNESMonitor=b; }
    void setShowKSPConvergedReason( bool b ) { M_showKSPConvergedReason=b; }
    void setShowSNESConvergedReason( bool b ) { M_showSNESConvergedReason=b; }

    void setReusePrec( bool b ) { M_reuse_prec=b; }
    void setReuseJac( bool b) { M_reuse_jac=b; }

    void setReusePrecRebuildAtFirstNewtonStep(bool b) { M_reusePrecRebuildAtFirstNewtonStep=b; }
    void setReuseJacRebuildAtFirstNewtonStep(bool b) { M_reuseJacRebuildAtFirstNewtonStep=b; }


    //@}

    /** @name  Methods
     */
    //@{

    /**
     * clean up
     */
    virtual void clear();

    /**
     * \return \f$ r = x^T * y \f$
     */
    virtual real_type dot( vector_type const& x, vector_type const& y ) const;


    /**
     * \return \f$ r = x^T * y \f$
     */
    real_type dot( vector_ptrtype const& x, vector_ptrtype const& y ) const
    {
        return this->dot( *x, *y );
    }
    /**
     * \return \f$ y = A * x \f$
     */
    virtual void prod( sparse_matrix_type const& A, vector_type const& x, vector_type& y ) const = 0;

    /**
     * \return \f$ y = A * x \f$
     */
    void prod( sparse_matrix_ptrtype const& A, vector_ptrtype const& x, vector_ptrtype& y ) const
    {
        this->prod( *A, *x, *y );
    }

    /**
     * solve for \f$P A x = P b\f$ where \f$P\f$ is an approximation
     * of the inverse of \f$A\f$. this interface uses the
     * boost.parameter library to ease the function usage
     *
     * \param A matrix to inverse
     * \param rhs right hand side vector
     * \param solution solution of the system
     * \param P preconditioner
     * \param maxit maximum number of iterations
     * \param tolerance tolerance on the residual
     * \param reuse_prec if true use adaptive preconditioning strategy
     * \param transpose if true solve the transpose problem
     *
     * \warning some parameter may not be meaningful for all backends
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( solve_return_type ),
                                     solve,
                                     tag,
                                     ( required
                                       //( matrix,*(mpl::or_<sparse_matrix_ptrtype,shell_matrix_ptrtype>) )
                                       ( matrix,(sparse_matrix_ptrtype) )
                                       //( in_out( solution ),*( mpl::or_<mpl::or_<boost::is_convertible<mpl::_,vector_type&>,boost::is_convertible<mpl::_,vector_type> >,boost::is_convertible<mpl::_,vector_ptrtype> > ) )
                                       ( in_out( solution ),* )
                                       ( rhs,( vector_ptrtype ) ) )
                                     ( optional
                                       //(prec,(sparse_matrix_ptrtype), matrix )
                                       ( prec,( preconditioner_ptrtype ), preconditioner( _prefix=this->prefix(),_matrix=matrix,_pc=this->pcEnumType()/*LU_PRECOND*/,
                                                                                          _pcfactormatsolverpackage=this->matSolverPackageEnumType(), _backend=this->shared_from_this() ) )
                                       ( maxit,( size_type ), M_maxitKSP/*1000*/ )
                                       ( rtolerance,( double ), M_rtolerance/*1e-13*/ )
                                       ( atolerance,( double ), M_atolerance/*1e-50*/ )
                                       ( dtolerance,( double ), M_dtolerance/*1e5*/ )
                                       ( reuse_prec,( bool ), M_reuse_prec )
                                       ( transpose,( bool ), false )
                                       ( constant_null_space,( bool ), false )
                                       ( pc,( std::string ),M_pc/*"lu"*/ )
                                       ( ksp,( std::string ),M_ksp/*"gmres"*/ )
                                       ( pcfactormatsolverpackage,( std::string ), M_pcFactorMatSolverPackage )
                                       ( verbose,   ( bool ), option(_prefix=this->prefix(),_name="backend.verbose").template as<bool>() )
                                     )
                                   )
    {
        if ( verbose )
        {
            Environment::logMemoryUsage( "backend::solve begin" );
        }
        this->setTolerances( _dtolerance=dtolerance,
                             _rtolerance=rtolerance,
                             _atolerance=atolerance,
                             _maxit=maxit );

        this->setSolverType( _pc=pc, _ksp=ksp,
                             _constant_null_space=constant_null_space,
                             _pcfactormatsolverpackage = pcfactormatsolverpackage );
        this->attachPreconditioner( prec );
        // make sure matrix and rhs are closed
        matrix->close();
        rhs->close();

        // print them in matlab format
        if ( !M_export.empty() )
        {
            matrix->printMatlab( M_export+"_A.m" );
            rhs->printMatlab( M_export+"_b.m" );
        }

        vector_ptrtype _sol( this->newVector( Feel::detail::datamap( solution ) ) );
        // initialize
        *_sol = Feel::detail::ref( solution );
        this->setTranspose( transpose );
        solve_return_type ret;

        if ( reuse_prec == false )
        {
            ret = solve( matrix, matrix, _sol, rhs );
        }

        else
            ret = solve( matrix, matrix, _sol, rhs, reuse_prec );

        //new
        _sol->close();
        detail::ref( solution ) = *_sol;
        if ( verbose )
        {
            Environment::logMemoryUsage( "backend::solve end" );
        }
        return ret;
    }

    /**
     * solve for \f$P A x = P b\f$ where \f$P\f$ is an approximation of
     * the inverse of \f$A\f$.
     *
     * \param A matrix to inverse
     * \param rhs right hand side vector
     * \param solution solution of the system
     * \param P preconditioner
     * \param maxit maximum number of iterations
     * \param tolerance tolerance on the residual
     * \param transpose if true solve the transpose problem
     *
     * \warning some parameter may not be meaningful for all backends
     */
    virtual solve_return_type solve( sparse_matrix_ptrtype const& A,
                                     sparse_matrix_ptrtype const& P,
                                     vector_ptrtype& x,
                                     vector_ptrtype const& b
                                   ) = 0;

    /**
     * solve for \f$P A x = P b\f$ where \f$P\f$ is an approximation
     * of the inverse of \f$A\f$ using an adaptive preconditioning
     * strategy.
     *
     * \param A matrix to inverse
     * \param rhs right hand side vector
     * \param solution solution of the system
     * \param P preconditioner
     * \param maxit maximum number of iterations
     * \param tolerance tolerance on the residual
     * \param transpose if true solve the transpose problem
     *
     * \warning some parameter may not be meaningful for all backends
     */
    solve_return_type solve( sparse_matrix_ptrtype const& A,
                             sparse_matrix_ptrtype const& P,
                             vector_ptrtype& x,
                             vector_ptrtype const& b,
                             bool reuse_prec
                           );


    /**
     * solve for \f$P F(x)=0 b\f$
     */
    BOOST_PARAMETER_MEMBER_FUNCTION( ( nl_solve_return_type ),
                                     nlSolve,
                                     tag,
                                     ( required
                                       //( in_out( solution ),*( mpl::or_<boost::is_convertible<mpl::_,vector_type&>,boost::is_convertible<mpl::_,vector_ptrtype> > ) ) )
                                       ( in_out( solution ),*))
                                     ( optional
                                       ( jacobian,( sparse_matrix_ptrtype ), sparse_matrix_ptrtype() )
                                       ( residual,( vector_ptrtype ), vector_ptrtype() )
                                       //(prec,(sparse_matrix_ptrtype), jacobian )
                                       ( prec,( preconditioner_ptrtype ), preconditioner( _prefix=this->prefix(),_pc=this->pcEnumType()/*LU_PRECOND*/,_backend=this->shared_from_this(),
                                                                                          _pcfactormatsolverpackage=this->matSolverPackageEnumType() ) )
                                       ( maxit,( size_type ), M_maxitSNES/*50*/ )
                                       ( rtolerance,( double ), M_rtoleranceSNES/*1e-8*/ )
                                       ( atolerance,( double ), M_atoleranceSNES/*1e-50*/ )
                                       ( stolerance,( double ), M_stoleranceSNES/*1e-8*/ )
                                       ( reuse_prec,( bool ), M_reuse_prec )
                                       ( reuse_jac,( bool ), M_reuse_jac )
                                       ( transpose,( bool ), false )
                                       ( pc,( std::string ),M_pc/*"lu"*/ )
                                       ( ksp,( std::string ),M_ksp/*"gmres"*/ )
                                       ( pcfactormatsolverpackage,( std::string ), M_pcFactorMatSolverPackage )
                                       ( verbose,   ( bool ), option(_prefix=this->prefix(),_name="backend.verbose").template as<bool>() )
                                     )
                                   )
    {
        if ( verbose )
        {
            Environment::logMemoryUsage( "backend::nlSolve begin" );
        }
        this->setTolerancesSNES( _stolerance=stolerance,
                                 _rtolerance=rtolerance,
                                 _atolerance=atolerance,
                                 _maxit=maxit );
        this->setSolverType( _pc=pc, _ksp=ksp,
                             _pcfactormatsolverpackage = pcfactormatsolverpackage );
        vector_ptrtype _sol( this->newVector( detail::datamap( solution ) ) );
        // initialize
        *_sol = detail::ref( solution );
        this->setTranspose( transpose );
        solve_return_type ret;

        // this is done with nonlinerarsolver
        if ( !residual )
        {
            residual = this->newVector( ( detail::datamap( solution ) ) );
            //this->nlSolver()->residual( _sol, residual );
        }

        this->nlSolver()->setPrefix( this->prefix() );
        if ( !jacobian )
            this->nlSolver()->jacobian( _sol, jacobian );

        if ( prec && !this->nlSolver()->initialized() )
            this->nlSolver()->attachPreconditioner( prec );

        if ( reuse_prec == false && reuse_jac == false )
            ret = nlSolve( jacobian, _sol, residual, rtolerance, maxit );

        else
            ret = nlSolve( jacobian, _sol, residual, rtolerance, maxit, reuse_prec, reuse_jac );

        //new
        _sol->close();
        detail::ref( solution ) = *_sol;
        detail::ref( solution ).close();
        if ( verbose )
        {
            Environment::logMemoryUsage( "backend::nlSolve end" );
        }
        return ret;
    }

    /**
     * solve for the nonlinear problem \f$F( u ) = 0\f$
     */
    virtual nl_solve_return_type nlSolve( sparse_matrix_ptrtype& A,
                                          vector_ptrtype& x,
                                          vector_ptrtype& b,
                                          const double, const int );

    /**
     * solve for the nonlinear problem \f$F( u ) = 0\f$ with an
     * adaptive strategy to reuse the preconditioner
     */
    virtual nl_solve_return_type nlSolve( sparse_matrix_ptrtype& A,
                                          vector_ptrtype& x,
                                          vector_ptrtype& b,
                                          const double, const int,
                                          bool reusePC, bool reuseJAC );

    /**
     * Attaches a Preconditioner object to be used by the solver
     */
    void attachPreconditioner( preconditioner_ptrtype preconditioner )
    {
        if ( M_preconditioner && M_preconditioner != preconditioner )
            M_preconditioner->clear();
        M_preconditioner = preconditioner;
    }

    /**
     * register a backend observer for the delete signal of backend
     */
    template<typename Observer>
    void
    addDeleteObserver( Observer const& obs )
        {
            M_deleteObservers.connect( obs );
        }
    /**
     * register a backend observer for the delete signal of backend that is a
     * shared_ptr<>
     */
    template<typename Observer>
    void
    addDeleteObserver( boost::shared_ptr<Observer> const& obs )
        {
            M_deleteObservers.connect(boost::bind(&Observer::operator(), obs));
        }
    /**
     * send the delete signal to all observers
     */
    void
    sendDeleteSignal()
        {
            M_deleteObservers();
        }
    //@}



protected:
    preconditioner_ptrtype M_preconditioner;
private:

    void start();

    void stop();

    void reset();

private:
    WorldComm M_worldComm;

    po::variables_map M_vm;

    BackendType M_backend;

    std::string M_prefix;

    solvernonlinear_ptrtype M_nlsolver;

    MatrixStructure M_prec_matrix_structure;

    double M_totalSolveIter;
    double M_lastSolveIter;
    double M_firstSolveTime;
    double M_residual;
    double M_rtolerance;
    double M_dtolerance;
    double M_atolerance;
    double M_rtoleranceSNES, M_stoleranceSNES, M_atoleranceSNES;
    double M_rtoleranceKSPinSNES;

    bool M_reuse_prec;
    bool M_reuse_jac;
    bool M_reusePrecIsBuild,M_reusePrecRebuildAtFirstNewtonStep;
    bool M_reuseJacIsBuild,M_reuseJacRebuildAtFirstNewtonStep;
    size_t M_nUsePC;
    bool   M_converged;
    bool   M_reusePC;
    bool   M_reusedPC;
    bool   M_reuseFailed;
    boost::timer M_timer;
    bool   M_transpose;
    size_type    M_maxitKSP, M_maxitKSPinSNES, M_maxitSNES;
    size_type    M_maxitKSPReuse, M_maxitKSPinSNESReuse, M_maxitSNESReuse;
    size_type    M_iteration;
    std::string M_export;
    std::string M_ksp;
    std::string M_pc;
    std::string M_fieldSplit;
    std::string M_pcFactorMatSolverPackage;
    bool M_constant_null_space;
    bool M_showKSPMonitor, M_showSNESMonitor;
    bool M_showKSPConvergedReason, M_showSNESConvergedReason;
    //std::map<std::string,boost::tuple<std::string,std::string> > M_sub;

    boost::signals2::signal<void()> M_deleteObservers;
};


typedef Backend<double> backend_type;
typedef boost::shared_ptr<backend_type> backend_ptrtype;


namespace detail
{
class BackendManagerImpl:
    public std::map<boost::tuple<BackendType,std::string,int>, backend_ptrtype >,
    public boost::noncopyable
{
public:
    typedef backend_ptrtype value_type;
    typedef boost::tuple<BackendType,std::string,int> key_type;
    typedef std::map<key_type, value_type> backend_manager_type;

};
typedef Feel::Singleton<BackendManagerImpl> BackendManager;

struct BackendManagerDeleterImpl
{
    void operator()() const
        {
            VLOG(2) << "[BackendManagerDeleter] clear BackendManager Singleton: " << detail::BackendManager::instance().size() << "\n";
            detail::BackendManager::instance().clear();
            VLOG(2) << "[BackendManagerDeleter] clear BackendManager done\n";
        }
};
typedef Feel::Singleton<BackendManagerDeleterImpl> BackendManagerDeleter;
} // detail


BOOST_PARAMETER_FUNCTION(
    ( backend_ptrtype ), // return type
    backend,           // 2. function name
    tag,               // 3. namespace of tag types
    ( optional
      ( vm,           ( po::variables_map ), Environment::vm() )
      ( name,           ( std::string ), "" )
      ( kind,           ( BackendType ), BACKEND_PETSC )
      ( rebuild,        ( bool ), false )
      ( worldcomm,      (WorldComm), Environment::worldComm() )
    ) )
{
    // register the BackendManager into Feel::Environment so that it gets the
    // BackendManager is cleared up when the Environment is deleted
    static bool observed=false;
    if ( !observed )
    {
        Environment::addDeleteObserver( detail::BackendManagerDeleter::instance() );
        observed = true;
    }


    Feel::detail::ignore_unused_variable_warning( args );

    auto git = detail::BackendManager::instance().find( boost::make_tuple( kind, name, worldcomm.globalSize() ) );

    if (  git != detail::BackendManager::instance().end() && ( rebuild == false ) )
    {
        VLOG(2) << "[backend] found backend name=" << name << " kind=" << kind << " rebuild=" << rebuild << " worldcomm.globalSize()=" << worldcomm.globalSize() << "\n";
        return git->second;
    }

    else
    {
        if (  git != detail::BackendManager::instance().end() && ( rebuild == true ) )
            git->second->clear();

        VLOG(2) << "[backend] building backend name=" << name << " kind=" << kind << " rebuild=" << rebuild << " worldcomm.globalSize()=" << worldcomm.globalSize() << "\n";

        backend_ptrtype b;
        if ( vm.empty() )
        {
            b = Feel::backend_type::build( kind, worldcomm );
        }
        else
            b = Feel::backend_type::build( vm, name, worldcomm );
        VLOG(2) << "storing backend in singleton" << "\n";
        detail::BackendManager::instance().operator[]( boost::make_tuple( kind, name, worldcomm.globalSize() ) ) = b;
        return b;
    }

}

}
#endif /* Backend_H */
