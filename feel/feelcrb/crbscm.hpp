/* -*- mode: c++ -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
       Date: 2009-08-07

  Copyright (C) 2009 Universit� Joseph Fourier (Grenoble I)

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
   \file opusscm.hpp
   \author Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
   \date 2009-08-07
 */
#ifndef __CRBSCM_H
#define __CRBSCM_H 1

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/timer.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <feel/feelcore/feel.hpp>
#include <feel/feelcore/parameter.hpp>
#include <feel/feelalg/solvereigen.hpp>
#include <feel/feelcrb/parameterspace.hpp>
#include <feel/feelcrb/crbdb.hpp>
#include <feel/feelcore/serialization.hpp>
#include <glpk.h>

namespace Feel
{
/**
 * \class CRBSCM
 * \brief SCM algorithm
 * \anchor crbscm
 *
 * \tparam TruthModelType the type of the class that describes the truth (e.g. fem) model problem
 *
 * TruthModelType must support a certain interface: affine decomposition, solve
 * for fm problem and problem associated with with affine decomposition. As to
 * the SCM, it should problem
 *  - eigensolves for the full Truth problem
 *  - eigensolves associated with the affine decomposition
 *
 * @author Christophe Prud'homme
 * @see page \ref scm
 */
template<typename TruthModelType>
class CRBSCM : public CRBDB
{
    typedef CRBDB super;
public:


    /** @name Constants
     */
    //@{


    //@}

    /** @name Typedefs
     */
    //@{

    typedef TruthModelType truth_model_type;
    typedef truth_model_type model_type;
    typedef boost::shared_ptr<truth_model_type> truth_model_ptrtype;

    typedef double value_type;
    typedef boost::tuple<double,double> bounds_type;

    typedef ParameterSpace<TruthModelType::ParameterSpaceDimension> parameterspace_type;
    typedef boost::shared_ptr<parameterspace_type> parameterspace_ptrtype;
    typedef typename parameterspace_type::element_type parameter_type;
    typedef typename parameterspace_type::element_ptrtype parameter_ptrtype;
    typedef typename parameterspace_type::sampling_type sampling_type;
    typedef typename parameterspace_type::sampling_ptrtype sampling_ptrtype;

    typedef boost::tuple<double, parameter_type, size_type, double, double> relative_error_type;

    typedef typename model_type::backend_type backend_type;
    typedef typename model_type::sparse_matrix_ptrtype sparse_matrix_ptrtype;
    typedef typename model_type::vector_ptrtype vector_ptrtype;
    typedef typename model_type::theta_vector_type theta_vector_type;


    typedef Eigen::VectorXd y_type;
    typedef std::vector<y_type> y_set_type;
    typedef std::vector<boost::tuple<double,double> > y_bounds_type;
    //@}

    /** @name Constructors, destructor
     */
    //@{

    //! default constructor
    CRBSCM()
        :
        super( "noname" ),
        M_is_initialized( false ),
        M_model(),
        M_tolerance( 1e-2),
        M_iter_max( 3 ),
        M_Mplus( 10 ),
        M_Malpha( 4 ),
        M_Dmu( new parameterspace_type ),
        M_Xi( M_Dmu ),
        M_C( M_Dmu, 1, M_Xi ),
        M_C_complement(M_Dmu, 1, M_Xi )
        {
        }

    //! constructor from command line options
    CRBSCM( std::string const& name,
            po::variables_map const& vm )
        :
        super( "scm",
               (boost::format( "%1%" ) % name ).str(),
               (boost::format( "%1%" ) % name ).str(),
               vm ),
        M_is_initialized( false ),
        M_model(),
        M_tolerance( vm["crb.scm.tol"].template as<double>() ),
        M_iter_max( vm["crb.scm.iter-max"].template as<int>() ),
        M_Mplus( vm["crb.scm.Mplus"].template as<int>() ),
        M_Malpha( vm["crb.scm.Malpha"].template as<int>()  ),
        M_Dmu( new parameterspace_type ),
        M_Xi( new sampling_type( M_Dmu ) ),
        M_C( new sampling_type( M_Dmu, 1, M_Xi ) ),
        M_C_complement(new sampling_type( M_Dmu, 1, M_Xi ) ),
        M_vm( vm )
        {
            if ( this->loadDB() )
                std::cout << "Database " << this->lookForDB() << " available and loaded\n";
        }

    //! copy constructor
    CRBSCM( CRBSCM const & o )
        :
        super( o ),
        M_is_initialized( o.M_is_initialized ),
        M_tolerance( o.M_tolerance ),
        M_iter_max( o.M_iter_max ),
        M_Mplus( o.M_Mplus ),
        M_Malpha( o.M_Malpha ),
        M_Dmu( o.M_Dmu ),
        M_Xi( o.M_Xi ),
        M_C( o.M_C ),
        M_C_complement( o.M_C_complement ),
        M_vm( o.M_vm )
        {
        }

    //! destructor
    ~CRBSCM()
        {}

    //@}

    /** @name Operator overloads
     */
    //@{

    //! copy operator
    CRBSCM& operator=( CRBSCM const & o)
        {
            if (this != &o )
            {
            }
            return *this;
        }
    //@}

    /** @name Accessors
     */
    //@{

    //! Returns maximum value of K
    size_type KMax() const { return M_C->size(); }

    //! return max iterations
    int maxIter() const { return M_iter_max; }

    //! return the parameter space
    parameterspace_ptrtype Dmu() const { return M_Dmu; }

    //! get M+
    int Mplus() const { return M_Mplus; }

    //! get Malpha
    int Malpha() { return M_Malpha; }

    //@}

    /** @name  Mutators
     */
    //@{

    //! set offline tolerance
    void setTolerance( double tolerance ) { M_tolerance = tolerance; }

    //! set M+
    void setMplus( int Mplus ) { M_Mplus = Mplus; }

    //! set Malpha
    void setMalpha( int Malpha ) { M_Malpha = Malpha; }

