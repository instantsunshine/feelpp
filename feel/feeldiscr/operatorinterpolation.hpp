/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2008-02-01

  Copyright (C) 2008-2012 Universite Joseph Fourier (Grenoble I)

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
   \file operatorinterpolation.hpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \author Chabannes Vincent <vincent.chabannes@imag.fr>
   \date 2008-02-01
 */
#ifndef __OperatorInterpolation_H
#define __OperatorInterpolation_H 1

#include <feel/feeldiscr/operatorlinear.hpp>

namespace Feel
{

class InterpolationTypeBase
{
public :
    InterpolationTypeBase( bool useComm=true, bool compAreSamePt=true, bool onlyLocalizeOnBoundary=false, int nbNearNeighborInKdTree=15 )
    :
        M_searchWithCommunication( useComm ),
        M_componentsAreSamePoint( compAreSamePt ),
        M_onlyLocalizeOnBoundary( onlyLocalizeOnBoundary ),
        M_nbNearNeighborInKdTree( nbNearNeighborInKdTree )
    {}

    InterpolationTypeBase( InterpolationTypeBase const& a)
    :
        M_searchWithCommunication( a.M_searchWithCommunication ),
        M_componentsAreSamePoint( a.M_componentsAreSamePoint ),
        M_onlyLocalizeOnBoundary( a.M_onlyLocalizeOnBoundary ),
        M_nbNearNeighborInKdTree( a.M_nbNearNeighborInKdTree )
    {}

    bool searchWithCommunication() const { return M_searchWithCommunication; }
    bool componentsAreSamePoint() const { return M_componentsAreSamePoint; }
    bool onlyLocalizeOnBoundary() const { return M_onlyLocalizeOnBoundary; }
    int nbNearNeighborInKdTree() const { return M_nbNearNeighborInKdTree; }

private :

    bool M_searchWithCommunication;
    bool M_componentsAreSamePoint;
    bool M_onlyLocalizeOnBoundary;
    int M_nbNearNeighborInKdTree;
};

struct InterpolationNonConforme : public InterpolationTypeBase
{
    static const uint16_type value=0;

    InterpolationNonConforme( bool useComm=true, bool compAreSamePt=true, bool onlyLocalizeOnBoundary=false, int nbNearNeighborInKdTree=15 )
    :
        InterpolationTypeBase(useComm,compAreSamePt, onlyLocalizeOnBoundary,nbNearNeighborInKdTree)
    {}
};

struct InterpolationConforme : public InterpolationTypeBase
{
    static const uint16_type value=1;

    InterpolationConforme(bool useComm=true, bool compAreSamePt=true, bool onlyLocalizeOnBoundary=false, int nbNearNeighborInKdTree=15 )
    :
        InterpolationTypeBase(useComm,compAreSamePt,onlyLocalizeOnBoundary,nbNearNeighborInKdTree)
    {}
};


namespace detailsup
{

template < typename EltType >
size_type
idElt( EltType & elt,mpl::size_t<MESH_ELEMENTS> )
{
    return elt.id();
}

template < typename EltType >
size_type
idElt( EltType & elt,mpl::size_t<MESH_FACES> )
{
    return elt.element0().id();
}


} //detailsup


/**
 * \class OperatorInterpolation
 * \brief Global interpolation operator
 *
 * @author Christophe Prud'homme
 * @see
 */

template<typename DomainSpaceType,
         typename ImageSpaceType,
         typename IteratorRange = boost::tuple<mpl::size_t<MESH_ELEMENTS>,
         typename MeshTraits<typename ImageSpaceType::mesh_type>::element_const_iterator,
         typename MeshTraits<typename ImageSpaceType::mesh_type>::element_const_iterator>,
         typename InterpType = InterpolationNonConforme >
class OperatorInterpolation : public OperatorLinear<DomainSpaceType, ImageSpaceType >
{
    typedef OperatorLinear<DomainSpaceType, ImageSpaceType> super;
public:


    /** @name Typedefs
     */
    //@{

    /*
     * domain
     */
    typedef typename super::domain_space_type domain_space_type;
    typedef typename super::domain_space_ptrtype domain_space_ptrtype;
    typedef typename domain_space_type::mesh_type domain_mesh_type;
    typedef typename domain_mesh_type::element_type domain_geoelement_type;
    typedef typename domain_mesh_type::element_iterator domain_mesh_element_iterator;


    typedef typename super::backend_ptrtype backend_ptrtype;



    /*
     * image
     */
    typedef typename super::dual_image_space_type dual_image_space_type;
    typedef typename super::dual_image_space_ptrtype dual_image_space_ptrtype;
    typedef typename dual_image_space_type::value_type value_type;
    typedef typename dual_image_space_type::mesh_type image_mesh_type;
    typedef typename image_mesh_type::element_type image_geoelement_type;
    typedef typename image_mesh_type::element_iterator image_mesh_element_iterator;

    // geometric mapping context
    typedef typename image_mesh_type::gm_type image_gm_type;
    typedef typename image_mesh_type::gm_ptrtype image_gm_ptrtype;
    typedef typename image_mesh_type::template gmc<vm::POINT>::type image_gmc_type;
    typedef typename image_mesh_type::template gmc<vm::POINT>::ptrtype image_gmc_ptrtype;

    // dof
    typedef typename dual_image_space_type::dof_type dof_type;

    // basis
    typedef typename dual_image_space_type::basis_type image_basis_type;
    typedef typename domain_space_type::basis_type domain_basis_type;

    typedef typename boost::tuples::template element<0, IteratorRange>::type idim_type;
    typedef typename boost::tuples::template element<1, IteratorRange>::type iterator_type;
    typedef IteratorRange range_iterator;


    static const uint16_type nLocalDofInDualImageElt = mpl::if_< mpl::equal_to< idim_type ,mpl::size_t<MESH_ELEMENTS> >,
                             mpl::int_< image_basis_type::nLocalDof > ,
                             mpl::int_< image_mesh_type::face_type::numVertices*dual_image_space_type::fe_type::nDofPerVertex +
                             image_mesh_type::face_type::numEdges*dual_image_space_type::fe_type::nDofPerEdge +
                             image_mesh_type::face_type::numFaces*dual_image_space_type::fe_type::nDofPerFace > >::type::value;

    // type conforme or non conforme
    typedef InterpType interpolation_type;

    // matrix graph
    typedef GraphCSR graph_type;
    typedef boost::shared_ptr<graph_type> graph_ptrtype;

    // node type
    typedef typename matrix_node<typename image_mesh_type::value_type>::type matrix_node_type;


    //@}

    /** @name Constructors, destructor
     */
    //@{

    /**
     * default constructor
     */
    OperatorInterpolation() : super() {}

    /**
     * Construction the global interpolation operator from \p
     * domainspace to \p imagespace and represent it in matrix form
     * using the backend \p backend
     */
    OperatorInterpolation( domain_space_ptrtype const& domainspace,
                           dual_image_space_ptrtype const& imagespace,
                           backend_ptrtype const& backend,
                           InterpType const& interptype,
                           bool ddmethod=false);

    OperatorInterpolation( domain_space_ptrtype const& domainspace,
                           dual_image_space_ptrtype const& imagespace,
                           IteratorRange const& r,
                           backend_ptrtype const& backend,
                           InterpType const& interptype,
                           bool ddmethod=false);


    /**
     * copy constructor
     */
    OperatorInterpolation( OperatorInterpolation const & oi )
        :
        super( oi ),
        _M_range( oi._M_range ),
        _M_WorldCommFusion( oi._M_WorldCommFusion ),
        _M_interptype( oi._M_interptype )
    {}

    ~OperatorInterpolation() {}

    //@}

    /** @name Operator overloads
     */
    //@{


    //@}

    /** @name Accessors
     */
    //@{

    WorldComm const& worldCommFusion() const { return _M_WorldCommFusion; }

    InterpType const& interpolationType() const { return _M_interptype; }

    bool isDomainMeshRelatedToImageMesh() const { return this->domainSpace()->mesh()->isSubMeshFrom( this->dualImageSpace()->mesh() ); }

    bool isImageMeshRelatedToDomainMesh() const { return this->dualImageSpace()->mesh()->isSubMeshFrom( this->domainSpace()->mesh() ); }

    //@}

    /** @name  Mutators
     */
    //@{


    //@}

    /** @name  Methods
     */
    //@{


    //@}



protected:

    virtual void update();

private:

    void updateSameMesh();
    void updateNoRelationMesh();
#if defined(FEELPP_ENABLE_MPI_MODE)
    void updateNoRelationMeshMPI();
    void updateNoRelationMeshMPI_run(bool buildNonZeroMatrix=true);

    typedef std::vector<std::list<boost::tuple<int,
                                               typename image_mesh_type::node_type,
                                               typename image_mesh_type::node_type,
                                               std::vector<std::pair<size_type,size_type> >, // col gdof and gdofInGlobalCluster
                                               uint16_type // comp
                                               > > > extrapolation_memory_type;

    // point distrubition
    boost::tuple<std::vector< std::vector<size_type> >,
                 std::vector< std::vector<uint16_type> >,
                 std::vector<std::vector<typename image_mesh_type::node_type> >,
                 std::vector<std::vector< std::vector<typename image_mesh_type::node_type > > > >
    updateNoRelationMeshMPI_pointDistribution(const std::vector< std::list<boost::tuple<int,size_type,double> > > & memory_valueInMatrix,
                                              std::vector<std::set<size_type> > & dof_searchWithProc);

    // search in my world
    std::list<boost::tuple<size_type,uint16_type> >
    updateNoRelationMeshMPI_upWithMyWorld(const std::vector< std::vector<size_type> > & memmapGdof,
                                          const std::vector< std::vector<uint16_type> > & memmapComp,
                                          const std::vector<std::vector<typename image_mesh_type::node_type> > & pointsSearched,
                                          const std::vector<std::vector< std::vector<typename image_mesh_type::node_type > > > & memmap_vertices,
                                          graph_ptrtype & sparsity_graph,
                                          std::vector< std::list<boost::tuple<int,size_type,double> > > & memory_valueInMatrix,
                                          std::vector<std::map<size_type,size_type> > & memory_col_globalProcessToGlobalCluster,
                                          std::vector<std::set<size_type> > & dof_searchWithProc,
                                          bool extrapolation_mode,
                                          extrapolation_memory_type & dof_extrapolationData);

    // search in other world (MPI communication)
    std::list<boost::tuple<size_type,uint16_type> >
    updateNoRelationMeshMPI_upWithOtherWorld(const std::vector< std::vector<size_type> > & memmapGdof,
                                             const std::vector< std::vector<uint16_type> > & memmapComp,
                                             const std::vector<std::vector<typename image_mesh_type::node_type> > & pointsSearched,
                                             const std::vector<std::vector< std::vector<typename image_mesh_type::node_type > > > & memmap_vertices,
                                             graph_ptrtype & sparsity_graph,
                                             std::vector< std::list<boost::tuple<int,size_type,double> > > & memory_valueInMatrix,
                                             std::vector<std::map<size_type,size_type> > & memory_col_globalProcessToGlobalCluster,
                                             std::vector<std::set<size_type> > & dof_searchWithProc,
                                             bool extrapolation_mode,
                                             extrapolation_memory_type & dof_extrapolationData);
#endif // MPI_MODE

