/**
 * @File    swerve_drive_hardware.cpp
 * @Time    2026/02/24
 * @Author  OpenArmX
 * @Version 1.0
 * @Desc    ros2_control SystemInterface 实现，管理 4 个 RS06 + 4 个 UM 电机
 */

#include "swerve_hardware/swerve_drive_hardware.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <string>
#include <thread>
#include <chrono>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

#include "swerve_hardware/rs06_driver.hpp"
#include "swerve_hardware/um_driver.hpp"

namespace swerve_hardware
{

// 模块顺序: [FL, FR, BL, BR]
static const std::array<std::string, 4> kModulePrefixes = {"fl", "fr", "bl", "br"};


hardware_interface::CallbackReturn SwerveDriveHardware::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SystemInterface::on_init(info) !=
      hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  // 验证关节数量: 4 转向 + 4 驱动 = 8 个关节
  if (info_.joints.size() != 8) {
    RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
      "Expected 8 joints (4 steering + 4 wheel), got %zu", info_.joints.size());
    return hardware_interface::CallbackReturn::ERROR;
  }

  // 读取硬件级参数
  steering_can_interface_ = info_.hardware_parameters.count("steering_can_interface")
    ? info_.hardware_parameters.at("steering_can_interface") : "can5";

  driving_can_interface_ = info_.hardware_parameters.count("driving_can_interface")
    ? info_.hardware_parameters.at("driving_can_interface") : "can4";

  um_position_resolution_ = info_.hardware_parameters.count("um_position_resolution")
    ? std::stoi(info_.hardware_parameters.at("um_position_resolution"))
    : (info_.hardware_parameters.count("driving_encoder_resolution")
      ? std::stoi(info_.hardware_parameters.at("driving_encoder_resolution"))
      : 2097152);
  if (um_position_resolution_ <= 0) {
    RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
      "um_position_resolution must be > 0, got %d", um_position_resolution_);
    return hardware_interface::CallbackReturn::ERROR;
  }

  wheel_radius_ = info_.hardware_parameters.count("wheel_radius")
    ? std::stod(info_.hardware_parameters.at("wheel_radius")) : 0.075;
  steering_speed_limit_rad_s_ = info_.hardware_parameters.count("steering_speed_limit_rad_s")
    ? std::stod(info_.hardware_parameters.at("steering_speed_limit_rad_s")) : 15.0;
  um_max_speed_rpm_ = info_.hardware_parameters.count("um_max_speed_rpm")
    ? std::stoi(info_.hardware_parameters.at("um_max_speed_rpm"))
    : 330;
  um_profile_accel_ms_ = info_.hardware_parameters.count("um_profile_accel_ms")
    ? static_cast<uint32_t>(std::stoul(info_.hardware_parameters.at("um_profile_accel_ms")))
    : 1000;
  um_profile_decel_ms_ = info_.hardware_parameters.count("um_profile_decel_ms")
    ? static_cast<uint32_t>(std::stoul(info_.hardware_parameters.at("um_profile_decel_ms")))
    : 1000;
  um_can_watchdog_timeout_ms_ = info_.hardware_parameters.count("um_can_watchdog_timeout_ms")
    ? static_cast<uint32_t>(std::stoul(info_.hardware_parameters.at("um_can_watchdog_timeout_ms")))
    : 100;
  um_can_watchdog_action_ = info_.hardware_parameters.count("um_can_watchdog_action")
    ? static_cast<uint16_t>(std::stoul(info_.hardware_parameters.at("um_can_watchdog_action")))
    : 2;
  um_tpdo_period_ms_ = info_.hardware_parameters.count("um_tpdo_period_ms")
    ? static_cast<uint16_t>(std::stoul(info_.hardware_parameters.at("um_tpdo_period_ms")))
    : 20;
  if (um_max_speed_rpm_ <= 0) {
    RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
      "um_max_speed_rpm must be > 0, got %d", um_max_speed_rpm_);
    return hardware_interface::CallbackReturn::ERROR;
  }
  if (um_can_watchdog_action_ > 2) {
    RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
      "um_can_watchdog_action must be 0, 1 or 2, got %u", um_can_watchdog_action_);
    return hardware_interface::CallbackReturn::ERROR;
  }
  max_wheel_velocity_rad_s_ =
    static_cast<double>(um_max_speed_rpm_) * 2.0 * M_PI / 60.0;

  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
    "CAN: steering=%s (1Mbps), driving=%s (UM CANopen)",
    steering_can_interface_.c_str(), driving_can_interface_.c_str());
  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
    "UM position_resolution=%d, wheel_radius=%.4f m, max_speed=%d rpm (%.2f rad/s), profile_accel=%u ms, profile_decel=%u ms",
    um_position_resolution_, wheel_radius_, um_max_speed_rpm_,
    max_wheel_velocity_rad_s_, um_profile_accel_ms_, um_profile_decel_ms_);
  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
    "RS06 steering speed limit=%.2f rad/s",
    steering_speed_limit_rad_s_);
  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
    "UM CAN watchdog: timeout=%u ms action=%u tpdo_period=%u ms",
    um_can_watchdog_timeout_ms_, um_can_watchdog_action_, um_tpdo_period_ms_);

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SwerveDriveHardware::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // 按名称前缀映射关节 (fl, fr, bl, br) 和类型 (steering, wheel)
  std::array<int, 4> steering_joint_idx = {-1, -1, -1, -1};
  std::array<int, 4> wheel_joint_idx = {-1, -1, -1, -1};

  for (size_t j = 0; j < info_.joints.size(); ++j) {
    const auto & joint = info_.joints[j];
    for (size_t m = 0; m < 4; ++m) {
      if (joint.name.find(kModulePrefixes[m]) != std::string::npos) {
        if (joint.name.find("steering") != std::string::npos) {
          steering_joint_idx[m] = static_cast<int>(j);
        } else if (joint.name.find("wheel") != std::string::npos) {
          wheel_joint_idx[m] = static_cast<int>(j);
        }
      }
    }
  }

  // 验证所有关节已找到
  for (size_t m = 0; m < 4; ++m) {
    if (steering_joint_idx[m] < 0 || wheel_joint_idx[m] < 0) {
      RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
        "Missing joint for module %s (steering=%d, wheel=%d)",
        kModulePrefixes[m].c_str(), steering_joint_idx[m], wheel_joint_idx[m]);
      return hardware_interface::CallbackReturn::ERROR;
    }
  }

  // 打开 CAN 总线
  if (!steering_can_.open(steering_can_interface_)) {
    RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
      "Failed to open steering CAN: %s", steering_can_interface_.c_str());
    return hardware_interface::CallbackReturn::ERROR;
  }

  if (!driving_can_.open(driving_can_interface_)) {
    RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
      "Failed to open driving CAN: %s", driving_can_interface_.c_str());
    steering_can_.close();
    return hardware_interface::CallbackReturn::ERROR;
  }

  // 设置 CAN 过滤器（延后到 motor_id 读取后）

  // 创建电机驱动实例
  std::array<uint8_t, 4> steer_ids{};
  std::array<uint8_t, 4> drive_ids{};
  for (size_t m = 0; m < 4; ++m) {
    const auto & steer_joint = info_.joints[steering_joint_idx[m]];
    const auto & wheel_joint = info_.joints[wheel_joint_idx[m]];

    // 读取每个关节的 motor_id 和 direction 参数
    uint8_t steer_motor_id = steer_joint.parameters.count("motor_id")
      ? static_cast<uint8_t>(std::stoi(steer_joint.parameters.at("motor_id")))
      : static_cast<uint8_t>(m + 5);
    int steer_direction = steer_joint.parameters.count("direction")
      ? std::stoi(steer_joint.parameters.at("direction")) : 1;
    float steer_zero_offset = steer_joint.parameters.count("zero_offset")
      ? std::stof(steer_joint.parameters.at("zero_offset")) : 0.0f;

    uint8_t drive_node_id = wheel_joint.parameters.count("motor_id")
      ? static_cast<uint8_t>(std::stoi(wheel_joint.parameters.at("motor_id")))
      : static_cast<uint8_t>(m + 1);
    int drive_direction = wheel_joint.parameters.count("direction")
      ? std::stoi(wheel_joint.parameters.at("direction")) : 1;

    // 读取转向限位
    if (steer_joint.parameters.count("steering_min")) {
      steering_min_[m] = std::stod(steer_joint.parameters.at("steering_min"));
    }
    if (steer_joint.parameters.count("steering_max")) {
      steering_max_[m] = std::stod(steer_joint.parameters.at("steering_max"));
    }

    steering_drivers_[m] = std::make_unique<RS06Driver>(
      steer_motor_id, steering_can_, steer_direction,
      static_cast<float>(steering_speed_limit_rad_s_), steer_zero_offset);

    driving_drivers_[m] = std::make_unique<UMDriver>(
      drive_node_id, driving_can_,
      um_position_resolution_, drive_direction, um_max_speed_rpm_,
      um_profile_accel_ms_, um_profile_decel_ms_,
      um_can_watchdog_timeout_ms_, um_can_watchdog_action_, um_tpdo_period_ms_);

    steer_ids[m] = steer_motor_id;
    drive_ids[m] = drive_node_id;

    RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
      "Module %s: RS06(id=%d,dir=%d,zero_offset=%.4f) UM(id=%d,dir=%d) steer_limit=[%.3f,%.3f]",
      kModulePrefixes[m].c_str(),
      steer_motor_id, steer_direction, steer_zero_offset, drive_node_id, drive_direction,
      steering_min_[m], steering_max_[m]);
  }

  // CAN 过滤器按实际 motor_id 安装（必须在实例化之后，open 之前的发送不会受影响）
  configure_swerve_can_filters(steering_can_, driving_can_, steer_ids, drive_ids);
  steer_motor_ids_ = steer_ids;
  drive_node_ids_ = drive_ids;

  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface>
SwerveDriveHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> interfaces;

  for (size_t j = 0; j < info_.joints.size(); ++j) {
    const auto & joint = info_.joints[j];

    // 查找模块索引
    int m = -1;
    bool is_steering = false;
    for (size_t k = 0; k < 4; ++k) {
      if (joint.name.find(kModulePrefixes[k]) != std::string::npos) {
        m = static_cast<int>(k);
        is_steering = (joint.name.find("steering") != std::string::npos);
        break;
      }
    }
    if (m < 0) continue;

    if (is_steering) {
      interfaces.emplace_back(joint.name, hardware_interface::HW_IF_POSITION,
        &steering_position_state_[m]);
      interfaces.emplace_back(joint.name, hardware_interface::HW_IF_VELOCITY,
        &steering_velocity_state_[m]);
    } else {
      interfaces.emplace_back(joint.name, hardware_interface::HW_IF_POSITION,
        &driving_position_state_[m]);
      interfaces.emplace_back(joint.name, hardware_interface::HW_IF_VELOCITY,
        &driving_velocity_state_[m]);
    }
  }

  return interfaces;
}

std::vector<hardware_interface::CommandInterface>
SwerveDriveHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> interfaces;

  for (size_t j = 0; j < info_.joints.size(); ++j) {
    const auto & joint = info_.joints[j];

    int m = -1;
    bool is_steering = false;
    for (size_t k = 0; k < 4; ++k) {
      if (joint.name.find(kModulePrefixes[k]) != std::string::npos) {
        m = static_cast<int>(k);
        is_steering = (joint.name.find("steering") != std::string::npos);
        break;
      }
    }
    if (m < 0) continue;

    if (is_steering) {
      interfaces.emplace_back(joint.name, hardware_interface::HW_IF_POSITION,
        &steering_position_cmd_[m]);
    } else {
      interfaces.emplace_back(joint.name, hardware_interface::HW_IF_VELOCITY,
        &driving_velocity_cmd_[m]);
    }
  }

  return interfaces;
}

