/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#include "Pin.hpp"
#include "hwio_pindef.h"
#include <type_traits>

namespace buddy::hw {

void InputPin::configure(Pull pull) const {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Pull = static_cast<uint32_t>(pull);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

void OutputPin::configure() const {
    HAL_GPIO_WritePin(getHalPort(), m_halPin, static_cast<GPIO_PinState>(m_initState));
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Speed = static_cast<uint32_t>(m_speed);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

void OutputInputPin::enableInput(Pull pull) const {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = static_cast<uint32_t>(pull);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

static constexpr uint32_t toMarlinPin(IoPort port, IoPin pin) {
    return (16 * static_cast<uint32_t>(port) + static_cast<uint32_t>(pin));
}

bool physicalPinExist(uint32_t marlinPin) {
#define ALL_PHYSICAL_PINS(TYPE, NAME, PORTPIN, PARAMETERS) case toMarlinPin(PORTPIN):
    switch (marlinPin) {
        PIN_TABLE(ALL_PHYSICAL_PINS)
        return true;
    default:
        return false;
    }
#undef ALL_PHYSICAL_PINS
}

bool isOutputPin(uint32_t marlinPin) {
#define ALL_OUTPUT_PINS(TYPE, NAME, PORTPIN, PARAMETERS) \
    case toMarlinPin(PORTPIN):                           \
        return (std::is_same_v<TYPE, OutputPin>);
    switch (marlinPin) {
        PIN_TABLE(ALL_OUTPUT_PINS)
    default:
        return false;
    }
#undef ALL_OUTPUT_PINS
}

} //namespace buddy::hw
