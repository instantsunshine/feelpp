/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

   This file is part of the Feel library

   Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   Date: 2005-01-18

   Copyright (C) 2005,2006 EPFL
   Copyright (C) 2006-2010 Université Joseph Fourier (Grenoble I)

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
 *  \file bilinearform.hpp
 *  \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
 *  \date 2005-01-18
 */
#ifndef __BilinearForm_H
#define __BilinearForm_H 1

#include <Eigen/Eigen>
#include <Eigen/StdVector>


#include <set>

#include <boost/parameter.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/spirit/home/phoenix.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <feel/feelcore/context.hpp>
#include <feel/feelalg/matrixvalue.hpp>
#include <feel/feelalg/vectorublas.hpp>
#include <feel/feelalg/backend.hpp>
#include <feel/feeldiscr/stencil.hpp>
#include <feel/feelvf/block.hpp>
#include <feel/feelvf/fec.hpp>
#include <feel/feelvf/formcontextbase.hpp>


namespace Feel
{
namespace parameter = boost::parameter;
namespace fusion = boost::fusion;
namespace vf
{

/// \cond detail
template<typename FE1,typename FE2,typename ElemContType> class BilinearForm;
namespace detail
{


template<typename BFType, typename ExprType>
struct BFAssign2
{
    typedef typename BFType::matrix_type matrix_type;
    BFAssign2( BFAssign2 const& lfa )
        :
        _M_bf( lfa._M_bf ),
        _M_expr( lfa._M_expr ),
        _M_test_index( lfa._M_test_index )
    {}
    BFAssign2( BFType& lf, ExprType const& expr )
        :
        _M_bf( lf ),
        _M_expr( expr ),
        _M_test_index( 0 )
    {}
    template<typename SpaceType>
    void operator()( boost::shared_ptr<SpaceType> const& X ) const
    {
        DVLOG(2) << "[BFAssign2::operator()] start loop on trial spaces against test space index: " << _M_test_index << "\n";

        if ( _M_bf.testSpace()->worldsComm()[_M_test_index].isActive() )
        {
            fusion::for_each( _M_bf.trialSpace()->functionSpaces(),
                              make_bfassign1( _M_bf, _M_expr, X, _M_test_index ) );
        }

        DVLOG(2) << "[BFAssign2::operator()] stop loop on trial spaces against test space index: " << _M_test_index << "\n";
        ++_M_test_index;

    }
private:
    BFType& _M_bf;
    ExprType const& _M_expr;
    mutable size_type _M_test_index;
};
template<typename BFType, typename ExprType>
BFAssign2<BFType,ExprType>
make_bfassign2( BFType& lf, ExprType const& expr )
{
    return BFAssign2<BFType,ExprType>( lf, expr );
}

template<typename BFType, typename ExprType, typename TestSpaceType>
struct BFAssign1
{
    typedef typename BFType::matrix_type matrix_type;
    BFAssign1( BFAssign1 const& lfa )
        :
        _M_bf( lfa._M_bf ),
        _M_test( lfa._M_test ),
        _M_expr( lfa._M_expr ),
        _M_trial_index( lfa._M_trial_index )
    {}
    BFAssign1( BFType& lf,
               ExprType const& expr,
               boost::shared_ptr<TestSpaceType> const& Testh,
               size_type test_index )
        :
        _M_bf( lf ),
        _M_test( Testh ),
        _M_expr( expr ),
        _M_trial_index( 0 ),
        _M_test_index( test_index )
    {}
    template<typename SpaceType>
    void operator()( boost::shared_ptr<SpaceType> const& trial ) const;

private:
    BFType& _M_bf;
    boost::shared_ptr<TestSpaceType> _M_test;
    ExprType const& _M_expr;
    mutable size_type _M_trial_index;
    size_type _M_test_index;
};
template<typename BFType, typename ExprType, typename TestSpaceType>
BFAssign1<BFType,ExprType,TestSpaceType>
make_bfassign1( BFType& lf,
                ExprType const& expr,
                boost::shared_ptr<TestSpaceType> const& test_space,
                size_type test_index )
{
    return BFAssign1<BFType,ExprType,TestSpaceType>( lf, expr, test_space, test_index );
}


template<typename BFType, typename ExprType, typename TrialSpaceType>
struct BFAssign3
{
    typedef typename BFType::matrix_type matrix_type;
    BFAssign3( BFAssign3 const& lfa )
        :
        _M_bf( lfa._M_bf ),
        _M_trial( lfa._M_trial ),
        _M_expr( lfa._M_expr ),
        _M_test_index( lfa._M_test_index )
    {}
    BFAssign3( BFType& lf,
               ExprType const& expr,
               boost::shared_ptr<TrialSpaceType> const& Trialh,
               size_type trial_index )
        :
        _M_bf( lf ),
        _M_trial( Trialh ),
        _M_expr( expr ),
        _M_trial_index( trial_index ),
        _M_test_index( 0 )
    {}
    template<typename SpaceType>
    void operator()( boost::shared_ptr<SpaceType> const& test ) const;

private:
    BFType& _M_bf;
    boost::shared_ptr<TrialSpaceType> _M_trial;
    ExprType const& _M_expr;
    size_type _M_trial_index;
    mutable size_type _M_test_index;
};
template<typename BFType, typename ExprType, typename TrialSpaceType>
BFAssign3<BFType,ExprType,TrialSpaceType>
make_bfassign3( BFType& lf,
                ExprType const& expr,
                boost::shared_ptr<TrialSpaceType> const& trial_space,
                size_type trial_index )
{
    return BFAssign3<BFType,ExprType,TrialSpaceType>( lf, expr, trial_space, trial_index );
}


/*!
  \class BilinearForm
  \brief brief description

  @author Christophe Prud'homme
  @see
*/
template<typename FE1,
         typename FE2,
         typename ElemContType = VectorUblas<typename FE1::value_type> >
class BilinearForm
{
public:

    /** @name Typedefs
     */
    //@{
    enum { nDim = FE1::nDim };

    typedef FE1 space_1_type;
    typedef boost::shared_ptr<FE1> space_1_ptrtype;
    typedef space_1_type test_space_type;
    typedef boost::shared_ptr<space_1_type> test_space_ptrtype;

    typedef FE2 space_2_type;
    typedef boost::shared_ptr<FE2> space_2_ptrtype;
    typedef space_2_type trial_space_type;
    typedef boost::shared_ptr<space_2_type> trial_space_ptrtype;

    typedef typename FE1::value_type value_type;
    typedef typename space_1_type::template Element<value_type,ElemContType> element_1_type;

    typedef typename space_2_type::template Element<value_type,ElemContType> element_2_type;
    typedef BilinearForm<FE1, FE2, ElemContType> self_type;

#if 0
    typedef typename space_1_type::component_fespace_type component_space_1_type;
    typedef typename element_1_type::component_type component_1_type;
    typedef typename space_2_type::component_fespace_type component_space_2_type;
    typedef typename element_2_type::component_type component_2_type;
#endif // 0

    typedef typename space_1_type::mesh_type mesh_1_type;
    typedef typename mesh_1_type::element_type mesh_element_1_type;
    typedef typename mesh_element_1_type::permutation_type permutation_1_type;

    typedef typename space_2_type::mesh_type mesh_2_type;
    typedef typename mesh_2_type::element_type mesh_element_2_type;
    typedef typename mesh_element_2_type::permutation_type permutation_2_type;

