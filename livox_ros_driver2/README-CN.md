# Livox ROS Driver2 - MID360 使用说明

[English](./README.md) | 中文

---

本包用于启动 Livox MID360 的 ROS 2 驱动，并发布激光点云和内置 IMU 数据。

## 前置条件：配置有线网络

启动驱动前，必须先把连接 MID360 的有线网卡配置到正确网段。当前 `MID360_config.json` 中约定主机网络为：

| 项目 | 值 |
|---|---|
| 主机 IP | `192.168.1.50` |
| 子网掩码 | `255.255.255.0` |
| MID360 IP | `192.168.1XX` |

在 Ubuntu 图形界面中按下列步骤配置：

1. 打开 `设置` -> `网络`。
2. 选择连接 MID360 的有线网卡，例如 `eno1`。
3. 点击齿轮图标进入配置。
4. 打开 `IPv4` 页面。
5. 将 IPv4 方式改为 `手动`。
6. 添加地址：
   - 地址：`192.168.1.50`
   - 子网掩码：`255.255.255.0`
   - 网关：一般留空，除非当前网络环境要求填写
7. 点击 `应用`。
8. 重新插拔网线，或关闭再打开该有线连接。

如果没有完成这一步，驱动通常可以启动，但收不到 MID360 的点云和 IMU 数据。

也可以用命令行临时配置：

```bash
sudo ip addr flush dev eno1
sudo ip addr add 192.168.1.50/24 dev eno1
sudo ip link set eno1 up
```

如果实际有线网卡不是 `eno1`，请替换为实际网卡名。

## 网络配置文件

MID360 网络配置文件位置：

```bash
config/MID360_config.json
```

关键配置如下：

```json
"host_net_info": {
  "cmd_data_ip": "192.168.1.50",
  "point_data_ip": "192.168.1.50",
  "imu_data_ip": "192.168.1.50"
}
```

```json
"lidar_configs": [
  {
    "ip": "192.168.1.173"
  }
]
```

也就是说：电脑有线网卡应配置为 `192.168.1.50`，MID360 的设备 IP 为 `192.168.1.173`。

## 加载工作空间环境

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
```

## 启动 MID360 驱动

不启动 RViz，仅启动驱动：

```bash
ros2 launch livox_ros_driver2 msg_MID360_launch.py
```

对应 launch 文件：

```bash
launch_ROS2/msg_MID360_launch.py
```

默认参数：

| 参数 | 默认值 | 含义 |
|---|---|---|
| `xfer_format` | `1` | 发布 Livox 自定义点云消息 |
| `multi_topic` | `0` | 所有雷达共用一个点云话题和一个 IMU 话题 |
| `publish_freq` | `10.0` | 点云发布频率 |
| `frame_id` | `livox_frame` | 消息坐标系 |

## 启动驱动并打开 RViz

```bash
ros2 launch livox_ros_driver2 rviz_MID360_launch.py
```

该 launch 会启动驱动和 RViz，并且默认设置：

```bash
xfer_format = 0
```

因此 `/livox/lidar` 会发布为标准 ROS 点云 `sensor_msgs/msg/PointCloud2`。

## 发布的话题

默认 MID360 启动方式：

| 话题 | 类型 | 说明 |
|---|---|---|
| `/livox/lidar` | `livox_interfaces2/msg/CustomMsg` | 激光点云 |
| `/livox/imu` | `sensor_msgs/msg/Imu` | MID360 内置 IMU |

RViz 启动方式：

| 话题 | 类型 | 说明 |
|---|---|---|
| `/livox/lidar` | `sensor_msgs/msg/PointCloud2` | 标准 ROS 点云 |
| `/livox/imu` | `sensor_msgs/msg/Imu` | MID360 内置 IMU |

## 可选：使用 LivoxViewer2 可视化

也可以使用 Livox 官方上位机软件 LivoxViewer2 直接可视化查看激光雷达数据。

如果本机已经带有 OpenFlex 工具包，可运行：

```bash
cd "~/MID_360_nav/openflex_toolbox/LivoxViewer2 for Ubuntu v2.3.0"
./LivoxViewer2.sh
```

对应的可执行文件路径为：

```bash
~/MID_360_nav/openflex_toolbox/LivoxViewer2 for Ubuntu v2.3.0/LivoxViewer2/Binaries/Linux/LivoxViewer2
```

如果本机没有该工具，也可以从 Livox 官方下载页面下载 LivoxViewer2：

```text
https://www.livoxtech.com/downloads
```

注意：使用 LivoxViewer2 前也需要先完成有线网络配置，即当前配置下主机 IP 应为 `192.168.1.50/24`。

## 检查驱动输出

查看 Livox 话题：

```bash
ros2 topic list | grep livox
```

查看话题类型：

```bash
ros2 topic info /livox/lidar
ros2 topic info /livox/imu
```

查看发布频率：

```bash
ros2 topic hz /livox/lidar
ros2 topic hz /livox/imu
```

检查主机是否能连通 MID360：

```bash
ping 192.168.1.173
```

## 常见注意事项

- 如果 `/livox/lidar` 存在但没有频率，优先检查有线网卡 IP 是否为 `192.168.1.50`。
- 如果主机 IP 不想使用 `192.168.1.50`，需要同步修改 `config/MID360_config.json` 中的 `host_net_info`。
- 如果使用多台 Livox 雷达，可将 `multi_topic` 改为 `1`；话题会变成带 IP 后缀的形式，例如 `/livox/lidar_192_168_1_173` 和 `/livox/imu_192_168_1_173`。
