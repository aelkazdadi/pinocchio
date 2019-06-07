//
// Copyright (c) 2019 INRIA
//

#include <boost/variant.hpp> // to avoid C99 warnings

#include <casadi/casadi.hpp>
#include <Eigen/Core>

#include <boost/test/unit_test.hpp>
#include <boost/utility/binary.hpp>

BOOST_AUTO_TEST_SUITE(BOOST_TEST_MODULE)

BOOST_AUTO_TEST_CASE(test_eigen)
{
  Eigen::Matrix<casadi::SX, 3, 3> A, B;
  Eigen::Matrix<casadi::SX, 3, 1> a, b;
  Eigen::Matrix<casadi::SX, 3, 1> c = A * a - B.transpose() * b;
}

// A function working with Eigen::Matrix'es parameterized by the Scalar type
template <typename Scalar, typename T1, typename T2, typename T3, typename T4>
Eigen::Matrix<Scalar, Eigen::Dynamic, 1>
eigenFun(Eigen::MatrixBase<T1> const& A,
         Eigen::MatrixBase<T2> const& a,
         Eigen::MatrixBase<T3> const& B,
         Eigen::MatrixBase<T4> const& b)
{
  Eigen::Matrix<Scalar, Eigen::Dynamic, 1> c(4);
  c.segment(1, 3) = A * a.segment(1, 3) - B.transpose() * b;
  c[0] = 0.;
  
  return c;
}

BOOST_AUTO_TEST_CASE(test_example)
{
  // Declare casadi symbolic matrix arguments
  // casadi::SX cs_A = casadi::SX::sym("A", 3, 3);
  // casadi::SX cs_B = casadi::SX::sym("B", 3, 3);
  casadi::SX cs_a = casadi::SX::sym("a", 4);
  casadi::SX cs_b = casadi::SX::sym("b", 3);
  
  // Declare Eigen matrices
  Eigen::Matrix<casadi::SX, 3, 3> A, B;
  Eigen::Matrix<casadi::SX, -1, 1> a (4), c (4);
  Eigen::Matrix<casadi::SX, 3, 1> b;
  
  // Let A, B be some numeric matrices
  for (Eigen::Index i = 0; i < A.rows(); ++i)
  {
    for (Eigen::Index j = 0; j < A.cols(); ++j)
    {
      A(i, j) = 10 * i + j;
      B(i, j) = -10 * i - j;
    }
  }
  
  // Let a, b be symbolic arguments of a function
  for (Eigen::Index i = 0; i < b.size(); ++i)
    b[i] = cs_b(i);
  
  for (Eigen::Index i = 0; i < a.size(); ++i)
    a[i] = cs_a(i);
  
  // Call the function taking Eigen matrices
  c = eigenFun<casadi::SX>(A, a, B, b);
  
  // Copy the result from Eigen matrices to casadi matrix
  casadi::SX cs_c = casadi::SX(casadi::Sparsity::dense(c.rows(), 1));
  for (Eigen::Index i = 0; i < c.rows(); ++i)
    cs_c(i) = c[i];
  
  // Display the resulting casadi matrix
  std::cout << "c = " << cs_c << std::endl;
  
  // Do some AD
  casadi::SX dc_da = jacobian(cs_c, cs_a);
  
  // Display the resulting jacobian
  std::cout << "dc/da = " << dc_da << std::endl;
  
  // Create a function which takes a, b and returns c and dc_da
  casadi::Function fun("fun", casadi::SXVector {cs_a, cs_b}, casadi::SXVector {cs_c, dc_da});
  std::cout << "fun = " << fun << std::endl;
  
  // Evaluate the function
  casadi::DMVector res = fun(casadi::DMVector {std::vector<double> {1., 2., 3., 4.}, std::vector<double> {-1., -2., -3.}});
  std::cout << "fun(a, b)=" << res << std::endl;
}

BOOST_AUTO_TEST_CASE(test_jacobian)
{
  casadi::SX cs_x = casadi::SX::sym("x", 3);
  
  casadi::SX cs_y = casadi::SX::sym("y", 1);
  cs_y(0) = cs_x(0) + cs_x(1) + cs_x(2);
  
  // Display the resulting expression
  std::cout << "y = " << cs_y << std::endl;
  
  // Do some AD
  casadi::SX dy_dx = jacobian(cs_x, cs_x);

  // Display the resulting jacobian
  std::cout << "dy/dx = " << dy_dx << std::endl;
}


BOOST_AUTO_TEST_SUITE_END()