    typedef typename space_1_type::fe_type fe_1_type;
    typedef typename space_2_type::fe_type fe_2_type;

    typedef typename space_1_type::gm_type gm_1_type;
    typedef typename space_1_type::gm_ptrtype gm_1_ptrtype;

    typedef typename space_1_type::gm1_type gm1_1_type;
    typedef typename space_1_type::gm1_ptrtype gm1_1_ptrtype;

    typedef typename space_2_type::gm_type gm_2_type;
    typedef typename space_2_type::gm_ptrtype gm_2_ptrtype;

    typedef typename space_2_type::gm1_type gm1_2_type;
    typedef typename space_2_type::gm1_ptrtype gm1_2_ptrtype;

    //typedef ublas::compressed_matrix<value_type, ublas::row_major> csr_matrix_type;
    typedef MatrixSparse<value_type> matrix_type;
    typedef boost::shared_ptr<matrix_type> matrix_ptrtype;
    static const bool is_row_major = true;//matrix_type::is_row_major;

    typedef typename mpl::if_<mpl::equal_to<mpl::bool_<is_row_major>, mpl::bool_<true> >,
            mpl::identity<ublas::row_major>,
            mpl::identity<ublas::column_major> >::type::type layout_type;

    template<int _N = 0>
    struct test_precompute
    {
        //typedef typename space_1_type::basis_0_type::template precompute<_N>::type type;
        typedef typename space_1_type::basis_0_type::PreCompute type;
        typedef boost::shared_ptr<type> ptrtype;
    };
    template<int _N = 0>
    struct trial_precompute
    {
        //typedef typename space_2_type::basis_0_type::template precompute<_N>::type type;
        typedef typename space_2_type::basis_0_type::PreCompute type;
        typedef boost::shared_ptr<type> ptrtype;
    };

    //@}


    /**
     * @name Inner Structures
     */
    //@{

    /**
     * \class Context
     * \brief element-wise representation of the bilinear form
     *
     * Local represents the bilinear form on a element (or a face of
     * an element) The algebraic representation of the local form is a
     * dense matrix. This local representation shall eventually be
     * used later to fill global matrices, or in matrix-vector product
     * depending on the global bilinear form representation.
     *
     *
     */
    template<typename GeomapTestContext,
             typename ExprT,
             typename IM,
             typename GeomapExprContext = GeomapTestContext,
             typename GeomapTrialContext = GeomapTestContext
             >
    class Context //: public FormContextBase<GeomapTestContext,IM,GeomapExprContext>
    {
        typedef FormContextBase<GeomapTestContext,IM,GeomapExprContext> super;

    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW


        typedef Context<GeomapTestContext,ExprT,IM,GeomapExprContext,GeomapTrialContext> form_context_type;
        typedef BilinearForm<FE1, FE2, ElemContType> form_type;
        typedef typename FE1::dof_type dof_1_type;
        typedef typename FE2::dof_type dof_2_type;

        typedef typename form_type::value_type value_type;


        typedef typename super::map_geometric_mapping_context_type map_test_geometric_mapping_context_type;
        typedef typename super::geometric_mapping_context_ptrtype test_geometric_mapping_context_ptrtype;
        typedef typename super::geometric_mapping_context_type test_geometric_mapping_context_type;
        typedef typename super::geometric_mapping_type test_geometric_mapping_type;
        typedef typename super::map_size test_map_size;
        typedef typename super::gmc1 test_gmc1;
        typedef typename super::left_gmc_ptrtype test_left_gmc_ptrtype;
        typedef typename super::right_gmc_ptrtype test_right_gmc_ptrtype;
        typedef typename super::map_left_gmc_type test_map_left_gmc_type;
        typedef typename super::map_right_gmc_type test_map_right_gmc_type;


        typedef FormContextBase<GeomapTrialContext,IM,GeomapExprContext> super2;

        typedef typename super2::map_geometric_mapping_context_type map_trial_geometric_mapping_context_type;
        typedef typename super2::geometric_mapping_context_ptrtype trial_geometric_mapping_context_ptrtype;
        typedef typename super2::geometric_mapping_context_type trial_geometric_mapping_context_type;
        typedef typename super2::geometric_mapping_type trial_geometric_mapping_type;
        typedef typename super2::map_size trial_map_size;
        typedef typename super2::gmc1 trial_gmc1;
        typedef typename super2::left_gmc_ptrtype trial_left_gmc_ptrtype;
        typedef typename super2::right_gmc_ptrtype trial_right_gmc_ptrtype;
        typedef typename super2::map_left_gmc_type trial_map_left_gmc_type;
        typedef typename super2::map_right_gmc_type trial_map_right_gmc_type;





        static const uint16_type nDim = test_geometric_mapping_type::nDim;

        typedef ExprT expression_type;

        typedef typename test_precompute<>::type test_precompute_type;
        typedef typename test_precompute<>::ptrtype test_precompute_ptrtype;
        typedef typename trial_precompute<>::type trial_precompute_type;
        typedef typename trial_precompute<>::ptrtype trial_precompute_ptrtype;

        typedef typename FE2::fe_type trial_fe_type;
        typedef boost::shared_ptr<trial_fe_type> trial_fe_ptrtype;
        typedef typename trial_fe_type::template Context< trial_geometric_mapping_context_type::context,
                trial_fe_type,
                trial_geometric_mapping_type,
                mesh_element_2_type> trial_fecontext_type;
        typedef boost::shared_ptr<trial_fecontext_type> trial_fecontext_ptrtype;
        typedef typename mpl::if_<mpl::equal_to<trial_map_size, mpl::int_<1> >,
                mpl::identity<fusion::map<fusion::pair<gmc<0>, trial_fecontext_ptrtype> > >,
                mpl::identity<fusion::map<fusion::pair<gmc<0>, trial_fecontext_ptrtype>,
                fusion::pair<gmc<1>, trial_fecontext_ptrtype> > > >::type::type map_trial_fecontext_type;

        typedef fusion::map<fusion::pair<gmc<0>, trial_fecontext_ptrtype> > map_left_trial_fecontext_type;
        typedef fusion::map<fusion::pair<trial_gmc1, trial_fecontext_ptrtype> > map_right_trial_fecontext_type;

        typedef typename FE1::fe_type test_fe_type;
        typedef boost::shared_ptr<test_fe_type> test_fe_ptrtype;
        typedef typename test_fe_type::template Context< test_geometric_mapping_context_type::context,
                test_fe_type,
                test_geometric_mapping_type,
                mesh_element_1_type> test_fecontext_type;
        typedef boost::shared_ptr<test_fecontext_type> test_fecontext_ptrtype;
        typedef typename mpl::if_<mpl::equal_to<test_map_size, mpl::int_<1> >,
                mpl::identity<fusion::map<fusion::pair<gmc<0>, test_fecontext_ptrtype> > >,
                mpl::identity<fusion::map<fusion::pair<gmc<0>, test_fecontext_ptrtype>,
                fusion::pair<gmc<1>, test_fecontext_ptrtype> > > >::type::type map_test_fecontext_type;
        typedef fusion::map<fusion::pair<gmc<0>, test_fecontext_ptrtype> > map_left_test_fecontext_type;
        typedef fusion::map<fusion::pair<test_gmc1, test_fecontext_ptrtype> > map_right_test_fecontext_type;

        //--------------------------------------------------------------------------------------//

        typedef typename super::map_geometric_mapping_expr_context_type map_geometric_mapping_expr_context_type;

