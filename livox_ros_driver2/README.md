# Livox ROS Driver2 - MID360

English | [中文](./README-CN.md)

---

This package starts the Livox MID360 ROS 2 driver and publishes LiDAR point cloud and IMU data.

## Prerequisite: Configure Wired Network

Before starting the driver, configure the wired network interface connected to the MID360. The current `MID360_config.json` expects the host computer to use:

| Item | Value |
|---|---|
| Host IP | `192.168.1.50` |
| Netmask | `255.255.255.0` |
| MID360 IP | `192.168.1.173` |

In Ubuntu Settings:

1. Open `Settings` -> `Network`.
2. Select the wired interface, for example `eno1`.
3. Open the gear icon.
4. Go to `IPv4`.
5. Set IPv4 method to `Manual`.
6. Add:
   - Address: `192.168.1.50`
   - Netmask: `255.255.255.0`
   - Gateway: leave empty unless your network requires one
7. Click `Apply`.
8. Reconnect the wired network or toggle it off and on.

The driver will not receive data correctly until this network step is complete.

Optional terminal equivalent:

```bash
sudo ip addr flush dev eno1
sudo ip addr add 192.168.1.50/24 dev eno1
sudo ip link set eno1 up
```

Replace `eno1` with the actual wired interface name if needed.

## Network Configuration File

The MID360 network configuration is stored in:

```bash
config/MID360_config.json
```

Important values:

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

## Source Workspace

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
```

## Start MID360 Driver

Start the MID360 driver without RViz:

```bash
ros2 launch livox_ros_driver2 msg_MID360_launch.py
```

This launch uses:

```bash
launch_ROS2/msg_MID360_launch.py
```

Default parameters:

| Parameter | Default | Meaning |
|---|---|---|
| `xfer_format` | `1` | Publish Livox custom point cloud message |
| `multi_topic` | `0` | All LiDARs share one point cloud topic and one IMU topic |
| `publish_freq` | `10.0` | Point cloud publish frequency |
| `frame_id` | `livox_frame` | Message frame id |

## Start with RViz

```bash
ros2 launch livox_ros_driver2 rviz_MID360_launch.py
```

This launch starts the driver and RViz. It also sets:

```bash
xfer_format = 0
```

So `/livox/lidar` is published as `sensor_msgs/msg/PointCloud2`.

## Published Topics

Default MID360 launch:

| Topic | Type | Description |
|---|---|---|
| `/livox/lidar` | `livox_interfaces2/msg/CustomMsg` | LiDAR point cloud |
| `/livox/imu` | `sensor_msgs/msg/Imu` | MID360 built-in IMU |

RViz launch:

| Topic | Type | Description |
|---|---|---|
| `/livox/lidar` | `sensor_msgs/msg/PointCloud2` | Standard ROS point cloud |
| `/livox/imu` | `sensor_msgs/msg/Imu` | MID360 built-in IMU |

## Optional: Visualize with LivoxViewer2

You can also use the official LivoxViewer2 desktop tool to visualize LiDAR data directly.

If the OpenFlex toolbox is available locally, run:

```bash
cd "~/MID_360_nav/openflex_toolbox/LivoxViewer2 for Ubuntu v2.3.0"
./LivoxViewer2.sh
```

The executable is also located at:

```bash
~/MID_360_nav/openflex_toolbox/LivoxViewer2 for Ubuntu v2.3.0/LivoxViewer2/Binaries/Linux/LivoxViewer2
```

If the local toolbox is not available, download LivoxViewer2 from the official Livox download page:

```text
https://www.livoxtech.com/downloads
```

The same wired network prerequisite still applies: configure the host IP as `192.168.1.50/24` before using LivoxViewer2 with the current MID360 setup.

## Check Driver Output

```bash
ros2 topic list | grep livox
ros2 topic info /livox/lidar
ros2 topic info /livox/imu
ros2 topic hz /livox/lidar
ros2 topic hz /livox/imu
```

Check whether the host can reach the MID360:

```bash
ping 192.168.1.173
```

## Notes

- If `/livox/lidar` exists but has no frequency, check the wired IP first.
- If the host IP is not `192.168.1.50`, update `config/MID360_config.json` or change the network interface IP.
- If multiple LiDARs are used, set `multi_topic = 1`; topics become IP-suffixed, for example `/livox/lidar_192_168_1_173` and `/livox/imu_192_168_1_173`.
