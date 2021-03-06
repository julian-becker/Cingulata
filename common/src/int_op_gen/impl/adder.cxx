/*
    (C) Copyright 2019 CEA LIST. All Rights Reserved.
    Contributor(s): Cingulata team

    This software is governed by the CeCILL-C license under French law and
    abiding by the rules of distribution of free software.  You can  use,
    modify and/ or redistribute the software under the terms of the CeCILL-C
    license as circulated by CEA, CNRS and INRIA at the following URL
    "http://www.cecill.info".

    As a counterpart to the access to the source code and  rights to copy,
    modify and redistribute granted by the license, users are provided only
    with a limited warranty  and the software's author,  the holder of the
    economic rights,  and the successive licensors  have only  limited
    liability.

    The fact that you are presently reading this means that you have had
    knowledge of the CeCILL-C license and that you accept its terms.
*/

#include <int_op_gen/impl/adder.hxx>

#include <cmath>
#include <iostream>
#include <queue>
#include <tuple>

using namespace std;
using namespace cingulata;
using namespace cingulata::int_ops;

CiBitVector RippleCarryAdder::oper(const CiBitVector &lhs,
                                   const CiBitVector &rhs,
                                   const CiBit &carry_in) const {
  int size = lhs.size();
  CiBitVector res(size);

  CiBit carry{carry_in};
  for (int i = 0; i < size; ++i) {
    CiBit n1 = carry ^ lhs[i];
    CiBit n2 = carry ^ rhs[i];
    res[i] = n1 ^ rhs[i];
    if (i < size - 1)
      carry = (n1 & n2) ^ carry;
  }

  return res;
}

namespace {
void pre_computation(std::vector<CiBitVector> &P, std::vector<CiBitVector> &G,
                     const CiBitVector &lhs, const CiBitVector &rhs) {
  const int size = lhs.size();
  for (int i = 0; i < size; ++i) {
    P[i][i] = lhs[i] ^ rhs[i];
  }
  /* G[size-1][size-1] will be unused */
  for (int i = 0; i < size - 1; ++i) {
    G[i][i] = lhs[i] & rhs[i];
  }
}

void evaluate_G(std::vector<CiBitVector> &P, std::vector<CiBitVector> &G,
                int row_index, int col_index, int step) {
  int k = col_index + (int)pow(2, step - 1);
  G[row_index][col_index] =
      G[row_index][k] ^ (P[row_index][k] & G[k - 1][col_index]);
}

void evaluate_P(std::vector<CiBitVector> &P, std::vector<CiBitVector> &G,
                int row_index, int col_index, int step) {
  int k = col_index + (int)pow(2, step - 1);
  P[row_index][col_index] = P[row_index][k] & P[k - 1][col_index];
}

CiBitVector post_computation(std::vector<CiBitVector> &P,
                             std::vector<CiBitVector> &G, int size) {
  CiBitVector res(size);
  res[0] = P[0][0];
  for (int i = 1; i < size; ++i)
    res[i] = P[i][i] ^ G[i - 1][0];
  return res;
}
} // namespace

CiBitVector SklanskyAdder::oper(const CiBitVector &lhs, const CiBitVector &rhs,
                                const CiBit &carry_in) const {
  CiBitVector res;

  int size = lhs.size();
  std::vector<CiBitVector> P(size, CiBitVector(size));
  std::vector<CiBitVector> G(size, CiBitVector(size));
  int num_steps = 0;
  if (size > 1)
    num_steps = (int)floor(log2((double)size - 1)) + 1;

  /* compute initial G and P*/
  pre_computation(P, G, lhs, rhs);

  /* do for each level */
  for (int step = 1; step <= num_steps; ++step) {
    int row = 0;
    int col = 0;
    /* shift row */
    row += (int)pow(2, step - 1);
    /*   do while the size of enter is not reach*/
    while (row < size - 1) {
      /* define column value */
      col = (int)floor(row / pow(2, step)) * (int)pow(2, step);
      for (int i = 0; i < (int)pow(2, step - 1); ++i) {
        evaluate_G(P, G, row, col, step);
        if (col != 0)
          evaluate_P(P, G, row, col, step);
        /* increment row value */
        row += 1;
        /* case size is not multiple of 2 */
        if (row == size - 1)
          break;
      }
      row += (int)pow(2, step - 1);
    }
  }
  /* compute results*/
  res = post_computation(P, G, size);

  return res;
}