        typedef typename ExprT::template tensor<map_geometric_mapping_expr_context_type,
                map_left_test_fecontext_type,
                map_left_trial_fecontext_type> eval00_expr_type;
        typedef boost::shared_ptr<eval00_expr_type> eval00_expr_ptrtype;

        typedef typename ExprT::template tensor<map_geometric_mapping_expr_context_type,
                map_left_test_fecontext_type,
                map_right_trial_fecontext_type> eval01_expr_type;
        typedef boost::shared_ptr<eval01_expr_type> eval01_expr_ptrtype;

        typedef typename ExprT::template tensor<map_geometric_mapping_expr_context_type,
                map_right_test_fecontext_type,
                map_left_trial_fecontext_type> eval10_expr_type;
        typedef boost::shared_ptr<eval10_expr_type> eval10_expr_ptrtype;

        typedef typename ExprT::template tensor<map_geometric_mapping_expr_context_type,
                map_right_test_fecontext_type,
                map_right_trial_fecontext_type> eval11_expr_type;
        typedef boost::shared_ptr<eval11_expr_type> eval11_expr_ptrtype;


        static const int rep_shape = 4;//2+(eval_expr_type::shape::M-1>0)+(eval_expr_type::shape::N-1>0);
        //typedef boost::multi_array<value_type, rep_shape> local_matrix_type;

        typedef typename FE1::dof_type test_dof_type;
        typedef typename FE2::dof_type trial_dof_type;
        static const int nDofPerElementTest = FE1::dof_type::nDofPerElement;
        static const int nDofPerElementTrial = FE2::dof_type::nDofPerElement;
        static const int nDofPerComponentTest = FE1::dof_type::fe_type::nLocalDof;
        static const int nDofPerComponentTrial = FE2::dof_type::fe_type::nLocalDof;
        static const int local_mat_traits = mpl::if_<mpl::equal_to<mpl::int_<nDofPerElementTrial>,mpl::int_<1> >,
                         mpl::int_<Eigen::ColMajor>,
                         mpl::int_<Eigen::RowMajor> >::type::value;
        typedef Eigen::Matrix<value_type, nDofPerElementTest, nDofPerElementTrial,local_mat_traits> local_matrix_type;
        typedef Eigen::Matrix<value_type, 2*nDofPerElementTest, 2*nDofPerElementTrial,Eigen::RowMajor> local2_matrix_type;
        typedef Eigen::Matrix<value_type, nDofPerComponentTest, nDofPerComponentTrial,local_mat_traits> c_local_matrix_type;
        typedef Eigen::Matrix<value_type, 2*nDofPerComponentTest, 2*nDofPerComponentTrial,Eigen::RowMajor> c_local2_matrix_type;
        typedef Eigen::Matrix<int, nDofPerElementTest, 1> local_row_type;
        typedef Eigen::Matrix<int, 2*nDofPerElementTest, 1> local2_row_type;
        typedef Eigen::Matrix<int, nDofPerElementTrial, 1> local_col_type;
        typedef Eigen::Matrix<int, 2*nDofPerElementTrial, 1> local2_col_type;

        typedef Eigen::Matrix<int, nDofPerComponentTest, 1> c_local_row_type;
        typedef Eigen::Matrix<int, 2*nDofPerComponentTest, 1> c_local2_row_type;
        typedef Eigen::Matrix<int, nDofPerComponentTrial, 1> c_local_col_type;
        typedef Eigen::Matrix<int, 2*nDofPerComponentTrial, 1> c_local2_col_type;


    public:

        template<typename Left, typename Right>
        map_trial_fecontext_type getMap( Left  left, Right right )
        {
            return getMap( left, right, boost::is_same<Left, map_trial_fecontext_type>() );
        }
        template<typename Left, typename Right>
        map_trial_fecontext_type getMap( Left  left, Right /*right*/, mpl::bool_<true> )
        {
            return left;
        }
        template<typename Left, typename Right>
        map_trial_fecontext_type getMap( Left  /*left*/, Right right, mpl::bool_<false> )
        {
            return map_trial_fecontext_type( right );
        }

        template<typename Left, typename Right>
        map_left_trial_fecontext_type getMapL( Left  left, Right right )
        {
            return getMap( left, right, boost::is_same<Left, map_left_trial_fecontext_type>() );
        }
        template<typename Left, typename Right>
        map_left_trial_fecontext_type getMapL( Left  left, Right /*right*/, mpl::bool_<true> )
        {
            return left;
        }
        template<typename Left, typename Right>
        map_left_trial_fecontext_type getMapL( Left  /*left*/, Right right, mpl::bool_<false> )
        {
            return right;
        }

        Context( form_type& __form,
                 map_test_geometric_mapping_context_type const& gmcTest,
                 map_trial_geometric_mapping_context_type const& _gmcTrial,
                 map_geometric_mapping_expr_context_type const & gmcExpr,
                 ExprT const& expr,
                 IM const& im );

        template<typename IM2>
        Context( form_type& __form,
                 map_test_geometric_mapping_context_type const& gmcTest,
                 map_trial_geometric_mapping_context_type const& _gmcTrial,
                 map_geometric_mapping_expr_context_type const & gmcExpr,
                 ExprT const& expr,
                 IM const& im,
                 IM2 const& im2 );

        template<typename IM2>
        Context( form_type& __form,
                 map_test_geometric_mapping_context_type const& gmcTest,
                 map_trial_geometric_mapping_context_type const& _gmcTrial,
                 map_geometric_mapping_expr_context_type const & gmcExpr,
                 ExprT const& expr,
                 IM const& im,
                 IM2 const& im2,
                 mpl::int_<2> );

        size_type trialElementId( size_type trial_eid ) const
            {
                size_type idElem = trial_eid;
                size_type domain_eid = idElem;
                const bool test_related_to_trial = _M_form.testSpace()->mesh()->isSubMeshFrom( _M_form.trialSpace()->mesh() );
                const bool trial_related_to_test = _M_form.trialSpace()->mesh()->isSubMeshFrom( _M_form.testSpace()->mesh() );
                if ( test_related_to_trial )
                {
                    domain_eid = _M_form.testSpace()->mesh()->subMeshToMesh( idElem );
                    DVLOG(2) << "[test_related_to_trial] test element id: "  << idElem << " trial element id : " << domain_eid << "\n";
                }
                if( trial_related_to_test )
                {
                    domain_eid = _M_form.trialSpace()->mesh()->meshToSubMesh( idElem );
                    DVLOG(2) << "[trial_related_to_test] test element id: "  << idElem << " trial element id : " << domain_eid << "\n";
                }
                return domain_eid;
            }
        size_type trialElementId( typename mesh_1_type::element_iterator it ) const
            {
                return trialElementId( it->id() );
            }
        size_type trialElementId( typename mesh_1_type::element_type const& e ) const
            {
                return trialElementId( e.id() );
            }
        bool isZero( size_type i ) const
            {
                size_type domain_eid = trialElementId( i );
                if ( domain_eid == invalid_size_type_value )
                    return true;
                return false;
            }
        bool isZero( typename mesh_1_type::element_iterator it ) const
            {
                return this->isZero( it->id() );
            }
        bool isZero( typename mesh_1_type::element_type const& e ) const
            {
                return this->isZero( e.id() );
            }

        void update( map_test_geometric_mapping_context_type const& gmcTest,
                     map_trial_geometric_mapping_context_type const& _gmcTrial,
                     map_geometric_mapping_expr_context_type const& gmcExpr );

