/* * keyvi - A key value store.
 *
 * Copyright 2015 Hendrik Muhs<hendrik.muhs@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * bounded_weighted_traversal.h
 *
 *  Created on: Nov 17, 2015
 *      Author: hendrik
 */

#ifndef KEYVI_DICTIONARY_FSA_TRAVERSAL_BOUNDED_WEIGHTED_TRAVERSAL_H_
#define KEYVI_DICTIONARY_FSA_TRAVERSAL_BOUNDED_WEIGHTED_TRAVERSAL_H_

#include <algorithm>
#include <cstdint>

#include "keyvi/dictionary/fsa/traversal/weighted_traversal.h"
#include "keyvi/dictionary/util/bounded_priority_queue.h"

// #define ENABLE_TRACING
#include "keyvi/dictionary/util/trace.h"

namespace keyvi {
namespace dictionary {
namespace fsa {
namespace traversal {

struct BoundedWeightedTransition : public WeightedTransition {
  using WeightedTransition::WeightedTransition;
};

template <>
struct TraversalPayload<BoundedWeightedTransition> {
 TraversalPayload() : current_depth(0), lookup_key() , best_scores(10) {}

 explicit TraversalPayload(std::shared_ptr<std::string>& lookup_key, uint32_t num_results)
 : current_depth(0)
 , lookup_key(lookup_key)
 , best_scores(num_results) {}

 size_t current_depth;
 std::shared_ptr<std::string> lookup_key;
 util::BoundedPriorityQueue<uint32_t> best_scores;
};

template <>
inline void TraversalState<BoundedWeightedTransition>::PostProcess(
    TraversalPayload<BoundedWeightedTransition>* payload
) {
  if (traversal_state_payload.transitions.size() > 0) {
    std::sort(traversal_state_payload.transitions.begin(), traversal_state_payload.transitions.end(),
              WeightedTransitionCompare);
  }
  if (traversal_state_payload.transitions.size() > payload->best_scores.size()) {
    // can't call resize because transition has no default constructor
    traversal_state_payload.transitions.erase(
      traversal_state_payload.transitions.begin() + payload->best_scores.size(),
      traversal_state_payload.transitions.end()
    );
  }
}

template<>
inline void TraversalState<BoundedWeightedTransition>::Add(
  uint64_t s, uint32_t w, unsigned char l, TraversalPayload<BoundedWeightedTransition> *payload
) {
  if (
    payload->lookup_key &&
    payload->current_depth < payload->lookup_key->size() &&
    static_cast<const unsigned char>(payload->lookup_key->operator[](payload->current_depth)) != l
  ) {
    return;
  }
  if (w <= payload->best_scores.Back()) {
    return;
  }
  traversal_state_payload.transitions.push_back(BoundedWeightedTransition(s, w, l));
  payload->best_scores.Put(w);
}

template <>
inline uint32_t TraversalState<BoundedWeightedTransition>::GetNextInnerWeight() const {
  return traversal_state_payload.transitions[traversal_state_payload.position].weight;
}

} /* namespace traversal */
} /* namespace fsa */
} /* namespace dictionary */
} /* namespace keyvi */

#endif  // KEYVI_DICTIONARY_FSA_TRAVERSAL_BOUNDED_WEIGHTED_TRAVERSAL_H_
