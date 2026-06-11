/**
 * @file    SPI_driver.h
 * @brief   Platform-agnostic SPI bus / slave abstraction layer.
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │  Architecture overview                                                  │
 * │                                                                         │
 * │   ┌─────────────────────────────────────────────┐                       │
 * │   │   main.c  /  device drivers  (ads1220.c …)  │                       │
 * │   │                                             │                       │
 * │   │   #include "SPI_driver.h"  ◄── ONLY this   │                       │
 * │   │   SPI_Init();                               │                       │
 * │   │   SPI_Transfer(bus, &xfer);                 │                       │
 * │   └──────────────────┬──────────────────────────┘                       │
 * │                      │                                                  │
 * │   ┌──────────────────▼──────────────────────────┐                       │
 * │   │            SPI_driver.c                     │  ← platform-agnostic  │
 * │   │                                             │                       │
 * │   │  • Owns spi_bus[] array                     │                       │
 * │   │  • SPI_Init() calls SPI_Config_RegisterAll()│                       │
 * │   │    which fills spi_bus[] from port layer    │                       │
 * │   │  • All runtime transfers go via vtable      │                       │
 * │   └──────────────────┬──────────────────────────┘                       │
 * │     calls at startup │  calls at runtime via port->fn() vtable          │
 * │   ┌──────────────────▼──────────────────────────┐                       │
 * │   │         SPI_portconf_stm32.c                │  ← platform-specific  │
 * │   │                                             │                       │
 * │   │  • SPI_Config_RegisterAll() fills spi_bus[] │                       │
 * │   │  • vtable functions wrap STM32 HAL calls    │                       │
 * │   │  • All project CS pins and bus configs here │                       │
 * │   └─────────────────────────────────────────────┘                       │
 * │                                                                         │
 * │  To port to a new MCU:                                                  │
 * │    Write SPI_portconf_<platform>.c/.h                                   │
 * │    implementing SPI_Config_RegisterAll() and the SPI_Port_t vtable.     │
 * │    SPI_driver.c and SPI_driver.h are NEVER modified.                   │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * ─── Minimal usage example (main.c) ───────────────────────────────────────
 *
 *  #include "SPI_driver.h"
 *
 *  int main(void)
 *  {
 *      // 1. Board-level init (CubeMX generated)
 *      HAL_Init();
 *      SystemClock_Config();
 *      MX_GPIO_Init();
 *      MX_SPI1_Init();
 *      MX_SPI3_Init();
 *
 *      // 2. Initialise the SPI driver.
 *      //    Internally this calls SPI_Config_RegisterAll() which fills
 *      //    spi_bus[] with your hardware config, then calls port->init()
 *      //    on every registered bus.
 *      if (SPI_Init() != SPI_OK) { Error_Handler(); }
 *
 *      // 3. Use it – nothing below here knows about STM32.
 *      uint8_t cmd    = 0x20;
 *      uint8_t result = 0x00;
 *      SPI_Transfer_t xfer = {
 *          .tx_buf      = &cmd,
 *          .rx_buf      = &result,
 *          .length      = 1,
 *          .slave_index = 0,
 *          .direction   = SPI_DIR_TXRX
 *      };
 *      SPI_Status_t rc = SPI_Transfer(BUS_SPI1_INDEX, &xfer);
 *  }
 *
 *  // BUS_SPI1_INDEX is defined in SPI_portconf_stm32.h and can be included
 *  // in device drivers that need to know which bus index to use.
 * ──────────────────────────────────────────────────────────────────────────
 */

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

/*
 * Deliberately only standard C headers.
 * This file must compile on any target without modification.
 */
#include <stdint.h>
#include <stddef.h>     /* NULL */

/* ═══════════════════════════════════════════════════════════════════════════
 * Compile-time limits
 *
 * Adjust these once before first compilation.
 * They size the static arrays – no heap is used anywhere in this driver.
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Total bus slots available. Unused slots stay NULL after RegisterAll. */
#define SPI_MAX_BUS_COUNT       4

/** Maximum slave devices per bus. */
#define SPI_MAX_SLAVE_COUNT     4

/** Maximum length of any descriptor string including the NUL terminator. */
#define SPI_MAX_DESCRIPTOR_LEN  32

/* ═══════════════════════════════════════════════════════════════════════════
 * Return codes
 *
 * Every public function returns one of these.
 * The port layer maps platform-specific errors (HAL_TIMEOUT etc.) into these
 * values so upper layers never see raw platform codes.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    SPI_OK          = 0,  /**< Success.                                       */
    SPI_ERROR       = 1,  /**< Generic / unspecified error.                   */
    SPI_BUSY        = 2,  /**< Bus locked by an ongoing transfer.             */
    SPI_TIMEOUT     = 3,  /**< Transfer exceeded timeout_ms.                  */
    SPI_INVALID_ARG = 4   /**< NULL pointer or out-of-range index.            */
} SPI_Status_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Configuration enumerations
 *
 * Pure logical values – no platform types.
 * The port layer translates these into HAL / register constants.
 * ═══════════════════════════════════════════════════════════════════════════ */