    range_iterator _M_range;
    WorldComm _M_WorldCommFusion;
    InterpType _M_interptype;
};

template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::OperatorInterpolation( domain_space_ptrtype const& domainspace,
                                                                                                        dual_image_space_ptrtype const& imagespace,
                                                                                                        backend_ptrtype const& backend,
                                                                                                        InterpType const& interptype,
                                                                                                        bool ddmethod )
    :
    super( domainspace, imagespace, backend, false ),
    _M_range( elements( imagespace->mesh() ) ),
    _M_WorldCommFusion( (ddmethod) ? this->domainSpace()->worldComm() : this->domainSpace()->worldComm()+this->dualImageSpace()->worldComm() ),
    _M_interptype(interptype)
{
    update();
}


template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::OperatorInterpolation( domain_space_ptrtype const& domainspace,
                                                                                                        dual_image_space_ptrtype const& imagespace,
                                                                                                        IteratorRange const& r,
                                                                                                        backend_ptrtype const& backend,
                                                                                                        InterpType const& interptype,
                                                                                                        bool ddmethod )
    :
    super( domainspace, imagespace, backend, false ),
    _M_range( r ),
    _M_WorldCommFusion( (ddmethod) ? this->domainSpace()->worldComm() : this->domainSpace()->worldComm()+this->dualImageSpace()->worldComm() ),
    _M_interptype(interptype)
{
    update();
}


template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
void
OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::update()
{
    if ( this->dualImageSpace()->mesh()->numElements() == 0 )
        {
            //std::cout << "OperatorInterpolation : update nothing!" << std::endl;
            this->matPtr() = this->backend()->newZeroMatrix( this->domainSpace()->mapOnOff(),
                                                             this->dualImageSpace()->mapOn() );
            return;
        }

    // if same mesh but not same function space (e.g. different polynomial
    // order, different basis) or if the image of domain mesh are related to
    // each other through an extraction (one of them is the sub mesh of the
    // other)
#if 1
    if ( this->dualImageSpace()->mesh()->isRelatedTo( this->domainSpace()->mesh() ) )
#else
        if ( ( this->dualImageSpace()->mesh().get() == ( image_mesh_type* )this->domainSpace()->mesh().get() ) )
#endif
    {
        VLOG(2) << "OperatorInterpolation: use same mesh\n";
        VLOG(2) << "isDomainMeshRelatedToImageMesh: "  << isDomainMeshRelatedToImageMesh() << "\n";
        VLOG(2) << "isImageMeshRelatedToDomainMesh: "  << isImageMeshRelatedToDomainMesh() << "\n";
        this->updateSameMesh();
    }
    else // no relation between meshes
    {
#if defined(FEELPP_ENABLE_MPI_MODE)
        if ( this->dualImageSpace()->worldComm().localSize() > 1 ||
             this->domainSpace()->worldComm().localSize() > 1 )
            this->updateNoRelationMeshMPI();

        else
            this->updateNoRelationMesh();
#else
        this->updateNoRelationMesh();
#endif
    }

    // close matrix after build
    this->mat().close();
}

//-----------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------//

template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
void
OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::updateSameMesh()
{
    //std::cout << "OperatorInterpolation::updateSameMesh start " << std::endl;
#if !defined(FEELPP_ENABLE_MPI_MODE) // NOT MPI
    const size_type proc_id              = this->dualImageSpace()->mesh()->worldComm().localRank();
    const size_type nrow_dof_on_proc     = this->dualImageSpace()->nLocalDof();
    const size_type firstrow_dof_on_proc = this->dualImageSpace()->dof()->firstDof( proc_id );
    const size_type lastrow_dof_on_proc  = this->dualImageSpace()->dof()->lastDof( proc_id );
    const size_type firstcol_dof_on_proc = this->domainSpace()->dof()->firstDof( proc_id );
    const size_type lastcol_dof_on_proc  = this->domainSpace()->dof()->lastDof( proc_id );
#else
    const size_type proc_id              = this->dualImageSpace()->worldsComm()[0].localRank();
    const size_type nrow_dof_on_proc     = this->dualImageSpace()->nLocalDof();
    const size_type firstrow_dof_on_proc = this->dualImageSpace()->dof()->firstDofGlobalCluster( proc_id );
    const size_type lastrow_dof_on_proc  = this->dualImageSpace()->dof()->lastDofGlobalCluster( proc_id );
    const size_type firstcol_dof_on_proc = this->domainSpace()->dof()->firstDofGlobalCluster( proc_id );
    const size_type lastcol_dof_on_proc  = this->domainSpace()->dof()->lastDofGlobalCluster( proc_id );
#endif
    graph_ptrtype sparsity_graph( new graph_type( nrow_dof_on_proc,
                                                  firstrow_dof_on_proc, lastrow_dof_on_proc,
                                                  firstcol_dof_on_proc, lastcol_dof_on_proc,
                                                  this->dualImageSpace()->mesh()->worldComm().subWorldComm() ) );

    auto const* imagedof = this->dualImageSpace()->dof().get();
    auto const* domaindof = this->domainSpace()->dof().get();
    auto const* imagebasis = this->dualImageSpace()->basis().get();
    auto const* domainbasis = this->domainSpace()->basis().get();


    std::vector<bool> dof_done( nrow_dof_on_proc, false );
    std::vector< std::list<std::pair<size_type,double> > > memory_valueInMatrix( nrow_dof_on_proc );

    // Local assembly: compute the Mloc matrix by evaluating
    // the domain space basis function at the dual image space
    // dof points (nodal basis) since we have only computation
    // in the ref elements and the basis and dof points in ref
    // element are the same, we compute Mloc outside the
    // element loop.

    //typename matrix_node<value_type>::type Mloc(domain_basis_type::nLocalDof*domain_basis_type::nComponents1,1);
    auto const& Mloc = domainbasis->evaluate( imagebasis->dual().points() );

    DVLOG(2) << "[interpolate] Same mesh but not same space\n";

    iterator_type it, en;
    boost::tie( boost::tuples::ignore, it, en ) = _M_range;

    const bool image_related_to_domain = this->dualImageSpace()->mesh()->isSubMeshFrom( this->domainSpace()->mesh() );
    const bool domain_related_to_image = this->domainSpace()->mesh()->isSubMeshFrom( this->dualImageSpace()->mesh() );

    for ( ; it != en; ++ it )
    {
        auto idElem = detailsup::idElt( *it,idim_type() );
        auto domain_eid = idElem;
        if ( image_related_to_domain )
        {
            domain_eid = this->dualImageSpace()->mesh()->subMeshToMesh( idElem );
            DVLOG(2) << "[image_related_to_domain] image element id: "  << idElem << " domain element id : " << domain_eid << "\n";
        }
        if( domain_related_to_image )
        {
            domain_eid = this->domainSpace()->mesh()->meshToSubMesh( idElem );
            DVLOG(2) << "[domain_related_to_image] image element id: "  << idElem << " domain element id : " << domain_eid << "\n";
        }

        if ( domain_eid == invalid_size_type_value )
            continue;
        // Global assembly
        for ( uint16_type iloc = 0; iloc < nLocalDofInDualImageElt; ++iloc )
        {
            for ( uint16_type comp = 0; comp < image_basis_type::nComponents; ++comp )
            {
                size_type i =  boost::get<0>( imagedof->localToGlobal( *it, iloc, comp ) );

                if ( !dof_done[i] )
                {
#if !defined(FEELPP_ENABLE_MPI_MODE) // NOT MPI
                    const auto ig1 = i;
                    const auto theproc = imagedof->worldComm().localRank();
#else // WITH MPI
                    const auto ig1 = imagedof->mapGlobalProcessToGlobalCluster()[i];
                    const auto theproc = imagedof->procOnGlobalCluster( ig1 );
#endif
                    auto& row = sparsity_graph->row( ig1 );
                    row.template get<0>() = theproc;
                    const size_type il1 = ig1 - imagedof->firstDofGlobalCluster( theproc );
                    row.template get<1>() = il1;
                    //row.template get<1>() = i;

                    uint16_type ilocprime=imagedof->localDofInElement( *it, iloc, comp ) ;

                    for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                    {
                        // get column
                        const size_type j =  boost::get<0>( domaindof->localToGlobal( domain_eid, jloc, comp ) );
                        //up the pattern graph
#if !defined(FEELPP_ENABLE_MPI_MODE) // NOT MPI
                        row.template get<2>().insert( j );
#else // WITH MPI
                        row.template get<2>().insert( domaindof->mapGlobalProcessToGlobalCluster()[j] );
#endif
                        // get interpolated value
                        const value_type v = Mloc( domain_basis_type::nComponents1*jloc +
                                                   comp*domain_basis_type::nComponents1*domain_basis_type::nLocalDof +
                                                   comp,
                                                   ilocprime );
                        // save in matrux
                        memory_valueInMatrix[i].push_back( std::make_pair( j,v ) );
                    }

                    dof_done[i]=true;
                }
            }
        }
    }

    //-----------------------------------------
    // compute graph
    sparsity_graph->close();
    //-----------------------------------------
    // create matrix
    this->matPtr() = this->backend()->newMatrix( this->domainSpace()->mapOnOff(),
                                                 this->dualImageSpace()->mapOn(),
                                                 sparsity_graph  );
    //-----------------------------------------

    // assemble matrix
    for ( size_type idx_i=0 ; idx_i<nrow_dof_on_proc; ++idx_i )
    {
        for ( auto it_j=memory_valueInMatrix[idx_i].begin(),en_j=memory_valueInMatrix[idx_i].end() ; it_j!=en_j ; ++it_j )
        {
            this->matPtr()->set( idx_i,it_j->first,it_j->second );
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------//

template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
void
OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::updateNoRelationMesh()
{
    DVLOG(2) << "[interpolate] different meshes\n";
    //std::cout << "OperatorInterpolation::updateNoRelationMesh start " << std::endl;

    const size_type proc_id = this->dualImageSpace()->mesh()->worldComm().localRank();
    const size_type n1_dof_on_proc = this->dualImageSpace()->nLocalDof();
    const size_type firstrow_dof_on_proc = this->dualImageSpace()->dof()->firstDof( proc_id );
    const size_type lastrow_dof_on_proc = this->dualImageSpace()->dof()->lastDof( proc_id );
    const size_type firstcol_dof_on_proc = this->domainSpace()->dof()->firstDof( proc_id );
    const size_type lastcol_dof_on_proc = this->domainSpace()->dof()->lastDof( proc_id );

    graph_ptrtype sparsity_graph( new graph_type( n1_dof_on_proc,
                                                  firstrow_dof_on_proc, lastrow_dof_on_proc,
                                                  firstcol_dof_on_proc, lastcol_dof_on_proc,
                                                  this->dualImageSpace()->mesh()->worldComm().subWorldCommSeq() ) );

    auto const* imagedof = this->dualImageSpace()->dof().get();
    auto const* domaindof = this->domainSpace()->dof().get();
    auto const* domainbasis = this->domainSpace()->basis().get();

    //-----------------------------------------
    //init the localization tool
    auto locTool = this->domainSpace()->mesh()->tool_localization();
    if ( this->interpolationType().onlyLocalizeOnBoundary() ) locTool->updateForUseBoundaryFaces();
    else locTool->updateForUse();
    // kdtree parameter
    locTool->kdtree()->nbNearNeighbor(this->interpolationType().nbNearNeighborInKdTree());

    //locTool->kdtree()->nbNearNeighbor(3);
    //locTool->kdtree()->nbNearNeighbor(this->domainSpace()->mesh()->numElements());
    //locTool->setExtrapolation(false);

    //-----------------------------------------
    // usefull data
    matrix_node_type ptsReal( image_mesh_type::nRealDim, 1 );
    matrix_node_type ptsRef( domain_mesh_type::nDim , 1 );
    typename domain_mesh_type::Localization::container_search_iterator_type itanal,itanal_end;
    typename domain_mesh_type::Localization::container_output_iterator_type itL,itL_end;
    matrix_node_type MlocEval( domain_basis_type::nLocalDof*domain_basis_type::nComponents1,1 );

    std::vector<bool> dof_done( this->dualImageSpace()->nLocalDof(), false );
    std::vector< std::list<std::pair<size_type,double> > > memory_valueInMatrix( this->dualImageSpace()->nLocalDof() );

    //-----------------------------------------
    // for each element in range
    iterator_type it, en;
    boost::tie( boost::tuples::ignore, it, en ) = _M_range;
    size_type eltIdLocalised = 0;

    for ( ; it != en; ++ it )
        {
            for ( uint16_type iloc = 0; iloc < nLocalDofInDualImageElt; ++iloc )
                {
                    for ( uint16_type comp = 0; comp < image_basis_type::nComponents; ++comp )
                        {
                            const auto gdof =  boost::get<0>(imagedof->localToGlobal( *it, iloc, comp ));
                            if (!dof_done[gdof])
                                {
                                    //------------------------
                                    // get the graph row
#if !defined(FEELPP_ENABLE_MPI_MODE) // NOT MPI
                                    const auto ig1 = gdof;
                                    const auto theproc = imagedof->worldComm().localRank();
#else // WITH MPI
                                    const auto ig1 = imagedof->mapGlobalProcessToGlobalCluster()[gdof];
                                    const auto theproc = imagedof->procOnGlobalCluster( ig1 );
#endif
                                    auto& row = sparsity_graph->row(ig1);
                                    row.template get<0>() = theproc;
                                    row.template get<1>() = gdof;
                                    //------------------------
                                    // the dof point
                                    ublas::column(ptsReal,0 ) = boost::get<0>(imagedof->dofPoint(gdof));
                                    //------------------------
                                    // localisation process
                                    eltIdLocalised = locTool->run_analysis(ptsReal,eltIdLocalised,it->vertices()/*it->G()*/,mpl::int_<interpolation_type::value>()).template get<1>();
                                    //------------------------
                                    // for each localised points
                                    itanal = locTool->result_analysis_begin();
                                    itanal_end = locTool->result_analysis_end();
                                    for ( ;itanal!=itanal_end;++itanal)
                                        {
                                            itL=itanal->second.begin();
                                            ublas::column( ptsRef, 0 ) = boost::get<1>( *itL );

                                            MlocEval = domainbasis->evaluate( ptsRef );

                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                {
                                                    //get global dof
                                                    size_type j =  boost::get<0>( domaindof->localToGlobal( itanal->first,jloc,comp ) );
                                                    value_type v = MlocEval( domain_basis_type::nComponents1*jloc
                                                                             + comp*domain_basis_type::nComponents1*domain_basis_type::nLocalDof
                                                                             + comp,
                                                                             0 );
#if !defined(FEELPP_ENABLE_MPI_MODE) // NOT MPI
                                                    row.template get<2>().insert( j );
#else // WITH MPI
                                                    row.template get<2>().insert( domaindof->mapGlobalProcessToGlobalCluster()[j] );
#endif
                                                    memory_valueInMatrix[gdof].push_back( std::make_pair( j,v ) );
                                                }
                                        }
                                    dof_done[gdof]=true;
                                } // if (!dof_done[gdof])
                        } //  for ( uint16_type comp = 0; comp < image_basis_type::nComponents; ++comp )
                } // for ( uint16_type iloc = 0; iloc < nLocalDofInDualImageElt; ++iloc )
        } // for( ; it != en; ++ it )


    //-----------------------------------------
    // compute graph
    sparsity_graph->close(); //sparsity_graph->printPython("mygraphpython.py");
    //-----------------------------------------
    // create matrix
    this->matPtr() = this->backend()->newMatrix( this->domainSpace()->mapOnOff(),
                                                 this->dualImageSpace()->mapOn(),
                                                 sparsity_graph );
    //-----------------------------------------
    // assemble matrix
    for (size_type idx_i=0 ; idx_i<this->dualImageSpace()->nLocalDof() ;++idx_i)
        {
            for (auto it_j=memory_valueInMatrix[idx_i].begin(),en_j=memory_valueInMatrix[idx_i].end() ; it_j!=en_j ; ++it_j)
                {
                    this->matPtr()->set(idx_i,it_j->first,it_j->second);
                }
        }

}

//-----------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------//

#if defined(FEELPP_ENABLE_MPI_MODE) // WITH MPI

template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
void
OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::updateNoRelationMeshMPI()
{
    //std::cout << "OperatorInterpolation::updateNoRelationMeshMPI start " << std::endl;

    auto testCommActivities_image=this->dualImageSpace()->worldComm().hasMultiLocalActivity();

    if (testCommActivities_image.template get<0>())
        {
            //std::cout << "OperatorInterpolation::updateNoRelationMeshMPI hasMultiLocalActivity " << std::endl;
            // save initial activities
            std::vector<int> saveActivities_image = this->dualImageSpace()->worldComm().activityOnWorld();
            // iterate on each local activity
            const auto colorWhichIsActive = testCommActivities_image.template get<1>();
            auto it_color=colorWhichIsActive.begin();
            auto const en_color=colorWhichIsActive.end();
            for ( ;it_color!=en_color;++it_color )
                {
                    this->dualImageSpace()->worldComm().applyActivityOnlyOn( *it_color );
                    this->dualImageSpace()->mapOn().worldComm().applyActivityOnlyOn( *it_color );
                    this->dualImageSpace()->mapOnOff().worldComm().applyActivityOnlyOn( *it_color );
                    this->updateNoRelationMeshMPI_run(false);
                }
            // revert initial activities
            this->dualImageSpace()->worldComm().setIsActive(saveActivities_image);
            this->dualImageSpace()->mapOn().worldComm().setIsActive(saveActivities_image);
            this->dualImageSpace()->mapOnOff().worldComm().setIsActive(saveActivities_image);
        }
    else
        {
            //std::cout << "OperatorInterpolation::updateNoRelationMeshMPI has One LocalActivity " << std::endl;
            if ( !this->dualImageSpace()->worldComm().isActive() && !this->domainSpace()->worldComm().isActive() )
                {
                    this->matPtr() = this->backend()->newZeroMatrix( this->domainSpace()->mapOnOff(),
                                                                     this->dualImageSpace()->mapOn() );
                }
            else
                {
                    this->updateNoRelationMeshMPI_run(true);
                }
        }

}

template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
void
OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::updateNoRelationMeshMPI_run(bool buildNonZeroMatrix)
{

    //std::cout << "OperatorInterpolation::updateNoRelationMeshMPI_run start " << std::endl;

    //-----------------------------------------------------------------------------------------
    // PreProcess : datamap properties and graph
    //-----------------------------------------------------------------------------------------

    const size_type proc_id = this->dualImageSpace()->worldComm().localRank();
    const size_type proc_id_row = this->dualImageSpace()->worldComm().localRank();
    const size_type proc_id_col = this->domainSpace()->worldComm().localRank();
    const size_type nProc = this->dualImageSpace()->mesh()->worldComm().size();
    const size_type nProc_row = this->dualImageSpace()->mesh()->worldComm().localSize();
    const size_type nProc_col = this->domainSpace()->mesh()->worldComm().localSize();
    const size_type nrow_dof_on_proc = this->dualImageSpace()->nLocalDof();
    const size_type firstrow_dof_on_proc = this->dualImageSpace()->dof()->firstDofGlobalCluster( proc_id_row );
    const size_type lastrow_dof_on_proc = this->dualImageSpace()->dof()->lastDofGlobalCluster( proc_id_row );
    const size_type firstcol_dof_on_proc = this->domainSpace()->dof()->firstDofGlobalCluster( proc_id_col );
    const size_type lastcol_dof_on_proc = this->domainSpace()->dof()->lastDofGlobalCluster( proc_id_col );



    std::vector<size_type> first_col_entry( this->domainSpace()->worldComm().globalSize() );
    std::vector<size_type> last_col_entry( this->domainSpace()->worldComm().globalSize() );
    mpi::all_gather( this->domainSpace()->worldComm().globalComm(),
                     firstcol_dof_on_proc,//this->firstColEntryOnProc(),
                     first_col_entry );
    mpi::all_gather( this->domainSpace()->worldComm().globalComm(),
                     lastcol_dof_on_proc,//this->lastColEntryOnProc(),
                     last_col_entry );
    size_type thefirstCol = *std::min_element( first_col_entry.begin(),first_col_entry.end() );
    size_type thelastCol = *std::max_element( last_col_entry.begin(),last_col_entry.end() );

#if 0
    graph_ptrtype sparsity_graph( new graph_type( nrow_dof_on_proc,
                                                  firstrow_dof_on_proc, lastrow_dof_on_proc,
                                                  firstcol_dof_on_proc, lastcol_dof_on_proc,
                                                  this->dualImageSpace()->worldComm() ) );
#else
    graph_ptrtype sparsity_graph( new graph_type( nrow_dof_on_proc,
                                                  firstrow_dof_on_proc, lastrow_dof_on_proc,
                                                  thefirstCol,thelastCol,
                                                  this->dualImageSpace()->worldComm() ) );
#endif

    size_type new_nLocalDofWithoutGhost=this->domainSpace()->nDof()/nProc_row;
    size_type new_nLocalDofWithoutGhost_tempp=this->domainSpace()->nDof()/nProc_row;
    size_type new_nLocalDofWithoutGhostMiss=this->domainSpace()->nDof()%nProc_row;
    if (this->dualImageSpace()->worldComm().globalSize()==this->domainSpace()->worldComm().globalSize() )
    {
        new_nLocalDofWithoutGhost = this->domainSpace()->mapOnOff().nLocalDofWithoutGhost();
    }
    else
    {
        if (this->dualImageSpace()->worldComm().globalRank()==this->dualImageSpace()->worldComm().masterRank())  new_nLocalDofWithoutGhost+=new_nLocalDofWithoutGhostMiss;
    }

    size_type new_firstdofcol=0,new_lastdofcol=new_nLocalDofWithoutGhost-1;
    bool findMyProc=false;
    int currentProc=0;
    while(!findMyProc)
        {
            if (currentProc==this->dualImageSpace()->worldComm().globalRank())
                {
                    findMyProc=true;
                }
            else if (currentProc==this->dualImageSpace()->worldComm().masterRank())
                {
                    new_firstdofcol+=new_nLocalDofWithoutGhost_tempp+new_nLocalDofWithoutGhostMiss;
                    new_lastdofcol+=new_nLocalDofWithoutGhost_tempp+new_nLocalDofWithoutGhostMiss;
                }
            else
                {
                    new_firstdofcol+=new_nLocalDofWithoutGhost_tempp;
                    new_lastdofcol+=new_nLocalDofWithoutGhost_tempp;
                }
            ++currentProc;
        }


    //-----------------------------------------------------------------------------------------
    // Start localization process
    //-----------------------------------------------------------------------------------------

    //memory containers
    std::vector< std::list<boost::tuple<int,size_type,double> > > memory_valueInMatrix( this->dualImageSpace()->nLocalDof() );
    for (size_type k=0;k<memory_valueInMatrix.size();++k) memory_valueInMatrix[k].clear();

    std::vector<std::map<size_type,size_type> > memory_col_globalProcessToGlobalCluster(nProc_col);
    std::vector<std::set<size_type> > dof_searchWithProc(this->dualImageSpace()->nLocalDof());

    extrapolation_memory_type dof_extrapolationData(this->dualImageSpace()->nLocalDof());

    //init the localization tool
    auto locTool = this->domainSpace()->mesh()->tool_localization();
    bool doExtrapolationAtStart = locTool->doExtrapolation();
    // kdtree parameter
    locTool->kdtree()->nbNearNeighbor(this->interpolationType().nbNearNeighborInKdTree());
    // points in kdtree
    if ( this->interpolationType().onlyLocalizeOnBoundary() ) locTool->updateForUseBoundaryFaces();
    else locTool->updateForUse();
    // no extrapolation in first
    if ( doExtrapolationAtStart ) locTool->setExtrapolation(false);


    uint16_type nMPIsearch=15;//5;
    if( InterpType::value==1) nMPIsearch=this->domainSpace()->mesh()->worldComm().localSize();
    else if (this->domainSpace()->mesh()->worldComm().localSize()<nMPIsearch) nMPIsearch=this->domainSpace()->mesh()->worldComm().localSize();
   // only one int this case
   if (!this->interpolationType().searchWithCommunication()) nMPIsearch=1;
   uint16_type counterMPIsearch=1;
   bool FinishMPIsearch=false;
   //if (this->interpolationType().searchWithCommunication()) FinishMPIsearch=true;// not run algo1 !!!!

   size_type nbLocalisationFail=1;
   while(!FinishMPIsearch)
       {
           auto pointDistribution = this->updateNoRelationMeshMPI_pointDistribution(memory_valueInMatrix,dof_searchWithProc);
           auto memmapGdof = pointDistribution.template get<0>();
           auto memmapComp = pointDistribution.template get<1>();
           auto pointsSearched = pointDistribution.template get<2>();
           auto memmapVertices = pointDistribution.template get<3>();
           //std::cout <<  "proc " << this->worldCommFusion().globalRank() <<  " pointsSearched.size() " << pointsSearched.size() << std::endl;

           auto memory_localisationFail = this->updateNoRelationMeshMPI_upWithMyWorld( memmapGdof, // input
                                                                                       memmapComp, // input
                                                                                       pointsSearched, // input
                                                                                       memmapVertices, // input
                                                                                       sparsity_graph, // output
                                                                                       memory_valueInMatrix, // output
                                                                                       memory_col_globalProcessToGlobalCluster, // output
                                                                                       dof_searchWithProc, // output,
                                                                                       false,// extrapolation_mode
                                                                                       dof_extrapolationData // empty
                                                                                       );
           //std::cout <<  "proc " << this->worldCommFusion().globalRank() <<  " memory_localisationFail.size() " << memory_localisationFail.size() << std::endl;

           if (this->interpolationType().searchWithCommunication())
               {
                   auto memory_localisationFail2 = this->updateNoRelationMeshMPI_upWithOtherWorld( memmapGdof, // input
                                                                                                   memmapComp, // input
                                                                                                   pointsSearched, // input
                                                                                                   memmapVertices, // input
                                                                                                   sparsity_graph, // output
                                                                                                   memory_valueInMatrix, // output
                                                                                                   memory_col_globalProcessToGlobalCluster, // output
                                                                                                   dof_searchWithProc, // output
                                                                                                   false,// extrapolation_mode
                                                                                                   dof_extrapolationData // empty
                                                                                                   );

                   const size_type nbLocalisationFail_loc = memory_localisationFail.size()+memory_localisationFail2.size();
                   mpi::all_reduce( this->worldCommFusion().globalComm(),
                                    nbLocalisationFail_loc,
                                    nbLocalisationFail,
                                    std::plus<double>() );
               }
           else nbLocalisationFail = memory_localisationFail.size();
           //nbLocalisationFail = memory_localisationFail.size()+memory_localisationFail2.size();
           //std::cout <<  "proc " << this->worldCommFusion().globalRank()
           //          << " et " <<nbLocalisationFail << std::endl;
           if (counterMPIsearch<nMPIsearch && nbLocalisationFail>0) ++counterMPIsearch;
           else FinishMPIsearch=true;
       }

   //std::cout << "\n FINISH SEARCH!!!!!!!!! " << std::endl;
   if ( doExtrapolationAtStart ) locTool->setExtrapolation(true);

   if ( doExtrapolationAtStart && nbLocalisationFail>0 )
       {
           //std::cout << " Start Extrapolation" << std::endl;
           std::vector<std::set<size_type> > dof_searchWithProcExtrap(this->dualImageSpace()->nLocalDof());
           //locTool->setExtrapolation(true);
           uint16_type nMPIsearchExtrap=5;
           if (this->domainSpace()->mesh()->worldComm().localSize()<5) nMPIsearchExtrap=this->domainSpace()->mesh()->worldComm().localSize();
           // only one int this case
           if (!this->interpolationType().searchWithCommunication()) nMPIsearchExtrap=1;
           uint16_type counterMPIsearchExtrap=1;
           bool FinishMPIsearchExtrap=false;
           // localisation process
           while(!FinishMPIsearchExtrap)
               {
                   auto pointDistribution = this->updateNoRelationMeshMPI_pointDistribution(memory_valueInMatrix,dof_searchWithProcExtrap);
                   auto memmapGdof = pointDistribution.template get<0>();
                   auto memmapComp = pointDistribution.template get<1>();
                   auto pointsSearched = pointDistribution.template get<2>();
                   auto memmapVertices = pointDistribution.template get<3>();
                   //std::cout <<  "proc " << this->worldCommFusion().globalRank() <<  " pointsSearched.size() " << pointsSearched.size() << std::endl;
                   auto memory_localisationFail = this->updateNoRelationMeshMPI_upWithMyWorld( memmapGdof, // input
                                                                                               memmapComp, // input
                                                                                               pointsSearched, // input
                                                                                               memmapVertices, // input
                                                                                               sparsity_graph, // output
                                                                                               memory_valueInMatrix, // output
                                                                                               memory_col_globalProcessToGlobalCluster, // output
                                                                                               dof_searchWithProcExtrap, // output,
                                                                                               true,// extrapolation_mode
                                                                                               dof_extrapolationData // output
                                                                                               );
                   //std::cout <<  "proc " << this->worldCommFusion().globalRank() <<  " memory_localisationFail.size() " << memory_localisationFail.size() << std::endl;
                   if (this->interpolationType().searchWithCommunication())
                       {
                           auto memory_localisationFail2 = this->updateNoRelationMeshMPI_upWithOtherWorld( memmapGdof, // input
                                                                                                           memmapComp, // input
                                                                                                           pointsSearched, // input
                                                                                                           memmapVertices, // input
                                                                                                           sparsity_graph, // output
                                                                                                           memory_valueInMatrix, // output
                                                                                                           memory_col_globalProcessToGlobalCluster, // output
                                                                                                           dof_searchWithProcExtrap, // output
                                                                                                           true, // extrapolation_mode
                                                                                                           dof_extrapolationData // output
                                                                                                           );
                       }
                   //std::cout <<  "proc " << this->worldCommFusion().globalRank() <<  " memory_localisationFail2.size() " << memory_localisationFail.size() << std::endl;
                   if (counterMPIsearchExtrap<nMPIsearchExtrap) ++counterMPIsearchExtrap;
                   else FinishMPIsearchExtrap=true;
               } // while(!FinishMPIsearchExtrap)

           auto const* imagedof = this->dualImageSpace()->dof().get();
           auto const* domaindof = this->domainSpace()->dof().get();
           auto const* domainbasis = this->domainSpace()->basis().get();
           matrix_node_type ptsRef( image_mesh_type::nRealDim , 1 );
           matrix_node_type MlocEval(domain_basis_type::nLocalDof*domain_basis_type::nComponents1,1);

           // analysis result
           auto it_extrap = dof_extrapolationData.begin();
           auto const en_extrap = dof_extrapolationData.end();
           for (size_type cpt_gdof=0 ; it_extrap!=en_extrap ; ++it_extrap,++cpt_gdof )
               {
                   auto const gdof = cpt_gdof;
                   auto const pointExtrap = imagedof->dofPoint(gdof).template get<0>();

                   // search nearer
                   int procExtrapoled = 0;
                   double distMin= INT_MAX;
                   auto it_proc = it_extrap->begin();
                   auto const en_proc = it_extrap->end();
                   for ( ; it_proc!=en_proc; ++it_proc)
                       {
                           auto const procCurrent = it_proc->template get<0>();
                           auto const bary = it_proc->template get<1>();
                           // COMPUTE DISTANCE
                           double normDist = 0;
                           for (int q=0;q<image_mesh_type::nRealDim;++q) normDist+=std::pow(bary(q)-pointExtrap(q),2);
                           //std::cout << std::sqrt(normDist) << " "<< bary.size() << " " << pointExtrap.size() << std::endl;
                           if (std::sqrt(normDist)<distMin) { distMin=std::sqrt(normDist);procExtrapoled=procCurrent;}
                       }
                   it_proc = it_extrap->begin();
                   for ( ; it_proc!=en_proc; ++it_proc)
                       {
                           if ( it_proc->template get<0>()==procExtrapoled)
                               {
                                   // get the graph row
                                   auto const ig1 = imagedof->mapGlobalProcessToGlobalCluster()[gdof];
                                   auto const theproc = imagedof->procOnGlobalCluster(ig1);
                                   auto& row = sparsity_graph->row(ig1);
                                   row.template get<0>() = theproc;
                                   row.template get<1>() = ig1 - imagedof->firstDofGlobalCluster(theproc);
                                   // get ref point
                                   ublas::column( ptsRef, 0 ) = it_proc->template get<2>();
                                   // evaluate basis functions for this point
                                   MlocEval = domainbasis->evaluate( ptsRef );
                                   //auto it_jdof = it_proc->template get<3>().begin();
                                   //auto const en_jdof = it_proc->template get<3>().end();
                                   auto const comp = it_proc->template get<4>();
                                   auto const vec_jloc = it_proc->template get<3>();
                                   for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                       //for (uint16_type jloc=0 ; it_jdof != en_jdof ; ++it_jdof,++jloc)
                                       {
                                           //const size_type j_gdof=it_jdof->first;
                                           //const size_type j_gdofGlobalCluster=it_jdof->second;
                                           const size_type j_gdof=vec_jloc[jloc].first;
                                           const size_type j_gdofGlobalCluster=vec_jloc[jloc].second;
                                           row.template get<2>().insert(j_gdofGlobalCluster);//domaindof->mapGlobalProcessToGlobalCluster()[j_gdof]);
                                           // get value
                                           value_type v = MlocEval( domain_basis_type::nComponents1*jloc +
                                                                    comp*domain_basis_type::nComponents1*domain_basis_type::nLocalDof +
                                                                    comp, 0 );
                                           // save value
                                           memory_valueInMatrix[gdof].push_back(boost::make_tuple(procExtrapoled,j_gdof,v));
                                           memory_col_globalProcessToGlobalCluster[procExtrapoled][j_gdof]=j_gdofGlobalCluster;//domaindof->mapGlobalProcessToGlobalCluster()[j_gdof];
                                       }
                               } // if ( it_proc->template get<0>()==procExtrapoled)
                       } // for ( ; it_proc!=en_proc; ++it_proc)

               } // for (size_type cpt_gdof=0 ; it_extrap!=en_extrap ; ++it_extrap,++cpt_gdof )
       } // if ( doExtrapolationAtStart )


    //-----------------------------------------------------------------------------------------
    this->worldCommFusion().barrier();
    //std::cout << "Op---1----- " << std::endl;
    //-----------------------------------------------------------------------------------------
    // compute graph
    sparsity_graph->close();//sparsity_graph->printPython("mygraphpythonMPI.py");
    //-----------------------------------------------------------------------------------------
    //std::cout << "Op---2----- " << std::endl;
    this->worldCommFusion().barrier();
    //-----------------------------------------------------------------------------------------
    size_type mapCol_nLocalDof = 0;
    for (int p=0;p<nProc_col;++p)
        {
            mapCol_nLocalDof += memory_col_globalProcessToGlobalCluster[p].size();
        }
    //std::cout << "mapCol_nLocalDof " << mapCol_nLocalDof << std::endl;
    std::vector<size_type> mapCol_globalProcessToGlobalCluster(mapCol_nLocalDof);
    std::vector<size_type> new_mapGlobalClusterToGlobalProcess(new_nLocalDofWithoutGhost);
    std::vector<std::map<size_type,size_type> > mapCol_LocalSpaceDofToLocalInterpDof(nProc_col);
    size_type currentLocalDof=0;
    for (int p = 0 ; p<nProc_col;++p)
        {
            auto it_map = memory_col_globalProcessToGlobalCluster[p].begin();
            auto en_map = memory_col_globalProcessToGlobalCluster[p].end();
            for ( ; it_map!=en_map ; ++it_map)
                {
                    mapCol_globalProcessToGlobalCluster[currentLocalDof]=it_map->second;//memory_col_globalProcessToGlobalCluster[proc_id][i];
                    mapCol_LocalSpaceDofToLocalInterpDof[p][it_map->first]=currentLocalDof;

                    if ( new_firstdofcol<=it_map->second && new_lastdofcol>=it_map->second)
                        new_mapGlobalClusterToGlobalProcess[it_map->second-new_firstdofcol]=currentLocalDof;

                    ++currentLocalDof;
                }
        }

    //-----------------------------------------
    //std::cout << "Op---3----- " << this->worldCommFusion().godRank() << std::endl;
    this->worldCommFusion().barrier();
    //-----------------------------------------
    // build data map for the columns
    //this->domainSpace()->mapOnOff().showMeMapGlobalProcessToGlobalCluster();
    //this->dualImageSpace()->worldComm().showMe();
    DataMap mapColInterp(this->dualImageSpace()->worldComm());// this->domainSpace()->mapOnOff().worldComm());
    mapColInterp.setNDof(this->domainSpace()->mapOnOff().nDof());

    mapColInterp.setNLocalDofWithoutGhost( proc_id, new_nLocalDofWithoutGhost );//  this->domainSpace()->mapOnOff().nLocalDofWithoutGhost() );
    mapColInterp.setNLocalDofWithGhost( proc_id, mapCol_nLocalDof/*this->domainSpace()->mapOnOff().nLocalDofWithGhost()*/ );
    mapColInterp.setFirstDof( proc_id, this->domainSpace()->mapOnOff().firstDof() );
    mapColInterp.setLastDof( proc_id,  this->domainSpace()->mapOnOff().lastDof() );
    mapColInterp.setFirstDofGlobalCluster( proc_id, new_firstdofcol );
    mapColInterp.setLastDofGlobalCluster( proc_id, new_lastdofcol );
    mapColInterp.setMapGlobalProcessToGlobalCluster(mapCol_globalProcessToGlobalCluster);
    mapColInterp.setMapGlobalClusterToGlobalProcess(new_mapGlobalClusterToGlobalProcess);
    //if ( this->dualImageSpace()->worldComm().isActive() ) mapColInterp.showMeMapGlobalProcessToGlobalCluster();

    //-----------------------------------------
    //std::cout << "Op---4----- " << this->worldCommFusion().godRank() << " isA " << this->dualImageSpace()->worldComm().isActive() << std::endl;
    this->worldCommFusion().barrier();
    //-----------------------------------------
    // create matrix for active process
    if ( this->dualImageSpace()->worldComm().isActive() )
        {
            this->matPtr() = this->backend()->newMatrix( mapColInterp,//this->domainSpace()->mapOnOff(),
                                                         this->dualImageSpace()->mapOn(),
                                                         sparsity_graph  );
        }
    //-----------------------------------------
    //std::cout << "Op---5----- " << this->worldCommFusion().godRank() << std::endl;
    this->worldCommFusion().barrier();
    //-----------------------------------------
    // create null matrix for inactive process
    if ( !this->dualImageSpace()->worldComm().isActive() && buildNonZeroMatrix )
        {
            this->matPtr() = this->backend()->newZeroMatrix( mapColInterp,//this->domainSpace()->mapOnOff(),
                                                             this->dualImageSpace()->mapOn() );
        }
    //-----------------------------------------
    //std::cout << "Op---6----- "  << this->worldCommFusion().godRank() << std::endl;
    this->worldCommFusion().barrier();
    //-----------------------------------------
    // assemble matrix
    if ( this->dualImageSpace()->worldComm().isActive() )
        {
            for (size_type idx_i=0 ; idx_i<nrow_dof_on_proc;++idx_i)
                {
                    for (auto it_j=memory_valueInMatrix[idx_i].begin(),en_j=memory_valueInMatrix[idx_i].end() ; it_j!=en_j ; ++it_j)
                        {
                            if (memory_valueInMatrix[idx_i].size()>0)
                                {
                                    this->matPtr()->set(idx_i,mapCol_LocalSpaceDofToLocalInterpDof[it_j->template get<0>()][it_j->template get<1>()],it_j->template get<2>());
                                }
                        }
                }
        }
    //-----------------------------------------
    //std::cout << "Op---7----- " << std::endl;
    this->worldCommFusion().barrier();
    //-----------------------------------------


}

//-----------------------------------------------------------------------------------------------------------------//

template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
std::list<boost::tuple<size_type,uint16_type> >
OperatorInterpolation<DomainSpaceType,
                      ImageSpaceType,
                      IteratorRange,
                      InterpType>::updateNoRelationMeshMPI_upWithMyWorld(const std::vector< std::vector<size_type> > & memmapGdof,
                                                                         const std::vector< std::vector<uint16_type> > & memmapComp,
                                                                         const std::vector<std::vector<typename image_mesh_type::node_type> > & pointsSearched,
                                                                         const std::vector<std::vector< std::vector<typename image_mesh_type::node_type > > > & memmap_vertices,
                                                                         graph_ptrtype & sparsity_graph,
                                                                         std::vector< std::list<boost::tuple<int,size_type,double> > > & memory_valueInMatrix,
                                                                         std::vector<std::map<size_type,size_type> > & memory_col_globalProcessToGlobalCluster,
                                                                         std::vector<std::set<size_type> > & dof_searchWithProc,
                                                                         bool extrapolation_mode,
                                                                         extrapolation_memory_type & dof_extrapolationData )
{
    std::list<boost::tuple<size_type,uint16_type> > memory_localisationFail;// gdof,comp

    const auto proc_id = this->domainSpace()->mesh()->worldComm().localRank();

    auto const* imagedof = this->dualImageSpace()->dof().get();
    auto const* domaindof = this->domainSpace()->dof().get();
    auto const* domainbasis = this->domainSpace()->basis().get();

    auto locTool = this->domainSpace()->mesh()->tool_localization();

    matrix_node_type ptsReal( image_mesh_type::nRealDim, 1 );
    matrix_node_type ptsRef( image_mesh_type::nRealDim , 1 );
    matrix_node_type MlocEval(domain_basis_type::nLocalDof*domain_basis_type::nComponents1,1);
    matrix_node_type verticesOfEltSearched;

    size_type eltIdLocalised = this->domainSpace()->mesh()->beginElementWithId(this->domainSpace()->mesh()->worldComm().localRank())->id();
    auto const& eltRandom = this->domainSpace()->mesh()->element(eltIdLocalised);

    for ( size_type k=0 ; k<memmapGdof[proc_id].size() ; ++k)
        {
            //----------------------------------------------------------------
            if (this->interpolationType().componentsAreSamePoint())
                //------------------------------------------------------------
                {
                    // the searched point
                    ublas::column(ptsReal,0 ) = pointsSearched[proc_id][k];
                    // vertice with conforme case
                    if (InterpType::value==1)
                        {
                            const uint16_type sizeVertices = memmap_vertices[proc_id][k].size();
                            verticesOfEltSearched.resize( image_mesh_type::nRealDim,sizeVertices);
                            for ( uint16_type v=0;v<sizeVertices;++v)
                                ublas::column(verticesOfEltSearched,v)=memmap_vertices[proc_id][k][v];
                        }
                    else // random
                        verticesOfEltSearched = eltRandom.vertices();

                    // localisation process
                    auto resLocalisation = locTool->run_analysis(ptsReal,eltIdLocalised,verticesOfEltSearched,
                                                                 mpl::int_<interpolation_type::value>());
                    if (!resLocalisation.template get<0>()[0]) // not find
                        {
                            for ( uint16_type comp = 0;comp < image_basis_type::nComponents;++comp )
                                {
                                    const auto gdof = memmapGdof[proc_id][k+comp];
                                    memory_localisationFail.push_back(boost::make_tuple(gdof,comp) );
                                    dof_searchWithProc[gdof].insert(proc_id);
                                }
                        }
                    else // point found
                        {
                            eltIdLocalised = resLocalisation.template get<1>();

                            if (extrapolation_mode)
                                {
                                    auto const& eltExtrapoled = this->domainSpace()->mesh()->element(eltIdLocalised);
                                    auto const& verticesExtrapoled = eltExtrapoled.vertices();
                                    typename image_mesh_type::node_type bary(verticesExtrapoled.size1());
                                    for (int qi=0;qi<verticesExtrapoled.size1();++qi) bary(qi)=0;//important
                                    for (int qj=0;qj<verticesExtrapoled.size2();++qj)
                                        {
                                            /**/                              bary(0) += ublas::column(verticesExtrapoled,qj)(0);
                                            if (verticesExtrapoled.size1()>1) bary(1) += ublas::column(verticesExtrapoled,qj)(1);
                                            if (verticesExtrapoled.size1()>2) bary(2) += ublas::column(verticesExtrapoled,qj)(2);
                                        }
                                    /**/                              bary(0) /= verticesExtrapoled.size2();
                                    if (verticesExtrapoled.size1()>1) bary(1) /= verticesExtrapoled.size2();
                                    if (verticesExtrapoled.size1()>2) bary(2) /= verticesExtrapoled.size2();
                                    typename image_mesh_type::node_type theRefPtExtrap = locTool->result_analysis().begin()->second.begin()->template get<1>();
                                    std::vector<std::pair<size_type,size_type> > j_gdofs(domain_basis_type::nLocalDof);
                                    for ( uint16_type comp = 0;comp < image_basis_type::nComponents;++comp )
                                        {
                                            const auto gdof = memmapGdof[proc_id][k+comp];
                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                {
                                                    const size_type j_gdof =  boost::get<0>(domaindof->localToGlobal( eltIdLocalised,jloc,comp ));
                                                    j_gdofs[jloc]=std::make_pair(j_gdof,domaindof->mapGlobalProcessToGlobalCluster()[j_gdof]);
                                                }
                                            dof_extrapolationData[gdof].push_back(boost::make_tuple(proc_id,bary,theRefPtExtrap,j_gdofs,comp));
                                        }
                                }
                            else // no extrapolation
                                {
                                    for ( uint16_type comp = 0;comp < image_basis_type::nComponents;++comp )
                                        {
                                            const auto gdof = memmapGdof[proc_id][k+comp];
                                            // get the graph row
                                            auto const ig1 = imagedof->mapGlobalProcessToGlobalCluster()[gdof];
                                            auto const theproc = imagedof->procOnGlobalCluster(ig1);
                                            auto& row = sparsity_graph->row(ig1);
                                            row.template get<0>() = theproc;
                                            row.template get<1>() = ig1 - imagedof->firstDofGlobalCluster(theproc);
                                            // for each localised points
                                            auto itanal = locTool->result_analysis().begin();//  result_analysis_begin();
                                            auto const itanal_end = locTool->result_analysis().end();//result_analysis_end();
                                            for ( ;itanal!=itanal_end;++itanal)
                                                {
                                                    const auto itL=itanal->second.begin();
                                                    ublas::column( ptsRef, 0 ) = boost::get<1>(*itL);
                                                    // evaluate basis functions for this point
                                                    MlocEval = domainbasis->evaluate( ptsRef );
                                                    for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                        {
                                                            //get global dof
                                                            const size_type j_gdof =  boost::get<0>(domaindof->localToGlobal( itanal->first,jloc,comp ));
                                                            // up graph
                                                            row.template get<2>().insert(domaindof->mapGlobalProcessToGlobalCluster()[j_gdof]);
                                                            // get value
                                                            const value_type v = MlocEval( domain_basis_type::nComponents1*jloc +
                                                                                           comp*domain_basis_type::nComponents1*domain_basis_type::nLocalDof +
                                                                                           comp, 0 );
                                                            // save value
                                                            memory_valueInMatrix[gdof].push_back(boost::make_tuple(proc_id,j_gdof,v));
                                                            memory_col_globalProcessToGlobalCluster[proc_id][j_gdof]=domaindof->mapGlobalProcessToGlobalCluster()[j_gdof];
                                                        }
                                                }
                                            // dof ok : not anymore localise
                                            dof_searchWithProc[gdof].insert(proc_id);
                                        } // comp
                                } // no extrapolation
                        } // else // point found
                    // change k in for
                    k=k+image_basis_type::nComponents-1;
                } // optimization : this->interpolationType().componentsAreSamePoint()
            //----------------------------------------------------
            else // component optimization
                //------------------------------------------------
                {
                    const auto gdof = memmapGdof[proc_id][k];
                    const auto comp = memmapComp[proc_id][k];
                    ublas::column(ptsReal,0 ) = imagedof->dofPoint(gdof).template get<0>();
                    // vertice with conforme case
                    if (InterpType::value==1)
                        {
                            const uint16_type sizeVertices = memmap_vertices[proc_id][k].size();
                            verticesOfEltSearched.resize( image_mesh_type::nRealDim,sizeVertices);
                            for ( uint16_type v=0;v<sizeVertices;++v)
                                ublas::column(verticesOfEltSearched,v)=memmap_vertices[proc_id][k][v];
                        }
                    else // random
                        verticesOfEltSearched = eltRandom.vertices();

                    // localisation process
                    auto resLocalisation = locTool->run_analysis(ptsReal, eltIdLocalised, verticesOfEltSearched, mpl::int_<interpolation_type::value>());
                    if (!resLocalisation.template get<0>()[0]) // not find
                        {
                            memory_localisationFail.push_back(boost::make_tuple(gdof,comp) );
                            dof_searchWithProc[gdof].insert(proc_id);
                        }
                    else // point found
                        {
                            eltIdLocalised = resLocalisation.template get<1>();
                            if (extrapolation_mode)
                                {
                                    auto const& eltExtrapoled = this->domainSpace()->mesh()->element(eltIdLocalised);
                                    auto const& verticesExtrapoled = eltExtrapoled.vertices();
                                    typename image_mesh_type::node_type bary(verticesExtrapoled.size1());
                                    for (int qi=0;qi<verticesExtrapoled.size1();++qi) bary(qi)=0;//important
                                    for (int qj=0;qj<verticesExtrapoled.size2();++qj)
                                        {
                                            /**/                              bary(0) += ublas::column(verticesExtrapoled,qj)(0);
                                            if (verticesExtrapoled.size1()>1) bary(1) += ublas::column(verticesExtrapoled,qj)(1);
                                            if (verticesExtrapoled.size1()>2) bary(2) += ublas::column(verticesExtrapoled,qj)(2);
                                        }
                                    /**/                              bary(0) /= verticesExtrapoled.size2();
                                    if (verticesExtrapoled.size1()>1) bary(1) /= verticesExtrapoled.size2();
                                    if (verticesExtrapoled.size1()>2) bary(2) /= verticesExtrapoled.size2();
                                    typename image_mesh_type::node_type theRefPtExtrap = locTool->result_analysis().begin()->second.begin()->template get<1>();
                                    std::vector<std::pair<size_type,size_type> > j_gdofs(domain_basis_type::nLocalDof);
                                    for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                        {
                                            const size_type j_gdof =  boost::get<0>(domaindof->localToGlobal( eltIdLocalised,jloc,comp ));
                                            j_gdofs[jloc]=std::make_pair(j_gdof,domaindof->mapGlobalProcessToGlobalCluster()[j_gdof]);
                                        }
                                    dof_extrapolationData[gdof].push_back(boost::make_tuple(proc_id,bary,theRefPtExtrap,j_gdofs,comp));
                                }
                            else
                                {
                                    // get the graph row
                                    auto const ig1 = imagedof->mapGlobalProcessToGlobalCluster()[gdof];
                                    auto const theproc = imagedof->procOnGlobalCluster(ig1);
                                    auto& row = sparsity_graph->row(ig1);
                                    row.template get<0>() = theproc;
                                    row.template get<1>() = ig1 - imagedof->firstDofGlobalCluster(theproc);
                                    // for each localised points
                                    auto itanal = locTool->result_analysis().begin();//  result_analysis_begin();
                                    auto const itanal_end = locTool->result_analysis().end();//result_analysis_end();
                                    for ( ;itanal!=itanal_end;++itanal)
                                        {
                                            const auto itL=itanal->second.begin();
                                            ublas::column( ptsRef, 0 ) = boost::get<1>(*itL);
                                            // evaluate basis functions for this point
                                            MlocEval = domainbasis->evaluate( ptsRef );
                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                {
                                                    //get global dof
                                                    const size_type j_gdof =  boost::get<0>(domaindof->localToGlobal( itanal->first,jloc,comp ));
                                                    // up graph
                                                    row.template get<2>().insert(domaindof->mapGlobalProcessToGlobalCluster()[j_gdof]);
                                                    // get value
                                                    const value_type v = MlocEval( domain_basis_type::nComponents1*jloc +
                                                                                   comp*domain_basis_type::nComponents1*domain_basis_type::nLocalDof +
                                                                                   comp, 0 );
                                                    // save value
                                                    memory_valueInMatrix[gdof].push_back(boost::make_tuple(proc_id,j_gdof,v));
                                                    memory_col_globalProcessToGlobalCluster[proc_id][j_gdof]=domaindof->mapGlobalProcessToGlobalCluster()[j_gdof];
                                                }
                                        }
                                    // dof ok : not anymore localise
                                    dof_searchWithProc[gdof].insert(proc_id);
                                }
                        } // else point found
                }// no optimization
        } // for ( size_type k=0 ; k<memmapGdof[proc_id].size() ; ++k)

    return memory_localisationFail;
}

//-----------------------------------------------------------------------------------------------------------------//

template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
std::list<boost::tuple<size_type,uint16_type> >
OperatorInterpolation<DomainSpaceType, ImageSpaceType,
                      IteratorRange,InterpType>::updateNoRelationMeshMPI_upWithOtherWorld(const std::vector< std::vector<size_type> > & memmapGdof,
                                                                                          const std::vector< std::vector<uint16_type> > & memmapComp,
                                                                                          const std::vector<std::vector<typename image_mesh_type::node_type> > & pointsSearched,
                                                                                          const std::vector<std::vector< std::vector<typename image_mesh_type::node_type > > > & memmap_vertices,
                                                                                          graph_ptrtype & sparsity_graph,
                                                                                          std::vector< std::list<boost::tuple<int,size_type,double> > > & memory_valueInMatrix,
                                                                                          std::vector<std::map<size_type,size_type> > & memory_col_globalProcessToGlobalCluster,
                                                                                          std::vector<std::set<size_type> > & dof_searchWithProc,
                                                                                          bool extrapolation_mode,
                                                                                          extrapolation_memory_type & dof_extrapolationData)
{
    std::list<boost::tuple<size_type,uint16_type> > memory_localisationFail;// gdof,comp

    auto const* imagedof = this->dualImageSpace()->dof().get();
    auto const* domaindof = this->domainSpace()->dof().get();
    auto const* domainbasis = this->domainSpace()->basis().get();

    const size_type proc_id = this->dualImageSpace()->worldComm()/*worldsComm()[0]*/.localRank();
    const size_type proc_id_image = this->dualImageSpace()->mesh()->worldComm().localRank();
    const size_type proc_id_domain = this->domainSpace()->mesh()->worldComm().localRank();
    const size_type nProc = this->dualImageSpace()->mesh()->worldComm().size();
    const size_type nProc_row = this->dualImageSpace()->mesh()->worldComm().localSize();
    const size_type nProc_col = this->domainSpace()->mesh()->worldComm().localSize();
    const size_type nProc_image = this->dualImageSpace()->mesh()->worldComm().localSize();
    const size_type nProc_domain = this->domainSpace()->mesh()->worldComm().localSize();

    // localisation tool with matrix node
    auto locTool = this->domainSpace()->mesh()->tool_localization();
    matrix_node_type ptsReal( image_mesh_type::nRealDim, 1 );
    matrix_node_type ptsRef( image_mesh_type::nRealDim , 1 );
    matrix_node_type MlocEval(domain_basis_type::nLocalDof*domain_basis_type::nComponents1,1);
    matrix_node_type verticesOfEltSearched;

    // random (just to start)
    size_type eltIdLocalised = this->domainSpace()->mesh()->beginElementWithId(this->domainSpace()->mesh()->worldComm().localRank())->id();
    auto const& eltRandom = this->domainSpace()->mesh()->element(eltIdLocalised);

    std::vector<bool> dof_done( this->dualImageSpace()->nLocalDof(), false);

    // usefull container
    std::vector<size_type> pointsSearchedSizeWorld(this->dualImageSpace()->mesh()->worldComm().localComm().size());
    std::vector<typename image_mesh_type::node_type> dataToRecv(1);
    std::vector<uint16_type> dataToRecv_Comp(1,0);
    std::vector< std::vector< typename image_mesh_type::node_type > > dataToRecv_Vertices(1);
    std::vector<typename image_mesh_type::node_type> pointsRefFinded(1);
    std::vector<typename image_mesh_type::node_type> pointsBaryFinded(1);// extrapolation only
    std::vector<bool> pointsRefIsFinded(1,false);
    std::vector<int> pointsIdEltFinded(1,0);
    std::vector<std::vector<int> > pointsDofsColFinded(1,std::vector<int>(1,0));
    std::vector<std::vector<int> > pointsDofsGlobalClusterColFinded(1,std::vector<int>(1,0));
    std::vector<uint16_type> pointsComp(1,0);


    // Attention : marche que si les 2 worldcomms qui s'emboite (mon cas)
    std::vector<int> localMeshRankToWorldCommFusion_domain(nProc_col);
    mpi::all_gather( this->domainSpace()->mesh()->worldComm().localComm(),
                     this->worldCommFusion().globalRank(),
                     localMeshRankToWorldCommFusion_domain );
    std::vector<int> localMeshRankToWorldCommFusion_image(nProc_row);
    mpi::all_gather( this->dualImageSpace()->mesh()->worldComm().localComm(),
                     this->worldCommFusion().globalRank(),
                     localMeshRankToWorldCommFusion_image );

    std::vector<int> domainProcIsActive_fusion(this->worldCommFusion().globalSize());
    mpi::all_gather( this->worldCommFusion().globalComm(),
                     (int)this->domainSpace()->worldComm().isActive(),
                     domainProcIsActive_fusion );
    std::vector<int> imageProcIsActive_fusion(this->worldCommFusion().globalSize());
    mpi::all_gather( this->worldCommFusion().globalComm(),
                     (int)this->dualImageSpace()->worldComm().isActive(),
                     imageProcIsActive_fusion );

    int firstActiveProc_image=0;
    bool findFirstActive_image=false;
    while (!findFirstActive_image)
        {
            if (imageProcIsActive_fusion[firstActiveProc_image])
                {
                    findFirstActive_image=true;
                }
            else ++firstActiveProc_image;
        }
    int firstActiveProc_domain=0;
    bool findFirstActive_domain=false;
    while (!findFirstActive_domain)
        {
            if (domainProcIsActive_fusion[firstActiveProc_domain])
                {
                    findFirstActive_domain=true;
                }
            else ++firstActiveProc_domain;
        }

    for (int p=0;p<localMeshRankToWorldCommFusion_image.size(); ++p)
        {
            if (!this->dualImageSpace()->worldComm().isActive()) localMeshRankToWorldCommFusion_image[p]=p%nProc_image+firstActiveProc_image; // FAIRE COMMMUNICATION!!!!!
        }
    for (int p=0;p<localMeshRankToWorldCommFusion_domain.size(); ++p)
        {
            if (!this->domainSpace()->worldComm().isActive()) localMeshRankToWorldCommFusion_domain[p]=p%nProc_domain+firstActiveProc_domain; // FAIRE COMMMUNICATION!!!!!
        }

    // searchDistribution (no comm with ourself)
    std::vector<std::list<int> > searchDistribution(nProc);
    for (int p=0;p<nProc_image;++p)
        {
            searchDistribution[p].clear();
            for (int q=0;q<nProc_domain;++q)
                {
                    //if (q!=p)
                    if( (localMeshRankToWorldCommFusion_image[p])!=localMeshRankToWorldCommFusion_domain[q] )
                        {
                            searchDistribution[p].push_back(q);
                        }
                }
        }


#if 0
    this->worldCommFusion().barrier();
    for (int p=0;p<this->worldCommFusion().globalSize();++p)
        {
            if (p==this->worldCommFusion().globalRank())
                {
                    std::cout << "I am proc " << p << "\n";
                    for (int q=0;q<nProc_image;++q)
                        {
                            auto it_list = searchDistribution[q].begin();
                            auto en_list = searchDistribution[q].end();
                            for ( ; it_list!=en_list;++it_list)
                                {
                                    std::cout << *it_list <<" ";
                                }
                            std::cout << std::endl;
                        }
                }
            this->worldCommFusion().barrier();
        }
#endif














#if 0 // OLD

    // tag for mpi msg
    const int tag_X = 0, tag_Y = 1, tag_Z = 2, tag_IsFind = 3, tag_IdElt = 4, tag_DofsCol = 5, tag_DofsColGC = 6, tag_Comp = 7, tag_Points=8, tag_Bary=9;
    //------------------------------
    // proc after proc
    for (int proc=0;proc<nProc_row;++proc)
        {
            for (auto it_rankLocalization=searchDistribution[proc].begin(),en_rankLocalization=searchDistribution[proc].end();
                 it_rankLocalization!=en_rankLocalization;++it_rankLocalization)
                {
                    const int rankLocalization = *it_rankLocalization;
                    //if(this->worldCommFusion().globalRank()==0)
                    //std::cout << "proc_id_image " << proc_id_image << " proc_id_domain " << proc_id_domain << " it_rankLocalization" << *it_rankLocalization << std::endl;
#if 1
                    if ( proc_id_image == proc && imageProcIsActive_fusion[this->worldCommFusion().globalRank()] )  // send info to rankLocalization
                        {

                            const int rankToSend = localMeshRankToWorldCommFusion_domain[rankLocalization];
                            this->worldCommFusion().globalComm().send(rankToSend,tag_Points,pointsSearched[rankLocalization]);
                            this->worldCommFusion().globalComm().send(rankToSend,tag_Comp,memmapComp[rankLocalization]);
                        }
                    else if ( proc_id_domain == rankLocalization && domainProcIsActive_fusion[this->worldCommFusion().globalRank()] ) // get request of proc
                        {
                            const int rankToRecv = localMeshRankToWorldCommFusion_image[proc];
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_Points,dataToRecv);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_Comp,dataToRecv_Comp);
                        }
#endif
                    //-----------------------------------------------------------------------------------------
                    //this->dualImageSpace()->mesh()->worldComm().localComm().barrier();
                    //-----------------------------------------------------------------------------------------
#if 1
                    if ( proc_id_domain == rankLocalization && domainProcIsActive_fusion[this->worldCommFusion().globalRank()] )
                        {
                            const size_type nDataRecv = dataToRecv.size();
                            // init container
                            pointsRefFinded.resize(nDataRecv);
                            pointsRefIsFinded.resize(nDataRecv);std::fill(pointsRefIsFinded.begin(),pointsRefIsFinded.end(),false);
                            pointsIdEltFinded.resize(nDataRecv);
                            if (extrapolation_mode) pointsBaryFinded.resize(nDataRecv);
                            pointsDofsColFinded.resize(nDataRecv,std::vector<int>(1,0));
                            pointsDofsGlobalClusterColFinded.resize(nDataRecv,std::vector<int>(1,0));
                            // iterate on points
                            for (size_type k=0;k<nDataRecv;++k)
                                {
                                    // get real point to search
                                    ublas::column(ptsReal,0) = dataToRecv[k];

                                    // search process

                                    auto resLocalisation = locTool->run_analysis(ptsReal,eltIdLocalised,eltRandom.vertices()/*it->G()*/,mpl::int_<interpolation_type::value>());
                                    if (resLocalisation.template get<0>()[0]) // is find
                                        {
                                            eltIdLocalised = resLocalisation.template get<1>();
                                            const uint16_type comp=dataToRecv_Comp[k];
                                            pointsRefIsFinded[k]=true;
                                            pointsIdEltFinded[k]=eltIdLocalised;
                                            // get point in reference element
                                            auto itanal = locTool->result_analysis_begin();
                                            auto const itanal_end = locTool->result_analysis_end();
                                            for ( ;itanal!=itanal_end;++itanal)
                                                {
                                                    auto const itL=itanal->second.begin();
                                                    ublas::column( ptsRef, 0 ) = boost::get<1>(*itL);
                                                    pointsRefFinded[k] = boost::get<1>(*itL);
                                                }
                                            // get global process dof and global cluster dof : column data map
                                            pointsDofsColFinded[k].resize(domain_basis_type::nLocalDof);
                                            pointsDofsGlobalClusterColFinded[k].resize(domain_basis_type::nLocalDof);
                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                {
                                                    const auto j_gdof = boost::get<0>(domaindof->localToGlobal( eltIdLocalised,jloc,comp ));
                                                    pointsDofsColFinded[k][jloc] = j_gdof;
                                                    pointsDofsGlobalClusterColFinded[k][jloc] = domaindof->mapGlobalProcessToGlobalCluster()[j_gdof];
                                                }

                                            if (extrapolation_mode)
                                                {
                                                    auto const& eltExtrapoled = this->domainSpace()->mesh()->element(eltIdLocalised);
                                                    auto const verticesExtrapoled = eltExtrapoled.vertices();
                                                    typename image_mesh_type::node_type bary(verticesExtrapoled.size1());
                                                    for (int qi=0;qi<verticesExtrapoled.size1();++qi) bary(qi)=0;//important
                                                    for (int qj=0;qj<verticesExtrapoled.size2();++qj)
                                                    {
                                                        /**/                              bary(0) += ublas::column(verticesExtrapoled,qj)(0);
                                                        if (verticesExtrapoled.size1()>1) bary(1) += ublas::column(verticesExtrapoled,qj)(1);
                                                        if (verticesExtrapoled.size1()>2) bary(2) += ublas::column(verticesExtrapoled,qj)(2);
                                                    }
                                                    /**/                              bary(0) /= verticesExtrapoled.size2();
                                                    if (verticesExtrapoled.size1()>1) bary(1) /= verticesExtrapoled.size2();
                                                    if (verticesExtrapoled.size1()>2) bary(2) /= verticesExtrapoled.size2();
                                                    pointsBaryFinded[k]=bary;
                                                }
                                            //std::cout << "F";
                                        }
                                    else // Not Find!
                                        {
                                            //memory_localisationFail
                                            //std::cout << "NOT FIND"<<std::endl;
                                        }

                                }
                        } // if ( proc_id == rankLocalization )
#endif
                    //-----------------------------------------------------------------------------------------
                    this->worldCommFusion().globalComm().barrier();
                    //-----------------------------------------------------------------------------------------
#if 1
                    // send the response after localization
                    if ( proc_id_domain == rankLocalization && domainProcIsActive_fusion[this->worldCommFusion().globalRank()])
                        {
                            const int rankToSend = localMeshRankToWorldCommFusion_image[proc];
                            this->worldCommFusion().globalComm().send(rankToSend,tag_Points,pointsRefFinded);
                            this->worldCommFusion().globalComm().send(rankToSend,tag_IsFind,pointsRefIsFinded);
                            this->worldCommFusion().globalComm().send(rankToSend,tag_IdElt,pointsIdEltFinded);
                            this->worldCommFusion().globalComm().send(rankToSend,tag_DofsCol,pointsDofsColFinded);
                            this->worldCommFusion().globalComm().send(rankToSend,tag_DofsColGC,pointsDofsGlobalClusterColFinded);
                            if (extrapolation_mode)
                                this->worldCommFusion().globalComm().send(rankToSend,tag_Bary,pointsBaryFinded);

                        }
                    else if ( proc_id_image == proc && imageProcIsActive_fusion[this->worldCommFusion().globalRank()] )
                        {
                            const int rankToRecv = localMeshRankToWorldCommFusion_domain[rankLocalization];
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_Points,pointsRefFinded);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_IsFind,pointsRefIsFinded);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_IdElt,pointsIdEltFinded);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_DofsCol,pointsDofsColFinded);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_DofsColGC,pointsDofsGlobalClusterColFinded);
                            if (extrapolation_mode)
                                this->worldCommFusion().globalComm().recv(rankToRecv,tag_Bary,pointsBaryFinded);

                        }
