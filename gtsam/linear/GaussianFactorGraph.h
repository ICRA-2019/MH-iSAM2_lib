/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    GaussianFactorGraph.h
 * @brief   Linear Factor Graph where all factors are Gaussians
 * @author  Kai Ni
 * @author  Christian Potthast
 * @author  Alireza Fathi
 * @author  Richard Roberts
 * @author  Frank Dellaert
 */

#pragma once

#include <gtsam/inference/FactorGraph.h>
#include <gtsam/inference/EliminateableFactorGraph.h>
#include <gtsam/linear/GaussianFactor.h>
#include <gtsam/linear/JacobianFactor.h>
#include <gtsam/linear/HessianFactor.h>
#include <gtsam/linear/Errors.h> // Included here instead of fw-declared so we can use Errors::iterator

namespace gtsam {

  // Forward declarations
  class GaussianFactorGraph;
  class GaussianFactor;
  class GaussianConditional;
  class GaussianBayesNet;
  class GaussianEliminationTree;
  class GaussianBayesTree;
  class GaussianJunctionTree;

  /* ************************************************************************* */
  template<> struct EliminationTraits<GaussianFactorGraph>
  {
    typedef GaussianFactor FactorType;                   ///< Type of factors in factor graph
    typedef GaussianFactorGraph FactorGraphType;         ///< Type of the factor graph (e.g. GaussianFactorGraph)
   
    //[MH-A]: now also works for MH 
    typedef GaussianConditional ConditionalType;         ///< Type of conditionals from elimination
    
    typedef GaussianBayesNet BayesNetType;               ///< Type of Bayes net from sequential elimination
    
    typedef GaussianEliminationTree EliminationTreeType; ///< Type of elimination tree
    typedef GaussianBayesTree BayesTreeType;             ///< Type of Bayes tree
    typedef GaussianJunctionTree JunctionTreeType;       ///< Type of Junction tree

    /// The default dense elimination function
    static std::pair<boost::shared_ptr<ConditionalType>, boost::shared_ptr<FactorType> >
      DefaultEliminate(const FactorGraphType& factors, const Ordering& keys) {
        return EliminatePreferCholesky(factors, keys); }
  };