        void updateInCaseOfInterpolate( map_test_geometric_mapping_context_type const& gmcTest,
                                        map_trial_geometric_mapping_context_type const& gmcTrial,
                                        map_geometric_mapping_expr_context_type const& gmcExpr,
                                        std::vector<boost::tuple<size_type,size_type> > const& indexLocalToQuad );

        void update( map_test_geometric_mapping_context_type const& gmcTest,
                     map_trial_geometric_mapping_context_type const& _gmcTrial,
                     map_geometric_mapping_expr_context_type const& gmcExpr,
                     mpl::int_<2> );

        void update( map_test_geometric_mapping_context_type const& gmcTest,
                     map_trial_geometric_mapping_context_type const& gmcTrial,
                     map_geometric_mapping_expr_context_type const& gmcExpr,
                     IM const& im )
        {
            M_integrator = im;
            update( gmcTest, gmcTrial, gmcExpr );
        }

        void updateInCaseOfInterpolate( map_test_geometric_mapping_context_type const& gmcTest,
                                        map_trial_geometric_mapping_context_type const& gmcTrial,
                                        map_geometric_mapping_expr_context_type const& gmcExpr,
                                        IM const& im,
                                        std::vector<boost::tuple<size_type,size_type> > const& indexLocalToQuad )
        {
            M_integrator = im;
            updateInCaseOfInterpolate( gmcTest, gmcTrial, gmcExpr, indexLocalToQuad );
        }

        void update( map_test_geometric_mapping_context_type const& gmcTest,
                     map_trial_geometric_mapping_context_type const& gmcTrial,
                     map_geometric_mapping_expr_context_type const& gmcExpr,
                     IM const& im, mpl::int_<2> )
        {
            M_integrator = im;
            update( gmcTest, gmcTrial, gmcExpr, mpl::int_<2>() );
        }

        void integrate()
        {
            integrate( mpl::int_<fusion::result_of::size<GeomapTestContext>::type::value>() );
        }
        void integrateInCaseOfInterpolate( std::vector<boost::tuple<size_type,size_type> > const& indexLocalToQuad,
                                           bool isFirstExperience )
        {
            integrateInCaseOfInterpolate( mpl::int_<fusion::result_of::size<GeomapTestContext>::type::value>(),
                                          indexLocalToQuad,
                                          isFirstExperience );
        }


        void assemble()
        {
            assemble( fusion::at_key<gmc<0> >( _M_test_gmc )->id() );
        }
        void assemble( size_type elt_0 );

        void assemble( mpl::int_<2> )
        {
            assemble( fusion::at_key<gmc<0> >( _M_test_gmc )->id(),
                      fusion::at_key<test_gmc1>( _M_test_gmc )->id() );
        }
        void assemble( size_type elt_0, size_type elt_1  );

        void assembleInCaseOfInterpolate();

        /**
         * precompute the basis function associated with the test and
         * trial space at a set of points
         */
        template<typename Pts>
        void precomputeBasisAtPoints( Pts const& pts )
        {
            _M_test_pc = test_precompute_ptrtype( new test_precompute_type( _M_form.testSpace()->fe(), pts ) );
            _M_trial_pc = trial_precompute_ptrtype( new trial_precompute_type( _M_form.trialSpace()->fe(), pts ) );
        }

        template<typename PtsTest,typename PtsTrial>
        void precomputeBasisAtPoints( PtsTest const& pts1,PtsTrial const& pts2  )
        {
            _M_test_pc = test_precompute_ptrtype( new test_precompute_type( _M_form.testSpace()->fe(), pts1 ) );
            _M_trial_pc = trial_precompute_ptrtype( new trial_precompute_type( _M_form.trialSpace()->fe(), pts2 ) );
        }


        /**
         * precompute the basis function associated with the test and
         * trial space at a set of points on the face of
         * the reference element
         */
        template<typename Pts>
        void precomputeBasisAtPoints( uint16_type __f, permutation_1_type const& __p, Pts const& pts )
        {
            _M_test_pc_face[__f][__p] = test_precompute_ptrtype( new test_precompute_type( _M_form.testSpace()->fe(), pts ) );
            //FEELPP_ASSERT( _M_test_pc_face.find(__f )->second )( __f ).error( "invalid test precompute type" );

            _M_trial_pc_face[__f][__p] = trial_precompute_ptrtype( new trial_precompute_type( _M_form.trialSpace()->fe(), pts ) );
            //FEELPP_ASSERT( _M_trial_pc_face.find(__f )->second )( __f ).error( "invalid trial precompute type" );
        }

        template<typename PtsSet>
        std::map<uint16_type, std::map<permutation_1_type,test_precompute_ptrtype> >
        precomputeTestBasisAtPoints( PtsSet const& pts )
        {
            typedef typename boost::is_same< permutation_1_type, typename QuadMapped<PtsSet>::permutation_type>::type is_same_permuation_type;
            return precomputeTestBasisAtPoints( pts, mpl::bool_<is_same_permuation_type::value>() );
        }

        template<typename PtsSet>
        std::map<uint16_type, std::map<permutation_1_type,test_precompute_ptrtype> >
        precomputeTestBasisAtPoints( PtsSet const& pts, mpl::bool_<false> )
        {
            std::map<uint16_type, std::map<permutation_1_type,test_precompute_ptrtype> > testpc;
            return testpc;
        }

        template<typename PtsSet>
        std::map<uint16_type, std::map<permutation_1_type,test_precompute_ptrtype> >
        precomputeTestBasisAtPoints( PtsSet const& pts, mpl::bool_<true> )
        {
            QuadMapped<PtsSet> qm;
            typedef typename QuadMapped<PtsSet>::permutation_type permutation_type;
            typename QuadMapped<PtsSet>::permutation_points_type ppts( qm( pts ) );

            std::map<uint16_type, std::map<permutation_type,test_precompute_ptrtype> > testpc;

            for ( uint16_type __f = 0; __f < pts.nFaces(); ++__f )
            {
                for ( permutation_type __p( permutation_type::IDENTITY );
                        __p < permutation_type( permutation_type::N_PERMUTATIONS ); ++__p )
                {
                    testpc[__f][__p] = test_precompute_ptrtype( new test_precompute_type( _M_form.testSpace()->fe(), ppts[__f].find( __p )->second ) );
                }
            }

            return testpc;
        }

        template<typename PtsSet>
        std::map<uint16_type, std::map<permutation_2_type,trial_precompute_ptrtype> >
        precomputeTrialBasisAtPoints( PtsSet const& pts )
        {
            typedef typename boost::is_same< permutation_2_type, typename QuadMapped<PtsSet>::permutation_type>::type is_same_permuation_type;
            return precomputeTrialBasisAtPoints( pts, mpl::bool_<is_same_permuation_type::value>() );
        }

        template<typename PtsSet>
        std::map<uint16_type, std::map<permutation_2_type,trial_precompute_ptrtype> >
        precomputeTrialBasisAtPoints( PtsSet const& pts,  mpl::bool_<false> )
        {
            std::map<uint16_type, std::map<permutation_2_type,trial_precompute_ptrtype> > trialpc;
            return trialpc;
        }

