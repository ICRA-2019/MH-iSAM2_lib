/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file testSignature
 * @brief Tests focusing on the details of Signatures to evaluate boost compliance
 * @author Alex Cunningham
 * @date Sept 19th 2011
 */

#include <boost/assign/std/vector.hpp>
#include <CppUnitLite/TestHarness.h>

#include <gtsam/base/Testable.h>
#include <gtsam/discrete/Signature.h>

using namespace std;
using namespace gtsam;
using namespace boost::assign;

DiscreteKey X(0,2), Y(1,3), Z(2,2);

/* ************************************************************************* */
TEST(testSignature, simple_conditional) {
  Signature sig(X | Y = "1/1 2/3 1/4");
  DiscreteKey actKey = sig.key();
  LONGS_EQUAL((long)X.first, (long)actKey.first);

  DiscreteKeys actKeys = sig.discreteKeysParentsFirst();
  LONGS_EQUAL(2, (long)actKeys.size());
  LONGS_EQUAL((long)Y.first, (long)actKeys.front().first);
  LONGS_EQUAL((long)X.first, (long)actKeys.back().first);

  vector<double> actCpt = sig.cpt();
  EXPECT_LONGS_EQUAL(6, (long)actCpt.size());
}

/* ************************************************************************* */
TEST(testSignature, simple_conditional_nonparser) {
  Signature::Table table;
  Signature::Row row1, row2, row3;
  row1 += 1.0, 1.0;
  row2 += 2.0, 3.0;
  row3 += 1.0, 4.0;
  table += row1, row2, row3;

  Signature sig(X | Y = table);
  DiscreteKey actKey = sig.key();
  EXPECT_LONGS_EQUAL((long)X.first, (long)actKey.first);

  DiscreteKeys actKeys = sig.discreteKeysParentsFirst();
  LONGS_EQUAL(2, (long)actKeys.size());
  LONGS_EQUAL((long)Y.first, (long)actKeys.front().first);
  LONGS_EQUAL((long)X.first, (long)actKeys.back().first);

  vector<double> actCpt = sig.cpt();
  EXPECT_LONGS_EQUAL(6, (long)actCpt.size());
}

/* ************************************************************************* */
int main() { TestResult tr; return TestRegistry::runAllTests(tr); }
/* ************************************************************************* */
