/**
 * \file doftablempi.hpp
 * \author Vincent Chabannes
 */
namespace Feel
{

    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//

template<typename MeshType, typename FEType, typename PeriodicityType>
void
DofTable<MeshType, FEType, PeriodicityType>::buildGhostDofMap( mesh_type& mesh )
{
    std::map<size_type,boost::tuple<size_type,size_type> > mapInterProcessDof;
    std::set<int> setInterProcessDofNotPresent;

    //std::cout << "[buildGhostDofMap] call buildGhostInterProcessDofMap() with god rank "<<  this->worldComm().godRank() << std::endl;
    buildGhostInterProcessDofMap(mesh,mapInterProcessDof);

    //std::cout << "[buildGhostDofMap] call buildDofNotPresent() with rank "<<  this->worldComm().rank() << std::endl;
    buildDofNotPresent(mesh,setInterProcessDofNotPresent);

    //std::cout << "[buildGhostDofMap] call buildGlobalProcessToGlobalClusterDofMap() with rank "<<  this->worldComm().rank() << std::endl;
    buildGlobalProcessToGlobalClusterDofMap(mesh,mapInterProcessDof,setInterProcessDofNotPresent);

    //std::cout << "[buildGhostDofMap] call localtoglobalOnCluster() with rank "<<  this->worldComm().rank() << std::endl;
    auto it_elt = mesh.beginElementWithProcessId( this->comm().rank() );
    auto en_elt = mesh.endElementWithProcessId( this->comm().rank() );
    for( ; it_elt != en_elt; ++it_elt )
        {
            size_type elid= it_elt->id();
            for( int i = 0; i < FEType::nLocalDof; ++i )
                {
                    int nc1 = (is_product?nComponents1:1);
                    for( int c1 =0; c1 < nc1; ++c1 )
                        {
                            int ind = FEType::nLocalDof*c1+i;
                            boost::tie( M_locglobOnCluster_indices[elid][ind],
                                        M_locglobOnCluster_signs[elid][ind], boost::tuples::ignore) =
                                localToGlobalOnCluster( elid, i, c1 );
                        }
                }
        }
    //std::cout << "[buildGhostDofMap] finish () with god rank "<< this->worldComm().godRank() << std::endl;

}

    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//

template<typename MeshType, typename FEType, typename PeriodicityType>
void
DofTable<MeshType, FEType, PeriodicityType>::buildGhostInterProcessDofMap( mesh_type& mesh,
                                                                           std::map<size_type,boost::tuple<size_type,size_type> > & mapInterProcessDof)
{
    size_type nbFaceDof = invalid_size_type_value;
    if ( !fe_type::is_modal )
        nbFaceDof = ( face_type::numVertices * fe_type::nDofPerVertex +
                      face_type::numEdges * fe_type::nDofPerEdge +
                      face_type::numFaces * fe_type::nDofPerFace );
    else
        nbFaceDof = face_type::numVertices * fe_type::nDofPerVertex;

    int myRank = this->worldComm().rank();

    //------------------------------------------------------------------------------//

    // faire d abord la construction de la carte : dof interprocesfaces myrank -> (other rank,dof interprocesfaces of this rank)
    //std::map<size_type,boost::tuple<size_type,size_type> > mapInterProcessDof;


    std::vector<int> nbMsgToSend(this->worldComm().size());
    std::fill(nbMsgToSend.begin(),nbMsgToSend.end(),0);

    std::vector< std::map<int,int> > mapMsg(this->worldComm().size());

    // iteration on all interprocessfaces
    auto face_it = mesh.interProcessFaces().first;
    auto face_en = mesh.interProcessFaces().second;
    for ( ; face_it != face_en ; ++face_it)
        {
            element_type eltOnProc;
            element_type eltOffProc;
            uint16_type faceIdInEltOnProc;

            auto const& elt0 = face_it->element0();
            auto const& elt1 = face_it->element1();
            if (elt0.processId()!=myRank)
                {
                    eltOffProc = elt0;
                    eltOnProc=elt1;
                    faceIdInEltOnProc = face_it->pos_second();
                }
            else if (elt1.processId()!=myRank)
                {
                    eltOffProc = elt1;
                    eltOnProc=elt0;
                    faceIdInEltOnProc = face_it->pos_first();
                }
            else std::cout << "\n[buildGhostInterProcessDofMap] PROBLEME1!!!!" << std::endl;

            int IdProcessOfGhost = eltOffProc.processId();
            int idFaceInPartition = face_it->idInPartition(IdProcessOfGhost);

            // for each dof in face
            const int ncdof = is_product?nComponents:1;
            for( int c = 0; c < ncdof; ++c )
                {
                    for ( uint16_type l = 0; l < nbFaceDof; ++l )
                        {
#if 0 // WithPermutation (get locDof with identy configuration)
                            auto nbVertexDofInFace = face_type::numVertices * fe_type::nDofPerVertex;
                            auto nbEdgeDofInFace = face_type::numEdges * fe_type::nDofPerEdge;

                            if (eltOnProc.edgePermutation(faceIdInEltOnProc).value()!=1){
                                //if (dataToRecv[1] < nbVertexDofInFace)
                                locDof= nbVertexDofInFace -1 -l;}
#else
                            int locDof=l;
#endif

                            // get dof local on Proc
                            auto thedof = faceLocalToGlobal(face_it->id(),locDof,c);
                            // save the tag of mpi send
                            mapMsg[IdProcessOfGhost].insert(std::make_pair(nbMsgToSend[IdProcessOfGhost],thedof.get<0>()) );

                            // get info to send
                            //boost::tuple<int,int,int> dataToSend = boost::make_tuple(idFaceInPartition,l,c);
                            ublas::vector<float/*int*/> dataToSend(4+nDim/*6*/);
                            dataToSend[0]=idFaceInPartition;
                            dataToSend[1]=l;
                            dataToSend[2]=c;
                            dataToSend[3]=eltOnProc.facePermutation(faceIdInEltOnProc).value();
                            dataToSend[4]=dofPoint(thedof.get<0>()).get<0>()[0];
                            if (nDim>1)
                                dataToSend[5]=dofPoint(thedof.get<0>()).get<0>()[1];
                            if (nDim>2)
                                dataToSend[6]=dofPoint(thedof.get<0>()).get<0>()[2];

                            // send
                            this->worldComm().send(IdProcessOfGhost , nbMsgToSend[IdProcessOfGhost], dataToSend);

#if 0
                            std::cout << "\nI am proc "<<  myRank
                                      << " I send to proc "<< IdProcessOfGhost
                                      << " with tag "<< nbMsgToSend[IdProcessOfGhost]
                                //<< " face_it->id() " << face_it->id()
                                      << " blabla " << thedof.get<0>() << " " << dofPoint(thedof.get<0>())
                                      << " face_it->G() " << face_it->G()
                                      << " with locDof " << locDof
                                      << std::endl;
#endif

                            // update nb send
                            ++nbMsgToSend[IdProcessOfGhost];

                        }
                }


        } // for ( ; face_it != face_en ; ++face_it)

    //--------------------------------------------------------------------------------------------------------//
    // counter of msg received for each process
    std::vector<int> nbMsgToRecv;
    mpi::all_to_all(this->worldComm(),
                    nbMsgToSend,
                    nbMsgToRecv);

    //--------------------------------------------------------------------------------------------------------//
    // recv dof asked and re-send
    for (int proc=0; proc<this->worldComm().size();++proc)
        {
            for ( int cpt=0;cpt<nbMsgToRecv[proc];++cpt)
                {
                    //boost::tuple<int,int,int> dataToRecv;
                    ublas::vector<float/*int*/> dataToRecv(4+nDim);
                    //recv
                    this->worldComm().recv( proc, cpt, dataToRecv);

                    auto const& elt0 = mesh.face(dataToRecv[0]).element0();
                    auto const& elt1 = mesh.face(dataToRecv[0]).element1();

                    element_type eltOnProc;
                    element_type eltOffProc;
                    uint16_type faceIdInEltOnProc;

                    if (elt0.processId()!=myRank)
                        {
                            eltOffProc = elt0;
                            eltOnProc=elt1;
                            faceIdInEltOnProc = face_it->pos_second();
                        }
                    else if (elt1.processId()!=myRank)
                        {
                            eltOffProc = elt1;
                            eltOnProc=elt0;
                            faceIdInEltOnProc = face_it->pos_first();
                        }
                    else std::cout << "\nPROBLEME2!!!!"
                                   << " elt0.processId() " << elt0.processId()
                                   << " elt1.processId() " << elt1.processId()
                                   <<std::endl;

#if 0 // WithPermutation (get locDof with identy configuration)
                    // only work in 2D with fe_type::nodal!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    // les permutations!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    int locDof=dataToRecv[1];
                    //if ( dataToRecv[3]!=eltOnProc.edgePermutation(faceIdInEltOnProc).value() )
                    if ( dataToRecv[3]==eltOnProc.facePermutation(faceIdInEltOnProc).value() && dataToRecv[3]==1 )
                    //if ( theotherelt.edgePermutation(face_idInOther).value()==1 )
                        {
                            auto nbVertexDofInFace = face_type::numVertices * fe_type::nDofPerVertex;
                            auto nbEdgeDofInFace = face_type::numEdges * fe_type::nDofPerEdge;

                            //if (dataToRecv[1] < nbVertexDofInFace)
                            locDof= nbVertexDofInFace -1 -dataToRecv[1];
                                //else if ( dataToRecv[1] < (nbVertexDofInFace+nbEdgeDofInFace) )
                                //locDof= nbVertexDofInFace + nbEdgeDofInFace - 1 - dataToRecv[1];
                        }
#else
                    int locDof;

                    //const int ncdof = is_product?nComponents:1;
                    //for( int c = 0; c < ncdof; ++c )
                    bool find=false;
                    for ( uint16_type l = 0; (l < nbFaceDof && !find) ; ++l )
                        //while (!find && l<nbFaceDof )
                        {
                            auto thedofInFace = faceLocalToGlobal(dataToRecv[0], l, dataToRecv[2]).get<0>();

                            if (nDim==1)
                                {
                                    if (std::abs(dofPoint(thedofInFace).get<0>()[0]-dataToRecv[4])<1e-5)
                                        {
                                            locDof = l;
                                            find=true;
                                        }
                                }
                            else if (nDim==2)
                                {
                                    if ( (std::abs(dofPoint(thedofInFace).get<0>()[0]-dataToRecv[4])<1e-5) &&
                                         (std::abs(dofPoint(thedofInFace).get<0>()[1]-dataToRecv[5])<1e-5) )
                                        {
                                            locDof = l;
                                            find=true;
                                        }
                                }
                            else if (nDim==3)
                                {
                                    if ( (std::abs(dofPoint(thedofInFace).get<0>()[0]-dataToRecv[4])<1e-5) &&
                                         (std::abs(dofPoint(thedofInFace).get<0>()[1]-dataToRecv[5])<1e-5) &&
                                         (std::abs(dofPoint(thedofInFace).get<0>()[2]-dataToRecv[6])<1e-5) )
                                        {
                                            locDof = l;
                                            find=true;
                                        }

                                }
                        }
                    if (!find) std::cout<<"\n PROBLEME not find " << std::endl;

#endif
                    auto thedof = faceLocalToGlobal(dataToRecv[0], locDof, dataToRecv[2]);

                    int dofGlobAsked = thedof.get<0>();
#if 0
                    std::cout << "\nI am proc "<<  myRank
                              << " I recv to proc "<< proc
                              << " with tag "<< cpt
                              << " blabla " << dofPoint(thedof.get<0>())
                        //<< " mapMsg " << mapMsg[proc][cpt]
                              << " dofGlobAsked " << dofGlobAsked
                        //<< " face_it->id() " << face_it->id()
                              << " face_it->G() " << mesh.face(dataToRecv[0]).G()
                              << " permutation recu "<< dataToRecv[3]
                              << "ma permutation " <<eltOnProc.facePermutation(faceIdInEltOnProc).value()
                              << std::endl;
#endif
                    // response
                    this->worldComm().send( proc, cpt, dofGlobAsked );

                }
        }

    //--------------------------------------------------------------------------------------------------------//
    // get response to initial request and update Feel::Mesh::Faces data
    for (int proc=0; proc<this->worldComm().size();++proc)
        {
            for ( int cpt=0;cpt<nbMsgToSend[proc];++cpt)
                {
                    int dofGlobRecv;
                    //recv
                    this->worldComm().recv( proc, cpt, dofGlobRecv);
#if 0
                    std::cout<< "I am the proc " << this->worldComm().rank()<<" I receive to proc " << proc
                             <<" with tag "<< cpt
                             << " idFacesRecv " << idFacesRecv[0] << " " << idFacesRecv[1] << " "<< idFacesRecv[2]
                             << std::endl;
#endif
                    //update data
                    if ( mapInterProcessDof.find(mapMsg[proc][cpt]) == mapInterProcessDof.end() )
                        mapInterProcessDof.insert(std::make_pair(mapMsg[proc][cpt], boost::make_tuple(proc,dofGlobRecv) ));
                    else
                        {
                            if ( mapInterProcessDof.find(mapMsg[proc][cpt])->second.get<0>() > proc )
                                { std::cout << "\n HJHJHJHJH"<<std::endl;
                                    mapInterProcessDof.insert(std::make_pair(mapMsg[proc][cpt], boost::make_tuple(proc,dofGlobRecv) ));
                                }
                        }
#if 0
                    std::cout << "I am the proc " << this->worldComm().rank() << " add mapInterProcessDof "
                              << mapMsg[proc][cpt] << " "
                              << proc << " "
                              << dofGlobRecv << std::endl;
#endif
                }
        }


    //--------------------------------------------------------------------------------------------------------//
    // check
#if 0
    std::vector<int> nbMsgToSendCheck(this->worldComm().size());
    std::fill(nbMsgToSendCheck.begin(),nbMsgToSendCheck.end(),0);
    //std::vector< std::map<int,int> > mapMsg(this->worldComm().size());

    auto nbVertexDofInFace = face_type::numVertices * fe_type::nDofPerVertex;
    auto nbEdgeDofInFace = face_type::numEdges * fe_type::nDofPerEdge;

    face_it = mesh.interProcessFaces().first;
    face_en = mesh.interProcessFaces().second;
    for ( ; face_it != face_en ; ++face_it)
        {

            auto const& elt0 = face_it->element0();
            auto const& elt1 = face_it->element1();

            element_type eltOnProc;
            element_type eltOffProc;
            uint16_type faceIdInEltOnProc;
            if (elt0.processId()!=myRank)
                {
                    eltOffProc = elt0;
                    eltOnProc = elt1;
                    faceIdInEltOnProc = face_it->pos_second();
                }
            else if (elt1.processId()!=myRank)
                {
                    eltOffProc = elt1;
                    eltOnProc = elt0;
                    faceIdInEltOnProc = face_it->pos_first();
                }
            else std::cout << "\nPROBLEME3!!!!"<<std::endl;

            //int IdProcessOfGhost = eltOffProc.processId();

            const int ncdof = is_product?nComponents:1;
            for( int c = 0; c < ncdof; ++c )
                {
                    for ( uint16_type l = 0; l < nbFaceDof; ++l )
                        {
                            int locDof=l;
#if 0
                            if (eltOnProc.facePermutation(faceIdInEltOnProc).value()!=1){
                                //if (dataToRecv[1] < nbVertexDofInFace)
                                locDof= nbVertexDofInFace -1 -l;}
#endif
                            auto thedof = faceLocalToGlobal(face_it->id(),locDof,c);

                            int IdProcessOfGhost = mapInterProcessDof[thedof.get<0>()].get<0>();

                            //boost::tuple<int,int,int> dataToSend = boost::make_tuple(idFaceInPartition,l,c);
                            ublas::vector<float> dataToSendCheck(3+nDim/*5*/);
                            dataToSendCheck[0]=mapInterProcessDof[thedof.get<0>()].get<1>();
                            dataToSendCheck[1]=dofPoint(thedof.get<0>()).get<0>()[0];
                            if (nDim==1)
                                {
                                    dataToSendCheck[2]=eltOnProc.facePermutation(faceIdInEltOnProc).value();
                                    dataToSendCheck[3]=face_it->idInPartition(IdProcessOfGhost);
                                }
                            else if (nDim==2)
                                {
                                    dataToSendCheck[2]=dofPoint(thedof.get<0>()).get<0>()[1];
                                    dataToSendCheck[3]=eltOnProc.facePermutation(faceIdInEltOnProc).value();
                                    dataToSendCheck[4]=face_it->idInPartition(IdProcessOfGhost);
                                }
                            else if (nDim==3)
                                {
                                    dataToSendCheck[2]=dofPoint(thedof.get<0>()).get<0>()[1];
                                    dataToSendCheck[3]=dofPoint(thedof.get<0>()).get<0>()[2];
                                    dataToSendCheck[4]=eltOnProc.facePermutation(faceIdInEltOnProc).value();
                                    dataToSendCheck[5]=face_it->idInPartition(IdProcessOfGhost);

                                }
                            // send
                            this->worldComm().send(IdProcessOfGhost , nbMsgToSendCheck[IdProcessOfGhost], dataToSendCheck);

#if 0
                    std::cout << "\nI am proc "<<  myRank
                              << " I send to proc "<< IdProcessOfGhost
                              << " with tag "<< nbMsgToSendCheck[IdProcessOfGhost]
                              << " dofGlobAsked " <<  thedof.get<0>() << " " << dofPoint( thedof.get<0>()).get<0>()
                        //<< "ma permutation " <<theotherelt.edgePermutation(face_idInOther).value()
                              << std::endl;
#endif


                            // update nb send
                            ++nbMsgToSendCheck[IdProcessOfGhost];
                        }
                }
        } // for ( ; face_it != face_en ; ++face_it)

    // counter of msg received for each process
    std::vector<int> nbMsgToRecvCheck;
    mpi::all_to_all(this->worldComm(),
                    nbMsgToSendCheck,
                    nbMsgToRecvCheck);

    // recv dof asked and re-send
    for (int proc=0; proc<this->worldComm().size();++proc)
        {
            for ( int cpt=0;cpt<nbMsgToRecvCheck[proc];++cpt)
                {
                    //boost::tuple<int,int,int> dataToRecv;
                    ublas::vector<float> dataToRecv(3+nDim);
                    //recv
                    this->worldComm().recv( proc, cpt, dataToRecv);

                    auto const& elt0 = mesh.face(dataToRecv[4]).element0();
                    auto const& elt1 = mesh.face(dataToRecv[4]).element1();

                    element_type eltOnProc;
                    element_type eltOffProc;
                    uint16_type faceIdInEltOnProc;

                    if (elt0.processId()!=myRank)
                        {
                            eltOffProc = elt0;
                            eltOnProc=elt1;
                            faceIdInEltOnProc = face_it->pos_second();
                        }
                    else if (elt1.processId()!=myRank)
                        {
                            eltOffProc = elt1;
                            eltOnProc=elt0;
                            faceIdInEltOnProc = face_it->pos_first();
                        }
                    else std::cout << "\nPROBLEME4!!!!"<<std::endl;


#if 0
                    auto ptDof = dofPoint((int)dataToRecv[0]).get<0>();
                    if (nDim ==2)
                        {
                            if ( (std::abs(ptDof[0]-dataToRecv[1])> 1e-4) ||
                                 (std::abs(ptDof[1]-dataToRecv[2])> 1e-4 ) )
                                {
                                    std::cout << "\nPPRRROBLEME "
                                              << "\nI am proc "<<  myRank
                                              << " I recv to proc "<< proc
                                              << " with tag "<< cpt
                                              << " dofGlobAsked " <<  dataToRecv[0] << " " << dofPoint((int)dataToRecv[0]).get<0>()
                                              << " dof X " << dataToRecv[1]
                                              << " dof Y "<< dataToRecv[2]
                                              << " ma permutation " << eltOnProc.facePermutation(faceIdInEltOnProc).value()
                                              << " other perm " << dataToRecv[3]
                                              << std::endl;
                                }
                            else if (nDim==3)
                                {
                                    if ( (std::abs(ptDof[0]-dataToRecv[1])> 1e-4) ||
                                         (std::abs(ptDof[1]-dataToRecv[2])> 1e-4) ||
                                         (std::abs(ptDof[2]-dataToRecv[3])> 1e-4 ) )
                                        {
                                            std::cout << "\nPPRRROBLEME "
                                                      << "\nI am proc "<<  myRank
                                                      << " I recv to proc "<< proc
                                                      << " with tag "<< cpt
                                                      << " dofGlobAsked " <<  dataToRecv[0] << " " << dofPoint((int)dataToRecv[0]).get<0>()
                                                      << " dof X " << dataToRecv[1]
                                                      << " dof Y "<< dataToRecv[2]
                                                      << " ma permutation " << eltOnProc.facePermutation(faceIdInEltOnProc).value()
                                                      << " other perm " << dataToRecv[3]
                                                      << std::endl;
                                        }

                                }

                        }
#endif

                }
        }
#endif
}


    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//

template<typename MeshType, typename FEType, typename PeriodicityType>
void
DofTable<MeshType, FEType, PeriodicityType>::buildDofNotPresent( mesh_type& mesh,
                                                                 std::set<int> & setInterProcessDofNotPresent )

{
    //calcul nbre dof locaux sans ghost

    int myRank = this->worldComm().rank();
    size_type nbFaceDof = invalid_size_type_value;
    if ( !fe_type::is_modal )
        nbFaceDof = ( face_type::numVertices * fe_type::nDofPerVertex +
                      face_type::numEdges * fe_type::nDofPerEdge +
                      face_type::numFaces * fe_type::nDofPerFace );
    else
        nbFaceDof = face_type::numVertices * fe_type::nDofPerVertex;


    std::vector< std::map<int,int> > mapGhost;
    std::set< int > setInterProcessDof;
    //std::set< int > setOfInterProcessDofNotPresent;
    std::set< int > setInterProcessDofPresent;

    auto face_it = mesh.interProcessFaces().first;
    auto face_en = mesh.interProcessFaces().second;
    for ( ; face_it != face_en ; ++face_it)
        {
            auto const& elt0 = face_it->element0();
            auto const& elt1 = face_it->element1();

            element_type theelt;
            if (elt0.processId() < myRank)
                theelt = elt0;
            else if (elt1.processId() < myRank)
                theelt = elt1;
            else if (elt0.processId() != myRank)
                theelt = elt0;
            else if (elt1.processId() != myRank)
                theelt = elt1;
            else std::cout << "\nPROBLEME5!!!!"<<std::endl;


            const int ncdof = is_product?nComponents:1;
            for( int c = 0; c < ncdof; ++c )
                {
                    for ( uint16_type l = 0; l < nbFaceDof; ++l )
                        {
                            auto blabla = faceLocalToGlobal(face_it->id(),l , c);
#if 0
                            std::cout <<  "\n je suis proc "<<  myRank <<" face_it->id() " << face_it->id()
                                      << " face_it->G() " << face_it->G()
                                      << " blabla " << blabla << std::endl;
#endif
                            setInterProcessDof.insert(blabla.get<0>());

                            if (theelt.processId()<myRank)
                                setInterProcessDofNotPresent.insert(blabla.get<0>());
                            //mapGhost[theelet.processId()].insert(blabla.get<0>(), );
                        }
                }
        } // for ( ; face_it != face_en ; ++face_it)



    int nGlobalDofInProcess = setInterProcessDof.size() - setInterProcessDofNotPresent.size();

    auto dof_it = setInterProcessDof.begin();
    auto dof_en = setInterProcessDof.end();
    for ( ; dof_it!=dof_en ; ++dof_it)
        {
            if ( setInterProcessDofNotPresent.find(*dof_it)==setInterProcessDofNotPresent.end())
                setInterProcessDofPresent.insert(*dof_it);
        }
#if 0
    std::cout <<  "\n je suis proc "<<  myRank
              <<" nbDof interprocess " << setInterProcessDof.size()
              <<" nbDof interprocess not present " << setInterProcessDofNotPresent.size()
              <<" nbDof interprocess present " << setInterProcessDofPresent.size()
              << std::endl;
#endif

    size_type mynDofWithoutGhost = this->_M_last_df[myRank] - this->_M_first_df[myRank] + 1 - setInterProcessDofNotPresent.size();

    mpi::all_gather( this->worldComm(),
                     mynDofWithoutGhost,
                     this->_M_n_localWithoutGhost_df );

    this->_M_n_dofs=0;
    for (int proc=0;proc<this->worldComm().size();++proc) this->_M_n_dofs+=this->_M_n_localWithoutGhost_df[proc];

    this->_M_first_df_globalcluster=this->_M_first_df;
    this->_M_last_df_globalcluster=this->_M_last_df;
    for( int i=1;i<this->worldComm().size();++i)
        {
            this->_M_first_df_globalcluster[i]=this->_M_last_df_globalcluster[i-1]+1;
            //this->_M_last_df_globalcluster[i]+=this->_M_n_localWithoutGhost_df[i];
            this->_M_last_df_globalcluster[i]=this->_M_first_df_globalcluster[i]+this->_M_n_localWithoutGhost_df[i]-1;
        }


}

    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//

template<typename MeshType, typename FEType, typename PeriodicityType>
void
DofTable<MeshType, FEType, PeriodicityType>::buildGlobalProcessToGlobalClusterDofMap( mesh_type& mesh,
                                                                                      std::map<size_type,boost::tuple<size_type,size_type> > const& mapInterProcessDof,
                                                                                      std::set<int> const& setInterProcessDofNotPresent)
{
    //NEWBARRIER this->worldComm().barrier();
    int myRank = this->worldComm().rank();
    size_type nbFaceDof = invalid_size_type_value;
    if ( !fe_type::is_modal )
        nbFaceDof = ( face_type::numVertices * fe_type::nDofPerVertex +
                      face_type::numEdges * fe_type::nDofPerEdge +
                      face_type::numFaces * fe_type::nDofPerFace );
    else
        nbFaceDof = face_type::numVertices * fe_type::nDofPerVertex;

    //------------------------------------------------------------------------------//
    //construction des map local proc to global : part dof present
    //std::map<size_type,size_type> mapGlobalProcessToGlobalCluster;
    //std::vector<size_type> mapGlobalProcessToGlobalCluster(this->_M_n_localWithGhost_df[myRank]);
    this->M_mapGlobalProcessToGlobalCluster.resize(this->_M_n_localWithGhost_df[myRank]);
    this->M_mapGlobalClusterToGlobalProcess.resize(this->_M_n_localWithoutGhost_df[myRank]);

    std::fill( this->M_mapGlobalProcessToGlobalCluster.begin(),this->M_mapGlobalProcessToGlobalCluster.end(),invalid_size_type_value);
    std::fill( this->M_mapGlobalClusterToGlobalProcess.begin(),this->M_mapGlobalClusterToGlobalProcess.end(),invalid_size_type_value);

    size_type firstGlobIndex = this->_M_first_df_globalcluster[myRank];
    size_type nextGlobIndex = firstGlobIndex;

#if 0
    // je pense que ce serais mieux de parcourir dans l'ordre de pDof (mapGDof_it->second)
    // idee : et si je faisait plus for ( int=0;i<nDof;++i) : normalement les dof sont contigue!!
    auto mapGDof_it = mapGDof().begin();
    auto mapGDof_en = mapGDof().end();
    for ( ; mapGDof_it!=mapGDof_en; ++mapGDof_it)
        {
            //std::cout << "\n rank " << myRank << " mapGDof_it " << mapGDof_it->first << " second " << mapGDof_it->second << std::endl;

            if (setInterProcessDofNotPresent.find(mapGDof_it->second)==setInterProcessDofNotPresent.end())
                {
                    this->M_mapGlobalProcessToGlobalCluster[mapGDof_it->second]=nextGlobIndex;
                    this->M_mapGlobalClusterToGlobalProcess[nextGlobIndex]=mapGDof_it->second;
                    ++nextGlobIndex;
                }
            else
                {
                    //mapGlobalProcessToGlobalCluster[mapGDof_it->second]=mapInterProcessDof[ ]
                }
        } // for ( ; mapGDof_it!=mapGDof_en; ++mapGDof_it)
#else
    for ( int i=0; i<this->_M_n_localWithGhost_df[myRank]; ++i)
        {
            //std::cout << "\n rank " << myRank << " mapGDof_it " << mapGDof_it->first << " second " << mapGDof_it->second << std::endl;

            if (setInterProcessDofNotPresent.find(i)==setInterProcessDofNotPresent.end())
                {
                    this->M_mapGlobalProcessToGlobalCluster[i]=nextGlobIndex;
                    this->M_mapGlobalClusterToGlobalProcess[nextGlobIndex-firstGlobIndex]=i;
                    ++nextGlobIndex;
                }
            else
                {
                    //mapGlobalProcessToGlobalCluster[mapGDof_it->second]=mapInterProcessDof[ ]
                }
        } // for ( ; mapGDof_it!=mapGDof_en; ++mapGDof_it)

#endif

    //------------------------------------------------------------------------------//
    //update des interprocessdof non present

#if 1
    // this for is important to build properly the interprocess dofs
    for ( int p = 1 ; p<this->comm().size() ; ++p )
        this->updateGhostGlobalDof(mapInterProcessDof,
                                   setInterProcessDofNotPresent,
                                   p);

#else

    std::vector<int> nbMsgToSend2(this->worldComm().size());
    std::fill(nbMsgToSend2.begin(),nbMsgToSend2.end(),0);

    std::vector< std::map<int,int> > mapMsg2(this->worldComm().size());

    auto dofNP_it = setInterProcessDofNotPresent.begin();
    auto dofNP_en = setInterProcessDofNotPresent.end();
    for ( ; dofNP_it!=dofNP_en ; ++dofNP_it)
        {
            int IdProcessOfGhost = mapInterProcessDof[*dofNP_it].get<0>();
            int dofFaceInPartition = mapInterProcessDof[*dofNP_it].get<1>();

            this->worldComm().send(IdProcessOfGhost , nbMsgToSend2[IdProcessOfGhost], dofFaceInPartition);

            mapMsg2[IdProcessOfGhost].insert(std::make_pair(nbMsgToSend2[IdProcessOfGhost],*dofNP_it) );

            ++nbMsgToSend2[IdProcessOfGhost];
        }

    // counter of msg received for each process
    std::vector<int> nbMsgToRecv2;
    mpi::all_to_all(this->worldComm(),
                    nbMsgToSend2,
                    nbMsgToRecv2);

    //NEWBARRIER this->worldComm().barrier();

    // recv id asked and re-send set of face id
    for (int proc=0; proc<this->worldComm().size();++proc)
        {
            for ( int cpt=0;cpt<nbMsgToRecv2[proc];++cpt)
                {
                    int dofRecv;
                    //recv
                    this->worldComm().recv( proc, cpt, dofRecv);

                    // response // attention pas tout le temps exact : A mediter vincent (vrai pour 2 proc sur!)
                    // je pense qu'il faut faire une boucle tant que pas tout est OK
                    int dofToSend= this->M_mapGlobalProcessToGlobalCluster[dofRecv];
                    if (dofToSend==invalid_size_type_value) std::cout << "\n AIEIAIIE rank "<< this->worldComm().rank() << "recv du proc " << proc <<  std::endl;
                    this->worldComm().send( proc, cpt,dofToSend );
                }
        }

    // get response to initial request and update Feel::Mesh::Faces data
    for (int proc=0; proc<this->worldComm().size();++proc)
        {
            for ( int cpt=0;cpt<nbMsgToSend2[proc];++cpt)
                {
                    int dofGlobClusterRecv;
                    //recv
                    this->worldComm().recv( proc, cpt, dofGlobClusterRecv);
                    //update data

                    this->M_mapGlobalProcessToGlobalCluster[mapMsg2[proc][cpt]]=dofGlobClusterRecv;
                }
        }
#endif
    //------------------------------//
    // check

    for ( int i=0; i<this->_M_n_localWithGhost_df[myRank]; ++i)
        {

        }


}

    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//
    //--------------------------------------------------------------------------------------------------------//

template<typename MeshType, typename FEType, typename PeriodicityType>
void
DofTable<MeshType, FEType, PeriodicityType>::updateGhostGlobalDof(std::map<size_type,boost::tuple<size_type,size_type> > const& mapInterProcessDof,
                                                                  std::set<int> const& setInterProcessDofNotPresent,
                                                                  int procToUpdate)
{
    std::vector<int> nbMsgToSend2(this->worldComm().size());
    std::fill(nbMsgToSend2.begin(),nbMsgToSend2.end(),0);

    std::vector< std::map<int,int> > mapMsg2(this->worldComm().size());

    // send msg only on proc
    if (procToUpdate == this->comm().rank() )
        {
            auto dofNP_it = setInterProcessDofNotPresent.begin();
            auto dofNP_en = setInterProcessDofNotPresent.end();
            for ( ; dofNP_it!=dofNP_en ; ++dofNP_it)
                {
                    int IdProcessOfGhost = mapInterProcessDof.find(*dofNP_it)->second.get<0>();
                    int dofFaceInPartition = mapInterProcessDof.find(*dofNP_it)->second.get<1>();

                    this->worldComm().send(IdProcessOfGhost , nbMsgToSend2[IdProcessOfGhost], dofFaceInPartition);

                    mapMsg2[IdProcessOfGhost].insert(std::make_pair(nbMsgToSend2[IdProcessOfGhost],*dofNP_it) );

                    ++nbMsgToSend2[IdProcessOfGhost];
                }
        }
    // counter of msg received for each process
    std::vector<int> nbMsgToRecv2;
    mpi::all_to_all(this->worldComm(),
                    nbMsgToSend2,
                    nbMsgToRecv2);

    //NEWBARRIER this->worldComm().barrier();

    // recv id asked and re-send set of face id
    for (int proc=0; proc<this->worldComm().size();++proc)
        {
            for ( int cpt=0;cpt<nbMsgToRecv2[proc];++cpt)
                {
                    int dofRecv;
                    //recv
                    this->worldComm().recv( proc, cpt, dofRecv);

                    // response // attention pas tout le temps exact : A mediter vincent (vrai pour 2 proc sur!)
                    // je pense qu'il faut faire une boucle tant que pas tout est OK
                    int dofToSend= this->M_mapGlobalProcessToGlobalCluster[dofRecv];
                    if (dofToSend==-1) std::cout << "\n AIEIAIIE rank "<< this->worldComm().rank() << "recv du proc " << proc <<  std::endl;
                    if (proc != procToUpdate ) std::cout << "\n[updateGhostGlobalDof] IL Y A UN SOUCIS !!!";
                    this->worldComm().send( proc, cpt,dofToSend );
                }
        }

    // get response to initial request and update Feel::Mesh::Faces data
    size_type firstGlobIndex = this->_M_first_df_globalcluster[this->comm().rank()];
    for (int proc=0; proc<this->worldComm().size();++proc)
        {
            for ( int cpt=0;cpt<nbMsgToSend2[proc];++cpt)
                {
                    int dofGlobClusterRecv;
                    //recv
                    this->worldComm().recv( proc, cpt, dofGlobClusterRecv);
                    //update data

                    this->M_mapGlobalProcessToGlobalCluster[mapMsg2[proc][cpt]]=dofGlobClusterRecv;
                    //this->M_mapGlobalClusterToGlobalProcess[dofGlobClusterRecv-firstGlobIndex]=mapMsg2[proc][cpt];
                }
        }

}

//--------------------------------------------------------------------------------------------------------//



} // namespace Feel