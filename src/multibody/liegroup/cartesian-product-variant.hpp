//
// Copyright (c) 2018 CNRS
//

#ifndef __pinocchio_cartesian_product_variant_hpp__
#define __pinocchio_cartesian_product_variant_hpp__

#include "pinocchio/multibody/liegroup/liegroup-base.hpp"
#include "pinocchio/multibody/liegroup/liegroup-collection.hpp"
#include "pinocchio/multibody/liegroup/liegroup-variant-visitors.hpp"

#include <vector>

namespace pinocchio
{
  
  template<typename Scalar, int Options = 0,
    template<typename,int> class LieGroupCollectionTpl = LieGroupCollectionDefaultTpl>
      struct CartesianProductOperationVariantTpl;
  typedef CartesianProductOperationVariantTpl<double, 0, LieGroupCollectionDefaultTpl> CartesianProductOperationVariant;
  
  template<typename _Scalar, int _Options, template<typename,int> class LieGroupCollectionTpl>
  struct traits<CartesianProductOperationVariantTpl<_Scalar, _Options, LieGroupCollectionTpl> >
  {
    typedef _Scalar Scalar;
    enum {
      Options = _Options,
      NQ = Eigen::Dynamic,
      NV = Eigen::Dynamic
    };
  };
  
  ///
  /// \brief Dynamic Cartesian product composed of elementary Lie groups defined in LieGroupVariant
  ///
  template<typename _Scalar, int _Options, template<typename,int> class LieGroupCollectionTpl>
  struct CartesianProductOperationVariantTpl : public LieGroupBase<CartesianProductOperationVariantTpl<_Scalar, _Options, LieGroupCollectionTpl> >
  {
    PINOCCHIO_LIE_GROUP_TPL_PUBLIC_INTERFACE(CartesianProductOperationVariantTpl);

    typedef LieGroupCollectionTpl<Scalar, Options> LieGroupCollection;
    typedef typename LieGroupCollection::LieGroupVariant LieGroupVariant;
    typedef LieGroupGenericTpl<LieGroupCollection> LieGroupGeneric;
    
    /// \brief Default constructor
    CartesianProductOperationVariantTpl()
    : m_nq(0), m_nv(0)
    , lg_nqs(0), lg_nvs(0)
    , m_neutral(0)
    {};
    
    ///
    /// \brief Constructor with one single Lie group
    ///
    /// \param[in] lg Lie group variant to insert inside the Cartesian product
    ///
    CartesianProductOperationVariantTpl(const LieGroupGeneric & lg)
    : m_nq(0), m_nv(0)
    , lg_nqs(0), lg_nvs(0)
    , m_neutral(0)
    {
      append(lg);
    };
    
    ///
    /// \brief Constructor with two Lie groups
    ///
    /// \param[in] lg1 Lie group variant to insert inside the Cartesian product
    /// \param[in] lg2 Lie group variant to insert inside the Cartesian product
    ///
    CartesianProductOperationVariantTpl(const LieGroupGeneric & lg1,
                                        const LieGroupGeneric & lg2)
    : m_nq(0), m_nv(0)
    , lg_nqs(0), lg_nvs(0)
    , m_neutral(0)
    {
      append(lg1); append(lg2);
    };
    
    ///
    /// \brief Append a Lie group to the Cartesian product
    ///
    /// \param[in] lg Lie group variant to insert inside the Cartesian product
    ///
    void append(const LieGroupGeneric & lg)
    {
      liegroups.push_back(lg);
      const Index lg_nq = ::pinocchio::nq(lg); lg_nqs.push_back(lg_nq); m_nq += lg_nq;
      const Index lg_nv = ::pinocchio::nv(lg); lg_nvs.push_back(lg_nv); m_nv += lg_nv;
      
      if(liegroups.size() > 1)
        m_name += " x ";
      m_name += ::pinocchio::name(lg);
      
      m_neutral.conservativeResize(m_nq);
      m_neutral.tail(lg_nq) = ::pinocchio::neutral(lg);
      
    }