/** MCU role on the bus. */
typedef enum {
    SPI_D_MODE_SLAVE  = 0,
    SPI_D_MODE_MASTER = 1
} SPI_Mode_t;

/**
 * @brief  Clock polarity (CPOL).
 *   LOW  → clock idles at 0 V  (CPOL = 0)
 *   HIGH → clock idles at VCC  (CPOL = 1)
 */
typedef enum {
    SPI_D_CLK_POLARITY_LOW  = 0,
    SPI_D_CLK_POLARITY_HIGH = 1
} SPI_CLK_Polarity_t;

/**
 * @brief  Clock phase (CPHA).
 *   1EDGE → data captured on leading  edge  (CPHA = 0)
 *   2EDGE → data captured on trailing edge  (CPHA = 1)
 *
 *   SPI modes:
 *     Mode 0: CPOL=0 CPHA=0    Mode 1: CPOL=0 CPHA=1
 *     Mode 2: CPOL=1 CPHA=0    Mode 3: CPOL=1 CPHA=1
 */
typedef enum {
    SPI_D_CLK_PHASE_1EDGE = 0,
    SPI_D_CLK_PHASE_2EDGE = 1
} SPI_CLK_Phase_t;

/** Data line configuration. */
typedef enum {
    SPI_D_DIRECTION_2LINES        = 0,  /**< Full-duplex MOSI + MISO.          */
    SPI_D_DIRECTION_2LINES_RXONLY = 1,  /**< Receive-only, 2 lines.            */
    SPI_D_DIRECTION_1LINE         = 2   /**< Half-duplex, 1 data line.         */
} SPI_Direction_t;

/** Frame width. */
typedef enum {
    SPI_D_DATASIZE_8BIT  = 0,
    SPI_D_DATASIZE_16BIT = 1
} SPI_DataSize_t;

/** NSS / CS management method. */
typedef enum {
    SPI_D_NSS_SOFT        = 0,  /**< Software GPIO – used in this driver.      */
    SPI_D_NSS_HARD_INPUT  = 1,
    SPI_D_NSS_HARD_OUTPUT = 2
} SPI_NSS_t;

/** Bit order. */
typedef enum {
    SPI_D_FIRSTBIT_MSB = 0,
    SPI_D_FIRSTBIT_LSB = 1
} SPI_FirstBit_t;

/**
 * @brief  Clock prescaler.
 *   SCK = APB_clock / (2 ^ (value + 1))
 *   Port layer converts this to the platform prescaler register field.
 */
typedef enum {
    SPI_D_PRESCALER_2   = 0,
    SPI_D_PRESCALER_4   = 1,
    SPI_D_PRESCALER_8   = 2,
    SPI_D_PRESCALER_16  = 3,
    SPI_D_PRESCALER_32  = 4,
    SPI_D_PRESCALER_64  = 5,
    SPI_D_PRESCALER_128 = 6,
    SPI_D_PRESCALER_256 = 7
} SPI_Prescaler_t;

/** TI frame format. */
typedef enum {
    SPI_D_TIMODE_DISABLE = 0,
    SPI_D_TIMODE_ENABLE  = 1
} SPI_TIMode_t;

/** Hardware CRC. */
typedef enum {
    SPI_D_CRC_DISABLE = 0,
    SPI_D_CRC_ENABLE  = 1
} SPI_CRCEnable_t;