#endif
                    //-----------------------------------------------------------------------------------------
                    this->worldCommFusion().globalComm().barrier();
                    //-----------------------------------------------------------------------------------------
#if 1
                    if ( proc_id_image==proc && imageProcIsActive_fusion[this->worldCommFusion().globalRank()] )
                        {
                            const int rankRecv=rankLocalization;
                            for ( int k=0;k<pointsRefFinded.size();++k)
                                {
                                    if (!pointsRefIsFinded[k])
                                        {
                                            memory_localisationFail.push_back(boost::make_tuple(memmapGdof[rankLocalization][k],memmapComp[rankLocalization][k]));
                                            const auto i_gdof = memmapGdof[rankLocalization][k];
                                            dof_searchWithProc[i_gdof].insert(rankRecv);
                                        }
                                    else
                                        {
                                            const auto i_gdof = memmapGdof[rankLocalization][k];
                                            if (!dof_done[i_gdof])
                                                {

                                                    if (extrapolation_mode)
                                                        {
                                                            auto const bary = pointsBaryFinded[k];
                                                            auto const theRefPtExtrap = pointsRefFinded[k];
                                                            const auto comp = memmapComp[rankLocalization][k];
                                                            std::vector<std::pair<size_type,size_type> > j_gdofs(domain_basis_type::nLocalDof);
                                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                                {
                                                                    const size_type j_gdof =  pointsDofsColFinded[k][jloc];
                                                                    const size_type j_gdof_gc = pointsDofsGlobalClusterColFinded[k][jloc];
                                                                    j_gdofs[jloc]=std::make_pair(j_gdof,j_gdof_gc);
                                                                }
                                                            dof_extrapolationData[i_gdof].push_back(boost::make_tuple(rankLocalization,bary,theRefPtExtrap,j_gdofs,comp));
                                                        }
                                                    else
                                                        {
                                                            //std::cout << "T";
                                                            ublas::column( ptsRef, 0 ) = pointsRefFinded[k];
                                                            //evalute point on the reference element
                                                            MlocEval = domainbasis->evaluate( ptsRef );

                                                            const auto comp = memmapComp[rankLocalization][k];
                                                            const size_type myidElt = pointsIdEltFinded[k];
                                                            const auto ig1 = imagedof->mapGlobalProcessToGlobalCluster()[i_gdof];
                                                            const auto theproc = imagedof->procOnGlobalCluster(ig1);
                                                            auto& row = sparsity_graph->row(ig1);
                                                            row.template get<0>() = theproc;
                                                            row.template get<1>() = ig1 - imagedof->firstDofGlobalCluster(theproc);

                                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                                {
                                                                    //get global process dof
                                                                    const size_type j_gdof =  pointsDofsColFinded[k][jloc];
                                                                    //get global cluster dof
                                                                    const size_type j_gdof_gc = pointsDofsGlobalClusterColFinded[k][jloc];
                                                                    // up graph
                                                                    row.template get<2>().insert(j_gdof_gc);
                                                                    // get value
                                                                    const auto v = MlocEval( domain_basis_type::nComponents1*jloc +
                                                                                             comp*domain_basis_type::nComponents1*domain_basis_type::nLocalDof +
                                                                                             comp, 0 );
#if 1
                                                                    // save value
                                                                    memory_valueInMatrix[i_gdof].push_back(boost::make_tuple(rankRecv,j_gdof,v));
                                                                    // usefull to build datamap
                                                                    memory_col_globalProcessToGlobalCluster[rankRecv][j_gdof]=j_gdof_gc;
#endif
                                                                }
                                                            // dof ok : not anymore localise
                                                            dof_searchWithProc[i_gdof].insert(rankRecv);
                                                        }
                                                    dof_done[i_gdof]=true;
                                                } // if (!dof_done[i_gdof])
                                        }
                                }
                        }

