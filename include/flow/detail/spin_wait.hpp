#pragma once

#include <chrono>

#include <cppcoro/task.hpp>
#include <units/chrono.h>

namespace flow {
class spin_wait {
public:
  spin_wait(std::chrono::nanoseconds wait_time) : m_wait_time(wait_time) {}
  spin_wait(units::isq::Frequency auto frequency)
    : m_wait_time(units::quantity_cast<units::isq::si::nanosecond>(1 / frequency).number()) {}

  bool is_ready()
  {
    using namespace std::chrono;

    auto new_timestamp = std::chrono::steady_clock::now();
    auto time_delta = new_timestamp - m_last_timestamp;
    m_last_timestamp = new_timestamp;

    m_time_elapsed += duration_cast<nanoseconds>(time_delta);

    return m_time_elapsed >= m_wait_time;
  }

  void reset()
  {
    m_time_elapsed = decltype(m_time_elapsed){ 0 };
  }

  cppcoro::task<void> async_reset()
  {
    reset();
    co_return;
  }


  cppcoro::task<bool> async_is_ready()
  {
    co_return is_ready();
  }

private:
  std::chrono::steady_clock::time_point m_last_timestamp{ std::chrono::steady_clock::now() };

  std::chrono::nanoseconds m_wait_time{};
  std::chrono::nanoseconds m_time_elapsed{ 0 };
};
}// namespace flow