    //! set the truth offline model
    void setTruthModel( truth_model_ptrtype const& model )
        {
            M_model = model;
            M_Dmu = M_model->parameterSpace();
            M_Xi = sampling_ptrtype( new sampling_type( M_Dmu ) );
            M_C = sampling_ptrtype( new sampling_type( M_Dmu ) );
            M_C_complement = sampling_ptrtype( new sampling_type( M_Dmu ) );
        }

    //! set Kmax
    void setMaxIter( int K )
        {
            M_iter_max = K;
        }

    //@}

    /** @name  Methods
     */
    //@{

    /**
     * generate offline the space \f$C_K\f$
     */
    void generate();

    /**
     * check \f$C_K\f$ properties
     * \param K dimension of \f$C_K\f$
     */
    void checkC( size_type K ) const;

    boost::tuple<value_type,value_type> ex( parameter_type const& mu ) const;
    boost::tuple<value_type,value_type> ex( parameter_type const& mu, mpl::bool_<true>) const;
    boost::tuple<value_type,value_type> ex( parameter_type const& mu, mpl::bool_<false> ) const;

    /**
     * Returns the lower bound of the coercive constant given a parameter \p
     * \f$\mu\f$
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the coercive constant
     * \param K the dimension of \f$Y_{\mathrm{UB}}\f$
     *
     *\return compute online the lower bounmd
     */
    boost::tuple<value_type,value_type> lb( parameter_type const& mu, size_type K = invalid_size_type_value, int indexmu = -1 ) const;
    boost::tuple<value_type,value_type> lb( parameter_type const& mu, mpl::bool_<true>,  size_type K = invalid_size_type_value, int indexmu = -1 ) const;
    boost::tuple<value_type,value_type> lb( parameter_type const& mu, mpl::bool_<false>, size_type K = invalid_size_type_value, int indexmu = -1 ) const;
    /**
     * Returns the lower bound of the coercive constant given a parameter \p
     * \f$\mu\f$
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the coercive constant
     * \param K the dimension of \f$Y_{\mathrm{UB}}\f$
     *
     *\return compute online the lower bounmd
     */
    boost::tuple<value_type,value_type> lb( parameter_ptrtype const& mu, size_type K = invalid_size_type_value, int indexmu = -1 ) const
        {
            return lb( *mu, K, indexmu );
        }


    /**
     * Returns the upper bound of the coercive constant given a parameter \p
     * \f$\mu\f$
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the coercive constant
     * \param K the dimension of \f$Y_{\mathrm{UB}}\f$
     *
     *\return compute online the lower bounmd
     */
    boost::tuple<value_type,value_type> ub( parameter_type const& mu, size_type K = invalid_size_type_value ) const;
    boost::tuple<value_type,value_type> ub( parameter_type const& mu, mpl::bool_<true> , size_type K = invalid_size_type_value ) const;
    boost::tuple<value_type,value_type> ub( parameter_type const& mu, mpl::bool_<false>, size_type K = invalid_size_type_value ) const;

    /**
     * Returns the upper bound of the coercive constant given a parameter \p
     * \f$\mu\f$
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the coercive constant
     * \param K the dimension of \f$Y_{\mathrm{UB}}\f$
     *
     *\return compute online the lower bounmd
     */
    boost::tuple<value_type,value_type> ub( parameter_ptrtype const& mu, size_type K = invalid_size_type_value ) const
        {
            return ub( *mu, K );
        }


    /**
     * Offline computation
     */
    std::vector<boost::tuple<double,double,double> > offline();
    std::vector<boost::tuple<double,double,double> > offline( mpl::bool_<true>);
    std::vector<boost::tuple<double,double,double> > offline( mpl::bool_<false>);

    /**
     * Online computation
     */
    bounds_type
    online() const
        {
            return bounds_type();
        }

    /**
     * \brief Retuns maximum value of the relative error
     * \param Xi fine sampling of the parameter space
     */
    relative_error_type maxRelativeError( size_type K ) const;

    /**
     * \brief compute the bounds for \f$y \in \mathbb{R}^{Q_a}\f$
     */
    void computeYBounds();

    /**
     * \param K size of C_K
     * \param mu parameter set
     */
    std::vector<double> run( parameter_type const& mu, int K );

    /**
     * run scm for a set of parameter
     * \param X - input parameter
     * \param N - number of input parameter
     * \param Y - outputs
     * \param P - number of outputs
     *
     * the output is the lower/upper bounds, hence P=2
     */
    void run( const double * X, unsigned long N, double * Y, unsigned long P );

    /**
     * save the CRB database
     */
    void saveDB();

    /**
     * load the CRB database
     */
    bool loadDB();

//@}



protected:

private:
private:
    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const;

    template<class Archive>
    void load(Archive & ar, const unsigned int version) ;

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    private:

    bool M_is_initialized;

    truth_model_ptrtype M_model;

    double M_tolerance;
    int M_iter_max;
    size_type M_Mplus;
    size_type M_Malpha;

    // parameter space
    parameterspace_ptrtype M_Dmu;

    // sampling of parameter space
    sampling_ptrtype M_Xi;

    // sampling of parameter space (subset of M_Xi later on)
    sampling_ptrtype M_C, M_C_complement;

    //! eigenvalues associated with the parameters used to build M_C
    std::map<size_type,value_type> M_C_eigenvalues;
    std::vector<double> M_eig;
    mutable std::map<size_type,std::map<size_type,value_type> > M_C_alpha_lb;

    //! y_set_type \f$\mathcal{U}_{\mathrm{UB}}\f$
    y_set_type M_Y_ub;

    //! bounds for y
    y_bounds_type M_y_bounds;

