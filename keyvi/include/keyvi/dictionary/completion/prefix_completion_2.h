/* * keyvi - A key value store.
 *
 * Copyright 2021 Hendrik Muhs<hendrik.muhs@gmail.com>
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

#ifndef KEYVI_DICTIONARY_COMPLETION_PREFIX_COMPLETION_2_H_
#define KEYVI_DICTIONARY_COMPLETION_PREFIX_COMPLETION_2_H_

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/range/adaptor/reversed.hpp>

#include "keyvi/dictionary/fsa/automata.h"
#include "keyvi/dictionary/fsa/codepoint_state_traverser.h"
#include "keyvi/dictionary/fsa/comparable_state_traverser.h"
#include "keyvi/dictionary/fsa/traverser_types.h"
#include "keyvi/dictionary/fsa/zip_state_traverser.h"
#include "keyvi/dictionary/match.h"

// #define ENABLE_TRACING
#include "keyvi/dictionary/util/trace.h"

namespace keyvi {
namespace index {
namespace internal {
template <class MatcherT, class DeletedT>
keyvi::dictionary::Match NextFilteredMatchSingle(const MatcherT&, const DeletedT&);
template <class MatcherT, class DeletedT>
keyvi::dictionary::Match NextFilteredMatch(const MatcherT&, const DeletedT&);
}  // namespace internal
}  // namespace index
namespace dictionary {
namespace matching {

template <class innerTraverserType = fsa::ComparableStateTraverser<fsa::WeightedStateTraverser>>
class WeightedMatching final {
 private:
  using fsa_start_state_payloads_t = std::vector<
      std::tuple<
          fsa::automata_t,
          uint64_t,
          fsa::traversal::TraversalPayload<fsa::traversal::WeightedTransition>
      >
  >;

 public:
  /**
   * Create a weighted matcher from a single Fsa
   *
   * @param fsa the fsa
   * @param query the query
   * @param max_num_results return only top-N results. if 0 return all.
   */
  static WeightedMatching FromSingleFsa(
    const fsa::automata_t& fsa,
    const std::string& query,
    const size_t max_num_results = 0
  ) {
    uint64_t state = fsa->GetStartState();

    return FromSingleFsa(fsa, state, query, max_num_results);
  }

  /**
   * Create a weighted matcher from a single Fsa
   *
   * @param fsa the fsa
   * @param query the query
   * @param greedy if true matches everything below minimum prefix
   */
  static WeightedMatching FromSingleFsa(
    const fsa::automata_t& fsa,
    const uint64_t start_state,
    const std::string& query,
    const size_t max_num_results = 0
  ) {
    Match first_match;
    /* question for Hendrik: what is *score* vs *weight*? why is it exact_prefix in near_matching.h?
      is it because near_matching wants to give longest matches the highest score?
    */
    if (fsa->IsFinalState(start_state)) {
      first_match = Match(0, query.size(), query, fsa->GetWeightValue(start_state), fsa, fsa->GetStateValue(start_state));
    }

    std::shared_ptr<std::string> weighted_key = std::make_shared<std::string>(query);

    auto payload = fsa::traversal::TraversalPayload<fsa::traversal::WeightedTransition>(weighted_key);

    // todo: switch to make_unique, requires C++14
    std::unique_ptr<fsa::ComparableStateTraverser<fsa::WeightedStateTraverser>> traverser;
    traverser.reset(
      new fsa::ComparableStateTraverser<fsa::WeightedStateTraverser>(fsa, start_state, std::move(payload), true, 0)
    );

    return WeightedMatching(std::move(traverser), query, std::move(first_match), max_num_results);
  }

  /**
   * Create a weighted matcher from multiple Fsas
   *
   * @param fsas a vector of fsas
   * @param query the query
   * @param minimum_exact_prefix the minimum exact prefix to match before matching approximate
   * @param greedy if true matches everything below minimum prefix, if false everything at the longest matched prefix
   */
  static WeightedMatching FromMulipleFsas(
    const std::vector<fsa::automata_t>& fsas,
    const std::string& query,
    const size_t max_num_results = 0
  ) {
    fsa_start_state_payloads_t fsa_start_state_payloads = FilterWithExactPrefix(fsas, query);

    return FromMulipleFsas(std::move(fsa_start_state_payloads), query, max_num_results);
  }

  /**
   * Create a weighted matcher with already matched exact prefix.
   *
   * @param fsa_start_state_pairs pairs of fsa and current state
   * @param query the query
   * @param exact_prefix the exact prefix that already matched
   * @param greedy if true matches everything below minimum prefix, if false everything at the longest matched prefix
   */

  static WeightedMatching FromMulipleFsas(
    fsa_start_state_payloads_t&& fsa_start_state_payloads,
    const std::string& query,
    const size_t max_num_results = 0
  ) {
    if (fsa_start_state_payloads.size() == 0) {
      return WeightedMatching();
    }

    std::shared_ptr<std::string> weighted_key = std::make_shared<std::string>(query);
    std::vector<
      std::tuple<fsa::automata_t, uint64_t, fsa::traversal::TraversalPayload<fsa::traversal::WeightedTransition>>
    > fsas_with_payload;
    Match first_match;

    // check if the prefix is already the query
    for (const auto& fsa_state : boost::adaptors::reverse(fsa_start_state_payloads)) {
      if (std::get<0>(fsa_state)->IsFinalState(std::get<1>(fsa_state))) {
        first_match = Match(
          0,
          query.size(),
          query,
          std::get<0>(fsa_state)->GetWeightValue(std::get<1>(fsa_state)),
          std::get<0>(fsa_state),
          std::get<0>(fsa_state)->GetStateValue(std::get<1>(fsa_state))
        );
        break;
      }
    }

    // todo: switch to make_unique, requires C++14
    std::unique_ptr<fsa::ZipStateTraverser<fsa::WeightedStateTraverser>> traverser;
    traverser.reset(new fsa::ZipStateTraverser<fsa::WeightedStateTraverser>(std::move(fsa_start_state_payloads)));

    return WeightedMatching<fsa::ZipStateTraverser<fsa::WeightedStateTraverser>>(
      std::move(traverser), query, std::move(first_match), max_num_results
    );
  }

  static inline fsa_start_state_payloads_t FilterWithExactPrefix(
    const std::vector<fsa::automata_t>& fsas,
    const std::string& query
  ) {
    fsa_start_state_payloads_t fsa_start_state_payloads;

    for (const fsa::automata_t& fsa : fsas) {
      uint64_t state = fsa->GetStartState();
      size_t depth = 0;
      while (state != 0 && depth < query.size()) {
        state = fsa->TryWalkTransition(state, query[depth++]);
      }

      if (state && depth == query.size()) {
        std::shared_ptr<std::string> query_ptr = std::make_shared<std::string>(query);
        auto payload = fsa::traversal::TraversalPayload<fsa::traversal::WeightedTransition>(query_ptr);
        fsa_start_state_payloads.emplace_back(fsa, state, std::move(payload));
      }
    }
    return fsa_start_state_payloads;
  }

  Match FirstMatch() { num_matched_++; return first_match_; }

  Match NextMatch() {
    TRACE("call next match %lu", matched_depth_);
    for (; traverser_ptr_ && (num_matched_ < max_num_results || max_num_results == 0);) {
      if (traverser_ptr_->IsFinalState()) {
        // optimize? fill vector upfront?
        std::string match_str = query_ + std::string(
          reinterpret_cast<const char*>(traverser_ptr_->GetStateLabels().data()),
          traverser_ptr_->GetDepth()
        );

        // length should be query.size???
        Match m(
          0,
          traverser_ptr_->GetDepth() + query_.size(),
          match_str,
          traverser_ptr_->GetInnerWeight(),
          traverser_ptr_->GetFsa(),
          traverser_ptr_->GetStateValue()
        );

//        if (!greedy_) {
//          // remember the depth
//          TRACE("found a match, remember depth, only allow matches with same depth %ld",
//                traverser_ptr_->GetTraversalPayload().exact_depth);
//          matched_depth_ = traverser_ptr_->GetTraversalPayload().exact_depth;
//        }

        (*traverser_ptr_)++;
        num_matched_++;
        return m;
      }
      (*traverser_ptr_)++;
    }

    return Match();
  }

 private:
  std::unique_ptr<innerTraverserType> traverser_ptr_;
  const std::string query_;
  const Match first_match_;
  const size_t max_num_results = 0;
  size_t matched_depth_ = 0;
  size_t num_matched_ = 0;

  WeightedMatching(
    std::unique_ptr<innerTraverserType>&& traverser,
    const std::string& query,
    Match&& first_match,
    const size_t max_num_results = 0
  )  : traverser_ptr_(std::move(traverser))
     , query_(std::move(query))
     , first_match_(std::move(first_match))
     , max_num_results(max_num_results)
  {}

  WeightedMatching() {}

  // reset method for the index in the special case the match is deleted
  template <class MatcherT, class DeletedT>
  friend Match index::internal::NextFilteredMatchSingle(const MatcherT&, const DeletedT&);
  template <class MatcherT, class DeletedT>
  friend Match index::internal::NextFilteredMatch(const MatcherT&, const DeletedT&);

  void ResetLastMatch() { matched_depth_ = 0; }
};

} /* namespace matching */
} /* namespace dictionary */
} /* namespace keyvi */
#endif  // KEYVI_DICTIONARY_COMPLETION_PREFIX_COMPLETION_2_H_