        template<typename PtsSet>
        std::map<uint16_type, std::map<permutation_2_type,trial_precompute_ptrtype> >
        precomputeTrialBasisAtPoints( PtsSet const& pts, mpl::bool_<true> )
        {
            QuadMapped<PtsSet> qm;
            typedef typename QuadMapped<PtsSet>::permutation_type permutation_type;
            typename QuadMapped<PtsSet>::permutation_points_type ppts( qm( pts ) );

            std::map<uint16_type, std::map<permutation_type,trial_precompute_ptrtype> > trialpc;

            for ( uint16_type __f = 0; __f < pts.nFaces(); ++__f )
            {
                for ( permutation_type __p( permutation_type::IDENTITY );
                        __p < permutation_type( permutation_type::N_PERMUTATIONS ); ++__p )
                {
                    trialpc[__f][__p] = trial_precompute_ptrtype( new trial_precompute_type( _M_form.trialSpace()->fe(), ppts[__f].find( __p )->second ) );
                }
            }

            return trialpc;
        }

        /**
         * Return the structure that holds the test basis functions
         * evaluated at a previously given set of points on the face of
         * the reference element
         *
         * \see precomputeBasisAtPoints()
         */
        test_precompute_ptrtype const& testPc( uint16_type __f,
                                               permutation_1_type __p = permutation_1_type( permutation_1_type::NO_PERMUTATION ) ) const
        {
            if ( __f == invalid_uint16_type_value )
                return  _M_test_pc;

            //FEELPP_ASSERT( _M_test_pc_face.find(__f )->second )( __f ).error( "invalid test precompute type" );
            return _M_test_pc_face.find( __f )->second.find( __p )->second;
        }

        /**
         * Return the structure that holds the trial basis functions
         * evaluated at a previously given set of points on the face of
         * the reference element
         * \see precomputeBasisAtPoints()
         */
        trial_precompute_ptrtype const& trialPc( uint16_type __f,
                permutation_2_type __p = permutation_2_type( permutation_2_type::NO_PERMUTATION ) ) const
        {
            if ( __f == invalid_uint16_type_value )
                return  _M_trial_pc;

            //FEELPP_ASSERT( _M_trial_pc_face.find(__f )->second )( __f ).error( "invalid trial precompute type" );
            return _M_trial_pc_face.find( __f )->second.find( __p )->second;
        }


    private:

        void update( map_test_geometric_mapping_context_type const& gmcTest,
                     map_trial_geometric_mapping_context_type const& gmcTrial,
                     map_geometric_mapping_expr_context_type const& gmcExpr,
                     mpl::bool_<false> );

        void update( map_test_geometric_mapping_context_type const& gmcTest,
                     map_trial_geometric_mapping_context_type const& gmcTrial,
                     map_geometric_mapping_expr_context_type const& gmcExpr,
                     mpl::bool_<true> );

        void updateInCaseOfInterpolate( map_test_geometric_mapping_context_type const& gmcTest,
                                        map_trial_geometric_mapping_context_type const& gmcTrial,
                                        map_geometric_mapping_expr_context_type const& gmcExpr,
                                        mpl::bool_<false> );

        void updateInCaseOfInterpolate( map_test_geometric_mapping_context_type const& gmcTest,
                                        map_trial_geometric_mapping_context_type const& gmcTrial,
                                        map_geometric_mapping_expr_context_type const& gmcExpr,
                                        mpl::bool_<true> );


        void integrate( mpl::int_<1> );

        void integrate( mpl::int_<2> );

        void integrateInCaseOfInterpolate( mpl::int_<1>,
                                           std::vector<boost::tuple<size_type,size_type> > const& indexLocalToQuad,
                                           bool isFirstExperience );
    private:

        form_type& _M_form;
        const list_block_type& _M_lb;
        dof_1_type* _M_test_dof;
        dof_2_type* _M_trial_dof;

        test_precompute_ptrtype _M_test_pc;
        std::map<uint16_type, std::map<permutation_1_type,test_precompute_ptrtype> > _M_test_pc_face;
        trial_precompute_ptrtype _M_trial_pc;
        std::map<uint16_type, std::map<permutation_2_type, trial_precompute_ptrtype> > _M_trial_pc_face;


        map_test_geometric_mapping_context_type _M_test_gmc;
        map_trial_geometric_mapping_context_type _M_trial_gmc;

        map_test_fecontext_type _M_test_fec;
        map_left_test_fecontext_type _M_test_fec0;
        map_right_test_fecontext_type _M_test_fec1;
        map_trial_fecontext_type _M_trial_fec;
        map_left_trial_fecontext_type _M_trial_fec0;
        map_right_trial_fecontext_type _M_trial_fec1;


        local_matrix_type _M_rep;
        local2_matrix_type _M_rep_2;
        local_row_type M_local_rows;
        local2_row_type M_local_rows_2;
        local_col_type M_local_cols;
        local2_col_type M_local_cols_2;
        local_row_type M_local_rowsigns;
        local2_row_type M_local_rowsigns_2;
        local_col_type M_local_colsigns;
        local2_col_type M_local_colsigns_2;

        c_local_matrix_type M_c_rep;
        c_local2_matrix_type M_c_rep_2;
        c_local_row_type M_c_local_rows;
        c_local2_row_type M_c_local_rows_2;
        c_local_col_type M_c_local_cols;
        c_local2_col_type M_c_local_cols_2;
        c_local_row_type M_c_local_rowsigns;
        c_local2_row_type M_c_local_rowsigns_2;
        c_local_col_type M_c_local_colsigns;
        c_local2_col_type M_c_local_colsigns_2;

        eval00_expr_ptrtype _M_eval_expr00;
        eval01_expr_ptrtype _M_eval_expr01;
        eval10_expr_ptrtype _M_eval_expr10;
        eval11_expr_ptrtype _M_eval_expr11;

        IM M_integrator;

    }; // Context

    //@}

    /** @name Constructors, destructor
     */
    //@{

    /**

     */
    BilinearForm( space_1_ptrtype const& __X1,
                  space_2_ptrtype const& __X2,
                  matrix_ptrtype& __M,
                  size_type rowstart = 0,
                  size_type colstart = 0,
                  bool build = true,
                  bool do_threshold = false,
                  value_type threshold = type_traits<value_type>::epsilon(),
                  size_type graph_hints = Pattern::COUPLED );

    BilinearForm( space_1_ptrtype const& __X1,
                  space_2_ptrtype const& __X2,
                  matrix_ptrtype& __M,
                  list_block_type const& __lb,
                  size_type rowstart = 0,
                  size_type colstart = 0,
                  bool do_threshold = false,
                  value_type threshold = type_traits<value_type>::epsilon(),
                  size_type graph_hints = Pattern::COUPLED );

    BilinearForm( BilinearForm const& __vf )
        :
        M_pattern( __vf.M_pattern ),
        _M_X1( __vf._M_X1 ),
        _M_X2( __vf._M_X2 ),
        _M_matrix( __vf._M_matrix ),
        _M_lb( __vf._M_lb ),
        _M_row_startInMatrix( __vf._M_row_startInMatrix ),
        _M_col_startInMatrix( __vf._M_col_startInMatrix ),
        _M_do_threshold( __vf._M_do_threshold ),
        _M_threshold( __vf._M_threshold )
    {}

    ~BilinearForm()
    {}



    //@}

    /** @name Operator overloads
     */
    //@{

