/**
 * @File    swerve_drive_hardware.hpp
 * @Time    2026/02/24
 * @Author  OpenArmX
 * @Version 1.0
 * @Desc    ros2_control SystemInterface 硬件接口，管理 4 个 RS06 转向电机 + 4 个 UM 驱动电机
 */

#ifndef SWERVE_HARDWARE__SWERVE_DRIVE_HARDWARE_HPP_
#define SWERVE_HARDWARE__SWERVE_DRIVE_HARDWARE_HPP_

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp_lifecycle/state.hpp"

#include "swerve_hardware/can_bus_manager.hpp"
#include "swerve_hardware/rs06_driver.hpp"
#include "swerve_hardware/um_driver.hpp"

namespace swerve_hardware
{

/// ros2_control SystemInterface，四转四驱舵轮底盘硬件接口
///
/// CAN 总线配置（启动前需执行）:
///   sudo ip link set can5 type can bitrate 1000000 && sudo ip link set can5 up
///   sudo ip link set can4 type can bitrate 1000000 && sudo ip link set can4 up
///
/// URDF hardware_parameters:
///   steering_can_interface        — RS06 CAN 接口（默认 "can5", 1 Mbps）
///   driving_can_interface         — UM CAN 接口（默认 "can4", 1 Mbps）
///   um_position_resolution        — UM 位置反馈每圈计数（默认 2097152，即 21 位单圈）
///   wheel_radius                  — 车轮半径（默认 0.075 m）
///   um_max_speed_rpm              — UM 最高转速上限（默认 330 rpm，对应 UM10540）
///   um_profile_accel_ms           — UM PV 模式加速时间（默认 1000 ms）
///   um_profile_decel_ms           — UM PV 模式减速时间（默认 1000 ms）
///   um_can_watchdog_timeout_ms    — UM CAN 看门狗时间（默认 100 ms, Pn5.00B 范围 0~111）
///   um_can_watchdog_action        — UM CAN 看门狗动作（默认 2，即给零速）
///   um_tpdo_period_ms             — UM TPDO1 周期上报时间（默认 20 ms）
///
/// 每个 joint 的参数（在 URDF <joint> 标签中设置）:
///   motor_id   — RS06 电机 ID 或 CANopen 节点 ID
///   direction  — 1 或 -1（默认 1）
class SwerveDriveHardware : public hardware_interface::SystemInterface
{
public:
  RCLCPP_SHARED_PTR_DEFINITIONS(SwerveDriveHardware)

  hardware_interface::CallbackReturn on_init(
    const hardware_interface::HardwareInfo & info) override;

  hardware_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  // CAN 总线管理器
  CanBusManager steering_can_;   // can5 — RS06 (1 Mbps)
  CanBusManager driving_can_;    // can4 — UM (CANopen)

  // 电机驱动
  std::array<std::unique_ptr<RS06Driver>, 4> steering_drivers_;
  std::array<std::unique_ptr<UMDriver>, 4> driving_drivers_;
  std::array<uint8_t, 4> steer_motor_ids_ = {5, 6, 7, 8};
  std::array<uint8_t, 4> drive_node_ids_ = {1, 2, 3, 4};

  // 接口数据: [FL, FR, BL, BR]
  std::array<double, 4> steering_position_cmd_ = {};
  std::array<double, 4> steering_position_state_ = {};
  std::array<double, 4> steering_velocity_state_ = {};

  std::array<double, 4> driving_velocity_cmd_ = {};
  std::array<double, 4> driving_position_state_ = {};
  std::array<double, 4> driving_velocity_state_ = {};

  // 配置参数
  std::string steering_can_interface_;
  std::string driving_can_interface_;
  int um_position_resolution_ = 2097152;
  double wheel_radius_ = 0.075;
  double steering_speed_limit_rad_s_ = 15.0;
  int um_max_speed_rpm_ = 330;
  uint32_t um_profile_accel_ms_ = 1000;
  uint32_t um_profile_decel_ms_ = 1000;
  uint32_t um_can_watchdog_timeout_ms_ = 100;
  uint16_t um_can_watchdog_action_ = 2;
  uint16_t um_tpdo_period_ms_ = 20;
  double max_wheel_velocity_rad_s_ = 330.0 * 2.0 * 3.14159265358979323846 / 60.0;

  // 安全限位
  std::array<double, 4> steering_min_ = {-1.5708, -1.5708, -1.5708, -1.5708};
  std::array<double, 4> steering_max_ = { 1.5708,  1.5708,  1.5708,  1.5708};

  // 超时错误节流
  int consecutive_timeout_count_ = 0;
  static constexpr int kMaxConsecutiveTimeouts = 20;     // ~200ms at 100Hz
};

}  // namespace swerve_hardware

#endif  // SWERVE_HARDWARE__SWERVE_DRIVE_HARDWARE_HPP_
