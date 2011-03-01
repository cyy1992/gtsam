/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    Conditional.h
 * @brief   Base class for conditional densities
 * @author  Frank Dellaert
 */

// \callgraph

#pragma once

#include <iostream>
#include <boost/utility.hpp> // for noncopyable
#include <boost/foreach.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/serialization/nvp.hpp>
#include <gtsam/base/types.h>
#include <gtsam/base/Testable.h>
#include <gtsam/inference/FactorBase.h>
#include <gtsam/inference/Permutation.h>

namespace gtsam {

/**
 * Base class for conditional densities, templated on KEY type.  This class
 * provides storage for the keys involved in a conditional, and iterators and
 * access to the frontal and separator keys.
 *
 * todo:  Move permutation functions to IndexConditional.
 *
 * Derived classes *must* redefine the Factor and shared_ptr typedefs to refer
 * to the associated factor type and shared_ptr type of the derived class.  See
 * IndexConditional and GaussianConditional for examples.
 *
 * We make it noncopyable so we enforce the fact that factors are
 * kept in pointer containers. To be safe, you should make them
 * immutable, i.e., practicing functional programming.
 */
template<typename KEY>
class ConditionalBase: public gtsam::FactorBase<KEY>, boost::noncopyable, public Testable<ConditionalBase<KEY> > {

protected:

  /** The first nFrontal variables are frontal and the rest are parents. */
  size_t nrFrontals_;

  /** Debugging invariant that the keys should be in order, including that the
   * conditioned variable is numbered lower than the parents.
   */
  void assertInvariants() const;

public:

  typedef KEY Key;
  typedef ConditionalBase<Key> This;

  /**
   * Typedef to the factor type that produces this conditional and that this
   * conditional can be converted to using a factor constructor. Derived
   * classes must redefine this.
   */
  typedef gtsam::FactorBase<Key> FactorType;

  /** A shared_ptr to this class.  Derived classes must redefine this. */
  typedef boost::shared_ptr<This> shared_ptr;

  /** Iterator over keys */
  typedef typename FactorType::iterator iterator;

  /** Const iterator over keys */
  typedef typename FactorType::const_iterator const_iterator;

  /** View of the frontal keys (call frontals()) */
  typedef boost::iterator_range<const_iterator> Frontals;

  /** View of the separator keys (call parents()) */
  typedef boost::iterator_range<const_iterator> Parents;

  /** Empty Constructor to make serialization possible */
  ConditionalBase() : nrFrontals_(0) {}

  /** No parents */
  ConditionalBase(Key key) : FactorType(key), nrFrontals_(1) {}

  /** Single parent */
  ConditionalBase(Key key, Key parent) : FactorType(key, parent), nrFrontals_(1) {}

  /** Two parents */
  ConditionalBase(Key key, Key parent1, Key parent2) : FactorType(key, parent1, parent2), nrFrontals_(1) {}

  /** Three parents */
  ConditionalBase(Key key, Key parent1, Key parent2, Key parent3) : FactorType(key, parent1, parent2, parent3), nrFrontals_(1) {}

  /** Constructor from a frontal variable and a vector of parents */
  ConditionalBase(Key key, const std::vector<Key>& parents) : nrFrontals_(1) {
    FactorType::keys_.resize(1 + parents.size());
    *(beginFrontals()) = key;
    std::copy(parents.begin(), parents.end(), beginParents());
  }

  /** Constructor from a frontal variable and an iterator range of parents */
  template<class DERIVED, typename ITERATOR>
  static typename DERIVED::shared_ptr FromRange(Key key, ITERATOR firstParent, ITERATOR lastParent) {
    typename DERIVED::shared_ptr conditional(new DERIVED);
    conditional->nrFrontals_ = 1;
    conditional->keys_.push_back(key);
    std::copy(firstParent, lastParent, back_inserter(conditional->keys_));
    return conditional;
  }

  /** Named constructor from any number of frontal variables and parents */
  template<typename DERIVED, typename ITERATOR>
  static typename DERIVED::shared_ptr FromRange(ITERATOR firstKey, ITERATOR lastKey, size_t nrFrontals) {
    typename DERIVED::shared_ptr conditional(new DERIVED);
    conditional->nrFrontals_ = nrFrontals;
    std::copy(firstKey, lastKey, back_inserter(conditional->keys_));
    return conditional;
  }

