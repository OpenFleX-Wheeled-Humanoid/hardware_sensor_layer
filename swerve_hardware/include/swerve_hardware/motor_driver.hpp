/**
 * @File    motor_driver.hpp
 * @Time    2026/02/24
 * @Author  OpenArmX
 * @Version 1.0
 * @Desc    电机驱动抽象接口，定义转向/驱动电机的统一操作
 */

#ifndef SWERVE_HARDWARE__MOTOR_DRIVER_HPP_
#define SWERVE_HARDWARE__MOTOR_DRIVER_HPP_

#include "openflex_can/chassis/motor_driver.hpp"

namespace swerve_hardware
{
using MotorDriver = openflex_can::MotorDriver;
}  // namespace swerve_hardware

#endif  // SWERVE_HARDWARE__MOTOR_DRIVER_HPP_