hardware_interface::CallbackReturn SwerveDriveHardware::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"), "Activating: initializing motors...");

  if (!steering_can_.is_open()) {
    if (!steering_can_.open(steering_can_interface_)) {
      RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
        "Failed to reopen steering CAN: %s", steering_can_interface_.c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }
  }

  if (!driving_can_.is_open()) {
    if (!driving_can_.open(driving_can_interface_)) {
      RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
        "Failed to reopen driving CAN: %s", driving_can_interface_.c_str());
      steering_can_.close();
      return hardware_interface::CallbackReturn::ERROR;
    }
  }

  configure_swerve_can_filters(steering_can_, driving_can_, steer_motor_ids_, drive_node_ids_);

  const auto stop_all_motors = [this]() {
    for (auto & driver : driving_drivers_) {
      if (driver) {
        driver->stop();
      }
    }
    for (auto & driver : steering_drivers_) {
      if (driver) {
        driver->stop();
      }
    }
    steering_can_.close();
    driving_can_.close();
  };

  for (size_t m = 0; m < 4; ++m) {
    // 每个电机 init 前短暂让位，降低残留帧干扰但避免激活过慢
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (!steering_drivers_[m]->init()) {
      RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
        "Failed to init RS06 steering motor %s", kModulePrefixes[m].c_str());
      stop_all_motors();
      return hardware_interface::CallbackReturn::ERROR;
    }
    RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
      "RS06 %s initialized", kModulePrefixes[m].c_str());
  }

  for (size_t m = 0; m < 4; ++m) {
    if (!driving_drivers_[m]->init()) {
      RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
        "Failed to init UM driving motor %s", kModulePrefixes[m].c_str());
      stop_all_motors();
      return hardware_interface::CallbackReturn::ERROR;
    }
    RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
      "UM %s initialized", kModulePrefixes[m].c_str());
  }

  // 读取驱动轮初始状态
  UMDriver::drain_bus(driving_can_);
  for (size_t m = 0; m < 4; ++m) {
    double pos, vel;
    driving_drivers_[m]->read(pos, vel);
    driving_velocity_cmd_[m] = 0.0;
    driving_position_state_[m] = pos;
    driving_velocity_state_[m] = vel;
  }

  // 转向轮归零：命令所有转向电机回到机械零位（0.0 rad = 车轮水平向前）
  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
    "Homing: commanding all steering motors to zero position...");
  for (size_t m = 0; m < 4; ++m) {
    steering_position_cmd_[m] = 0.0;
    steering_drivers_[m]->write(0.0);
  }

  // 等待所有转向轮到达零位
  constexpr double kHomingToleranceRad = 0.05;   // 到位容差 ~2.9°
  constexpr int kHomingTimeoutMs = 3000;         // 最长等待 3 秒
  constexpr int kHomingPollMs = 20;              // 轮询间隔
  int elapsed_ms = 0;
  bool homed = false;

  while (elapsed_ms < kHomingTimeoutMs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(kHomingPollMs));
    elapsed_ms += kHomingPollMs;

    // 持续发送零位命令（确保 CAN 帧不丢失）
    for (size_t m = 0; m < 4; ++m) {
      steering_drivers_[m]->write(0.0);
    }

    // 读取当前位置
    RS06Driver::request_all_feedback(steering_can_);
    RS06Driver::drain_bus(steering_can_);
    bool all_at_zero = true;
    for (size_t m = 0; m < 4; ++m) {
      double pos, vel;
      steering_drivers_[m]->read(pos, vel);
      steering_position_state_[m] = pos;
      steering_velocity_state_[m] = vel;
      if (std::abs(pos) > kHomingToleranceRad) {
        all_at_zero = false;
      }
    }

    if (all_at_zero) {
      homed = true;
      break;
    }
  }

  if (homed) {
    RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"),
      "Homing complete: all steering motors at zero position (%d ms)", elapsed_ms);
  } else {
    RCLCPP_WARN(rclcpp::get_logger("SwerveDriveHardware"),
      "Homing timeout after %d ms. Current positions: FL=%.3f FR=%.3f BL=%.3f BR=%.3f rad",
      kHomingTimeoutMs,
      steering_position_state_[0], steering_position_state_[1],
      steering_position_state_[2], steering_position_state_[3]);
    RCLCPP_WARN(rclcpp::get_logger("SwerveDriveHardware"),
      "Continuing with current positions. Consider re-calibrating with set_zero_rs06.py");
  }

  // 初始化转向命令为零位
  for (size_t m = 0; m < 4; ++m) {
    steering_position_cmd_[m] = 0.0;
  }

  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"), "All 8 motors activated");
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SwerveDriveHardware::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"), "Deactivating: stopping all motors...");

  for (size_t m = 0; m < 4; ++m) {
    driving_drivers_[m]->stop();
    steering_drivers_[m]->stop();
  }

  steering_can_.close();
  driving_can_.close();

  RCLCPP_INFO(rclcpp::get_logger("SwerveDriveHardware"), "All motors stopped, CAN closed");
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::return_type SwerveDriveHardware::read(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  // 每条总线只 drain 一次：批量请求 + 排空
  RS06Driver::request_all_feedback(steering_can_);
  RS06Driver::drain_bus(steering_can_);
  UMDriver::drain_bus(driving_can_);

  int timeout_count = 0;

  for (size_t m = 0; m < 4; ++m) {
    steering_drivers_[m]->read(steering_position_state_[m], steering_velocity_state_[m]);
    if (steering_drivers_[m]->has_timeout()) ++timeout_count;

    driving_drivers_[m]->read(driving_position_state_[m], driving_velocity_state_[m]);
    if (driving_drivers_[m]->has_timeout()) ++timeout_count;
  }

  // 超时计数和错误报告
  if (timeout_count > 0) {
    ++consecutive_timeout_count_;
    if (consecutive_timeout_count_ % 100 == 1) {
      RCLCPP_WARN(rclcpp::get_logger("SwerveDriveHardware"),
        "%d motors timed out (consecutive=%d)", timeout_count, consecutive_timeout_count_);
    }
  } else {
    consecutive_timeout_count_ = 0;
  }

  if (timeout_count >= 8 && consecutive_timeout_count_ >= kMaxConsecutiveTimeouts) {
    RCLCPP_ERROR(rclcpp::get_logger("SwerveDriveHardware"),
      "All motors timed out for %d consecutive cycles", consecutive_timeout_count_);
    return hardware_interface::return_type::ERROR;
  }

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type SwerveDriveHardware::write(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  // 发驱动命令前先排空 CAN4 上积压的 SDO 回复帧，防止堵塞
  UMDriver::drain_bus(driving_can_);

  for (size_t m = 0; m < 4; ++m) {
    // 转向位置安全限幅
    double steer_cmd = std::clamp(
      steering_position_cmd_[m], steering_min_[m], steering_max_[m]);
    steering_drivers_[m]->write(steer_cmd);

    // 驱动速度安全限幅
    double drive_cmd = std::clamp(
      driving_velocity_cmd_[m], -max_wheel_velocity_rad_s_, max_wheel_velocity_rad_s_);
    driving_drivers_[m]->write(drive_cmd);
  }
  return hardware_interface::return_type::OK;
}

}  // namespace swerve_hardware

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(
  swerve_hardware::SwerveDriveHardware,
  hardware_interface::SystemInterface)