    CartesianProductOperationVariantTpl operator* (const CartesianProductOperationVariantTpl& other) const
    {
      CartesianProductOperationVariantTpl res;

      res.liegroups.reserve(liegroups.size() + other.liegroups.size());
      res.liegroups.insert(res.liegroups.end(), liegroups.begin(), liegroups.end());
      res.liegroups.insert(res.liegroups.end(), other.liegroups.begin(), other.liegroups.end());

      res.lg_nqs.reserve(lg_nqs.size() + other.lg_nqs.size());
      res.lg_nqs.insert(res.lg_nqs.end(), lg_nqs.begin(), lg_nqs.end());
      res.lg_nqs.insert(res.lg_nqs.end(), other.lg_nqs.begin(), other.lg_nqs.end());

      res.lg_nvs.reserve(lg_nvs.size() + other.lg_nvs.size());
      res.lg_nvs.insert(res.lg_nvs.end(), lg_nvs.begin(), lg_nvs.end());
      res.lg_nvs.insert(res.lg_nvs.end(), other.lg_nvs.begin(), other.lg_nvs.end());

      res.m_nq = m_nq + other.m_nq;
      res.m_nv = m_nv + other.m_nv;

      if(liegroups.size() > 0)
        res.m_name = m_name;
      if(other.liegroups.size() > 0) {
        res.m_name += " x ";
        res.m_name += other.m_name;
      }
      
      res.m_neutral.resize(res.m_nq);
      res.m_neutral.head(m_nq) = m_neutral;
      res.m_neutral.tail(other.m_nq) = other.m_neutral;

      return res;
    }
    
    int nq() const { return m_nq; }
    int nv() const { return m_nv; }
    
    std::string name() const { return m_name; }
    
    ConfigVector_t neutral() const { return m_neutral; }
    
    template <class ConfigL_t, class ConfigR_t, class Tangent_t>
    void difference_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                         const Eigen::MatrixBase<ConfigR_t> & q1,
                         const Eigen::MatrixBase<Tangent_t> & d) const
    {
      Index id_q = 0, id_v = 0;
      for(size_t k = 0; k < liegroups.size(); ++k)
      {
        const Index & nq = lg_nqs[k];
        const Index & nv = lg_nvs[k];
        ::pinocchio::difference(liegroups[k],
                         q0.segment(id_q,nq),
                         q1.segment(id_q,nq),
                         PINOCCHIO_EIGEN_CONST_CAST(Tangent_t, d).segment(id_v,nv));
        
        id_q += nq; id_v += nv;
      }
    }

    template <class ConfigIn_t, class Velocity_t, class ConfigOut_t>
    void integrate_impl(const Eigen::MatrixBase<ConfigIn_t> & q,
                        const Eigen::MatrixBase<Velocity_t> & v,
                        const Eigen::MatrixBase<ConfigOut_t> & qout) const
    {
      ConfigOut_t & qout_ = PINOCCHIO_EIGEN_CONST_CAST(ConfigOut_t,qout);
      Index id_q = 0, id_v = 0;
      for(size_t k = 0; k < liegroups.size(); ++k)
      {
        const Index & nq = lg_nqs[k];
        const Index & nv = lg_nvs[k];
        ::pinocchio::integrate(liegroups[k],
                         q.segment(id_q,nq),
                         v.segment(id_v,nv),
                         qout_.segment(id_q,nq));
        
        id_q += nq; id_v += nv;
      }

    }
    

    template <class ConfigL_t, class ConfigR_t>
    Scalar squaredDistance_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                const Eigen::MatrixBase<ConfigR_t> & q1) const
    {
      Scalar d2 = 0;
      Index id_q = 0;
      for(size_t k = 0; k < liegroups.size(); ++k)
      {
        const Index & nq = lg_nqs[k];
        d2 += ::pinocchio::squaredDistance(liegroups[k],
            q0.segment(id_q,nq),
            q1.segment(id_q,nq));
        id_q += nq;
      }
    }
    template <class Config_t>
    void random_impl (const Eigen::MatrixBase<Config_t>& qout) const
    {
      Index id_q = 0;
      for(size_t k = 0; k < liegroups.size(); ++k)
      {
        const Index & nq = lg_nqs[k];
        ::pinocchio::random(liegroups[k], qout.segment(id_q,nq));
        id_q += nq;
      }
    }

    template <class ConfigL_t, class ConfigR_t, class ConfigOut_t>
    void randomConfiguration_impl(const Eigen::MatrixBase<ConfigL_t> & lower,
                                  const Eigen::MatrixBase<ConfigR_t> & upper,
                                  const Eigen::MatrixBase<ConfigOut_t> & qout)
      const
    {
      Index id_q = 0;
      for(size_t k = 0; k < liegroups.size(); ++k)
      {
        const Index & nq = lg_nqs[k];
        ::pinocchio::randomConfiguration(liegroups[k],
                         lower.segment(id_q,nq),
                         upper.segment(id_q,nq),
                         PINOCCHIO_EIGEN_CONST_CAST(ConfigOut_t, qout).segment(id_q,nq));
        
        id_q += nq;
      }
    }

    
  protected:
    
    std::vector<LieGroupGeneric> liegroups;
    Index m_nq, m_nv;
    std::vector<Index> lg_nqs, lg_nvs;
    std::string m_name;
    
    ConfigVector_t m_neutral;
    
  };
  
} // namespace pinocchio

#endif // ifndef __pinocchio_cartesian_product_variant_hpp__