#endif
                    //-----------------------------------------------------------------------------------------
                    this->worldCommFusion().globalComm().barrier();
                    //-----------------------------------------------------------------------------------------

                } // for (auto it_rankLocalization=searchDistribution[proc].begin(),...


        } // for (int proc=0;proc<this->dualImageSpace()->mesh()->worldComm().localSize();++proc)

#else // NEW

    // tag for mpi msg
    const int tag_X = 0, tag_Y = 1, tag_Z = 2, tag_IsFind = 3, tag_IdElt = 4, tag_DofsCol = 5, tag_DofsColGC = 6, tag_Comp = 7, tag_Points=8, tag_Bary=9, tag_Vertices=10;
    //------------------------------
    // proc after proc
    for (int proc=0;proc<nProc_row;++proc)
        {
            if ( proc_id_image == proc && imageProcIsActive_fusion[this->worldCommFusion().globalRank()] )  // send info to rankLocalization
                {
                    for (auto it_rankLocalization=searchDistribution[proc].begin(),en_rankLocalization=searchDistribution[proc].end();
                         it_rankLocalization!=en_rankLocalization;++it_rankLocalization)
                        {
                            const int rankLocalization = *it_rankLocalization;
                            const int rankToSend = localMeshRankToWorldCommFusion_domain[rankLocalization];
                            this->worldCommFusion().globalComm().send(rankToSend,tag_Points,pointsSearched[rankLocalization]);
                            this->worldCommFusion().globalComm().send(rankToSend,tag_Comp,memmapComp[rankLocalization]);
                            if (InterpType::value==1)
                                this->worldCommFusion().globalComm().send(rankToSend,tag_Vertices,memmap_vertices[rankLocalization]);
                        }
                }
            else
                {
                    for (auto it_rankLocalization=searchDistribution[proc].begin(),en_rankLocalization=searchDistribution[proc].end();
                         it_rankLocalization!=en_rankLocalization;++it_rankLocalization)
                        {
                            const int rankLocalization = *it_rankLocalization;
                            if ( proc_id_domain == rankLocalization && domainProcIsActive_fusion[this->worldCommFusion().globalRank()] ) // get request of proc
                                {
                                    const int rankToRecv = localMeshRankToWorldCommFusion_image[proc];
                                    this->worldCommFusion().globalComm().recv(rankToRecv,tag_Points,dataToRecv);
                                    this->worldCommFusion().globalComm().recv(rankToRecv,tag_Comp,dataToRecv_Comp);
                                    if (InterpType::value==1)
                                        this->worldCommFusion().globalComm().recv(rankToRecv,tag_Vertices,dataToRecv_Vertices);

                                    const size_type nDataRecv = dataToRecv.size();
                                    // init container
                                    pointsRefFinded.resize(nDataRecv);
                                    pointsRefIsFinded.resize(nDataRecv);std::fill(pointsRefIsFinded.begin(),pointsRefIsFinded.end(),false);
                                    pointsIdEltFinded.resize(nDataRecv);
                                    if (extrapolation_mode) pointsBaryFinded.resize(nDataRecv);
                                    pointsDofsColFinded.resize(nDataRecv,std::vector<int>(1,0));
                                    pointsDofsGlobalClusterColFinded.resize(nDataRecv,std::vector<int>(1,0));
                                    // iterate on points
                                    for (size_type k=0;k<nDataRecv;++k)
                                        {
                                            // get real point to search
                                            ublas::column(ptsReal,0) = dataToRecv[k];
                                            // vertice with conforme case
                                            if (InterpType::value==1)
                                                {
                                                    const uint16_type sizeVertices = dataToRecv_Vertices[k].size();
                                                    verticesOfEltSearched.resize( image_mesh_type::nRealDim,sizeVertices);
                                                    for ( uint16_type v=0;v<sizeVertices;++v)
                                                        ublas::column(verticesOfEltSearched,v)=dataToRecv_Vertices[k][v];
                                                }
                                            else // random
                                                verticesOfEltSearched = eltRandom.vertices();
                                            // search process
                                            auto resLocalisation = locTool->run_analysis(ptsReal,eltIdLocalised,verticesOfEltSearched,mpl::int_<interpolation_type::value>());
                                            if (resLocalisation.template get<0>()[0]) // is find
                                                {
                                                    eltIdLocalised = resLocalisation.template get<1>();
                                                    //-------------------------------------------------------------------------
                                                    if (!this->interpolationType().componentsAreSamePoint() )// all component
                                                    //-------------------------------------------------------------------------
                                                        {
                                                            const uint16_type comp=dataToRecv_Comp[k];
                                                            pointsRefIsFinded[k]=true;
                                                            pointsIdEltFinded[k]=eltIdLocalised;
                                                            // get point in reference element
                                                            auto itanal = locTool->result_analysis_begin();
                                                            auto const itanal_end = locTool->result_analysis_end();
                                                            for ( ;itanal!=itanal_end;++itanal)
                                                                {
                                                                    auto const itL=itanal->second.begin();
                                                                    //ublas::column( ptsRef, 0 ) = boost::get<1>(*itL);
                                                                    pointsRefFinded[k] = boost::get<1>(*itL);
                                                                }
                                                            // get global process dof and global cluster dof : column data map
                                                            pointsDofsColFinded[k].resize(domain_basis_type::nLocalDof);
                                                            pointsDofsGlobalClusterColFinded[k].resize(domain_basis_type::nLocalDof);
                                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                                {
                                                                    const auto j_gdof = boost::get<0>(domaindof->localToGlobal( eltIdLocalised,jloc,comp ));
                                                                    pointsDofsColFinded[k][jloc] = j_gdof;
                                                                    pointsDofsGlobalClusterColFinded[k][jloc] = domaindof->mapGlobalProcessToGlobalCluster()[j_gdof];
                                                                }
                                                            if (extrapolation_mode)
                                                                {
                                                                    auto const& eltExtrapoled = this->domainSpace()->mesh()->element(eltIdLocalised);
                                                                    auto const verticesExtrapoled = eltExtrapoled.vertices();
                                                                    typename image_mesh_type::node_type bary(verticesExtrapoled.size1());
                                                                    for (int qi=0;qi<verticesExtrapoled.size1();++qi) bary(qi)=0;//important
                                                                    for (int qj=0;qj<verticesExtrapoled.size2();++qj)
                                                                        {
                                                                            /**/                              bary(0) += ublas::column(verticesExtrapoled,qj)(0);
                                                                            if (verticesExtrapoled.size1()>1) bary(1) += ublas::column(verticesExtrapoled,qj)(1);
                                                                            if (verticesExtrapoled.size1()>2) bary(2) += ublas::column(verticesExtrapoled,qj)(2);
                                                                        }
                                                                    /**/                              bary(0) /= verticesExtrapoled.size2();
                                                                    if (verticesExtrapoled.size1()>1) bary(1) /= verticesExtrapoled.size2();
                                                                    if (verticesExtrapoled.size1()>2) bary(2) /= verticesExtrapoled.size2();
                                                                    pointsBaryFinded[k]=bary;
                                                                }
                                                        }
                                                    //-------------------------------------------------------------------------
                                                    else // components optimization
                                                    //-------------------------------------------------------------------------
                                                        {
                                                            auto const ptRefFinded = locTool->result_analysis_begin()->second.begin()->template get<1>();
                                                            for ( uint16_type comp = 0;comp < image_basis_type::nComponents;++comp )
                                                                {
                                                                    pointsRefIsFinded[k+comp]=true;
                                                                    pointsIdEltFinded[k+comp]=eltIdLocalised;
                                                                    // get point in reference element
                                                                    pointsRefFinded[k+comp] = ptRefFinded;

                                                                    pointsDofsColFinded[k+comp].resize(domain_basis_type::nLocalDof);
                                                                    pointsDofsGlobalClusterColFinded[k+comp].resize(domain_basis_type::nLocalDof);
                                                                    for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                                        {
                                                                            const auto j_gdof = boost::get<0>(domaindof->localToGlobal( eltIdLocalised,jloc,comp ));
                                                                            pointsDofsColFinded[k+comp][jloc] = j_gdof;
                                                                            pointsDofsGlobalClusterColFinded[k+comp][jloc] = domaindof->mapGlobalProcessToGlobalCluster()[j_gdof];
                                                                        }
                                                                }

                                                                    if (extrapolation_mode)
                                                                        {
                                                                            auto const& eltExtrapoled = this->domainSpace()->mesh()->element(eltIdLocalised);
                                                                            auto const verticesExtrapoled = eltExtrapoled.vertices();
                                                                            typename image_mesh_type::node_type bary(verticesExtrapoled.size1());
                                                                            for (int qi=0;qi<verticesExtrapoled.size1();++qi) bary(qi)=0;//important
                                                                            for (int qj=0;qj<verticesExtrapoled.size2();++qj)
                                                                                {
                                                                                    /**/                              bary(0) += ublas::column(verticesExtrapoled,qj)(0);
                                                                                    if (verticesExtrapoled.size1()>1) bary(1) += ublas::column(verticesExtrapoled,qj)(1);
                                                                                    if (verticesExtrapoled.size1()>2) bary(2) += ublas::column(verticesExtrapoled,qj)(2);
                                                                                }
                                                                            /**/                              bary(0) /= verticesExtrapoled.size2();
                                                                            if (verticesExtrapoled.size1()>1) bary(1) /= verticesExtrapoled.size2();
                                                                            if (verticesExtrapoled.size1()>2) bary(2) /= verticesExtrapoled.size2();
                                                                            for ( uint16_type comp = 0;comp < image_basis_type::nComponents;++comp )
                                                                                pointsBaryFinded[k+comp]=bary;
                                                                        }
                                                                    // change increment in for
                                                                    k=k+image_basis_type::nComponents-1;
                                                        }

                                                    //std::cout << "F";
                                                }
                                            else // Not Find!
                                                {
                                                    //memory_localisationFail
                                                    //std::cout << "NOT FIND"<<std::endl;
                                                }

                                        } // for (size_type k=0;k<nDataRecv;++k)

                                    const int rankToSend = localMeshRankToWorldCommFusion_image[proc];
                                    this->worldCommFusion().globalComm().send(rankToSend,tag_Points,pointsRefFinded);
                                    this->worldCommFusion().globalComm().send(rankToSend,tag_IsFind,pointsRefIsFinded);
                                    this->worldCommFusion().globalComm().send(rankToSend,tag_IdElt,pointsIdEltFinded);
                                    this->worldCommFusion().globalComm().send(rankToSend,tag_DofsCol,pointsDofsColFinded);
                                    this->worldCommFusion().globalComm().send(rankToSend,tag_DofsColGC,pointsDofsGlobalClusterColFinded);
                                    if (extrapolation_mode)
                                        this->worldCommFusion().globalComm().send(rankToSend,tag_Bary,pointsBaryFinded);

                                }
                        } // for (auto it_rankLocalization=...
                }




            if ( proc_id_image == proc && imageProcIsActive_fusion[this->worldCommFusion().globalRank()] )
                {
                    for (auto it_rankLocalization=searchDistribution[proc].begin(),en_rankLocalization=searchDistribution[proc].end();
                         it_rankLocalization!=en_rankLocalization;++it_rankLocalization)
                        {
                            const int rankLocalization = *it_rankLocalization;
                            const int rankToRecv = localMeshRankToWorldCommFusion_domain[rankLocalization];
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_Points,pointsRefFinded);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_IsFind,pointsRefIsFinded);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_IdElt,pointsIdEltFinded);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_DofsCol,pointsDofsColFinded);
                            this->worldCommFusion().globalComm().recv(rankToRecv,tag_DofsColGC,pointsDofsGlobalClusterColFinded);
                            if (extrapolation_mode)
                                this->worldCommFusion().globalComm().recv(rankToRecv,tag_Bary,pointsBaryFinded);

                            const int rankRecv=rankLocalization;
                            for ( int k=0;k<pointsRefFinded.size();++k)
                                {
                                    if (!pointsRefIsFinded[k])
                                        {
                                            memory_localisationFail.push_back(boost::make_tuple(memmapGdof[rankLocalization][k],memmapComp[rankLocalization][k]));
                                            const auto i_gdof = memmapGdof[rankLocalization][k];
                                            dof_searchWithProc[i_gdof].insert(rankRecv);
                                        }
                                    else
                                        {
                                            const auto i_gdof = memmapGdof[rankLocalization][k];
                                            if (!dof_done[i_gdof])
                                                {

                                                    if (extrapolation_mode)
                                                        {
                                                            auto const bary = pointsBaryFinded[k];
                                                            auto const theRefPtExtrap = pointsRefFinded[k];
                                                            const auto comp = memmapComp[rankLocalization][k];
                                                            std::vector<std::pair<size_type,size_type> > j_gdofs(domain_basis_type::nLocalDof);
                                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                                {
                                                                    const size_type j_gdof =  pointsDofsColFinded[k][jloc];
                                                                    const size_type j_gdof_gc = pointsDofsGlobalClusterColFinded[k][jloc];
                                                                    j_gdofs[jloc]=std::make_pair(j_gdof,j_gdof_gc);
                                                                }
                                                            dof_extrapolationData[i_gdof].push_back(boost::make_tuple(rankLocalization,bary,theRefPtExtrap,j_gdofs,comp));
                                                        }
                                                    else
                                                        {
                                                            //std::cout << "T";
                                                            ublas::column( ptsRef, 0 ) = pointsRefFinded[k];
                                                            //evalute point on the reference element
                                                            MlocEval = domainbasis->evaluate( ptsRef );

                                                            const auto comp = memmapComp[rankLocalization][k];
                                                            const size_type myidElt = pointsIdEltFinded[k];
                                                            const auto ig1 = imagedof->mapGlobalProcessToGlobalCluster()[i_gdof];
                                                            const auto theproc = imagedof->procOnGlobalCluster(ig1);
                                                            auto& row = sparsity_graph->row(ig1);
                                                            row.template get<0>() = theproc;
                                                            row.template get<1>() = ig1 - imagedof->firstDofGlobalCluster(theproc);

                                                            for ( uint16_type jloc = 0; jloc < domain_basis_type::nLocalDof; ++jloc )
                                                                {
                                                                    //get global process dof
                                                                    const size_type j_gdof =  pointsDofsColFinded[k][jloc];
                                                                    //get global cluster dof
                                                                    const size_type j_gdof_gc = pointsDofsGlobalClusterColFinded[k][jloc];
                                                                    // up graph
                                                                    row.template get<2>().insert(j_gdof_gc);
                                                                    // get value
                                                                    const auto v = MlocEval( domain_basis_type::nComponents1*jloc +
                                                                                             comp*domain_basis_type::nComponents1*domain_basis_type::nLocalDof +
                                                                                             comp, 0 );
#if 1
                                                                    // save value
                                                                    memory_valueInMatrix[i_gdof].push_back(boost::make_tuple(rankRecv,j_gdof,v));
                                                                    // usefull to build datamap
                                                                    memory_col_globalProcessToGlobalCluster[rankRecv][j_gdof]=j_gdof_gc;
#endif
                                                                }
                                                            // dof ok : not anymore localise
                                                            dof_searchWithProc[i_gdof].insert(rankRecv);
                                                        }
                                                    dof_done[i_gdof]=true;
                                                } // if (!dof_done[i_gdof])
                                        }
                                }
                        }
                } // if ( proc_id_image == proc && imageProcIsActive_fusion[this->worldCommFusion().globalRank()] )
        }

