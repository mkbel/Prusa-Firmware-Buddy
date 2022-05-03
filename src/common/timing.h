#pragma once

#include "../../include/main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Ticks with second precision
///
/// Overflows every 136 years
uint32_t ticks_s();

/// Ticks with millisecond precision
///
/// Overflows every 49 days
uint32_t ticks_ms();

/// Ticks with microsecond precision
///
/// Overflows every 71 minutes
uint32_t ticks_us();

/// Ticks with nanosecond precision
///
/// Overflows every 4.29 seconds
uint32_t ticks_ns();

/// Return difference ticks_b - ticks_a while handling overflows
///
/// Helper function to properly handle overflows when using the
/// ticks_xx() functions.
///
/// Example:
///
///     scheduled_ticks_us = ...
///     if (ticks_diff(scheduled_ticks_us, ticks_us()) > 0)
///         // too early
///     else if (ticks_diff(scheduled_ticks_us, ticks_us()) == 0)
///         // right at time
///     elif (ticks_diff(scheduled_ticks_us, ticks_us()) < 0)
///         // too late
///
int32_t ticks_diff(uint32_t ticks_a, uint32_t ticks_b);

/// Time since the start of the system in nanoseconds
///
/// Given the datatype, overflows every ~584 years
uint64_t timestamp_ns();

/// Sys timer's overflow interrupt callback
///
void app_tick_timer_overflow();

typedef uint32_t (*time_fn_t)();

inline void delay__(time_fn_t time_fn, uint32_t delay) {
    const uint32_t time_to_wait = delay + 1; // fixes issue with maxint, now i have tu use <= instead <
    uint32_t start = time_fn();
    while ((time_fn() - start) <= time_to_wait)
        ;
}

inline void delay_ns(uint32_t ns) {
    delay__(ticks_ns, ns);
}

inline void delay_us(uint32_t us) {
    delay__(ticks_us, us);
}

inline void delay_ms(uint32_t ms) {
    delay__(ticks_ms, ms);
}

inline void delay_s(uint32_t s) {
    delay__(ticks_s, s);
}

#ifdef __cplusplus
}

    #include "stm32f4xx_hal.h"

/**Initializes the CPU, AHB and APB busses clocks  */
inline constexpr RCC_OscInitTypeDef RCC_OscInitStruct = [] {
    RCC_OscInitTypeDef rcc_OscInit {};
    rcc_OscInit.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
    rcc_OscInit.HSEState = RCC_HSE_ON;
    rcc_OscInit.LSIState = RCC_LSI_ON;
    rcc_OscInit.PLL = {
        .PLLState = RCC_PLL_ON,
        .PLLSource = RCC_PLLSOURCE_HSE,
        .PLLM = 6,
        .PLLN = 168,
        .PLLP = RCC_PLLP_DIV2,
        .PLLQ = 7
    };
    return rcc_OscInit;
}();

inline constexpr RCC_ClkInitTypeDef RCC_ClkInitStruct = {
    .ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
    .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
    .AHBCLKDivider = RCC_SYSCLK_DIV1,
    .APB1CLKDivider = RCC_HCLK_DIV4,
    .APB2CLKDivider = RCC_HCLK_DIV2,
};

constexpr unsigned long ConstexprSystemCoreClock_impl(RCC_OscInitTypeDef rcc_OscInit, RCC_ClkInitTypeDef rcc_ClkInit) {
    static_assert(2 == RCC_PLLP_DIV2);
    static_assert(4 == RCC_PLLP_DIV4);
    static_assert(6 == RCC_PLLP_DIV6);
    static_assert(8 == RCC_PLLP_DIV8);

    uint32_t clock_before_AHB_prescaler = 0;

    switch (rcc_ClkInit.SYSCLKSource) {
    case RCC_SYSCLKSOURCE_HSI:
        clock_before_AHB_prescaler = HSI_VALUE;
        break;
    case RCC_SYSCLKSOURCE_HSE:
        clock_before_AHB_prescaler = HSE_VALUE;
        break;
    case RCC_SYSCLKSOURCE_PLLCLK: {
        uint32_t src_clk = (RCC_PLLSOURCE_HSE == rcc_OscInit.PLL.PLLSource) ? HSE_VALUE : HSI_VALUE;
        uint32_t pllvco = (src_clk / rcc_OscInit.PLL.PLLM) * RCC_OscInitStruct.PLL.PLLN;
        clock_before_AHB_prescaler = pllvco / RCC_OscInitStruct.PLL.PLLP;
        break;
    }
    default:
        clock_before_AHB_prescaler = HSI_VALUE;
        break;
    }

    switch (rcc_ClkInit.AHBCLKDivider) {
    default:
    case RCC_SYSCLK_DIV1:
        return (clock_before_AHB_prescaler >>= 0);
    case RCC_SYSCLK_DIV2:
        return (clock_before_AHB_prescaler >>= 1);
    case RCC_SYSCLK_DIV4:
        return (clock_before_AHB_prescaler >>= 2);
    case RCC_SYSCLK_DIV8:
        return (clock_before_AHB_prescaler >>= 3);
    case RCC_SYSCLK_DIV16:
        return (clock_before_AHB_prescaler >>= 4);
    case RCC_SYSCLK_DIV64:
        return (clock_before_AHB_prescaler >>= 6);
    case RCC_SYSCLK_DIV128:
        return (clock_before_AHB_prescaler >>= 7);
    case RCC_SYSCLK_DIV256:
        return (clock_before_AHB_prescaler >>= 8);
    case RCC_SYSCLK_DIV512:
        return (clock_before_AHB_prescaler >>= 9);
    }
}

constexpr unsigned long ConstexprSystemCoreClock() {
    return ConstexprSystemCoreClock_impl(RCC_OscInitStruct, RCC_ClkInitStruct);
}

