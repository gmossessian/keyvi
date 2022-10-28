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
 * weighted_traversal.h
 *
 *  Created on: Nov 17, 2015
 *      Author: hendrik
 */

#ifndef KEYVI_DICTIONARY_FSA_TRAVERSAL_WEIGHTED_TRAVERSAL_H_
#define KEYVI_DICTIONARY_FSA_TRAVERSAL_WEIGHTED_TRAVERSAL_H_

#include <algorithm>
#include <cstdint>

#include "keyvi/dictionary/fsa/traversal/traversal_base.h"

// #define ENABLE_TRACING
#include "keyvi/dictionary/util/trace.h"

namespace keyvi {
namespace dictionary {
namespace fsa {
namespace traversal {

struct WeightedTransition {
  WeightedTransition(uint64_t s, uint32_t w, unsigned char l) : state(s), weight(w), label(l) {}

  uint64_t state;
  uint32_t weight;
  unsigned char label;
};

template <>
struct TraversalPayload<WeightedTransition> {
  TraversalPayload() : current_depth(0), lookup_key() {}
  explicit TraversalPayload(std::shared_ptr<std::string>& lookup_key) : current_depth(0), lookup_key(lookup_key) {}

  size_t current_depth;
  std::shared_ptr<std::string> lookup_key;
};


static bool WeightedTransitionCompare(const WeightedTransition& a, const WeightedTransition& b) {
  TRACE("compare %d %d", a.weight, b.weight);

  return a.weight > b.weight;
}

// neccessary to override so that transitions have a weight param
template <>
struct TraversalStatePayload<WeightedTransition> {
  std::vector<WeightedTransition> transitions;
  size_t position = 0;
};

template <>
inline void TraversalState<WeightedTransition>::PostProcess(TraversalPayload<WeightedTransition>* payload) {
  if (traversal_state_payload.transitions.size() > 0) {
    std::sort(
      traversal_state_payload.transitions.begin(),
      traversal_state_payload.transitions.end(),
      WeightedTransitionCompare
    );
  }
}

template<>
inline void TraversalState<WeightedTransition>::Add(
  uint64_t s,
  uint32_t w,
  unsigned char l,
  TraversalPayload<WeightedTransition>* payload
) {
  if (
    payload->current_depth < payload->lookup_key->size() &&
    static_cast<const unsigned char>(payload->lookup_key->operator[](payload->current_depth)) == l
  ) {
    traversal_state_payload.position = 0;
    traversal_state_payload.transitions[0] = WeightedTransition(s, w, l);
  }
  traversal_state_payload.transitions.push_back(WeightedTransition(s, w, l));
}

template <>
inline uint32_t TraversalState<WeightedTransition>::GetNextInnerWeight() const {
  return traversal_state_payload.transitions[traversal_state_payload.position].weight;
}

} /* namespace traversal */
} /* namespace fsa */
} /* namespace dictionary */
} /* namespace keyvi */

#endif  // KEYVI_DICTIONARY_FSA_TRAVERSAL_WEIGHTED_TRAVERSAL_H_
