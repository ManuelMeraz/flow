#pragma once

#include "flow/transformer.hpp"
#include "flow/producer.hpp"
#include "flow/consumer.hpp"
#include "flow/spinner.hpp"

namespace flow {

/**
 * Preferred way to make a routine from functions
 *
 * flow::make_spinner(function)
 * flow::make_producer(function, "channel_name_to_publish")
 * flow::make_consumer(function, "channel_name_to_subscribe")
 * flow::make_transformer(function, "subscriber_channel_name", "publisher_channel_name")
 */
template<flow::routine routine_t, typename... arguments_t>
auto make_routine(arguments_t&&... arguments)
{
  if constexpr (flow::transformer_routine<routine_t>) {
    return flow::make_transformer(std::forward<arguments_t>(arguments)...);
  }
  else if constexpr (flow::consumer_routine<routine_t>) {
    return flow::make_consumer(std::forward<arguments_t>(arguments)...);
  }
  else if constexpr (flow::producer_routine<routine_t>) {
    return flow::make_producer(std::forward<arguments_t>(arguments)...);
  }
  else if constexpr (flow::spinner_routine<routine_t>) {
    return flow::make_spinner(std::forward<arguments_t>(arguments)...);
  }
}
}// namespace flow