/**
 * Busy wait delay cycles routines:
 *
 *  DELAY_CYCLES(count): Delay execution in cycles
 *  DELAY_NS(count): Delay execution in nanoseconds
 *  DELAY_US(count): Delay execution in microseconds
 */

    #define FORCE_INLINE __attribute__((always_inline)) inline

    #if defined(__arm__) || defined(__thumb__)

        #if __CORTEX_M == 7
            #define PENDING(NOW, SOON) ((int32_t)(NOW - (SOON)) < 0)

// Cortex-M7 can use the cycle counter of the DWT unit
// http://www.anthonyvh.com/2017/05/18/cortex_m-cycle_counter/

FORCE_INLINE static void enableCycleCounter() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Unlock DWT.
    DWT->LAR = 0xC5ACCE55;

    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

FORCE_INLINE volatile uint32_t getCycleCount() { return DWT->CYCCNT; }

FORCE_INLINE static void timing_delay_cycles(const uint32_t x) {
    const uint32_t endCycles = getCycleCount() + x;
    while (PENDING(getCycleCount(), endCycles)) {
    }
}

            #undef PENDING
        #else

        // https://blueprints.launchpad.net/gcc-arm-embedded/+spec/delay-cycles

            #define nop() __asm__ __volatile__("nop;\n\t" :: \
                                                   :)

FORCE_INLINE static void timing_delay_4cycles(uint32_t cy) { // +1 cycle
            #if ARCH_PIPELINE_RELOAD_CYCLES < 2
                #define EXTRA_NOP_CYCLES " nop\n\t"
            #else
                #define EXTRA_NOP_CYCLES ""
            #endif

    __asm__ __volatile__(
        " .syntax unified\n\t" // is to prevent CM0,CM1 non-unified syntax
        "1:\n\t"
        " subs %[cnt],#1\n\t" //
        EXTRA_NOP_CYCLES
        " bne 1b\n\t"
        : [ cnt ] "+r"(cy) // output: +r means input+output
        :                  // input:
        : "cc"             // clobbers:
    );
}

// Delay in cycles
FORCE_INLINE static void timing_delay_cycles(uint32_t x) {

    if (__builtin_constant_p(x)) {
            #define MAXNOPS 4

        if (x <= (MAXNOPS)) {
            switch (x) {
            case 4:
                nop();
            case 3:
                nop();
            case 2:
                nop();
            case 1:
                nop();
            }
        } else { // because of +1 cycle inside delay_4cycles
            const uint32_t rem = (x - 1) % (MAXNOPS);
            switch (rem) {
            case 3:
                nop();
            case 2:
                nop();
            case 1:
                nop();
            }
            if ((x = (x - 1) / (MAXNOPS)))
                timing_delay_4cycles(x); // if need more then 4 nop loop is more optimal
        }
            #undef MAXNOPS
    } else if ((x >>= 2))
        timing_delay_4cycles(x);
}
            #undef nop

        #endif

    #elif defined(__AVR__)

        #define nop() __asm__ __volatile__("nop;\n\t" :: \
                                               :)

FORCE_INLINE static void timing_delay_4cycles(uint8_t cy) {
    __asm__ __volatile__(
        "1:\n\t"
        " dec %[cnt]:\n\t"
        " nop:\n\t"
        " brne 1b:\n\t"
        : [ cnt ] "+r"(cy) // output: +r means input+output
        :                  // input:
        : "cc"             // clobbers:
    );
}

// Delay in cycles
FORCE_INLINE static void timing_delay_cycles(uint16_t x) {

    if (__builtin_constant_p(x)) {
        #define MAXNOPS 4

        if (x <= (MAXNOPS)) {
            switch (x) {
            case 4:
                nop();
            case 3:
                nop();
            case 2:
                nop();
            case 1:
                nop();
            }
        } else {
            const uint32_t rem = (x) % (MAXNOPS);
            switch (rem) {
            case 3:
                nop();
            case 2:
                nop();
            case 1:
                nop();
            }
            if ((x = (x) / (MAXNOPS)))
                timing_delay_4cycles(x); // if need more then 4 nop loop is more optimal
        }

        #undef MAXNOPS
    } else if ((x >>= 2))
        timing_delay_4cycles(x);
}
        #undef nop

    #elif defined(ESP32)

FORCE_INLINE static void timing_delay_cycles(uint32_t x) {
    unsigned long ccount, stop;

    __asm__ __volatile__("rsr     %0, ccount"
                         : "=a"(ccount));

    stop = ccount + x; // This can overflow

    while (ccount < stop) { // This doesn't deal with overflows
        __asm__ __volatile__("rsr     %0, ccount"
                             : "=a"(ccount));
    }
}

    #elif defined(__PLAT_LINUX__)

    // specified inside platform

    #else

        #error "Unsupported MCU architecture"

    #endif

FORCE_INLINE constexpr uint32_t timing_nanoseconds_to_cycles(uint32_t ns) {
    return ((ns * (ConstexprSystemCoreClock() / 1000000UL)) / 1000UL);
}

FORCE_INLINE constexpr uint32_t timing_microseconds_to_cycles(uint32_t us) {
    return (us * (ConstexprSystemCoreClock() / 1000000UL));
}

    // Delay in nanoseconds
    #define DELAY_NS_PRECISE(ns)                                          \
        do {                                                              \
            constexpr uint32_t cycles = timing_nanoseconds_to_cycles(ns); \
            timing_delay_cycles(cycles);                                  \
        } while (0)

// Delay in microseconds
FORCE_INLINE void delay_us_precise(uint32_t us) {
    timing_delay_cycles(timing_microseconds_to_cycles(us));
}
#endif //__cplusplus
