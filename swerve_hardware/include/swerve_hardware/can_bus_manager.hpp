/**
 * @File    can_bus_manager.hpp
 * @Time    2026/02/24
 * @Author  OpenArmX
 * @Version 1.0
 * @Desc    SocketCAN 通信管理器，提供非阻塞 CAN 帧收发和内核级过滤
 */

#ifndef SWERVE_HARDWARE__CAN_BUS_MANAGER_HPP_
#define SWERVE_HARDWARE__CAN_BUS_MANAGER_HPP_

#include "openflex_can/chassis/can_bus_manager.hpp"

namespace swerve_hardware
{
using CanDiagnostics = openflex_can::CanDiagnostics;
using CanBusManager = openflex_can::CanBusManager;
using openflex_can::configure_swerve_can_filters;
}  // namespace swerve_hardware

#endif  // SWERVE_HARDWARE__CAN_BUS_MANAGER_HPP_