  /* ************************************************************************* */
  /**
   * A Linear Factor Graph is a factor graph where all factors are Gaussian, i.e.
   *   Factor == GaussianFactor
   *   VectorValues = A values structure of vectors
   * Most of the time, linear factor graphs arise by linearizing a non-linear factor graph.
   */
  class GTSAM_EXPORT GaussianFactorGraph :
    public FactorGraph<GaussianFactor>,
    public EliminateableFactorGraph<GaussianFactorGraph>
  {
  public:

    typedef GaussianFactorGraph This; ///< Typedef to this class
    typedef FactorGraph<GaussianFactor> Base; ///< Typedef to base factor graph type
    typedef EliminateableFactorGraph<This> BaseEliminateable; ///< Typedef to base elimination class
    typedef boost::shared_ptr<This> shared_ptr; ///< shared_ptr to this class

    /** Default constructor */
    GaussianFactorGraph() {}

    /** Construct from iterator over factors */
    template<typename ITERATOR>
    GaussianFactorGraph(ITERATOR firstFactor, ITERATOR lastFactor) : Base(firstFactor, lastFactor) {}

    /** Construct from container of factors (shared_ptr or plain objects) */
    template<class CONTAINER>
    explicit GaussianFactorGraph(const CONTAINER& factors) : Base(factors) {}

    /** Implicit copy/downcast constructor to override explicit template container constructor */
    template<class DERIVEDFACTOR>
    GaussianFactorGraph(const FactorGraph<DERIVEDFACTOR>& graph) : Base(graph) {}

    /** Virtual destructor */
    virtual ~GaussianFactorGraph() {}

    /// @name Testable
    /// @{

    bool equals(const This& fg, double tol = 1e-9) const;

    /// @}

    /** Add a factor by value - makes a copy */
    void add(const GaussianFactor& factor) { push_back(factor.clone()); }

    /** Add a factor by pointer - stores pointer without copying the factor */
    void add(const sharedFactor& factor) { push_back(factor); }

    /** Add a null factor */
    void add(const Vector& b) {
      add(JacobianFactor(b)); }

    /** Add a unary factor */
    void add(Key key1, const Matrix& A1,
        const Vector& b, const SharedDiagonal& model = SharedDiagonal()) {
      add(JacobianFactor(key1,A1,b,model)); }

    /** Add a binary factor */
    void add(Key key1, const Matrix& A1,
        Key key2, const Matrix& A2,
        const Vector& b, const SharedDiagonal& model = SharedDiagonal()) {
      add(JacobianFactor(key1,A1,key2,A2,b,model)); }

    /** Add a ternary factor */
    void add(Key key1, const Matrix& A1,
        Key key2, const Matrix& A2,
        Key key3, const Matrix& A3,
        const Vector& b, const SharedDiagonal& model = SharedDiagonal()) {
      add(JacobianFactor(key1,A1,key2,A2,key3,A3,b,model)); }

    /** Add an n-ary factor */
    template<class TERMS>
    void add(const TERMS& terms, const Vector &b, const SharedDiagonal& model = SharedDiagonal()) {
      add(JacobianFactor(terms,b,model)); }

    /**
     * Return the set of variables involved in the factors (computes a set
     * union).
     */
    typedef KeySet Keys;
    Keys keys() const;

    /* return a map of (Key, dimension) */
    std::map<Key, size_t> getKeyDimMap() const;

    /** unnormalized error */
    double error(const VectorValues& x) const {
      double total_error = 0.;
      for(const sharedFactor& factor: *this){
        if(factor)
          total_error += factor->error(x);
      }
      return total_error;
    }

    /** Unnormalized probability. O(n) */
    double probPrime(const VectorValues& c) const {
      return exp(-0.5 * error(c));
    }

    /**
     * Clone() performs a deep-copy of the graph, including all of the factors.
     * Cloning preserves null factors so indices for the original graph are still
     * valid for the cloned graph.
     */
    virtual GaussianFactorGraph clone() const;

    /**
     * CloneToPtr() performs a simple assignment to a new graph and returns it.
     * There is no preservation of null factors!
     */
    virtual GaussianFactorGraph::shared_ptr cloneToPtr() const;

    /**
     * Returns the negation of all factors in this graph - corresponds to antifactors.
     * Will convert all factors to HessianFactors due to negation of information.
     * Cloning preserves null factors so indices for the original graph are still
     * valid for the cloned graph.
     */
    GaussianFactorGraph negate() const;

    ///@name Linear Algebra
    ///@{

    /**
     * Return vector of i, j, and s to generate an m-by-n sparse Jacobian matrix,
     * where i(k) and j(k) are the base 0 row and column indices, s(k) a double.
     * The standard deviations are baked into A and b
     */
    std::vector<boost::tuple<size_t, size_t, double> > sparseJacobian() const;

    /**
     * Matrix version of sparseJacobian: generates a 3*m matrix with [i,j,s] entries
     * such that S(i(k),j(k)) = s(k), which can be given to MATLAB's sparse.
     * The standard deviations are baked into A and b
     */
    Matrix sparseJacobian_() const;

    /**
     * Return a dense \f$ [ \;A\;b\; ] \in \mathbb{R}^{m \times n+1} \f$
     * Jacobian matrix, augmented with b with the noise models baked
     * into A and b.  The negative log-likelihood is
     * \f$ \frac{1}{2} \Vert Ax-b \Vert^2 \f$.  See also
     * GaussianFactorGraph::jacobian and GaussianFactorGraph::sparseJacobian.
     */
    Matrix augmentedJacobian(boost::optional<const Ordering&> optionalOrdering = boost::none) const;

    /**
     * Return the dense Jacobian \f$ A \f$ and right-hand-side \f$ b \f$,
     * with the noise models baked into A and b. The negative log-likelihood
     * is \f$ \frac{1}{2} \Vert Ax-b \Vert^2 \f$.  See also
     * GaussianFactorGraph::augmentedJacobian and
     * GaussianFactorGraph::sparseJacobian.
     */
    std::pair<Matrix,Vector> jacobian(boost::optional<const Ordering&> optionalOrdering = boost::none) const;

    /**
     * Return a dense \f$ \Lambda \in \mathbb{R}^{n+1 \times n+1} \f$ Hessian
     * matrix, augmented with the information vector \f$ \eta \f$.  The
     * augmented Hessian is
     \f[ \left[ \begin{array}{ccc}
     \Lambda & \eta \\
     \eta^T & c
     \end{array} \right] \f]
     and the negative log-likelihood is
     \f$ \frac{1}{2} x^T \Lambda x + \eta^T x + c \f$.
     */
    Matrix augmentedHessian(boost::optional<const Ordering&> optionalOrdering = boost::none) const;

    /**
     * Return the dense Hessian \f$ \Lambda \f$ and information vector
     * \f$ \eta \f$, with the noise models baked in. The negative log-likelihood
     * is \frac{1}{2} x^T \Lambda x + \eta^T x + c.  See also
     * GaussianFactorGraph::augmentedHessian.
     */
    std::pair<Matrix,Vector> hessian(boost::optional<const Ordering&> optionalOrdering = boost::none) const;

    /** Return only the diagonal of the Hessian A'*A, as a VectorValues */
    virtual VectorValues hessianDiagonal() const;

    /** Return the block diagonal of the Hessian for this factor */
    virtual std::map<Key,Matrix> hessianBlockDiagonal() const;

    /** Solve the factor graph by performing multifrontal variable elimination in COLAMD order using
     *  the dense elimination function specified in \c function (default EliminatePreferCholesky),
     *  followed by back-substitution in the Bayes tree resulting from elimination.  Is equivalent
     *  to calling graph.eliminateMultifrontal()->optimize(). */
    VectorValues optimize(OptionalOrdering ordering = boost::none,
      const Eliminate& function = EliminationTraitsType::DefaultEliminate) const;

    /**
     * Optimize using Eigen's dense Cholesky factorization
     */
    VectorValues optimizeDensely() const;

    /**
     * Compute the gradient of the energy function,
     * \f$ \nabla_{x=x_0} \left\Vert \Sigma^{-1} A x - b \right\Vert^2 \f$,
     * centered around \f$ x = x_0 \f$.
     * The gradient is \f$ A^T(Ax-b) \f$.
     * @param fg The Jacobian factor graph $(A,b)$
     * @param x0 The center about which to compute the gradient
     * @return The gradient as a VectorValues
     */
    VectorValues gradient(const VectorValues& x0) const;

    /**
     * Compute the gradient of the energy function, \f$ \nabla_{x=0} \left\Vert \Sigma^{-1} A x - b
     * \right\Vert^2 \f$, centered around zero. The gradient is \f$ A^T(Ax-b) \f$.
     * @param fg The Jacobian factor graph $(A,b)$
     * @param [output] g A VectorValues to store the gradient, which must be preallocated,
     *        see allocateVectorValues
     * @return The gradient as a VectorValues */
    virtual VectorValues gradientAtZero() const;

    /** Optimize along the gradient direction, with a closed-form computation to perform the line
     *  search.  The gradient is computed about \f$ \delta x=0 \f$.
     *
     *  This function returns \f$ \delta x \f$ that minimizes a reparametrized problem.  The error
     *  function of a GaussianBayesNet is
     *
     *  \f[ f(\delta x) = \frac{1}{2} |R \delta x - d|^2 = \frac{1}{2}d^T d - d^T R \delta x +
     *  \frac{1}{2} \delta x^T R^T R \delta x \f]
     *
     *  with gradient and Hessian
     *
     *  \f[ g(\delta x) = R^T(R\delta x - d), \qquad G(\delta x) = R^T R. \f]
     *
     *  This function performs the line search in the direction of the gradient evaluated at \f$ g =
     *  g(\delta x = 0) \f$ with step size \f$ \alpha \f$ that minimizes \f$ f(\delta x = \alpha g)
     *  \f$:
     *
     *  \f[ f(\alpha) = \frac{1}{2} d^T d + g^T \delta x + \frac{1}{2} \alpha^2 g^T G g \f]
     *
     *  Optimizing by setting the derivative to zero yields \f$ \hat \alpha = (-g^T g) / (g^T G g)
     *  \f$.  For efficiency, this function evaluates the denominator without computing the Hessian
     *  \f$ G \f$, returning
     *
     *  \f[ \delta x = \hat\alpha g = \frac{-g^T g}{(R g)^T(R g)} \f] */
    VectorValues optimizeGradientSearch() const;

    /** x = A'*e */
    VectorValues transposeMultiply(const Errors& e) const;

    /** x += alpha*A'*e */
    void transposeMultiplyAdd(double alpha, const Errors& e, VectorValues& x) const;

    /** return A*x-b */
    Errors gaussianErrors(const VectorValues& x) const;

    ///** return A*x */
    Errors operator*(const VectorValues& x) const;

    ///** y += alpha*A'A*x */
    void multiplyHessianAdd(double alpha, const VectorValues& x,
        VectorValues& y) const;

    ///** In-place version e <- A*x that overwrites e. */
    void multiplyInPlace(const VectorValues& x, Errors& e) const;

    /** In-place version e <- A*x that takes an iterator. */
    void multiplyInPlace(const VectorValues& x, const Errors::iterator& e) const;

    /// @}

  private:
    /** Serialization function */
    friend class boost::serialization::access;
    template<class ARCHIVE>
    void serialize(ARCHIVE & ar, const unsigned int /*version*/) {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Base);
    }

  };

  /**
   * Evaluates whether linear factors have any constrained noise models
   * @return true if any factor is constrained.
   */
  GTSAM_EXPORT bool hasConstraints(const GaussianFactorGraph& factors);

  /****** Linear Algebra Opeations ******/

  ///* matrix-vector operations */
  //GTSAM_EXPORT void residual(const GaussianFactorGraph& fg, const VectorValues &x, VectorValues &r);
  //GTSAM_EXPORT void multiply(const GaussianFactorGraph& fg, const VectorValues &x, VectorValues &r);

/// traits
template<>
struct traits<GaussianFactorGraph> : public Testable<GaussianFactorGraph> {
};

} // \ namespace gtsam
