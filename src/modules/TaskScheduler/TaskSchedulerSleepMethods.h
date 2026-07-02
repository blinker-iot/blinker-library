/**
 * @file TaskSchedulerSleepMethods.h
 * @brief Platform-specific sleep method implementations for TaskScheduler library
 *
 * This file contains platform-specific implementations of the SleepMethod function
 * used by the TaskScheduler library for power management and idle state handling.
 * Each implementation is optimized for the specific microcontroller architecture
 * and its available low-power modes.
 *
 * The SleepMethod function is called by the scheduler when there are no tasks
 * ready to execute, allowing the system to enter a low-power state until the
 * next task becomes ready or an interrupt occurs.
 *
 * @note This file is automatically included by TaskScheduler.h when
 *       _TASK_SLEEP_ON_IDLE_RUN compilation option is enabled.
 *
 * @author Anatoli Arkhipenko
 * @copyright Copyright (c) 2015-2022 Anatoli Arkhipenko
 * @version 4.0.0
 *
 * Supported platforms:
 * - Arduino AVR (ATmega328P, ATmega32U4, etc.)
 * - Teensy (ARM Cortex-M4)
 * - ESP8266
 * - ESP32
 * - STM32F1 (libmaple)
 * - MSP432 (Energia)
 * - MSP430 (Energia)
 * - Generic/Default implementation
 */

// Cooperative multitasking library for Arduino
// Copyright (c) 2015-2022 Anatoli Arkhipenko

#ifndef _TASKSCHEDULERSLEEPMETHODS_H_
#define _TASKSCHEDULERSLEEPMETHODS_H_


#if defined( ARDUINO_ARCH_AVR ) // Could be used only for AVR-based boards.

#include <avr/sleep.h>
#include <avr/power.h>

/**
 * @brief AVR-specific sleep method implementation using IDLE sleep mode
 *
 * This implementation uses the AVR's IDLE sleep mode which stops the CPU
 * but keeps all peripherals running. This allows timers to continue operating
 * and wake up the processor when needed. The sleep duration parameter is
 * ignored as the wake-up is controlled by timer interrupts.
 *
 * @param aDuration Duration parameter (ignored in AVR implementation)
 *                  Wake-up is controlled by timer interrupts (~1ms)
 *
 * @note Only available on Arduino AVR-based boards (ATmega328P, ATmega32U4, etc.)
 * @note Uses SLEEP_MODE_IDLE which provides the best balance between power savings
 *       and peripheral functionality for cooperative multitasking
 * @note Timer0 and other timers continue to run, maintaining millis() accuracy
 * @note Interrupts will wake the processor from sleep mode
 *
 * Power consumption:
 * - CPU: Stopped
 * - Flash: Operational
 * - SRAM: Operational
 * - Timers: Operational
 * - I/O pins: Operational
 *
 * @see set_sleep_mode()
 * @see sleep_enable()
 * @see sleep_mode()
 * @see sleep_disable()
 *
 * Example power savings:
 * - ATmega328P: ~15mA active → ~6mA idle (60% power reduction)
 * - ATmega32U4: ~20mA active → ~8mA idle (60% power reduction)
 */
void SleepMethod( unsigned long aDuration ) {
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  /* Now enter sleep mode. */
  sleep_mode();

  /* The program will continue from here after the timer timeout ~1 ms */
  sleep_disable(); /* First thing to do is disable sleep. */
}
// ARDUINO_ARCH_AVR


#elif defined( CORE_TEENSY )
/**
 * @brief Teensy-specific sleep method using Wait For Interrupt (WFI)
 *
 * This implementation uses the ARM Cortex-M4 WFI (Wait For Interrupt) instruction
 * to put the processor into a low-power idle state. The processor will wake up
 * on any interrupt, including timer interrupts used by the TaskScheduler.
 *
 * @param aDuration Duration parameter (ignored in Teensy implementation)
 *                  Wake-up is controlled by interrupts
 *
 * @note Only available on Teensy boards (ARM Cortex-M4 based)
 * @note Uses the ARM Cortex-M4 WFI instruction for optimal power efficiency
 * @note All peripherals remain active and can generate wake-up interrupts
 * @note SysTick timer continues to run, maintaining millis() accuracy
 *
 * Power consumption:
 * - CPU core: Stopped
 * - System clock: Operational
 * - Peripherals: Operational
 * - RAM: Operational
 *
 * @see ARM Cortex-M4 Technical Reference Manual for WFI details
 *
 * Example power savings:
 * - Teensy 3.2: ~50mA active → ~20mA idle (60% power reduction)
 * - Teensy 4.0: ~100mA active → ~30mA idle (70% power reduction)
 */
void SleepMethod( unsigned long aDuration ) {
  asm("wfi");
}
//CORE_TEENSY


#elif defined( ARDUINO_ARCH_ESP8266 )

/**
 * @brief ESP8266 delay threshold for sleep optimization
 *
 * This threshold determines when to use a short delay instead of more complex
 * sleep operations. Values below this threshold trigger a 1ms delay which
 * allows the ESP8266's background WiFi operations to continue normally.
 *
 * @note Default value is 200 microseconds
 * @note Can be overridden by defining _TASK_ESP8266_DLY_THRESHOLD before including this file
 */
