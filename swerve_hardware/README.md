# swerve_hardware

English | [中文](./README-CN.md)

---

ros2_control hardware interface plugin for the 4WS4WD swerve drive chassis.

## Description

This package implements a `hardware_interface::SystemInterface` plugin that communicates with:

- **RS06 steering motors** (position control via CAN extended frames)
- **UM series hub motors** (velocity control via CANopen CiA402 Profile Velocity mode)

The plugin is loaded by the ros2_control framework as `swerve_hardware/SwerveDriveHardware`.

## Key Features

- Manages 4 steering joints (position command/state) and 4 drive joints (velocity command, position/velocity state)
- CAN bus communication via SocketCAN (Linux)
- Configurable CAN watchdog for drive motor safety
- Steering angle limit enforcement
- TPDO periodic position/velocity feedback from UM motors
- Timeout error throttling for robust operation

## Hardware Parameters (URDF)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `steering_can_interface` | `can5` | CAN interface for RS06 motors (1 Mbps) |
| `driving_can_interface` | `can4` | CAN interface for UM motors (1 Mbps) |
| `um_position_resolution` | `2097152` | UM encoder counts per revolution (21-bit) |
| `wheel_radius` | `0.075` | Wheel radius in meters |
| `um_max_speed_rpm` | `330` | UM motor max speed (RPM) |
| `um_profile_accel_ms` | `1000` | PV mode acceleration ramp time (ms) |
| `um_profile_decel_ms` | `1000` | PV mode deceleration ramp time (ms) |
| `steering_speed_limit_rad_s` | `4.0` | RS06 steering speed limit (rad/s) |
| `um_can_watchdog_timeout_ms` | `100` | CAN watchdog timeout (ms, 0=disabled) |
| `um_can_watchdog_action` | `2` | Watchdog action: 0=alarm, 1=disable, 2=zero-speed |
| `um_tpdo_period_ms` | `20` | TPDO1 upload period (ms) |

## Joint Parameters (per joint in URDF)

| Parameter | Description |
|-----------|-------------|
| `motor_id` | RS06 motor ID or CANopen node ID |
| `direction` | 1 or -1 (rotation direction) |
| `zero_offset` | Steering zero position offset (rad) |
| `steering_min` | Min steering angle (rad) |
| `steering_max` | Max steering angle (rad) |

## Prerequisites

Set up CAN interfaces before launching:

```bash
sudo ip link set can5 type can bitrate 1000000 && sudo ip link set can5 up
sudo ip link set can4 type can bitrate 1000000 && sudo ip link set can4 up
```

## Build

```bash
cd ~/openflex_all/openflex_ws
colcon build --packages-select swerve_hardware
source install/setup.bash
```

## Dependencies

- `hardware_interface`
- `pluginlib`
- `rclcpp`
- `rclcpp_lifecycle`

## Notes

- This plugin is not launched directly; it is loaded by the `controller_manager` via the URDF `<ros2_control>` tag.
- Motor IDs: steering FL=5, FR=6, BL=7, BR=8; drive FL=1, FR=2, BL=3, BR=4.

## License

Apache-2.0