    po::variables_map M_vm;
};

po::options_description crbSCMOptions( std::string const& prefix = "" );


template<typename TruthModelType>
std::vector<boost::tuple<double,double,double> >
CRBSCM<TruthModelType>::offline()
{
    return offline(  mpl::bool_<model_type::is_time_dependent>() );
}

template<typename TruthModelType>
std::vector<boost::tuple<double,double,double> >
CRBSCM<TruthModelType>::offline(mpl::bool_<true>)
{
    Log()<<"[CRBSCM]offline needs to be implement for time-dependent problems \n";
}

template<typename TruthModelType>
std::vector<boost::tuple<double,double,double> >
CRBSCM<TruthModelType>::offline(mpl::bool_<false>)
{
    std::ofstream os_y( "y.m" );
    std::ofstream os_C( "C.m" );

    std::vector<boost::tuple<double,double,double> > ckconv;
    // do the affine decomposition
    M_model->computeAffineDecomposition();


    // random sampling
    M_Xi->randomize( M_vm["crb.scm.sampling-size"].template as<int>() );
    //M_Xi->logEquidistribute( M_vm["crb.scm.sampling-size"].template as<int>() );
    M_C->setSuperSampling( M_Xi );
    parameter_type mu( M_Dmu );

#if 0
    //Test coercicivty
    //std::cout << "Testing coercivity at sample points\n" ;
    for (int i=0;i<M_Xi->size();i++) {
        double testalpha = ex(M_Xi->at(i));
        //std::cout << "mu[" << i+1 << "]\n" << M_Xi->at(i) << "\nalpha = " << testalpha << "\n" ;
    }
#endif
    double relative_error = 1e30;

    // compute the bounds for y in R^Q
    this->computeYBounds();

    // empty sets
    M_C->clear();
    M_Y_ub.clear();
    M_C_alpha_lb.clear();

    // start with M_C = { arg min mu, mu \in Xi }
    size_type index;
    boost::tie( mu, index ) = M_Xi->min();
    M_C->push_back( mu, index );

    M_C_complement = M_C->complement();
    //std::cout << " -- start with mu = " << mu << "\n";
    //std::cout << " -- C size :  " << M_C->size() << "\n";
    //std::cout << " -- C complement size :  " << M_C_complement->size() << "\n";

    sparse_matrix_ptrtype A,B,symmA;
    std::vector<vector_ptrtype> F;

    // dimension of Y_UB and U_LB
    size_type K = 1;

    std::vector<sparse_matrix_ptrtype> Aq;
    boost::tie( Aq, boost::tuples::ignore ) = M_model->computeAffineDecomposition();

    while ( relative_error > M_tolerance && K <= M_iter_max )
    {
        std::cout << "============================================================\n";
         std::cout << "K=" << K << "\n";

        os_C << M_C->at( K-1 ) << "\n";

        // resize y_ub
        M_Y_ub.resize( K );
        M_Y_ub[K-1].resize( M_model->Qa() );
        M_model->solve( mu );

        // for a given parameter \p mu assemble the left and right hand side
        boost::tie( A, F ) = M_model->update( mu );
        A->printMatlab( "offline_A.m" );
        symmA = M_model->newMatrix();symmA->close();
        A->symmetricPart( symmA );
        symmA->printMatlab( "offline_symmA.m" );
        B = M_model->innerProduct();
        B->printMatlab( "offline_B.m" );

        //
        // Build Y_UB
        //
        int nconv;
        double eigenvalue_real, eigenvalue_imag;
        vector_ptrtype eigenvector;
        // solve  for eigenvalue problem at \p mu
        SolverEigen<double>::eigenmodes_type modes;
        modes=
            eigs( _matrixA=symmA,
                  _matrixB=B,
                  _solver=(EigenSolverType)M_vm["solvereigen-solver-type"].as<int>(),
                  _spectrum=SMALLEST_REAL,
                  //_spectrum=LARGEST_MAGNITUDE,
                  _transform=SINVERT,
                  _ncv=M_vm["solvereigen-ncv"].template as<int>(),
                  _nev=M_vm["solvereigen-nev"].template as<int>(),
                  _tolerance=M_vm["solvereigen-tol"].template as<double>(),
                  _maxit=M_vm["solvereigen-maxiter"].template as<int>()
                );


        if ( modes.empty()  )
        {
            Log() << "eigs failed to converge\n";
            return ckconv;
        }
        std::cout << "[fe eig] mu=" << std::setprecision(4) << mu << "\n"
                  << "[fe eig] eigmin : " << std::setprecision(16) << modes.begin()->second.template get<0>() << "\n"
                  << "[fe eig] ndof:" << M_model->functionSpace()->nDof() << "\n";

        // extract the eigenvector associated with the smallest eigenvalue
        eigenvector = modes.begin()->second.template get<2>();

        // store the  eigenvalue associated with \p mu
        M_C_eigenvalues[index] = modes.begin()->second.template get<0>();
        typedef std::pair<size_type,value_type> key_t;
        BOOST_FOREACH( key_t eig, M_C_eigenvalues )
        {
            std::cout << "[fe eig] stored/map eig=" << eig.second <<" ( " << eig.first << " ) " << "\n";
        }
        M_eig.push_back( M_C_eigenvalues[index] );
        BOOST_FOREACH( value_type eig, M_eig )
        {
            std::cout << "[fe eig] stored/vec eig=" << eig << "\n";
        }
        /*
         * now apply eigenvector to the Aq to compute
         * y( eigenvector ) = ( a_q( eigenvector, eigenvector )/ ||eigenvector||^2
         */
        for( size_type q = 0; q < M_model->Qa(); ++q )
        {
            value_type aqw = Aq[q]->energy( eigenvector, eigenvector );
            value_type bw = B->energy( eigenvector, eigenvector );

            //std::cout << "[scm_offline] q=" << q << " aqw = " << aqw << ", bw = " << bw << "\n";
            M_Y_ub[K-1]( q ) = aqw/bw;
        }
        //checkC( K );
        //std::cout << "[scm_offline] M_Y_ub[" << K-1 << "]=" << M_Y_ub[K-1] << "\n";

        // save Y
        os_y << M_Y_ub[K-1] << "\n";

        double minerr, meanerr;
        boost::tie( relative_error, mu, index, minerr, meanerr ) = maxRelativeError( K );
        std::cout << " -- max relative error = " << relative_error
                  << " at mu = " << mu
                  << " at index = " << index << "\n";
        //ofs << K << " " << std::setprecision(16) << relative_error << std::endl;
        ckconv.push_back( boost::make_tuple( relative_error, minerr, meanerr ) );
        // could be that the max relative error is smaller than the tolerance if
        // the coercivity constant is independant of the parameter set
        if ( relative_error > M_tolerance && K < M_iter_max )
        {
            std::cout << " -- inserting mu in C (" << M_C->size() << ")\n";
            M_C->push_back( mu, index );
            for( int _i =0;_i < M_C->size(); ++_i )
                std::cout << " -- mu [" << _i << "]=" << M_C->at( _i ) << std::endl;

            M_C_complement = M_C->complement();
            for( int _i =0;_i < M_C_complement->size(); ++_i )
            {
                std::cout << " -- mu complement [" << _i << "]=" << M_C_complement->at( _i ) << std::endl;
            }

        }
        ++K;
        std::cout << "============================================================\n";
    }
    saveDB();
    return ckconv;
}
template<typename TruthModelType>
void
CRBSCM<TruthModelType>::checkC( size_type K ) const
{
    y_type err( M_Xi->size() );
    // check that for each mu in C_K the lb and ub are very close
    for( int k = 0; k < K; ++k )
    {
        std::cout << "[checkC] k= " << k << " K=" << K << " index in superSampling: " << M_C->indexInSuperSampling( k ) <<"\n";
        parameter_type const& mu = M_C->at( k );
        size_type index = M_C->indexInSuperSampling( k );
        double _lb = lb( mu, K, index  ).template get<0>();
        double _lblb = lb( mu, std::max(K-1,size_type(0)), index ).template get<0>();
        if ( _lblb - _lb > 1e-10 )
        {
            Log() << "[lberror] the lower bound is decreasing\n"
                  << "[lberror] _lblb = " << _lblb << "\n"
                  << "[lberror] _lb = " << _lb << "\n";
        }
        double _ub = ub( mu, K ).template get<0>();
        double _ex = M_C_eigenvalues.find( index )->second;
        std::cout << "_lb = " << _lb << " _lblb=" << _lblb << " ub = " << _ub << " _ex = " << _ex << "\n";

        err(k) = 1. - _lb/_ub;

        if ( std::abs( err(k) ) > 1e-6 )
        {
            std::ostringstream ostr;
            ostr << "error is not small as it should be " << k <<  " " << err(k) << "\n";
            ostr << "[checkC] relative error : " << mu << "\n"
                 << "[checkC] lb=" << std::setprecision(16) << _lb << "\n"
                 << "[checkC] ub=" << std::setprecision(16) << _ub << "\n"
                 << "[checkC] relerr(" << k << ")=" << std::setprecision(16) << err(k) << "\n"
                 << "[checkC] exact(" << k << ")=" << std::setprecision(16) << _ex << "\n";
            throw std::logic_error( ostr.str() );
        }
#if 1
        std::cout << "[checkC] relative error : " << mu << "\n"
                  << "[checkC] lb=" << std::setprecision(16) << _lb << "\n"
                  << "[checkC] ub=" << std::setprecision(16) << _ub << "\n"
                  << "[checkC] relerr(" << k << ")=" << std::setprecision(16) << err(k) << "\n"
                  << "[checkC] exerr(" << k << ")=" << std::setprecision(16) << _ex << "\n";
#endif
    }
}


template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type,
             typename CRBSCM<TruthModelType>::value_type>
CRBSCM<TruthModelType>::ex( parameter_type const& mu ) const
{
    return ex(mu, mpl::bool_<model_type::is_time_dependent>() );
}

template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type,
             typename CRBSCM<TruthModelType>::value_type>
CRBSCM<TruthModelType>::ex( parameter_type const& mu, mpl::bool_<true> ) const
{
    std::cout<<"[CRBSCM::ex] ERROR  : Need to be implemented for time dependent problems"<<std::endl;
    return boost::make_tuple( 0,0 );
}

template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type,
             typename CRBSCM<TruthModelType>::value_type>
CRBSCM<TruthModelType>::ex( parameter_type const& mu, mpl::bool_<false> ) const
{
    boost::timer ti;
    sparse_matrix_ptrtype D,symmA;
    sparse_matrix_ptrtype M = M_model->innerProduct();
    std::vector<vector_ptrtype> F;
    boost::tie( D, F ) = M_model->update( mu );
    symmA = M_model->newMatrix();symmA->close();
    D->symmetricPart( symmA );

    SolverEigen<double>::eigenmodes_type modesmin=
        eigs( _matrixA=D,
              _matrixB=M,
              _solver=(EigenSolverType)M_vm["solvereigen-solver-type"].as<int>(),
              //_spectrum=LARGEST_MAGNITUDE,
              _spectrum=SMALLEST_REAL,
              _transform=SINVERT,
              _ncv=M_vm["solvereigen-ncv"].as<int>(),
              _nev=M_vm["solvereigen-nev"].as<int>(),
              _tolerance=M_vm["solvereigen-tol"].as<double>(),
              _maxit=M_vm["solvereigen-maxiter"].as<int>()
            );

    if( modesmin.empty() )
    {
        std::cout << "no eigenmode converged: increase --solvereigen-ncv\n";
        return 0.;
    }
    double eigmin = modesmin.begin()->second.get<0>();
    //std::cout << std::setprecision(4) << mu << " eigmin = "
    //          << std::setprecision(16) << eigmin << "\n";

    return boost::make_tuple( eigmin, ti.elapsed() );
#if 0
    SolverEigen<double>::eigenmodes_type modesmax=
        eigs( _matrixA=D,
              _matrixB=M,
              _solver=(EigenSolverType)M_vm["solvereigen-solver-type"].as<int>(),
              _spectrum=LARGEST_MAGNITUDE,
              _ncv=M_vm["solvereigen-ncv"].as<int>(),
              _nev=M_vm["solvereigen-nev"].as<int>(),
              _tolerance=M_vm["solvereigen-tol"].as<double>(),
              _maxit=M_vm["solvereigen-maxiter"].as<int>()
            );
    if( modesmax.empty() )
    {
        std::cout << "no eigenmode converged: increase --solvereigen-ncv\n";
        return;
    }
    double eigmax = modesmax.rbegin()->second.get<0>();

    //std::cout << "[crbmodel] " << std::setprecision(4) << mu << " "
    //          << std::setprecision(16) << eigmin << " " << eigmax
    //          << " " << Xh->nDof() << "\n";

#endif

}



template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type, double>
CRBSCM<TruthModelType>::lb( parameter_type const& mu, size_type K, int indexmu ) const
{
    return lb( mu, mpl::bool_<model_type::is_time_dependent>(), K , indexmu );
}

template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type, double>
CRBSCM<TruthModelType>::lb( parameter_type const& mu, mpl::bool_<true> ,size_type K ,int indexmu) const
{
    std::cout<<"[CRBSCM::lb] need to be implemented in time-dependent case "<<std::endl;
    return boost::make_tuple( 0, 0);
}

template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type, double>
CRBSCM<TruthModelType>::lb( parameter_type const& mu, mpl::bool_<false> ,size_type K, int indexmu ) const
{
    if ( K == invalid_size_type_value ) K = this->KMax();
    if ( K > this->KMax() ) K = this->KMax();
    boost::timer ti;
    // value if K==0
    if ( K <= 0 ) return 0.0;
    if ( indexmu >= 0 && ( M_C_alpha_lb[indexmu].find(K) !=  M_C_alpha_lb[indexmu].end() ) )
        return  M_C_alpha_lb[indexmu][K] ;
    //if ( K == std::max(size_type(0),M_C->size()-(size_type)M_vm["crb.scm.level"].template as<int>() ) ) return 0.0;
    int level = M_vm["crb.scm.level"].template as<int>();

    //std::cout << "[CRBSCM::lb] Alphalb size " << M_C_alpha_lb.size() << "\n";

    theta_vector_type theta_q;
    glp_prob *lp;

    lp = glp_create_prob();
    glp_set_prob_name(lp, (boost::format( "scm_%1%" ) % K).str().c_str() );
    glp_set_obj_dir(lp, GLP_MIN);

    int Malpha = std::min(M_Malpha,std::min(K,M_Xi->size()));
    int Mplus = std::min(M_Mplus,M_Xi->size()-K);
    if (M_vm["crb.scm.strategy"].template as<int>()==2)
        Mplus = std::min(M_Mplus,std::min(K, M_Xi->size()-K ));
    // we have exactely Qa*(M+ + Malpha) entries in the matrix
    int nnz = M_model->Qa()*(Malpha+Mplus);
    int ia[1+1000], ja[1+1000];
    double ar[1+1000];
    int nnz_index = 1;

    // set the auxiliary variables: we have first Malpha of them from C_K and
    // Mplus from its complement
    glp_add_rows(lp,Malpha+Mplus);

    // search the the M_Malpha closest points in C_K, M_Malpha must be < K
    sampling_ptrtype C_neighbors =  M_C->searchNearestNeighbors( mu, Malpha );

    //std::cout << "[CRBSCM::lb] C_neighbors size = " << C_neighbors->size() << "\n";

    //std::cout << "[CRBSCM::lb] add rows associated with C_K\n";
    // first the constraints associated with C_K: make sure that Malpha is not
    // greater by taking the min of M_Malpha and K
    for( int m=0;m < Malpha; ++m )
    {
        //std::cout << "[CRBSCM::lb] add row " << m << " from C_K\n";
        parameter_type mup = C_neighbors->at( m );
        //std::cout << "[CRBSCM::lb] mup = " << mup << "\n";

        // update the theta_q associated with mup
        boost::tie( theta_q, boost::tuples::ignore ) = M_model->computeThetaq( mup );
        //std::cout << "[CRBSCM::lb] thetaq = " << theta_q << "\n";

        //std::cout << "[CRBSCM::lb] row name : " << (boost::format( "c_%1%_%2%" ) % K % m).str() << "\n";
        glp_set_row_name(lp, m+1, (boost::format( "c_%1%_%2%" ) % K % m).str().c_str() );

        //std::cout << "[CRBSCM::lb] row bounds\n";
        //std::cout << "[CRBSCM::lb] index in super sampling: " << C_neighbors->indexInSuperSampling( m ) << "\n";
        //std::cout << "[CRBSCM::lb] eig value: " << M_C_eigenvalues.find( C_neighbors->indexInSuperSampling( m ) )->second << "\n";
        glp_set_row_bnds(lp,
                         m+1,
                         GLP_LO,
                         M_C_eigenvalues.find( C_neighbors->indexInSuperSampling( m ) )->second,
                         0.0);

        //std::cout << "[CRBSCM::lb] constraints matrix\n";
        for( int q = 0; q < M_model->Qa(); ++q, ++nnz_index )
        {
            //std::cout << "[CRBSCM::lb] constraints matrix q = " << q << "\n";
            ia[nnz_index]=m+1;
            ja[nnz_index]=q+1;
            ar[nnz_index]=theta_q( q );
        }
    }
    //std::cout << "[CRBSCM::lb] add rows associated with C_K done. nnz=" << nnz_index << "\n";

    // search the the Mplus closest points in Xi\C_K
    sampling_ptrtype Xi_C_neighbors =  M_C_complement->searchNearestNeighbors( mu, Mplus );

    //std::cout << "[CRBSCM::lb] C_complement size = " << Xi_C_neighbors->size() << "\n";

    //std::cout << "[CRBSCM::lb] add rows associated with Xi\\C_K\n";
    //std::cout << "[CRBSCM::lb] Mplus =" << Mplus << " , neighbors= " << Xi_C_neighbors->size() << "\n";
    // second the monotonicity constraints associated with the complement of C_K
    for( int m=0;m < Mplus; ++m )
        //for( int m=0;m < Xi_C_neighbors->size(); ++m )
    {
        //std::cout << "[CRBSCM::lb] add row " << m << " from Xi\\C_K\n";
        parameter_type mup = Xi_C_neighbors->at( m );

        double _lb;

#if 1
        //std::cout << "[CRBSCM::lb] Entering find alphamap\n" << Xi_C_neighbors->indexInSuperSampling( m ) << std::endl ;
        //std::cout << "[CRBSCM::lb] Size of alphalb " << M_C_alpha_lb.find( Xi_C_neighbors->indexInSuperSampling( m ) )->second.size() << "\n" ;

        if ( M_C_alpha_lb[Xi_C_neighbors->indexInSuperSampling( m ) ].find(K-1) !=  M_C_alpha_lb[Xi_C_neighbors->indexInSuperSampling( m ) ].end() )
        {
            //std::cout << "[CRBSCM::lb] lb d�j� calcul�e\n" ;
        }
        else
        {
            //std::cout << "[CRBSCM::lb] Nouvelle lb\n" ;
            M_C_alpha_lb[  Xi_C_neighbors->indexInSuperSampling( m )][K-1] = lb( mup, K-1,  Xi_C_neighbors->indexInSuperSampling( m ) ).template get<0>();
        }
        _lb = M_C_alpha_lb[ Xi_C_neighbors->indexInSuperSampling( m )][K-1];
#endif

        // update the theta_q associated with mup
        boost::tie( theta_q, boost::tuples::ignore ) = M_model->computeThetaq( mup );

        glp_set_row_name(lp, Malpha+m+1, (boost::format( "xi_c_%1%_%2%" ) % K % m).str().c_str() );

        switch( M_vm["crb.scm.strategy"].template as<int>() )
        {
            // Patera
        case 0:
        {
            //std::cout << "Patera\n" ;
            glp_set_row_bnds(lp, Malpha+m+1, GLP_LO, 0.0, 0.0);
        }
        break;
        // Maday

        case 1:
            // Prud'homme
        case 2:
        {
            //std::cout << "Prud'homme\n" ;
            //    std::cout << "Lb = " << _lb << std::endl ;
            glp_set_row_bnds(lp, Malpha+m+1, GLP_LO, _lb, 0.0);
        }
        break;
        }
        for( int q = 0; q < M_model->Qa(); ++q, ++nnz_index )
        {
            ia[nnz_index]=Malpha+m+1;
            ja[nnz_index]=q+1;
            ar[nnz_index]=theta_q( q );
        }
    }
    //std::cout << "[CRBSCM::lb] add rows associated with C_K complement done. nnz=" << nnz_index << "\n";

    // set the structural variables, we have M_model->Qa() of them
    boost::tie( theta_q, boost::tuples::ignore ) = M_model->computeThetaq( mu );
    glp_add_cols(lp, M_model->Qa());
    for( int q = 0; q < M_model->Qa(); ++q )
    {
        glp_set_col_name( lp, q+1, (boost::format( "y_%1%" ) % q).str().c_str() );
        glp_set_col_bnds( lp, q+1, GLP_DB,
                          M_y_bounds[q].get<0>(),
                          M_y_bounds[q].get<1>() );
        glp_set_obj_coef( lp, q+1, theta_q(q) );
    }



    // now we need to build the matrix
    glp_load_matrix( lp, nnz, ia, ja, ar );
    glp_smcp parm;
    glp_init_smcp(&parm);
    parm.msg_lev = GLP_MSG_ERR;

    // use the simplex method and solve the LP
    glp_simplex(lp, &parm);

    // retrieve the minimum
    double Jobj = glp_get_obj_val(lp);
    //std::cout << "Jobj = " << Jobj << "\n";
    for( int q = 0; q < M_model->Qa(); ++q )
    {
        double y = glp_get_col_prim(lp,q+1);
        //std::cout << "y" << q << " = " << y << "\n";
    }
    glp_delete_prob(lp);

    if (indexmu >= 0 ) {

        M_C_alpha_lb[ indexmu ][K] = Jobj;

    }

    return boost::make_tuple( Jobj, ti.elapsed() );
}



template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type, double>
CRBSCM<TruthModelType>::ub( parameter_type const& mu, size_type K ) const
{
    return ub(mu, mpl::bool_<model_type::is_time_dependent>(), K );
}

template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type, double>
CRBSCM<TruthModelType>::ub( parameter_type const& mu,  mpl::bool_<true> ,size_type K ) const
{
    std::cout<<"[CRBSCM::ub] Need to be implemented in case of time-dependent problems"<<std::endl;
    return boost::make_tuple( 0 , 0);
}

template<typename TruthModelType>
boost::tuple<typename CRBSCM<TruthModelType>::value_type, double>
CRBSCM<TruthModelType>::ub( parameter_type const& mu, mpl::bool_<false> ,size_type K ) const
{
    if ( K == invalid_size_type_value ) K = this->KMax();
    if ( K > this->KMax() ) K = this->KMax();
    boost::timer ti;
    theta_vector_type theta_q;
    boost::tie( theta_q, boost::tuples::ignore ) = M_model->computeThetaq( mu );
    //std::cout << "[CRBSCM<TruthModelType>::ub] theta_q = " << theta_q << "\n";
    y_type y( K );
    for( size_type k = 0; k < K; ++k )
    {
        y( k ) = (theta_q.array()*M_Y_ub[k].array()).sum();
    }
    //std::cout << "[CRBSCM<TruthModelType>::ub] y = " << y << "\n";
    return boost::make_tuple( y.minCoeff(), ti.elapsed() );
}


template<typename TruthModelType>
typename CRBSCM<TruthModelType>::relative_error_type
CRBSCM<TruthModelType>::maxRelativeError( size_type K ) const
{
    //std::cout << "==================================================\n";
    y_type err( M_Xi->size() );
    for( size_type k = 0; k < M_Xi->size(); ++k )
    {
        parameter_type const& mu = M_Xi->at( k );
        //std::cout << "[maxRelativeError] Calcul de lb pour mu[" <<  k << "]\n" ;
        double _lb = lb( mu, K, k ).template get<0>();
        //std::cout << "[maxRelativeError] Calcul de lblb pour mu[" << k << "]\n" ;
        double _lblb = lb( mu, std::max(K-1,size_type(0) ), k ).template get<0>();
        if ( _lblb - _lb > 1e-10 )
        {
            //Log() << "[lberror] the lower bound is decreasing\n"
            //	<< "[lberror] _lblb = " << _lblb << "\n"
            //	<< "[lberror] _lb = " << _lb << "\n";
        }
        double _ub = ub( mu, K ).template get<0>();

        err(k) = 1. - _lb/_ub;
#if 0
        //std::cout << "[maxRelativeError] k = " << k << "\n";
        //std::cout << "[maxRelativeError] parameter : " << mu << "\n";
        //std::cout << "[maxRelativeError] lb = " << std::setprecision(16) << _lb << "\n";
        //std::cout << "[maxRelativeError] ub = " << std::setprecision(16) << _ub << "\n";
        //std::cout << "[maxRelativeError] rel err(" << k << ")=" << std::setprecision(16) << err(k) << "\n";
#endif
    }
    Eigen::MatrixXf::Index index;
    double maxerr = err.array().abs().maxCoeff( &index );
    //std::cout << "[maxRelativeError] K=" << K << " max Error = " << maxerr << " at index = " << index << "\n";
    Eigen::MatrixXf::Index indexmin;
    double minerr = err.array().abs().minCoeff( &indexmin );
    //std::cout << "[maxRelativeError] K=" << K << " min Error = " << minerr << " at index = " << indexmin << "\n";
    double meanerr = err.array().abs().sum()/err.size();
    //std::cout << "[maxRelativeError] K=" << K << " mean Error = " << meanerr << "\n";
    //std::cout << "==================================================\n";
    return boost::make_tuple( maxerr, M_Xi->at( index ), index, minerr, meanerr );
}

template<typename TruthModelType>
void
CRBSCM<TruthModelType>::computeYBounds()
{
    //std::cout << "************************************************************\n";
    Log() << "[CRBSCM<TruthModelType>::computeYBounds()] start...\n";
    int nconv;
    double eigenvalue_lb, eigenvalue_ub;
    sparse_matrix_ptrtype A, symmA=M_model->newMatrix(), B=M_model->innerProduct();
    B->close();
    // solve 2 * Q_a eigenproblems
    for( int q = 0; q < M_model->Qa();++q )
    {
        //std::cout << "================================================================================\n";
        //std::cout << "[ComputeYBounds] = q = " << q << " / " << M_model->Qa() << "\n";
        Log() << "[CRBSCM<TruthModelType>::computeYBounds()] q = " << q << "/" << M_model->Qa() << "\n";
        // for a given parameter \p mu assemble the left and right hand side
        std::ostringstream os;

        A = M_model->Aq( q );
        A->close();
        A->symmetricPart( symmA );
        os << "yb_A" << q << ".m";
        A->printMatlab( os.str() );
        os.str("");
        os << "yb_symmA" << q << ".m";
        symmA->printMatlab( os.str() );

        if (symmA->l1Norm()==0.0) {

            std::cout << "matrix is null\n" ;
            M_y_bounds.push_back( boost::make_tuple( 0.0, 1e-10 ) );

        }

        else {

#if 0
            // solve  for eigenvalue problem at \p mu
            boost::tie( nconv, eigenvalue_lb, boost::tuples::ignore, boost::tuples::ignore ) =
                eigs( _matrixA=symmA,
                      _matrixB=B,
                      _spectrum=SMALLEST_MAGNITUDE,
                      _ncv=15 );

            //std::cout << " -- lower bounds q=" << q
            //          << " nconv=" << nconv
            //          << " eigenvalue_lb = " << eigenvalue_lb << "\n";
            double eigmin=eigenvalue_lb;

            boost::tie( nconv, eigenvalue_ub, boost::tuples::ignore, boost::tuples::ignore ) =
                eigs( _matrixA=symmA,
                      _matrixB=B,
                      _spectrum=LARGEST_MAGNITUDE );

            //std::cout << " -- upper bounds q=" << q
            //          << " nconv=" << nconv
            //          << " eigenvalue_ub = " << eigenvalue_ub << "\n";
            double eigmax=eigenvalue_ub;
#else

            double eigenvalue_real, eigenvalue_imag;
            vector_ptrtype eigenvector;
            SolverEigen<double>::eigenmodes_type modes;
#if 1
            // solve  for eigenvalue problem at \p mu
            modes =
                eigs( _matrixA=symmA,
                      _matrixB=B,
                      //_problem=(EigenProblemType)PGNHEP,
                      _problem=(EigenProblemType)GHEP,
                      _solver=(EigenSolverType)M_vm["solvereigen-solver-type"].template as<int>(),
                      _spectrum=SMALLEST_REAL,
                      //_spectrum=SMALLEST_MAGNITUDE,
                      //_transform=SINVERT,
                      _ncv=M_vm["solvereigen-ncv"].template as<int>(),
                      _nev=M_vm["solvereigen-nev"].template as<int>(),
                      _tolerance=M_vm["solvereigen-tol"].template as<double>(),
                      _maxit=M_vm["solvereigen-maxiter"].template as<int>()
                    );
#endif
            if ( modes.empty() )
            {
                Log() << "[Computeybounds] eigmin did not converge for q=" << q << " (set to 0)\n";
            }
            double eigmin = modes.empty()?0:modes.begin()->second.template get<0>();
#if 1
            modes=
                eigs( _matrixA=symmA,
                      _matrixB=B,
                      //_problem=(EigenProblemType)PGNHEP,
                      _problem=(EigenProblemType)GHEP,
                      _solver=(EigenSolverType)M_vm["solvereigen-solver-type"].template as<int>(),
                      _spectrum=LARGEST_REAL,
                      //_spectrum=LARGEST_MAGNITUDE,
                      _ncv=M_vm["solvereigen-ncv"].template as<int>(),
                      //_ncv=20,
                      _nev=M_vm["solvereigen-nev"].template as<int>(),
                      //_tolerance=M_vm["solvereigen-tol"].template as<double>(),
                      _tolerance=1e-7,
                      _maxit=M_vm["solvereigen-maxiter"].template as<int>()
                    );
#endif
            if ( modes.empty() )
            {
                Log() << "[Computeybounds] eigmax did not converge for q=" << q << " (set to 0)\n";
            }
            double eigmax = modes.empty()?0:modes.rbegin()->second.template get<0>();

            //std::cout << "[Computeybounds] q= " << q << " eigmin=" << std::setprecision(16) << eigmin << " eigmax=" << std::setprecision(16) << eigmax << "\n";
            //std::cout << std::setprecision(16) << eigmin << " " << eigmax << "\n";

#endif
            if (eigmin==0.0 && eigmax==0.0) throw std::logic_error("eigs null\n");
            M_y_bounds.push_back( boost::make_tuple( eigmin, eigmax ) );
        }
    }
    Log() << "[CRBSCM<TruthModelType>::computeYBounds()] stop.\n";
    //std::cout << "************************************************************\n";
}

template<typename TruthModelType>
std::vector<double>
CRBSCM<TruthModelType>::run( parameter_type const& mu, int K )
{
    std::cout << "------------------------------------------------------------\n";
    double alpha_lb,alpha_lbti;
    boost::tie( alpha_lb, alpha_lbti ) = this->lb( mu, K );
    double alpha_ub,alpha_ubti;
    boost::tie( alpha_ub, alpha_ubti ) = this->ub( mu, K );
    double alpha_ex, alpha_exti;
    boost::tie( alpha_ex, alpha_exti ) = this->ex( mu );
    std::cout << "alpha_lb=" << alpha_lb << " alpha_ub=" << alpha_ub << " alpha_ex=" << alpha_ex << "\n";
    std::cout << (alpha_ex-alpha_lb)/(alpha_ub-alpha_lb) << "\n";
    std::cout << K << " "
              << std::setprecision( 16) << alpha_lb << " "
              << std::setprecision( 3 ) << alpha_lbti << " "
              << std::setprecision( 16) << alpha_ub << " "
              << std::setprecision( 3 ) << alpha_ubti << " "
              << std::setprecision( 16) << alpha_ex << " "
              << std::setprecision( 16) << alpha_exti << " "
              << std::setprecision( 16) << (alpha_ub-alpha_lb)/(alpha_ub) << " "
              << std::setprecision( 16) << (alpha_ex-alpha_lb)/(alpha_ex) << " "
              << std::setprecision( 16) << (alpha_ub-alpha_ex)/(alpha_ex) << " "
              << "\n";
    std::cout << "------------------------------------------------------------\n";
    return boost::assign::list_of( alpha_lb )( alpha_lbti )( alpha_ub )(alpha_ubti)(alpha_ex)(alpha_exti);
}

template<typename TruthModelType>
void
CRBSCM<TruthModelType>::run( const double * X, unsigned long N, double * Y, unsigned long P )
{
    parameter_type mu( M_Dmu );
    // the last parameter is the max error
    for( int p= 0; p < N-3; ++p )
        mu(p) = X[p];

    for(int i=0;i<N;i++) std::cout<<"X["<<i<<"] = "<<X[i]<<std::endl;
    double meshSize  = X[N-3];
    M_model->setMeshSize(meshSize);

    int K = this->KMax();
    double alpha_lb,lbti;
    boost::tie( alpha_lb, lbti ) = this->lb( mu, K );
    double alpha_ub,ubti;
    boost::tie( alpha_ub, ubti ) = this->ub( mu, K );
    double alpha_ex, alpha_exti;
    boost::tie( alpha_ex, alpha_exti ) = this->ex( mu );
    std::cout << "lb=" << alpha_lb << " ub=" << alpha_ub << " ex=" << alpha_ex << "\n";
    std::cout << (alpha_ex-alpha_lb)/(alpha_ub-alpha_lb) << "\n";
    Y[0] = alpha_lb;
    Y[1] = alpha_ub;

}

template<typename TruthModelType>
template<class Archive>
void
CRBSCM<TruthModelType>::save(Archive & ar, const unsigned int version) const
{
    ar & M_Malpha;
    ar & M_Mplus;
    ar & M_C_alpha_lb;
    ar & M_C;
    ar & M_C_complement;
    ar & M_C_eigenvalues;
    ar & M_y_bounds;
}

template<typename TruthModelType>
template<class Archive>
void
CRBSCM<TruthModelType>::load(Archive & ar, const unsigned int version)
{
    ar & M_Malpha;
    ar & M_Mplus;
    ar & M_C_alpha_lb;
    ar & M_C;
    ar & M_C_complement;
    ar & M_C_eigenvalues;
    ar & M_y_bounds;
}

template<typename TruthModelType>
void
CRBSCM<TruthModelType>::saveDB()
{
    fs::ofstream ofs( this->dbLocalPath() / this->dbFilename() );
    if ( ofs )
    {
        boost::archive::text_oarchive oa(ofs);
        // write class instance to archive
        oa << *this;
        // archive and stream closed when destructors are called
    }
}
template<typename TruthModelType>
bool
CRBSCM<TruthModelType>::loadDB()
{
    fs::path db = this->lookForDB();
    if ( db.empty() )
        return false;
    if ( !fs::exists( db ) )
        return false;
    std::cout << "Loading " << db << "...\n";
    fs::ifstream ifs( db );

    if ( ifs )
    {
        boost::archive::text_iarchive ia(ifs);
        // write class instance to archive
        ia >> *this;
        std::cout << "Loading " << db << " done...\n";
        this->setIsLoaded( true );
        // archive and stream closed when destructors are called
        return true;
    }
    return false;
}

} // Feel
#endif /* __CRBSCM_H */
