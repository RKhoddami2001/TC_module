/**
 * @file    SPI_portconf_stm32.h
 * @brief   STM32 HAL port + project configuration header.
 *
 * This header is included ONLY by SPI_portconf_stm32.c.
 * Application code and device drivers must never include this file directly –
 * they include SPI_driver.h only.  That is what keeps the platform details
 * hidden from the upper layers.
 *
 * ─── Contents ─────────────────────────────────────────────────────────────
 *  • SPI_STM32_CS_t   – concrete CS pin descriptor for STM32 GPIO.
 *  • Bus index defines – named constants for the spi_bus[] slots used by
 *                        this project; forward-declared here so device
 *                        drivers that DO include this header know which
 *                        index to pass to SPI_Transfer().
 *  • Slave index defines – similarly named slave slot constants.
 *  • extern spi_port_stm32 – the single vtable instance defined in the .c.
 *
 * ─── CS handle mechanism ──────────────────────────────────────────────────
 * SPI_Slave_t.cs_handle is a void* so the core driver needs no platform
 * types.  On STM32 we make it point to a static const SPI_STM32_CS_t.
 * The vtable cs_assert / cs_deassert functions cast it back and call
 * HAL_GPIO_WritePin().
 *
 * Example:
 * @code
 *   static const SPI_STM32_CS_t cs_ads1220 = { GPIOA, GPIO_PIN_10 };
 *
 *   // Then in the slave table:
 *   .cs_handle = (void *)&cs_ads1220
 * @endcode
 */

#ifndef SPI_PORTCONF_STM32_H
#define SPI_PORTCONF_STM32_H

#include "main.h"         /* STM32 HAL: GPIO_TypeDef, SPI_HandleTypeDef …   */
#include "spi.h"          /* CubeMX-generated: extern SPI_HandleTypeDef hspi1, hspi3 */
#include "SPI_driver.h"   /* SPI_Port_t, SPI_Status_t, SPI_Bus_t, spi_bus[] */

/* ═══════════════════════════════════════════════════════════════════════════
 * CS pin descriptor
 *
 * One of these per slave CS line.  Allocate as static const in
 * SPI_portconf_stm32.c so the pointer stored in cs_handle is always valid.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    GPIO_TypeDef *port;   /**< GPIO port, e.g. GPIOA, GPIOB.                */
    uint16_t      pin;    /**< GPIO_PIN_x bitmask  (16-bit, matches HAL).   */
} SPI_STM32_CS_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Bus slot indices
 *
 * These constants identify which spi_bus[] slot each peripheral occupies.
 * Device drivers (ads1220.c, w25q64.c …) use these when calling SPI_Transfer.
 *
 * Keep them in sync with the slot assignments in SPI_Config_RegisterAll().
 * ═══════════════════════════════════════════════════════════════════════════ */

#define BUS_SPI1_INDEX   0   /**< SPI1 – ADS1220 ADC.                       */
#define BUS_SPI3_INDEX   1   /**< SPI3 – W25Q64 Flash + LCD.                */

/* ═══════════════════════════════════════════════════════════════════════════
 * Slave slot indices
 *
 * Index into spi_bus[BUS_x_INDEX].slave[] for each device.
 * Pass these as xfer.slave_index to SPI_Transfer().
 * ═══════════════════════════════════════════════════════════════════════════ */

/* SPI1 slaves */
#define SLAVE_ADS1220    0   /**< ADS1220 24-bit ADC on SPI1.               */

/* SPI3 slaves */
#define SLAVE_FLASH      0   /**< W25Q64JVSSIQ 64 Mbit Flash on SPI3.       */
#define SLAVE_LCD        1   /**< LCD display on SPI3 (currently inactive). */

/* ═══════════════════════════════════════════════════════════════════════════
 * STM32 vtable instance
 *
 * Defined as static const in SPI_portconf_stm32.c.
 * One instance serves all STM32 buses because the per-bus and per-slave
 * context is carried through hw_handle and cs_handle respectively.
 * ═══════════════════════════════════════════════════════════════════════════ */

extern const SPI_Port_t spi_port_stm32;

#endif /* SPI_PORTCONF_STM32_H */