#ifndef _TASK_ESP8266_DLY_THRESHOLD
#define _TASK_ESP8266_DLY_THRESHOLD 200L
#endif
extern "C" {
#include "user_interface.h"
}

/**
 * @brief ESP8266-specific sleep method with WiFi compatibility
 *
 * This implementation provides power management for ESP8266 while maintaining
 * WiFi connectivity and background operations. For short durations, it uses
 * delay(1) which yields to the ESP8266's system tasks including WiFi management.
 *
 * @param aDuration Duration in microseconds until next task execution
 *                  If less than _TASK_ESP8266_DLY_THRESHOLD, triggers delay(1)
 *
 * @note Only available on ESP8266 boards
 * @note Uses delay(1) for short durations to maintain WiFi connectivity
 * @note ESP8266's delay() implementation includes yield() calls for background tasks
 * @note Longer sleep modes are not implemented to preserve WiFi functionality
 *
 * @warning Deep sleep modes would disconnect WiFi and require reconnection
 * @warning Modem sleep interferes with cooperative multitasking timing
 *
 * Power management strategy:
 * - Short durations (< 200µs): Use delay(1) with background task yielding
 * - Longer durations: No additional sleep (preserves WiFi)
 *
 * @see _TASK_ESP8266_DLY_THRESHOLD
 * @see delay()
 * @see yield()
 *
 * Example usage scenarios:
 * - Web server applications maintaining client connections
 * - IoT devices with continuous WiFi connectivity requirements
 * - Real-time data streaming applications
 */
void SleepMethod( unsigned long aDuration ) {
// to do: find suitable sleep function for esp8266
      if ( aDuration < _TASK_ESP8266_DLY_THRESHOLD) delay(1);   // ESP8266 implementation of delay() uses timers and yield
}
// ARDUINO_ARCH_ESP8266


#elif defined( ARDUINO_ARCH_ESP32 )

#include <esp_sleep.h>

/**
 * @brief ESP32 delay threshold for sleep optimization
 *
 * This threshold determines when to use ESP32's light sleep mode versus
 * staying awake. Values below this threshold do not trigger sleep to
 * avoid the overhead of sleep/wake cycles for very short durations.
 *
 * @note Default value is 200 microseconds
 * @note Can be overridden by defining _TASK_ESP32_DLY_THRESHOLD before including this file
 */
#ifndef _TASK_ESP32_DLY_THRESHOLD
#define _TASK_ESP32_DLY_THRESHOLD 200L
#endif
extern unsigned long tStart, tFinish;
/**
 * @brief Calculated remaining time threshold for sleep decisions
 *
 * This constant represents the effective sleep duration threshold after
 * accounting for the ESP32's sleep/wake overhead and the delay threshold.
 */
const unsigned long tRem = 1000-_TASK_ESP32_DLY_THRESHOLD;

/**
 * @brief ESP32-specific sleep method with light sleep capability
 *
 * This implementation is designed to use ESP32's light sleep mode for
 * power management while maintaining WiFi and Bluetooth connectivity.
 * Currently, the light sleep implementation is commented out pending
 * further testing and optimization.
 *
 * @param aDuration Duration in microseconds until next task execution
 *                  Used to determine if light sleep should be entered
 *
 * @note Only available on ESP32 boards
 * @note Light sleep implementation is currently disabled (commented out)
 * @note When enabled, uses timer wake-up to maintain precise timing
 * @note Preserves WiFi and Bluetooth connectivity during light sleep
 *
 * @todo Complete and test the light sleep implementation
 * @todo Verify compatibility with WiFi and Bluetooth operations
 * @todo Measure actual power savings vs. wake-up overhead
 *
 * Planned power management strategy:
 * - Short durations (< 800µs): Stay awake to avoid sleep overhead
 * - Longer durations: Enter light sleep with timer wake-up
 *
 * @see _TASK_ESP32_DLY_THRESHOLD
 * @see esp_sleep_enable_timer_wakeup()
 * @see esp_light_sleep_start()
 *
 * Expected power savings when implemented:
 * - Active: ~240mA (with WiFi)
 * - Light sleep: ~0.8mA (with WiFi maintained)
 * - Deep sleep: ~10µA (WiFi disconnected)
 */
void SleepMethod( unsigned long aDuration ) {
    // === NOT IMPLEMENTED ===
    // if ( aDuration < tRem ) {
    //     esp_sleep_enable_timer_wakeup((uint64_t) (1000 - aDuration));
    //     esp_light_sleep_start();
    // }
}
// ARDUINO_ARCH_ESP32


#elif defined( ARDUINO_ARCH_STM32F1 )

#include <libmaple/pwr.h>
#include <libmaple/scb.h>

