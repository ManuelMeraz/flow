#pragma once


#include <flow/data_structures/static_vector.hpp>
#include <flow/data_structures/tick_function.hpp>
#include <flow/registry.hpp>
#include <flow/task.hpp>

#include <chrono>
#include <random>
#include <vector>

#include "lidar_driver.hpp"

namespace {
static constexpr std::size_t TOTAL_MESSAGES = mock::defaults::total_messages;
mock::lidar_driver g_driver{};
}// namespace

namespace mock {
template<std::size_t total_messages_t = mock::defaults::total_messages, std::size_t num_tasks = mock::defaults::num_publishers>
class lidar_drive_task final : public flow::task<lidar_drive_task<total_messages_t, num_tasks>> {
public:
  void begin(auto& channel_registry)
  {
    // every publisher will tick concurrently
    const auto on_request = [this](lidar_message& message) {
      m_tick();
      message = g_driver.drive();
    };

    std::generate_n(std::back_inserter(m_callback_handles), num_tasks, [&] {
      return flow::publish<lidar_message>("lidar_points", channel_registry, on_request);
    });

    constexpr auto tick_cycle = total_messages_t;
    m_tick = flow::tick_function(tick_cycle, [this] {
      flow::logging::info("Test complete: {} messages have been processed.", total_messages_t);
      m_callback_handles.front().stop_everything();// choose front arbitrarily
    });
  }

  std::vector<flow::callback_handle> m_callback_handles{};
  flow::tick_function m_tick{};
};
}// namespace mock