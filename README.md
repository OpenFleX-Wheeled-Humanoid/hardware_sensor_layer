# Hardware and Sensor Adapter Layer

English | [中文](./README.zh-CN.md)

---

This layer connects physical devices to the ROS 2 graph. It contains the chassis ros2_control hardware interface and the Livox MID-360 driver.

## Packages

- `swerve_hardware`: ros2_control `SystemInterface` for 4 RS06 steering motors and 4 UM integrated wheel motors.
- `livox_ros_driver2`: ROS 2 driver for Livox 3D LiDAR devices, used here for MID-360 point cloud and IMU input.

## Boundary

This layer handles device communication, protocol parsing, command conversion, state conversion, and sensor publishing. Kinematics, odometry fusion, mapping, localization, and navigation policies belong to upper layers.

## Hardware Conventions

- RS06 steering motors: CAN5, extended CAN frames, position command.
- UM wheel motors: CAN4, CANopen CiA402 Profile Velocity mode.
- UM encoder resolution defaults to 21-bit absolute position: `2097152` counts/rev.
- UM TPDO1 maps actual velocity `6069h` and actual position `6063h`.
- MID-360 data is consumed by the mapping and localization layer.

## License

This package is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).

Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.

For details, please refer to the [LICENSE](LICENSE) file or visit: http://creativecommons.org/licenses/by-nc-sa/4.0/

## Acknowledgments

This package is part of the OpenFlex full-body humanoid robot platform ecosystem, developed specifically for research and industrial applications in the humanoid robotics field.

---

## 📞 Contact Us

### Chengdu Changshu Robot Co., Ltd.
**Chengdu Changshu Robotics Co., Ltd.**

| Contact | Information |
|---------|-------------|
| 📧 Email | openarmrobot@gmail.com |
| 📱 Phone/WeChat | +86-17746530375 |
| 🌐 Website | https://openarmx.com/ |
| 🌐 Docs | http://docs.openarmx.com/ |
| 📍 Address | Tianjin Xiqing District · Daochao Robot Experience Base (City of Tomorrow) · Tianjin Humanoid Robot Center |
| 👤 Contact Person | Mr. Wang |
