
#include "safe_state.h"
#include "hwio.h"
#include "wiring_digital.h"
#include "hwio_pindef.h"
#include "gpio.h"
#include "config.h"
#include "appmain.hpp"
#include "assert.h"

using namespace buddy::hw;

static void force_record_metric_zero_integer_and_disable(metric_t &metric) {
    assert(METRIC_VALUE_INTEGER == metric.type);
    const uint32_t last_interval = metric.min_interval_ms;
    metric.min_interval_ms = 0;
    metric_record_integer(&metric, 0);
    metric.enabled_handlers = METRIC_HANDLER_DISABLE_ALL;
    metric.min_interval_ms = last_interval;
}

//! @brief Put hardware into safe state
//!
//! Set fans to maximum, heaters to minimum and disable motors.
void hwio_safe_state(void) {
    // enable fans
#ifdef NEW_FANCTL
    fanCtlPrint.safeState();
    fanCtlHeatBreak.safeState();
#else
    gpio_init(MARLIN_PIN(FAN), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_init(MARLIN_PIN(FAN1), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PIN(FAN), 1);
    gpio_set(MARLIN_PIN(FAN1), 1);
#endif
    // disable heaters
    gpio_init(MARLIN_PIN(HEAT0), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_init(MARLIN_PIN(BED_HEAT), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PIN(HEAT0), 0);
    gpio_set(MARLIN_PIN(BED_HEAT), 0);
    // disable motors
    xEnable.write(Pin::State::high);
    yEnable.write(Pin::State::high);
    zEnable.write(Pin::State::high);
    e0Enable.write(Pin::State::high);

    force_record_metric_zero_integer_and_disable(metric_nozzle_pwm);
    force_record_metric_zero_integer_and_disable(metric_bed_pwm);
}
