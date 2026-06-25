/**
 * @File    rs06_driver.hpp
 * @Time    2026/02/24
 * @Author  OpenArmX
 * @Version 1.0
 * @Desc    RS06 转向电机驱动（CSP 位置控制模式），CAN 扩展帧协议
 */

#ifndef SWERVE_HARDWARE__RS06_DRIVER_HPP_
#define SWERVE_HARDWARE__RS06_DRIVER_HPP_

#include "openflex_can/chassis/rs06_driver.hpp"

namespace swerve_hardware
{
namespace RS06Func = openflex_can::RS06Func;
namespace RS06Param = openflex_can::RS06Param;
using openflex_can::RS06_HOST_ID;
using openflex_can::RS06_MODE_CSP;
using RS06State = openflex_can::RS06State;
using RS06Driver = openflex_can::RS06Driver;
}  // namespace swerve_hardware

#endif  // SWERVE_HARDWARE__RS06_DRIVER_HPP_