/** Transfer direction for SPI_Transfer_t. */
typedef enum {
    SPI_D_DIR_TX   = 0,  /**< Transmit only  – rx_buf ignored.                 */
    SPI_D_DIR_RX   = 1,  /**< Receive  only  – tx_buf ignored.                 */
    SPI_D_DIR_TXRX = 2   /**< Full-duplex    – both buffers used.               */
} SPI_TransferDir_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * SPI_BusConfig_t
 *
 * Logical hardware parameters for one SPI peripheral.
 * No platform types here.  The descriptor has been moved to SPI_Bus_t so
 * that it lives alongside the runtime state rather than inside the config.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {

    /* ── Timing & mode ──────────────────────────────────────────────────── */
    SPI_Mode_t         mode;       /**< Master or slave.                      */
    SPI_CLK_Polarity_t polarity;   /**< CPOL.                                 */
    SPI_CLK_Phase_t    phase;      /**< CPHA.                                 */
    SPI_Prescaler_t    prescaler;  /**< SCK divider.                          */

    /* ── Data format ────────────────────────────────────────────────────── */
    SPI_Direction_t    direction;  /**< Full-duplex / half-duplex / RX-only.  */
    SPI_DataSize_t     data_size;  /**< 8-bit or 16-bit frames.               */
    SPI_FirstBit_t     first_bit;  /**< MSB or LSB first.                     */

    /* ── Advanced / optional ────────────────────────────────────────────── */
    SPI_NSS_t          nss;             /**< NSS management.                  */
    SPI_TIMode_t       ti_mode;         /**< TI frame format.                 */
    SPI_CRCEnable_t    crc_enable;      /**< Hardware CRC on/off.             */
    uint16_t           crc_polynomial;  /**< Polynomial (used if CRC on).     */

    /* ── Timeout ────────────────────────────────────────────────────────── */
    uint32_t           timeout_ms; /**< Max wait time for a blocking transfer.*/

} SPI_BusConfig_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * SPI_Slave_t
 *
 * Describes one CS line / device on a bus.
 * cs_handle is opaque – the port layer knows its real type.
 *   STM32: points to an SPI_STM32_CS_t  (GPIO port + pin)
 *   Other: could be a file descriptor, a chip-select index, etc.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    char     descriptor[SPI_MAX_DESCRIPTOR_LEN]; /**< e.g. "ADS1220 ADC".    */
    uint8_t  active;      /**< 0 = device absent / disabled.                 */
    void    *cs_handle;   /**< Opaque CS context interpreted by port layer.   */
} SPI_Slave_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * SPI_Transfer_t
 *
 * Passed by the caller to SPI_Transfer().
 * direction determines which buffer pointers must be non-NULL.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint8_t          *tx_buf;       /**< Data to send   (NULL if DIR_RX).    */
    uint8_t          *rx_buf;       /**< Receive buffer (NULL if DIR_TX).    */
    uint16_t          length;       /**< Bytes to transfer.                  */
    uint8_t           slave_index;  /**< Which slave in spi_bus[x].slave[].  */
    SPI_TransferDir_t direction;    /**< TX / RX / TXRX.                     */
} SPI_Transfer_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * SPI_Port_t  –  the vtable
 *
 * This struct is the entire interface between SPI_driver.c and the hardware.
 * The port layer fills one static const instance and writes its address into
 * spi_bus[n].port inside SPI_Config_RegisterAll().
 *
 * hw_handle  – opaque bus context (e.g. &hspi1 on STM32).
 * cs_handle  – opaque CS  context (e.g. &cs_ads1220 on STM32).
 *
 * One vtable instance serves ALL buses on the same platform because the
 * per-bus and per-slave context travels through the handle pointers.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {

    /** Apply config to hardware and enable the peripheral. */
    SPI_Status_t (*init)(void *hw_handle, const SPI_BusConfig_t *config);

    /** Disable and power-gate the peripheral. */
    SPI_Status_t (*deinit)(void *hw_handle);

    /** Transmit only – received bytes discarded. */
    SPI_Status_t (*transmit)(void    *hw_handle,
                             uint8_t *data,
                             uint16_t length,
                             uint32_t timeout_ms);

    /** Receive only – TX line held idle. */
    SPI_Status_t (*receive)(void    *hw_handle,
                            uint8_t *data,
                            uint16_t length,
                            uint32_t timeout_ms);

    /** Full-duplex – transmit and receive simultaneously. */
    SPI_Status_t (*transmit_receive)(void    *hw_handle,
                                     uint8_t *tx_data,
                                     uint8_t *rx_data,
                                     uint16_t length,
                                     uint32_t timeout_ms);

    /** Drive CS low (select slave). */
    void (*cs_assert)(void *cs_handle);

    /** Drive CS high (deselect slave). */
    void (*cs_deassert)(void *cs_handle);

} SPI_Port_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * SPI_Bus_t  –  one entry in the global bus table
 *
 * Exposed here so SPI_portconf_stm32.c can access spi_bus[] fields directly.
 * All fields outside SPI_driver.c should be treated as read-only at runtime;
 * write access is only valid inside SPI_Config_RegisterAll() during startup.
 *
 * NOTE: descriptor is here (not in SPI_BusConfig_t) because it is an
 * identity label for the bus record, not a hardware parameter.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {

    /* ── Identity (written by port/config layer) ────────────────────────── */
    char              descriptor[SPI_MAX_DESCRIPTOR_LEN]; /**< e.g. "SPI1". */

    /* ── Hardware config (written by port/config layer) ─────────────────── */
    SPI_BusConfig_t   config;                     /**< Logical parameters.   */
    const SPI_Port_t *port;      /**< Vtable pointer – NULL = slot unused.   */
    void             *hw_handle; /**< Opaque platform handle (e.g. &hspi1).  */

    /* ── Slave table (written by port/config layer) ─────────────────────── */
    SPI_Slave_t       slave[SPI_MAX_SLAVE_COUNT];
    uint8_t           slave_count; /**< Number of valid entries in slave[].  */

    /* ── Runtime state (owned exclusively by SPI_driver.c) ──────────────── */
    uint8_t           initialised; /**< 1 after successful SPI_InitBus().    */
    uint8_t           busy;        /**< 1 during an active transfer.         */
    /*
     * RTOS NOTE: add a mutex handle here when moving to an RTOS.
     * Replace the busy flag check/set in SPI_Transfer() with mutex calls.
     * e.g.:  osMutexId_t mutex;
     */

} SPI_Bus_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Global bus table
 *
 * DEFINED in SPI_driver.c  (zero-initialised by the C runtime).
 * DECLARED here so SPI_portconf_stm32.c can access it via #include.
 *
 * Write to this array ONLY inside SPI_Config_RegisterAll() during startup.
 * At runtime (after SPI_Init returns) treat it as read-only from outside
 * SPI_driver.c.
 * ═══════════════════════════════════════════════════════════════════════════ */