#endif // NEW


    return memory_localisationFail;
}

template<typename DomainSpaceType, typename ImageSpaceType,typename IteratorRange,typename InterpType>
boost::tuple<std::vector< std::vector<size_type> >, std::vector< std::vector<uint16_type> >,
             std::vector<std::vector<typename OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::image_mesh_type::node_type> >,
             std::vector<std::vector< std::vector<typename OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::image_mesh_type::node_type > > > >
OperatorInterpolation<DomainSpaceType, ImageSpaceType,
                      IteratorRange,InterpType>::updateNoRelationMeshMPI_pointDistribution(const std::vector< std::list<boost::tuple<int,size_type,double> > > & memory_valueInMatrix,
                                                                                           std::vector<std::set<size_type> > & dof_searchWithProc)
{
    //std::cout << " pointDistribution--1--- " << this->domainSpace()->mesh()->worldComm().godRank() << std::endl;
    //const size_type proc_id = this->dualImageSpace()->worldsComm()[0].localRank();
    //const size_type nProc = this->dualImageSpace()->mesh()->worldComm().size();
    //const size_type nProc_image = this->dualImageSpace()->mesh()->worldComm().localSize();
    const size_type nProc_domain = this->domainSpace()->mesh()->worldComm().localSize();

    auto const* imagedof = this->dualImageSpace()->dof().get();
    iterator_type it, en;

    auto locTool = this->domainSpace()->mesh()->tool_localization();

    std::vector<bool> dof_done( this->dualImageSpace()->nLocalDof(), false);
    std::vector< std::list<boost::tuple<size_type,uint16_type> > > memSetGdofAndComp( nProc_domain );
    std::vector< std::list<matrix_node_type> > memSetVertices_conformeInterp( nProc_domain );

    // Warning communication!!
    std::vector<typename image_mesh_type::node_type> vecBarycenter(nProc_domain);
    mpi::all_gather( this->domainSpace()->mesh()->worldComm().localComm(),
                     locTool->barycenter(),
                     vecBarycenter );
    /*std::cout << " proc " << this->domainSpace()->mesh()->worldComm().localRank()
              << "  procFuion " << this->worldCommFusion().globalRank()
              << " bary " << locTool->barycenter()
              << std::endl;*/


    double distanceMin=0,distance=0,distanceSquare=0;
    int procForPt=0;

    if ( this->dualImageSpace()->worldComm().isActive() )
        {
            boost::tie( boost::tuples::ignore, it, en ) = _M_range;
            for ( ; it!=en;++it )
                {
                    for ( uint16_type iloc = 0; iloc < nLocalDofInDualImageElt; ++iloc )
                        {
                            for ( uint16_type comp = 0;comp < image_basis_type::nComponents;++comp )
                                {
                                    const auto gdof =  boost::get<0>(imagedof->localToGlobal( *it, iloc, comp ));
                                    if (!dof_done[gdof] && memory_valueInMatrix[gdof].size()==0)
                                        {
                                           // the dof point
                                            const auto imagePoint = imagedof->dofPoint(gdof).template get<0>();

                                            if (this->interpolationType().searchWithCommunication()) // mpi communication
                                                {
                                                    distanceMin=INT_MAX;
                                                    for ( int proc=0 ; proc<nProc_domain; ++proc)
                                                        {
                                                            const auto bary = vecBarycenter[proc];
                                                            /**/               distanceSquare  = std::pow(imagePoint(0)-bary(0),2);
                                                            if (bary.size()>1) distanceSquare += std::pow(imagePoint(1)-bary(1),2);
                                                            if (bary.size()>2) distanceSquare += std::pow(imagePoint(2)-bary(2),2);
                                                            distance = std::sqrt( distanceSquare );
                                                            if (distance<distanceMin && dof_searchWithProc[gdof].find(proc)==dof_searchWithProc[gdof].end() )
                                                                {
                                                                    procForPt = proc;
                                                                    distanceMin=distance;
                                                                }
                                                        }
                                                    memSetGdofAndComp[procForPt].push_back(boost::make_tuple(gdof,comp));
                                                    if (InterpType::value==1)
                                                        memSetVertices_conformeInterp[procForPt].push_back(it->vertices());
                                                }
                                            else // only with myself
                                                {
                                                    memSetGdofAndComp[this->domainSpace()->worldComm().globalRank()].push_back(boost::make_tuple(gdof,comp));
                                                    if (InterpType::value==1) // conforme case
                                                        {
                                                            memSetVertices_conformeInterp[this->domainSpace()->worldComm().globalRank()].push_back(it->vertices());
                                                        }
                                                }

                                            dof_done[gdof]=true;
                                        }
                                }
                        }
                }
        } // isActive

    // memory map (loc index pt) -> global dofs
    std::vector< std::vector<size_type> > memmapGdof( nProc_domain );
    // memory map (loc index pt) -> comp
    std::vector< std::vector<uint16_type> > memmapComp( nProc_domain );
    // points to lacalize
    std::vector<std::vector<typename image_mesh_type::node_type> > pointsSearched( nProc_domain );

    std::vector<std::vector< std::vector<typename image_mesh_type::node_type > > > memmap_vertices( nProc_domain );

    for (int proc=0; proc<nProc_domain;++proc)
        {
            const size_type nData = memSetGdofAndComp[proc].size();
            memmapGdof[proc].resize(nData);
            memmapComp[proc].resize(nData);
            pointsSearched[proc].resize(nData);
            // conforme case
            if(InterpType::value==1) memmap_vertices[proc].resize(nData);

            auto it_GdofAndComp = memSetGdofAndComp[proc].begin();
            auto it_vertices = memSetVertices_conformeInterp[proc].begin();
            for (int k=0 ; k<nData ; ++k, ++it_GdofAndComp)
                {
                    memmapGdof[proc][k]=it_GdofAndComp->template get<0>();//gdof;
                    memmapComp[proc][k]=it_GdofAndComp->template get<1>();//comp
                    pointsSearched[proc][k]=imagedof->dofPoint(it_GdofAndComp->template get<0>()).template get<0>();//node
                    if(InterpType::value==1) // conforme case
                        {
                            memmap_vertices[proc][k].resize(it_vertices->size2());
                            for (uint16_type v=0;v<it_vertices->size2();++v)
                                memmap_vertices[proc][k][v]=ublas::column(*it_vertices,v);
                            ++it_vertices;
                        }
                }
        }

    return boost::make_tuple(memmapGdof,memmapComp,pointsSearched,memmap_vertices);
}