    /**
     * copy operator
     */
    BilinearForm&
    operator=( BilinearForm const& form )
    {
        if ( this != &form )
        {
            M_pattern = form.M_pattern;
            _M_X1 = form._M_X1;
            _M_X2 = form._M_X2;
            _M_matrix = form._M_matrix;
            _M_row_startInMatrix = form._M_row_startInMatrix;
            _M_col_startInMatrix = form._M_col_startInMatrix;
            _M_lb = form._M_lb;
        }

        return *this;
    }

    /**
     * expression assignment
     */
    template <class ExprT>
    BilinearForm& operator=( Expr<ExprT> const& expr );

    /**
     * plus expression assignment
     */
    template <class ExprT>
    BilinearForm& operator+=( Expr<ExprT> const& expr );

    /**
     * Computes the energy norm associated with the bilinear form
     *
     * @param __v element of Space 1 (test space)
     * @param __u element of Space 2 (trial space)
     * @return the energy norm
     */
    value_type operator()( element_1_type const& __v,  element_2_type const& __u ) const
    {
        return _M_matrix->energy( __v, __u );
    }

    /**
     * \return the entry \f$M_{i,j}\f$
     */
    // value_type& operator()( size_type i, size_type j ) { return _M_matrix( i, j ); }
    //@}

    /** @name Accessors
     */
    //@{

    /**
     * return the pattern
     */
    size_type pattern() const
    {
        return M_pattern;
    }

    /**
     * \return true if the pattern is coupled with respect to the components,
     * false otherwise
     */
    bool isPatternCoupled() const
    {
        Feel::Context ctx( M_pattern );
        return ctx.test( Pattern::COUPLED );
    }

    /**
     * \return true if the pattern is the default one, false otherwise
     */
    bool isPatternDefault() const
    {
        Feel::Context ctx( M_pattern );
        return ctx.test( Pattern::DEFAULT );
    }

    /**
     * \return true if the pattern adds the neighboring elements, false otherwise
     */
    bool isPatternNeighbor() const
    {
        Feel::Context ctx( M_pattern );
        return ctx.test( Pattern::EXTENDED );
    }
    bool isPatternExtended() const
    {
        Feel::Context ctx( M_pattern );
        return ctx.test( Pattern::EXTENDED );
    }

    bool isPatternSymmetric() const
    {
        Feel::Context ctx( M_pattern );
        return ctx.test( Pattern::SYMMETRIC );
    }

    /**
     * \return the trial function space
     */
    space_2_ptrtype const& trialSpace() const
    {
        return _M_X2;
    }

    /**
     * \return the test function space
     */
    space_1_ptrtype const& testSpace() const
    {
        return _M_X1;
    }

    /**
     * Geometric transformation
     */
    gm_1_ptrtype const& gm() const
    {
        return _M_X1->gm();
    }

    /**
     * Geometric transformation
     */
    gm1_1_ptrtype const& gm1() const
    {
        return _M_X1->gm1();
    }

    /**
     * Geometric transformation
     */
    gm_2_ptrtype const& gmTrial() const
    {
        return _M_X2->gm();
    }

    /**
     * Geometric transformation
     */
    gm1_2_ptrtype const& gm1Trial() const
    {
        return _M_X2->gm1();
    }


    /**
     * \return the matrix associated to the bilinear form
     */
    matrix_type const& matrix() const
    {
        return *_M_matrix;
    }

    matrix_type& matrix()
    {
        return *_M_matrix;
    }

    matrix_ptrtype const& matrixPtr() const
    {
        return _M_matrix;
    }

    matrix_ptrtype& matrixPtr()
    {
        return _M_matrix;
    }

    list_block_type const& blockList() const
    {
        return _M_lb;
    }

    size_type rowStartInMatrix() const
    {
        return _M_row_startInMatrix;
    }

    size_type colStartInMatrix() const
    {
        return _M_col_startInMatrix;
    }

    /**
     * \return the threshold
     */
    value_type threshold() const
    {
        return _M_threshold;
    }

    /**
     * \return \c true if threshold applies, false otherwise
     */
    bool doThreshold( value_type const& v ) const
    {
        return ( math::abs( v ) > _M_threshold );
    }

    /**
     * return true if do threshold. false otherwise
     */
    bool doThreshold() const
    {
        return _M_do_threshold;
    }

    //@}

    /** @name  Mutators
     */
    //@{

    /**
     * set a threshold value for the matrix entries associated with the
     * bilinear form
     */
    void setThreshold( value_type eps )
    {
        _M_threshold = eps;
    }

    /**
     * set the threshold strategy, true threshold the matrix entries,
     * false do not threshold
     */
    void setDoThreshold( bool do_threshold )
    {
        _M_do_threshold = do_threshold;
    }

    //@}

    /** @name  Methods
     */
    //@{


    // close matrix
    void close() { _M_matrix->close(); }


    /**
     * Diagonalize representation(matrix) associated to the \p
     * BilinearForm at selected dofs \p dofs by putting 0 on
     * the extra diagonal terms and 1 on the diagonal.
     *
     * If \p ON_ELIMINATION_KEEP_DIAGONAL is set in \p on_context then
     * the diagonal value of the matrix is kept and the right habd
     * side \p rhs is modified accordingly.
     */
    void zeroRows( std::vector<int> const& __dofs,
                   std::vector<value_type> const& __values,
                   Vector<value_type>& rhs,
                   Feel::Context const& on_context );

    /**
     * add value \p v at position (\p i, \p j) of the matrix
     * associated with the bilinear form
     */
    void add( size_type i,  size_type j,  value_type const& v )
    {
        if ( _M_do_threshold )
        {
            if ( doThreshold( v ) )
                _M_matrix->add( i+this->rowStartInMatrix(),
                                j+this->colStartInMatrix(),
                                v );
        }

        else
            _M_matrix->add( i+this->rowStartInMatrix(),
                            j+this->colStartInMatrix(),
                            v );

    }
    /**
     * add value \p v at position (\p i, \p j) of the matrix
     * associated with the bilinear form
     */
    void addMatrix( int* rows, int nrows,
                    int* cols, int ncols,
                    value_type* data )
    {
        if ( this->rowStartInMatrix()!=0 )
            for ( int i=0; i<nrows; ++i )
                rows[i]+=this->rowStartInMatrix();

        if ( this->colStartInMatrix()!=0 )
            for ( int i=0; i<ncols; ++i )
                cols[i]+=this->colStartInMatrix();

        _M_matrix->addMatrix( rows, nrows, cols, ncols, data );
    }



    /**
     * set value \p v at position (\p i, \p j) of the matrix
     * associated with the bilinear form
     */
    void set( size_type i,  size_type j,  value_type const& v )
    {
        _M_matrix->set( i, j, v );
    }

    void addToNOz( size_type i, size_type n )
    {
        M_n_oz[i] += n;
    }
    void addToNNz( size_type i, size_type n )
    {
        M_n_nz[i] += n;
    }
    size_type nOz( size_type i ) const
    {
        return M_n_oz[i];
    }
    size_type nNz( size_type i ) const
    {
        return M_n_nz[i];
    }

    BOOST_PARAMETER_MEMBER_FUNCTION( ( typename Backend<value_type>::solve_return_type ),
                                     solve,
                                     tag,
                                     ( required
                                       ( in_out( solution ),* )
                                       ( rhs, * ) )
                                     ( optional
                                       ( name,           ( std::string ), "" )
                                       ( kind,           ( BackendType ), BACKEND_PETSC )
                                       ( rebuild,        ( bool ), false )) )
        {
            return backend( _name=name, _kind=kind, _rebuild=rebuild )->solve( _matrix=this->matrixPtr(), _rhs=rhs.vectorPtr(),
                                                                               _solution=solution);
        }