extern SPI_Bus_t spi_bus[SPI_MAX_BUS_COUNT];

/* ═══════════════════════════════════════════════════════════════════════════
 * Platform registration contract
 *
 * SPI_driver.c calls this function by name at the start of SPI_Init().
 * It MUST be implemented in exactly one port/config file per project
 * (e.g. SPI_portconf_stm32.c).
 *
 * On a different platform write a new SPI_portconf_<platform>.c that
 * implements this same function signature.  SPI_driver.c never changes.
 *
 * The function must:
 *   1. Fill spi_bus[n].descriptor, .config, .port, .hw_handle,
 *      .slave[], and .slave_count for every bus it wants to activate.
 *   2. Leave all other slots exactly as they are (zeroed by C runtime).
 *   3. Return SPI_OK on success, SPI_ERROR on any failure.
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Populate spi_bus[] with project-specific hardware configuration.
 *
 * Implemented in SPI_portconf_stm32.c (or equivalent for other platforms).
 * Called once by SPI_Init() before the hardware init loop.
 * Never call this function directly from application code.
 */
extern SPI_Status_t SPI_Config_RegisterAll(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Full startup sequence in one call.
 *
 *         1. Calls SPI_Config_RegisterAll() – port layer fills spi_bus[].
 *         2. Iterates spi_bus[]; skips slots with port == NULL.
 *         3. Calls SPI_InitBus() on every registered slot.
 *
 * @return SPI_OK  – all registered buses initialised successfully.
 *         SPI_ERROR – RegisterAll failed, or one or more buses failed init
 *                     (all buses are still attempted even if one fails).
 */
SPI_Status_t SPI_Init(void);

/**
 * @brief  Initialise one bus: calls port->init() with the stored config.
 * @param  bus_index  Index into spi_bus[].
 * @return SPI_OK / SPI_ERROR / SPI_INVALID_ARG.
 */
SPI_Status_t SPI_InitBus(uint8_t bus_index);

/**
 * @brief  Execute a blocking SPI transfer.
 *
 *  Sequence:
 *    1. Validate bus index, init state, slave index, active flag, buffers.
 *    2. Check + set busy flag.       (RTOS: take mutex)
 *    3. port->cs_assert()
 *    4. port->transmit / receive / transmit_receive
 *    5. port->cs_deassert()          ← always, even on error
 *    6. Clear busy flag.             (RTOS: give mutex)
 *    7. Return mapped status.
 *
 * @param  bus_index  Index into spi_bus[].
 * @param  xfer       Transfer descriptor (buffers, length, slave, direction).
 * @return SPI_OK / SPI_BUSY / SPI_TIMEOUT / SPI_ERROR / SPI_INVALID_ARG.
 */
SPI_Status_t SPI_Transfer(uint8_t bus_index, const SPI_Transfer_t *xfer);

/**
 * @brief  De-initialise one bus (calls port->deinit(), clears flags).
 * @param  bus_index  Index into spi_bus[].
 * @return SPI_OK / SPI_ERROR / SPI_INVALID_ARG.
 */
SPI_Status_t SPI_DeInitBus(uint8_t bus_index);

/**
 * @brief  De-initialise all registered buses.
 * @return SPI_OK if all succeed, SPI_ERROR if any fail.
 */
SPI_Status_t SPI_DeInit(void);

/**
 * @brief  Return a read-only pointer to one bus record (for inspection/debug).
 * @return Pointer to spi_bus[bus_index], or NULL if index is out of range.
 */
const SPI_Bus_t *SPI_GetBus(uint8_t bus_index);

#endif /* SPI_DRIVER_H */
