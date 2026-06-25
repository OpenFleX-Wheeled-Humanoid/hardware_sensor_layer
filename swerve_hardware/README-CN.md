# swerve_hardware

[English](./README.md) | 中文

---

四转四驱舵轮底盘 ros2_control 硬件接口插件。

## 简介

本包实现了一个 `hardware_interface::SystemInterface` 插件，与以下硬件通信：

- **灵足时代 RS06 转向电机**（CAN 扩展帧位置控制）
- **和利时 UM 系列伺服轮毂一体机**（CANopen CiA402 Profile Velocity 速度模式）

插件由 ros2_control 框架以 `swerve_hardware/SwerveDriveHardware` 名称加载。

## 主要功能

- 管理 4 个转向关节（位置命令/状态）和 4 个驱动关节（速度命令、位置/速度状态）
- 通过 SocketCAN（Linux）进行 CAN 总线通信
- 可配置 CAN 看门狗保护驱动电机安全
- 转向角度限位保护
- UM 电机 TPDO 周期性位置/速度反馈
- 超时错误节流，确保稳健运行

## 硬件参数（URDF 配置）

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `steering_can_interface` | `can5` | RS06 电机 CAN 接口（1 Mbps） |
| `driving_can_interface` | `can4` | UM 电机 CAN 接口（1 Mbps） |
| `um_position_resolution` | `2097152` | UM 编码器每圈计数（21 位） |
| `wheel_radius` | `0.075` | 车轮半径（m） |
| `um_max_speed_rpm` | `330` | UM 电机最高转速（RPM） |
| `um_profile_accel_ms` | `1000` | PV 模式加速时间（ms） |
| `um_profile_decel_ms` | `1000` | PV 模式减速时间（ms） |
| `steering_speed_limit_rad_s` | `4.0` | RS06 转向速度上限（rad/s） |
| `um_can_watchdog_timeout_ms` | `100` | CAN 看门狗超时时间（ms，0=关闭） |
| `um_can_watchdog_action` | `2` | 看门狗动作：0=报警，1=关使能，2=给零速 |
| `um_tpdo_period_ms` | `20` | TPDO1 周期上报时间（ms） |

## 关节参数（URDF 中每个关节）

| 参数 | 说明 |
|------|------|
| `motor_id` | RS06 电机 ID 或 CANopen 节点 ID |
| `direction` | 1 或 -1（旋转方向） |
| `zero_offset` | 转向零位偏移（rad） |
| `steering_min` | 最小转向角（rad） |
| `steering_max` | 最大转向角（rad） |

## 前置条件

启动前需配置 CAN 接口：

```bash
sudo ip link set can5 type can bitrate 1000000 && sudo ip link set can5 up
sudo ip link set can4 type can bitrate 1000000 && sudo ip link set can4 up
```

## 编译

```bash
cd ~/openflex_all/openflex_ws
colcon build --packages-select swerve_hardware
source install/setup.bash
```

## 依赖

- `hardware_interface`
- `pluginlib`
- `rclcpp`
- `rclcpp_lifecycle`

## 说明

- 本插件不直接启动，由 `controller_manager` 通过 URDF `<ros2_control>` 标签加载。
- 电机 ID：转向 FL=5、FR=6、BL=7、BR=8；驱动 FL=1、FR=2、BL=3、BR=4。

## 许可证

Apache-2.0