#endif // WITH MPI


//-----------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------//


template<typename DomainSpaceType, typename ImageSpaceType, typename IteratorRange, typename InterpType >
boost::shared_ptr<OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType> >
opInterp( boost::shared_ptr<DomainSpaceType> const& domainspace,
          boost::shared_ptr<ImageSpaceType> const& imagespace,
          IteratorRange const& r,
          typename OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType>::backend_ptrtype const& backend,
          InterpType const& interptype,
          bool ddmethod
        )
{
    typedef OperatorInterpolation<DomainSpaceType, ImageSpaceType,IteratorRange,InterpType> operatorinterpolation_type;

    boost::shared_ptr<operatorinterpolation_type> opI( new operatorinterpolation_type( domainspace,imagespace,r,backend,interptype,ddmethod ) );

    return opI;
}



template<typename Args>
struct compute_opInterpolation_return
{
    typedef typename boost::remove_reference<typename parameter::binding<Args, tag::domainSpace>::type>::type::element_type domain_space_type;
    typedef typename boost::remove_reference<typename parameter::binding<Args, tag::imageSpace>::type>::type::element_type image_space_type;

    typedef typename boost::remove_const<
    typename boost::remove_reference<
    typename parameter::binding<Args,
             tag::range,
             typename OperatorInterpolation<domain_space_type, image_space_type>::range_iterator
             >::type >::type >::type iterator_range_type;

