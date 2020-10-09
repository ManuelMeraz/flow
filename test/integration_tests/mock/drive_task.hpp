#pragma once


#include <flow/data_structures/static_vector.hpp>
#include <flow/data_structures/tick_function.hpp>
#include <flow/registry.hpp>
#include <flow/task.hpp>

#include <chrono>
#include <random>
#include <vector>

namespace mock {
template<typename config_t>
class drive_task final : public flow::task<drive_task<config_t>> {
public:
  void begin(auto& channel_registry)
  {
    // every publisher will tick concurrently
    static typename config_t::driver_t driver{};
    const auto on_request = [this](typename config_t::message_t& message) {
      m_tick();
      message = driver.drive();
    };

    std::generate_n(std::back_inserter(m_callback_handles), config_t::num_publishers, [&] {
      return flow::publish<typename config_t::message_t>(config_t::channel_name, channel_registry, on_request);
    });

    constexpr auto tick_cycle = config_t::total_messages + 1; // publisher sends one extra message at the end
    m_tick = flow::tick_function(tick_cycle + 1, [this] {
      flow::logging::info("Test complete: {} messages have been processed.", config_t::total_messages);
      m_callback_handles.front().stop_everything();// choose front arbitrarily
    });
  }

  void end() {}

private:
  std::vector<flow::callback_handle> m_callback_handles{};
  flow::tick_function m_tick{};
};
}// namespace mock
