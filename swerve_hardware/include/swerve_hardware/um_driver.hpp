/**
 * @File    um_driver.hpp
 * @Time    2026/04/16
 * @Author  OpenAI
 * @Version 1.2
 * @Desc    UM 系列伺服轮毂一体机驱动（CANopen PV 速度模式 + TPDO1 反馈）
 */

#ifndef SWERVE_HARDWARE__UM_DRIVER_HPP_
#define SWERVE_HARDWARE__UM_DRIVER_HPP_

#include "openflex_can/chassis/um_driver.hpp"

namespace swerve_hardware
{
using UMDriver = openflex_can::UMDriver;
}  // namespace swerve_hardware

#endif  // SWERVE_HARDWARE__UM_DRIVER_HPP_