    typedef typename boost::remove_const<
    typename boost::remove_reference<
    typename parameter::binding<Args,
             tag::type,
             InterpolationNonConforme
             >::type >::type >::type interpolation_type;


    typedef boost::shared_ptr<OperatorInterpolation<domain_space_type, image_space_type,iterator_range_type,interpolation_type> > type;
};

BOOST_PARAMETER_FUNCTION(
    ( typename compute_opInterpolation_return<Args>::type ), // 1. return type
    opInterpolation,                        // 2. name of the function template
    tag,                                        // 3. namespace of tag types
    ( required
      ( domainSpace,    *( boost::is_convertible<mpl::_,boost::shared_ptr<FunctionSpaceBase> > ) )
      ( imageSpace,     *( boost::is_convertible<mpl::_,boost::shared_ptr<FunctionSpaceBase> > ) )
    ) // required
    ( optional
      ( range,          *, elements( imageSpace->mesh() )  )
      ( backend,        *, Backend<typename compute_opInterpolation_return<Args>::domain_space_type::value_type>::build() )
      ( type,           *, InterpolationNonConforme()  )
      ( ddmethod,  (bool),  false )
    ) // optionnal
)
{
    Feel::detail::ignore_unused_variable_warning( args );

    return opInterp( domainSpace,imageSpace,range,backend,type,ddmethod );

} // opInterpolation




} // Feel
#endif /* __OperatorInterpolation_H */
