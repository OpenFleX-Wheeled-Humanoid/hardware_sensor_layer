# 硬件与传感器适配层

[English](./README.md) | 中文

---

本层负责把真实硬件和 ROS 2 计算图连接起来，包括舵轮底盘电机硬件接口和 Livox MID-360 雷达驱动。

## 包清单

- `swerve_hardware`: ros2_control `SystemInterface`，管理 4 个 RS06 转向电机和 4 个 UM 轮毂一体机。
- `livox_ros_driver2`: Livox 3D LiDAR ROS 2 驱动，提供 MID-360 点云和 IMU 数据输入。

## 职责边界

本层只做设备通信、协议解析、状态/命令转换和传感器数据发布。运动学解算、里程计融合、建图定位和导航策略不放在本层。

## 关键硬件约定

- RS06 转向电机：CAN5，扩展帧，位置命令。
- UM 轮毂一体机：CAN4，CANopen CiA402 Profile Velocity 模式。
- UM 编码器默认 21 位单圈绝对值，`um_position_resolution=2097152`。
- UM TPDO1 发布实际速度 `6069h` 和实际位置 `6063h`。
- MID-360 数据由 `livox_ros_driver2` 提供给建图定位层。

## 许可证

本包通过 知识共享 署名-非商业性使用-相同方式共享 4.0 国际许可协议 (CC BY-NC-SA 4.0) 进行许可。

版权所有 (c) 2026 成都长数机器人有限公司 (Chengdu Changshu Robot Co., Ltd.)

详情请参阅 [LICENSE](LICENSE) 文件或访问：http://creativecommons.org/licenses/by-nc-sa/4.0/

## 致谢

本包是 OpenFlex 全身人形机器人平台生态系统的一部分，专为人形机器人领域的研究和工业应用而开发。

---

## 📞 联系我们

### 成都长数机器人有限公司
**Chengdu Changshu Robotics Co., Ltd.**

| 联系方式 | 信息 |
|---------|------|
| 📧 邮箱 | openarmrobot@gmail.com |
| 📱 电话/微信 | +86-17746530375 |
| 🌐 官网 | https://openarmx.com/ |
| 🌐 文档 | http://docs.openarmx.com/ |
| 📍 地址 | 天津市西青区・稻潮机器人体验基地（明日之城）・天津市人形机器人中心 |
| 👤 联系人 | 王先生 |