  /** check equality */
  template<class DERIVED>
  bool equals(const DERIVED& c, double tol = 1e-9) const {
    return nrFrontals_ == c.nrFrontals_ && FactorType::equals(c, tol); }

	/** return the number of frontals */
	size_t nrFrontals() const { return nrFrontals_; }

	/** return the number of parents */
	size_t nrParents() const { return FactorType::keys_.size() - nrFrontals_; }

	/** Special accessor when there is only one frontal variable. */
	Key key() const { assert(nrFrontals_==1); return FactorType::keys_[0]; }

  /** Iterators over frontal and parent variables. */
  const_iterator beginFrontals() const { return FactorType::keys_.begin(); }
  const_iterator endFrontals() const { return FactorType::keys_.begin()+nrFrontals_; }
  const_iterator beginParents() const { return FactorType::keys_.begin()+nrFrontals_; }
  const_iterator endParents() const { return FactorType::keys_.end(); }

  /** Mutable iterators and accessors */
  iterator beginFrontals() { return FactorType::keys_.begin(); }
  iterator endFrontals() { return FactorType::keys_.begin()+nrFrontals_; }
  iterator beginParents() { return FactorType::keys_.begin()+nrFrontals_; }
  iterator endParents() { return FactorType::keys_.end(); }
  boost::iterator_range<iterator> frontals() { return boost::make_iterator_range(beginFrontals(), endFrontals()); }
  boost::iterator_range<iterator> parents() { return boost::make_iterator_range(beginParents(), endParents()); }

  /** return a view of the frontal keys */
  Frontals frontals() const {
    return boost::make_iterator_range(beginFrontals(), endFrontals()); }

	/** return a view of the parent keys */
	Parents parents() const {
	  return boost::make_iterator_range(beginParents(), endParents()); }

  /** print */
  void print(const std::string& s = "Conditional") const;

  /** Permute the variables when only separator variables need to be permuted.
   * Returns true if any reordered variables appeared in the separator and
   * false if not.
   */
  bool permuteSeparatorWithInverse(const Permutation& inversePermutation);

  /**
   * Permutes the Conditional, but for efficiency requires the permutation
   * to already be inverted.
   */
  void permuteWithInverse(const Permutation& inversePermutation);

private:
  /** Serialization function */
  friend class boost::serialization::access;
  template<class ARCHIVE>
  void serialize(ARCHIVE & ar, const unsigned int version) {
    ar & BOOST_SERIALIZATION_NVP(nrFrontals_);
  }
};


/* ************************************************************************* */
template<typename KEY>
void ConditionalBase<KEY>::print(const std::string& s) const {
  std::cout << s << " P(";
  BOOST_FOREACH(Key key, frontals()) std::cout << " " << key;
  if (nrParents()>0) std::cout << " |";
  BOOST_FOREACH(Key parent, parents()) std::cout << " " << parent;
  std::cout << ")" << std::endl;
}

/* ************************************************************************* */
template<typename KEY>
bool ConditionalBase<KEY>::permuteSeparatorWithInverse(const Permutation& inversePermutation) {
#ifndef NDEBUG
  BOOST_FOREACH(Key key, frontals()) { assert(key == inversePermutation[key]); }
#endif
  bool parentChanged = false;
  BOOST_FOREACH(Key& parent, parents()) {
    Key newParent = inversePermutation[parent];
    if(parent != newParent) {
      parentChanged = true;
      parent = newParent;
    }
  }
  return parentChanged;
}

/* ************************************************************************* */
template<typename KEY>
void ConditionalBase<KEY>::permuteWithInverse(const Permutation& inversePermutation) {
  // The permutation may not move the separators into the frontals
#ifndef NDEBUG
  BOOST_FOREACH(const Key frontal, this->frontals()) {
    BOOST_FOREACH(const Key separator, this->parents()) {
      assert(inversePermutation[frontal] < inversePermutation[separator]);
    }
  }
#endif
  FactorType::permuteWithInverse(inversePermutation);
}

}
