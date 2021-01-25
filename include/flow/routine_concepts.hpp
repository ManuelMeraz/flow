#pragma once

#include "flow/detail/metaprogramming.hpp"
#include "flow/configuration.hpp"

#include <concepts>

/**
 * A callable_routine is a core concept of this framework; a building block of a network.
 *
 * A callable_routine is a callable where it's dependencies (arguments) and return type define its category
 */

namespace flow {
template<typename callable_t>
using traits = detail::metaprogramming::function_traits<std::decay_t<callable_t>>;

template <typename configuration_t>
class network;

template<typename routine_t, typename network_t = flow::network<flow::configuration>>
concept is_user_routine = requires(routine_t routine, network_t network){
  routine.initialize(network);
};

struct consumer {};
struct spinner {};
struct producer {};
struct transformer {};

template<typename transformer_t>
concept transformer_routine = std::is_same_v<typename transformer_t::is_transformer, std::true_type> or std::is_same_v<transformer_t, flow::transformer>;

template<typename spinner_t>
concept spinner_routine = std::is_same_v<typename spinner_t::is_spinner, std::true_type> or std::is_same_v<spinner_t, flow::spinner>;

template<typename producer_t>
concept producer_routine = std::is_same_v<typename producer_t::is_producer, std::true_type> or std::is_same_v<producer_t, flow::producer>;

template<typename consumer_t>
concept consumer_routine = std::is_same_v<typename consumer_t::is_consumer, std::true_type> or std::is_same_v<consumer_t, flow::consumer>;

template<typename routine_t>
concept routine = spinner_routine<routine_t> or producer_routine<routine_t> or consumer_routine<routine_t> or transformer_routine<routine_t>;

template <typename network_t>
concept is_network = std::is_same_v<typename network_t::is_network, std::true_type>;

/**
 * A spinner_function is a callable which has a void return type and requires no arguments
 * @tparam callable_t Any callable type
 */
template<typename callable_t>
concept spinner_function = not is_network<callable_t> and not routine<callable_t> and not is_user_routine<callable_t> and traits<callable_t>::arity == 0 and std::is_void_v<typename traits<callable_t>::return_type>;

/**
 * A producer_function is a callable which has a return type and requires no arguments
 * @tparam callable_t Any callable type
 */
template<typename callable_t>
concept producer_function = not is_network<callable_t> and not routine<callable_t> and not is_user_routine<callable_t> and traits<callable_t>::arity == 0 and not std::is_void_v<typename traits<callable_t>::return_type>;

/**
 * A consumer_function is a callable which has no return type and requires at least one argument
 * @tparam callable_t Any callable type
 */
template<typename callable_t>
concept consumer_function = not is_network<callable_t> and not routine<callable_t> and not is_user_routine<callable_t> and traits<callable_t>::arity >= 1 and std::is_void_v<typename traits<callable_t>::return_type>;

/**
 * A transformer_function is a callable which has a return type and requires at least one argument
 * @tparam callable_t Any callable type
 */
template<typename callable_t>
concept transformer_function = not is_network<callable_t> and not routine<callable_t> and not is_user_routine<callable_t> and traits<callable_t>::arity >= 1 and not std::is_void_v<typename traits<callable_t>::return_type>;

template<typename callable_t>
concept function = spinner_function<callable_t> or producer_function<callable_t> or consumer_function<callable_t> or transformer_function<callable_t>;

template<typename... callables_t>
concept functions = (function<callables_t> and ...);
}// namespace flow