/**
 * @brief STM32F1-specific sleep method using Wait For Interrupt
 *
 * This implementation uses the ARM Cortex-M3 WFI (Wait For Interrupt) instruction
 * to put the STM32F1 processor into a low-power state. The processor will wake up
 * on any interrupt, including the SysTick interrupt that runs every millisecond.
 *
 * @param aDuration Duration parameter (ignored in STM32F1 implementation)
 *                  Wake-up is controlled by interrupts, primarily SysTick
 *
 * @note Only available on STM32F1 boards using libmaple framework
 * @note Uses ARM Cortex-M3 WFI instruction for power management
 * @note SysTick interrupt continues to run every 1ms maintaining system timing
 * @note All peripherals remain operational and can generate wake-up interrupts
 * @note Could potentially use STOP mode for deeper power savings
 *
 * Power consumption:
 * - CPU core: Stopped
 * - System clock: Operational
 * - Peripherals: Operational
 * - RAM: Operational
 * - Flash: Operational
 *
 * @see ARM Cortex-M3 Technical Reference Manual for WFI details
 * @see libmaple power management documentation
 *
 * Example power savings:
 * - STM32F103: ~36mA active → ~15mA idle (58% power reduction)
 * - STM32F107: ~50mA active → ~20mA idle (60% power reduction)
 *
 * @todo Consider implementing STOP mode for deeper power savings
 * @todo Evaluate impact on peripheral timing and wake-up latency
 */
void SleepMethod( unsigned long aDuration ) {
	  // Now go into stop mode, wake up on interrupt.
	  // Systick interrupt will run every 1 milliseconds.
	  asm("    wfi");
}
// ARDUINO_ARCH_STM32


#elif defined( ENERGIA_ARCH_MSP432 )

/**
 * @brief MSP432-specific sleep method using delay
 *
 * This implementation uses a simple 1-millisecond delay for the MSP432
 * platform under the Energia framework. The delay allows other system
 * tasks to execute while providing a basic form of power management.
 *
 * @param aDuration Duration parameter (ignored in MSP432 implementation)
 *                  Fixed 1ms delay is used regardless of parameter
 *
 * @note Only available on MSP432 boards using Energia framework
 * @note Uses delay(1) which yields processor time to other tasks
 * @note Simple implementation prioritizing system stability over power optimization
 * @note Maintains system timing and peripheral operations
 *
 * Power management strategy:
 * - Uses Energia's delay() function which may include yield operations
 * - Provides basic power management without complex sleep modes
 * - Maintains compatibility with Energia framework expectations
 *
 * @see Energia delay() implementation
 * @see MSP432 power management capabilities
 *
 * @todo Investigate MSP432-specific low-power modes for better power efficiency
 * @todo Evaluate Energia framework support for advanced power management
 */
void SleepMethod( unsigned long aDuration ) {
    delay(1);
}
// ENERGIA_ARCH_MSP432


#elif defined( ENERGIA_ARCH_MSP430 )

/**
 * @brief MSP430-specific sleep method using Energia sleep function
 *
 * This implementation uses the Energia framework's sleep() function
 * which is specifically designed for MSP430 microcontrollers' advanced
 * low-power modes. The MSP430 family is well-known for its excellent
 * power management capabilities.
 *
 * @param aDuration Duration parameter (ignored in MSP430 implementation)
 *                  Fixed 1ms sleep is used regardless of parameter
 *
 * @note Only available on MSP430 boards using Energia framework
 * @note Uses Energia's sleep() function optimized for MSP430 power modes
 * @note MSP430 has excellent low-power capabilities (µA range in sleep modes)
 * @note Sleep function automatically selects appropriate low-power mode
 *
 * Power management features:
 * - Automatic low-power mode selection by Energia framework
 * - Maintains timer operations for accurate wake-up timing
 * - Preserves RAM contents during sleep
 * - Fast wake-up times (typically < 1µs)
 *
 * @see Energia sleep() function documentation
 * @see MSP430 Family User's Guide for low-power modes
 *
 * Expected power consumption:
 * - Active mode: ~1-3mA (depending on clock speed)
 * - LPM0 (sleep): ~85µA
 * - LPM3 (deep sleep): ~0.7µA
 * - LPM4 (off): ~0.1µA
 */
void SleepMethod( unsigned long aDuration ) {
    sleep(1);
}
// ENERGIA_ARCH_MSP430


#else
/**
 * @brief Default/generic sleep method implementation
 *
 * This is the default implementation used when no platform-specific
 * sleep method is available. It provides no power management functionality
 * but ensures the SleepMethod function exists for compilation compatibility.
 *
 * @param aDuration Duration parameter (ignored in default implementation)
 *                  No sleep or delay operations are performed
 *
 * @note Used for platforms without specific sleep method implementations
 * @note Provides compilation compatibility across all platforms
 * @note No power savings are achieved with this implementation
 * @note The scheduler will continue normal operation without idle power management
 *
 * @warning This implementation provides no power management benefits
 * @warning Consider implementing platform-specific sleep methods for power efficiency
 *
 * @see Platform-specific implementations above for power management examples
 *
 * Usage scenarios:
 * - Development and testing on unsupported platforms
 * - Applications where power management is not required
 * - Fallback for platforms with unknown or untested sleep capabilities
 */
void SleepMethod( unsigned long aDuration ) {
}

#endif //  SLEEP METHODS

#endif // _TASKSCHEDULERSLEEPMETHODS_H_