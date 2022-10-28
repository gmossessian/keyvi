//
// keyvi - A key value store.
//
// Copyright 2015 Hendrik Muhs<hendrik.muhs@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/*
 * weighted_traversal_test.cpp
 *
 *  Created on: Oct 28, 2022
 *      Author: gmossessian
 */

#include <utility>

#include <boost/test/unit_test.hpp>

#include "keyvi/dictionary/fsa/automata.h"
#include "keyvi/dictionary/fsa/generator.h"
#include "keyvi/dictionary/fsa/state_traverser.h"
#include "keyvi/dictionary/fsa/traversal/weighted_traversal.h"
#include "keyvi/testing/temp_dictionary.h"

namespace keyvi {
namespace dictionary {
namespace fsa {

BOOST_AUTO_TEST_SUITE(WeightedTraversalTests)

#define check_next_state(S, C, W, D)              \
  (*(S))++;                                       \
  BOOST_CHECK_EQUAL((C), (S)->GetStateLabel());   \
  BOOST_CHECK_EQUAL((W), (S)->GetInnerWeight());  \
  BOOST_CHECK_EQUAL((D), (S)->GetDepth());

BOOST_AUTO_TEST_CASE(basicWeightedTraversal) {
  std::vector<std::pair<std::string, std::uint32_t>> test_data = {
    {"aa", 5}, {"ab", 4}, {"ac", 6}, {"aba", 3}, {"cd", 10}, {"cdd", 2}
  };

  testing::TempDictionary dictionary(&test_data);
  automata_t f = dictionary.GetFsa();

  // std::shared_ptr<std::string> weighted_key = std::make_shared<std::string>("a");
  auto payload = traversal::TraversalPayload<traversal::WeightedTransition>();

  StateTraverser<traversal::WeightedTransition> s(f, f->GetStartState(), std::move(payload));

  // first we go down "cd"
  BOOST_CHECK_EQUAL('c', s.GetStateLabel());
  BOOST_CHECK_EQUAL(10, s.GetInnerWeight());
  BOOST_CHECK_EQUAL(1, s.GetDepth());

  check_next_state(&s, 'd', 10, 2);

  // down to "cdd"
  check_next_state(&s, 'd', 2, 3);

  // back up to "ac", then "aa" and "ab", "aba"
  check_next_state(&s, 'a', 6, 1);
  check_next_state(&s, 'c', 6, 2);
  check_next_state(&s, 'a', 5, 2);
  check_next_state(&s, 'b', 4, 2);
  check_next_state(&s, 'a', 3, 3);

  // check we're done
  BOOST_CHECK(s.IsFinalState());
  check_next_state(&s, 0, 0, 0);
  check_next_state(&s, 0, 0, 0);  
}

BOOST_AUTO_TEST_CASE(prefixWeightedTraversal) {
  std::vector<std::pair<std::string, std::uint32_t>> test_data = {
    {"aa", 5}, {"ab", 4}, {"ac", 6}, {"aba", 3}, {"cd", 10}, {"cdd", 2}
  };

  testing::TempDictionary dictionary(&test_data);
  automata_t f = dictionary.GetFsa();

  std::shared_ptr<std::string> lookup_key = std::make_shared<std::string>("a");
  auto payload = traversal::TraversalPayload<traversal::WeightedTransition>(lookup_key);

  StateTraverser<traversal::WeightedTransition> s(f, f->GetStartState(), std::move(payload));

  // first we go down "ac"
  BOOST_CHECK_EQUAL('a', s.GetStateLabel());
  BOOST_CHECK_EQUAL(6, s.GetInnerWeight());
  BOOST_CHECK_EQUAL(1, s.GetDepth());

  check_next_state(&s, 'c', 6, 2);
  // then "aa" and "ab", "aba"
  check_next_state(&s, 'a', 5, 2);
  check_next_state(&s, 'b', 4, 2);
  check_next_state(&s, 'a', 3, 3);

  // check we're done
  BOOST_CHECK(s.IsFinalState());
  check_next_state(&s, 0, 0, 0);
  check_next_state(&s, 0, 0, 0);
}


BOOST_AUTO_TEST_SUITE_END()


} /* namespace fsa */
} /* namespace dictionary */
} /* namespace keyvi */