    //@}

protected:

    BilinearForm();

private:


    template <class ExprT> void assign( Expr<ExprT> const& __expr, bool init, mpl::bool_<false> );
    template <class ExprT> void assign( Expr<ExprT> const& __expr, bool init, mpl::bool_<true> );
    template <class ExprT> void assign( Expr<ExprT> const& __expr, mpl::bool_<true>, mpl::bool_<true> );
    template <class ExprT> void assign( Expr<ExprT> const& __expr, mpl::bool_<true>, mpl::bool_<false> );
    template <class ExprT> void assign( Expr<ExprT> const& __expr, mpl::bool_<true>, mpl::bool_<false>, mpl::bool_<true> );
    template <class ExprT> void assign( Expr<ExprT> const& __expr, mpl::bool_<true>, mpl::bool_<false>, mpl::bool_<false> );

private:

    size_type M_pattern;

    space_1_ptrtype _M_X1;
    space_2_ptrtype _M_X2;

    matrix_ptrtype _M_matrix;

    bool _M_do_build;

    list_block_type _M_lb;
    size_type _M_row_startInMatrix,_M_col_startInMatrix;

    bool _M_do_threshold;
    value_type _M_threshold;

    std::vector<size_type> M_n_nz;
    std::vector<size_type> M_n_oz;

};
template<typename FE1,  typename FE2, typename ElemContType>
BilinearForm<FE1, FE2, ElemContType>::BilinearForm( space_1_ptrtype const& Xh,
        space_2_ptrtype const& Yh,
        matrix_ptrtype& __M,
        size_type rowstart,
        size_type colstart,
        bool build,
        bool do_threshold,
        value_type threshold,
        size_type graph_hints )
    :
    M_pattern( graph_hints ),
    _M_X1( Xh ),
    _M_X2( Yh ),
    _M_matrix( __M ),
    _M_do_build( build ),
    _M_lb(),
    _M_row_startInMatrix( rowstart ),
    _M_col_startInMatrix( colstart ),
    _M_do_threshold( do_threshold ),
    _M_threshold( threshold )
{
    boost::timer tim;
    DVLOG(2) << "begin constructor with default listblock\n";

    if ( !_M_matrix ) _M_matrix = backend()->newMatrix( _test=_M_X1, _trial=_M_X2 );
    _M_lb.push_back( Block ( 0, 0, 0, 0 ) );

    DVLOG(2) << " - form init in " << tim.elapsed() << "\n";
    DVLOG(2) << "begin constructor with default listblock done\n";
}

template<typename FE1,  typename FE2, typename ElemContType>
BilinearForm<FE1, FE2, ElemContType>::BilinearForm( space_1_ptrtype const& Xh,
        space_2_ptrtype const& Yh,
        matrix_ptrtype& __M,
        list_block_type const& __lb,
        size_type rowstart,
        size_type colstart,
        bool do_threshold ,
        value_type threshold,
        size_type graph_hints )
    :
    M_pattern( graph_hints ),
    _M_X1( Xh ),
    _M_X2( Yh ),
    _M_matrix( __M ),
    _M_do_build( false ),
    _M_lb( __lb ),
    _M_row_startInMatrix( rowstart ),
    _M_col_startInMatrix( colstart ),
    _M_do_threshold( do_threshold ),
    _M_threshold( threshold )
{
    if ( !_M_matrix ) _M_matrix = backend()->newMatrix( _test=_M_X1, _trial=_M_X2 );
}

template<typename FE1,  typename FE2,  typename ElemContType>
template<typename ExprT>
void
BilinearForm<FE1, FE2, ElemContType>::assign( Expr<ExprT> const& __expr,
        bool init,
        mpl::bool_<false> )
{
    if ( init )
    {
        DVLOG(2) << "[BilinearForm::assign<false>] start\n";
        typedef ublas::matrix_range<matrix_type> matrix_range_type;
        typename list_block_type::const_iterator __bit = _M_lb.begin();
        typename list_block_type::const_iterator __ben = _M_lb.end();

        for ( ; __bit != __ben; ++__bit )
        {
            size_type g_ic_start = _M_row_startInMatrix + __bit->globalRowStart();
            size_type g_jc_start = _M_col_startInMatrix + __bit->globalColumnStart();
            _M_matrix->zero( g_ic_start, g_ic_start + _M_X1->nDof(),
                            g_jc_start, g_jc_start + _M_X2->nDof() );
        }
    }

    __expr.assemble( _M_X1, _M_X2, *this );

    DVLOG(2) << "[BilinearForm::assign<false>] stop\n";

}
template<typename FE1,  typename FE2,  typename ElemContType>
template<typename ExprT>
void
BilinearForm<FE1, FE2, ElemContType>::assign( Expr<ExprT> const& __expr,
        bool init,
        mpl::bool_<true> )
{
    DVLOG(2) << "BilinearForm::assign() start loop on test spaces\n";

    if ( init ) _M_matrix->zero();

    assign( __expr, mpl::bool_<true>(), mpl::bool_<( FE1::nSpaces > 1 && FE2::nSpaces > 1 )>() );
    DVLOG(2) << "BilinearForm::assign() stop loop on test spaces\n";

}
template<typename FE1,  typename FE2,  typename ElemContType>
template<typename ExprT>
void
BilinearForm<FE1, FE2, ElemContType>::assign( Expr<ExprT> const& __expr,
        mpl::bool_<true>,
        mpl::bool_<true> )
{
    fusion::for_each( _M_X1->functionSpaces(), make_bfassign2( *this, __expr ) );
}
template<typename FE1,  typename FE2,  typename ElemContType>
template<typename ExprT>
void
BilinearForm<FE1, FE2, ElemContType>::assign( Expr<ExprT> const& __expr,
        mpl::bool_<true>,
        mpl::bool_<false> )
{
    assign( __expr, mpl::bool_<true>(), mpl::bool_<false>(), mpl::bool_<( FE1::nSpaces > 1 )>() );
}

template<typename FE1,  typename FE2,  typename ElemContType>
template<typename ExprT>
void
BilinearForm<FE1, FE2, ElemContType>::assign( Expr<ExprT> const& __expr,
        mpl::bool_<true>,
        mpl::bool_<false>,
        mpl::bool_<true> )
{
    fusion::for_each( _M_X1->functionSpaces(), make_bfassign3( *this, __expr, _M_X2, 0 ) );
}
template<typename FE1,  typename FE2,  typename ElemContType>
template<typename ExprT>
void
BilinearForm<FE1, FE2, ElemContType>::assign( Expr<ExprT> const& __expr,
        mpl::bool_<true>,
        mpl::bool_<false>,
        mpl::bool_<false> )
{
    fusion::for_each( _M_X2->functionSpaces(), make_bfassign1( *this, __expr, _M_X1, 0 ) );

}
template<typename FE1,  typename FE2,  typename ElemContType>
template<typename ExprT>
BilinearForm<FE1, FE2, ElemContType>&
BilinearForm<FE1, FE2, ElemContType>::operator=( Expr<ExprT> const& __expr )
{
    // loop(fusion::for_each) over sub-functionspaces in SpaceType
    // pass expression and initialize
    this->assign( __expr, true, mpl::bool_<mpl::or_< mpl::bool_< ( FE1::nSpaces > 1 )>,
                  mpl::bool_< ( FE2::nSpaces > 1 )> >::type::value >() );
    return *this;
}

template<typename FE1,  typename FE2,  typename ElemContType>
template<typename ExprT>
BilinearForm<FE1, FE2, ElemContType>&
BilinearForm<FE1, FE2, ElemContType>::operator+=( Expr<ExprT> const& __expr )
{
    DVLOG(2) << "[BilinearForm::operator+=] start\n";
    this->assign( __expr, false, mpl::bool_<mpl::or_< mpl::bool_< ( FE1::nSpaces > 1 )>,
                  mpl::bool_< ( FE2::nSpaces > 1 )> >::type::value >() );
    DVLOG(2) << "[BilinearForm::operator+=] stop\n";
    return *this;
}


template<typename FE1,  typename FE2, typename ElemContType>
void
BilinearForm<FE1,FE2,ElemContType>::zeroRows( std::vector<int> const& __dofs,
        std::vector<value_type> const&__values,
        Vector<value_type>& rhs,
        Feel::Context const& on_context )
{
    _M_matrix->zeroRows( __dofs, __values, rhs, on_context );
}


template<typename BFType, typename ExprType, typename TestSpaceType>
template<typename SpaceType>
void BFAssign1<BFType,ExprType,TestSpaceType>::operator()( boost::shared_ptr<SpaceType> const& trial ) const
{
    if ( _M_bf.testSpace()->worldsComm()[_M_test_index].isActive() )
    {
        DVLOG(2) << "[BFAssign1::operator()] expression has test functions index "
                      << _M_test_index << " : "
                      << ExprType::template HasTestFunction<typename TestSpaceType::reference_element_type>::result << " (0:no, 1:yes)\n";
        DVLOG(2) << "[BFAssign1::operator()] expression has trial functions index "
                      << _M_trial_index << " :"
                      << ExprType::template HasTrialFunction<typename SpaceType::reference_element_type>::result << " (0:no, 1:yes)\n";

        if ( !ExprType::template HasTestFunction<typename TestSpaceType::reference_element_type>::result ||
                !ExprType::template HasTrialFunction<typename SpaceType::reference_element_type>::result )
        {
            ++_M_trial_index;
            return;
        }

        DVLOG(2) << "[BFAssign1::operator()] terms found with "
                      << "testindex: " << _M_test_index << " trialindex: " << _M_trial_index << "\n";
        typedef SpaceType trial_space_type;
        typedef TestSpaceType test_space_type;

        Feel::vf::list_block_type list_block;

        if ( _M_bf.testSpace()->worldsComm()[_M_test_index].globalSize()>1 )
        {
            //DVLOG(2) << "[BFAssign1::operator()] block: " << block << "\n";
            if (_M_bf.testSpace()->hasEntriesForAllSpaces())
                list_block.push_back( Feel::vf::Block( 0, 0,
                                                       _M_bf.testSpace()->nLocalDofStart( _M_test_index ),
                                                       _M_bf.trialSpace()->nLocalDofStart( _M_trial_index ) ) );
            else
                list_block.push_back( Feel::vf::Block( 0, 0, 0, _M_bf.trialSpace()->nLocalDofStart( _M_trial_index ) ) );

        }

        else
        {
            //DVLOG(2) << "[BFAssign1::operator()] block: " << block << "\n";
            list_block.push_back( Feel::vf::Block( 0,0,
                                                   _M_bf.testSpace()->nDofStart( _M_test_index ),
                                                   _M_bf.trialSpace()->nDofStart( _M_trial_index ) ) );
        }

        typedef typename BFType::matrix_type matrix_type;
        typedef Feel::vf::detail::BilinearForm<test_space_type,
                trial_space_type,
                ublas::vector_range<ublas::vector<double> > > bf_type;

        bf_type bf( _M_test,trial, _M_bf.matrixPtr(), list_block,  _M_bf.rowStartInMatrix(), _M_bf.colStartInMatrix(), _M_bf.doThreshold(), _M_bf.threshold(), _M_bf.pattern()  );

        bf += _M_expr;
    }

    ++_M_trial_index;
}

template<typename BFType, typename ExprType, typename TrialSpaceType>
template<typename SpaceType>
void BFAssign3<BFType,ExprType,TrialSpaceType>::operator()( boost::shared_ptr<SpaceType> const& test ) const
{
    if ( _M_bf.testSpace()->worldsComm()[_M_test_index].isActive() )
    {

        DVLOG(2) << "[BFAssign3::operator()] expression has trial functions index "
                      << _M_test_index << " : "
                      << ExprType::template HasTestFunction<typename SpaceType::reference_element_type>::result << " (0:no, 1:yes)\n";
        DVLOG(2) << "[BFAssign3::operator()] expression has test functions index "
                      << _M_trial_index << " :"
                      << ExprType::template HasTrialFunction<typename TrialSpaceType::reference_element_type>::result << " (0:no, 1:yes)\n";

        if ( !ExprType::template HasTrialFunction<typename TrialSpaceType::reference_element_type>::result ||
                !ExprType::template HasTestFunction<typename SpaceType::reference_element_type>::result )
        {
            ++_M_test_index;
            return;
        }

        DVLOG(2) << "[BFAssign3::operator()] terms found with "
                      << "testindex: " << _M_test_index << " trialindex: " << _M_trial_index << "\n";
        typedef SpaceType test_space_type;
        typedef TrialSpaceType trial_space_type;

        //Feel::vf::Block block ( 0, 0,
        //                        _M_bf.testSpace()->nDofStart( _M_test_index ),
        //                        _M_bf.trialSpace()->nDofStart( _M_trial_index ) );
        Feel::vf::list_block_type list_block;

        // with mpi, dof start to 0 (thanks to the LocalToGlobal mapping).
        if ( _M_bf.testSpace()->worldsComm()[_M_test_index].globalSize()>1 )
        {
            //DVLOG(2) << "[BFAssign1::operator()] block: " << block << "\n";
            if (_M_bf.testSpace()->hasEntriesForAllSpaces())
                list_block.push_back( Feel::vf::Block( 0, 0,
                                                       _M_bf.testSpace()->nLocalDofStart( _M_test_index ),
                                                       _M_bf.trialSpace()->nLocalDofStart( _M_trial_index ) ) );
            else
                list_block.push_back( Feel::vf::Block( 0, 0, 0, _M_bf.trialSpace()->nLocalDofStart( _M_trial_index ) ) );

        }

        else
        {
            //DVLOG(2) << "[BFAssign1::operator()] block: " << block << "\n";
            list_block.push_back( Feel::vf::Block( 0,0,
                                                   _M_bf.testSpace()->nDofStart( _M_test_index ),
                                                   _M_bf.trialSpace()->nDofStart( _M_trial_index ) ) );
        }

        typedef typename BFType::matrix_type matrix_type;
        typedef Feel::vf::detail::BilinearForm<test_space_type,
                trial_space_type,
                ublas::vector_range<ublas::vector<double> > > bf_type;

        bf_type bf( test, _M_trial, _M_bf.matrixPtr(), list_block, _M_bf.rowStartInMatrix(), _M_bf.colStartInMatrix(), _M_bf.doThreshold(), _M_bf.threshold(), _M_bf.pattern() );

        bf += _M_expr;
    }

    ++_M_test_index;
}


} // detail
} // vf
/// \endcond
} // feel

#include <feel/feelvf/bilinearformcontext.hpp>

#endif /* __BilinearForm_H */

