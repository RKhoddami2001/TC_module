/**
 * @file    SPI_portconf_stm32.c
 * @brief   STM32 HAL port implementation + project SPI configuration.
 *
 * ╔═════════════════════════════════════════════════════════════════════════╗
 * ║  THIS IS THE ONLY FILE YOU EDIT WHEN CHANGING YOUR HARDWARE SETUP.    ║
 * ║                                                                        ║
 * ║  SPI_driver.c and SPI_driver.h are never modified.                    ║
 * ║  To port to a new MCU: write a new SPI_portconf_<platform>.c/.h pair. ║
 * ╚═════════════════════════════════════════════════════════════════════════╝
 *
 * ─── Responsibilities of this file ────────────────────────────────────────
 *
 *  PORT layer (platform code – do not change when changing hardware):
 *    • HAL status → SPI_Status_t mapping.
 *    • SPI_BusConfig_t enum → SPI_InitTypeDef field translation.
 *    • vtable functions wrapping HAL_SPI_Transmit/Receive/TransmitReceive.
 *    • CS assert/de-assert via HAL_GPIO_WritePin.
 *    • The single static const SPI_Port_t vtable instance.
 *
 *  CONFIG layer (project-specific – edit when hardware changes):
 *    • CS pin descriptors (GPIO port + pin per slave).
 *    • Bus configuration structs (mode, polarity, phase, prescaler …).
 *    • Slave descriptors (name, active flag, cs_handle).
 *    • SPI_Config_RegisterAll() – fills spi_bus[] and returns.
 *
 * ─── Hardware map (this project) ──────────────────────────────────────────
 *
 *   SPI1 [slot 0]  ── ADS1220 24-bit ADC      CS: PA10
 *   SPI3 [slot 1]  ── W25Q64 64 Mbit Flash    CS: PB5
 *                  └─ LCD Display             CS: PB6   (currently inactive)
 *
 * ─── Startup sequence (main.c) ────────────────────────────────────────────
 *
 *   HAL_Init();
 *   SystemClock_Config();
 *   MX_GPIO_Init();      ← configure CS pins as GPIO output before SPI_Init
 *   MX_SPI1_Init();      ← CubeMX peripheral init (creates hspi1)
 *   MX_SPI3_Init();      ← CubeMX peripheral init (creates hspi3)
 *   SPI_Init();          ← calls SPI_Config_RegisterAll() then port->init()
 */

#include "SPI_portconf_stm32.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * ███  PORT LAYER  ████████████████████████████████████████████████████████
 * ═══════════════════════════════════════════════════════════════════════════
 * The functions in this section know nothing about the project.
 * They translate between SPI_driver.h types and STM32 HAL calls.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ── HAL status → driver status ─────────────────────────────────────────── */

/**
 * @brief  Convert HAL_StatusTypeDef to SPI_Status_t.
 *
 * Centralising the mapping keeps each vtable function short and ensures
 * every HAL error code is handled consistently in one place.
 */
static SPI_Status_t hal_to_spi_status(HAL_StatusTypeDef s)
{
    switch (s) {
        case HAL_OK:      return SPI_OK;
        case HAL_TIMEOUT: return SPI_TIMEOUT;
        case HAL_BUSY:    return SPI_BUSY;
        default:          return SPI_ERROR;   /* HAL_ERROR + any future codes */
    }
}

/* ── SPI_BusConfig_t → SPI_InitTypeDef field translators ────────────────── */
/*
 * Why not cast enums directly?
 * The HAL SPI_MODE_MASTER, SPI_POLARITY_LOW, SPI_BAUDRATEPRESCALER_x etc.
 * constants are NOT guaranteed to equal the integer values of our driver
 * enums.  Each mapping function makes the translation explicit and safe
 * against future HAL changes.
 */
/*
static uint32_t map_mode(SPI_Mode_t m)
{
    return (m == SPI_MODE_MASTER) ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
}

static uint32_t map_direction(SPI_Direction_t d)
{
    switch (d) {
        case SPI_D_DIRECTION_2LINES_RXONLY: return SPI_DIRECTION_2LINES_RXONLY;
        case SPI_D_DIRECTION_1LINE:         return SPI_DIRECTION_1LINE;
        default:                          return SPI_DIRECTION_2LINES;
    }
}

static uint32_t map_datasize(SPI_DataSize_t ds)
{
    return (ds == SPI_D_DATASIZE_16BIT) ? SPI_DATASIZE_16BIT : SPI_DATASIZE_8BIT;
}

static uint32_t map_polarity(SPI_CLK_Polarity_t p)
{
    return (p == SPI_D_CLK_POLARITY_HIGH) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
}

static uint32_t map_phase(SPI_CLK_Phase_t ph)
{
    return (ph == SPI_D_CLK_PHASE_2EDGE) ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
}

static uint32_t map_nss(SPI_NSS_t n)
{
    switch (n) {
        case SPI_D_NSS_HARD_INPUT:  return SPI_NSS_HARD_INPUT;
        case SPI_D_NSS_HARD_OUTPUT: return SPI_NSS_HARD_OUTPUT;
        default:                  return SPI_NSS_SOFT;
    }
}

static uint32_t map_firstbit(SPI_FirstBit_t fb)
{
    return (fb == SPI_D_FIRSTBIT_LSB) ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;
}

/**
 * @brief  Map SPI_Prescaler_t to HAL SPI_BAUDRATEPRESCALER_x.
 *
 * The HAL prescaler constants are not contiguous integers that match the
 * driver enum values, so a direct cast is incorrect.  The switch makes each
 * mapping visible and verifiable against the datasheet.
 */
static uint32_t map_prescaler(SPI_Prescaler_t pr)
{
    switch (pr) {
        case SPI_D_PRESCALER_4:   return SPI_BAUDRATEPRESCALER_4;
        case SPI_D_PRESCALER_8:   return SPI_BAUDRATEPRESCALER_8;
        case SPI_D_PRESCALER_16:  return SPI_BAUDRATEPRESCALER_16;
        case SPI_D_PRESCALER_32:  return SPI_BAUDRATEPRESCALER_32;
        case SPI_D_PRESCALER_64:  return SPI_BAUDRATEPRESCALER_64;
        case SPI_D_PRESCALER_128: return SPI_BAUDRATEPRESCALER_128;
        case SPI_D_PRESCALER_256: return SPI_BAUDRATEPRESCALER_256;
        default:                return SPI_BAUDRATEPRESCALER_2;
    }
}

static uint32_t map_timode(SPI_TIMode_t t)
{
    return (t == SPI_D_TIMODE_ENABLE) ? SPI_TIMODE_ENABLE : SPI_TIMODE_DISABLE;
}

static uint32_t map_crc(SPI_CRCEnable_t c)
{
    return (c == SPI_D_CRC_ENABLE) ? SPI_CRCCALCULATION_ENABLE
                                 : SPI_CRCCALCULATION_DISABLE;
}

/* ── vtable function implementations ────────────────────────────────────── */
/*
 * Every function receives hw_handle as void*.  On STM32 it is always a
 * SPI_HandleTypeDef* and we cast it back immediately.  The cast is safe
 * because SPI_Config_RegisterAll() below always stores &hspiN there.
 */

/**
 * @brief  Translate logical config into HAL init fields, then call HAL_SPI_Init.
 *
 * HAL_SPI_DeInit() is called first as a defensive measure: if CubeMX's
 * MX_SPIx_Init() already ran it left the peripheral in a known state, but
 * DeInit + re-Init guarantees our config values win.
 * Remove the DeInit call if your startup flow relies on CubeMX init persisting.
 */
 SPI_Status_t spi_init(void *hw_handle, const SPI_BusConfig_t *config)
{
	/*
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)hw_handle;

    /* Tear down first so HAL_SPI_Init starts from a clean slate. */
    /*
    HAL_SPI_DeInit(hspi);

    hspi->Init.Mode              = map_mode(config->mode);
    hspi->Init.Direction         = map_direction(config->direction);
    hspi->Init.DataSize          = map_datasize(config->data_size);
    hspi->Init.CLKPolarity       = map_polarity(config->polarity);
    hspi->Init.CLKPhase          = map_phase(config->phase);
    hspi->Init.NSS               = map_nss(config->nss);
    hspi->Init.BaudRatePrescaler = map_prescaler(config->prescaler);
    hspi->Init.FirstBit          = map_firstbit(config->first_bit);
    hspi->Init.TIMode            = map_timode(config->ti_mode);
    hspi->Init.CRCCalculation    = map_crc(config->crc_enable);
    hspi->Init.CRCPolynomial     = config->crc_polynomial;

    return hal_to_spi_status(HAL_SPI_Init(hspi));
    */
	/*Check the bus value ( hw_hanle ) and call the
	 * MX_SPIx();
	 *  */
	/*The config argument in this function will not be used*/
	return 0;



}

 SPI_Status_t spi_deinit(void *hw_handle)
{
   // return hal_to_spi_status(HAL_SPI_DeInit((SPI_HandleTypeDef *)hw_handle));
	/*MX_SPI_DEINIT()!!!
}

static SPI_Status_t stm32_transmit(void    *hw_handle,
                                   uint8_t *data,
                                   uint16_t length,
                                   uint32_t timeout_ms)
{
    return hal_to_spi_status(
        HAL_SPI_Transmit((SPI_HandleTypeDef *)hw_handle,
                         data, length, timeout_ms));
}

static SPI_Status_t stm32_receive(void    *hw_handle,
                                  uint8_t *data,
                                  uint16_t length,
                                  uint32_t timeout_ms)
{
    return hal_to_spi_status(
        HAL_SPI_Receive((SPI_HandleTypeDef *)hw_handle,
                        data, length, timeout_ms));
}

static SPI_Status_t stm32_transmit_receive(void    *hw_handle,
                                           uint8_t *tx_data,
                                           uint8_t *rx_data,
                                           uint16_t length,
                                           uint32_t timeout_ms)
{
    return hal_to_spi_status(
        HAL_SPI_TransmitReceive((SPI_HandleTypeDef *)hw_handle,
                                tx_data, rx_data, length, timeout_ms));
}

/**
 * @brief  Drive CS low – select the slave.
 *
 * cs_handle is cast to SPI_STM32_CS_t*.  This is safe because every slave
 * entry in the config section below stores the address of a static const
 * SPI_STM32_CS_t as cs_handle.
 */
void spi_cs_assert(void *cs_handle)
{
    const SPI_STM32_CS_t *cs = (const SPI_STM32_CS_t *)cs_handle;
    HAL_GPIO_WritePin(cs->port, cs->pin, GPIO_PIN_RESET);
}

/** @brief  Drive CS high – de-select the slave. */
void spi_cs_deassert(void *cs_handle)
{
    const SPI_STM32_CS_t *cs = (const SPI_STM32_CS_t *)cs_handle;
    HAL_GPIO_WritePin(cs->port, cs->pin, GPIO_PIN_SET);//This maybe change to the gpio_driver type!
}

/* ── vtable instance ─────────────────────────────────────────────────────── */
/*
 * One static const instance serves all STM32 buses.  Per-bus and per-slave
 * context travels through hw_handle and cs_handle; the function pointers
 * themselves are the same for every bus on this platform.
 *
 * extern-declared in SPI_portconf_stm32.h so this symbol is visible there,
 * though in practice only SPI_Config_RegisterAll() (below) uses it.
 */
/*not needed any more*/
const SPI_Port_t spi_port_stm32 = {
    .init             = stm32_init,
    .deinit           = stm32_deinit,
    .transmit         = stm32_transmit,
    .receive          = stm32_receive,
    .transmit_receive = stm32_transmit_receive,
    .cs_assert        = stm32_cs_assert,
    .cs_deassert      = stm32_cs_deassert
};

/* ═══════════════════════════════════════════════════════════════════════════
 * ███  CONFIG LAYER  ██████████████████████████████████████████████████████
 * ═══════════════════════════════════════════════════════════════════════════
 * Everything below this line is project-specific.
 * When you change your PCB / hardware setup, only edit this section.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ── CS pin descriptors ──────────────────────────────────────────────────── */
/*
 * One SPI_STM32_CS_t per slave.  Must be static (not stack-allocated) because
 * spi_bus[].slave[].cs_handle stores a pointer to it; the pointer must remain
 * valid for the entire program lifetime.
 *
 * Adjust GPIO port and pin to match your schematic.
 */
static const SPI_STM32_CS_t cs_ads1220 = { GPIOA, GPIO_PIN_10 };  /* SPI1 */
static const SPI_STM32_CS_t cs_flash   = { GPIOB, GPIO_PIN_5  };  /* SPI3 */
static const SPI_STM32_CS_t cs_lcd     = { GPIOB, GPIO_PIN_6  };  /* SPI3 */

/* ── Bus hardware configurations ─────────────────────────────────────────── */
/*
 * SPI_BusConfig_t contains only hardware parameters (mode, polarity …).
 * The descriptor (human label) lives in SPI_Bus_t.descriptor, not here.
 *
 * These are static const so the compiler can place them in flash.
 * SPI_Init() copies .config into each bus record so they do not need to
 * survive past SPI_Config_RegisterAll() – but static is harmless and safe.
 */

/*
 * SPI1 – ADS1220 24-bit ADC
 *   SPI Mode 1: CPOL=0 (LOW), CPHA=1 (2EDGE)
 *   APB2 @ 72 MHz / 16 = 4.5 MHz  (ADS1220 max SCK = 4 MHz → safe margin)
 */
static const SPI_BusConfig_t spi1_config = {
    .mode            = SPI_D_MODE_MASTER,
    .polarity        = SPI_D_CLK_POLARITY_LOW,   /* CPOL = 0 */
    .phase           = SPI_D_CLK_PHASE_2EDGE,    /* CPHA = 1 → Mode 1 */
    .prescaler       = SPI_D_PRESCALER_16,       /* 72 MHz / 16 = 4.5 MHz */
    .direction       = SPI_D_DIRECTION_2LINES,   /* full-duplex */
    .data_size       = SPI_D_DATASIZE_8BIT,
    .first_bit       = SPI_D_FIRSTBIT_MSB,
    .nss             = SPI_D_NSS_SOFT,           /* CS driven by this driver */
    .ti_mode         = SPI_D_TIMODE_DISABLE,
    .crc_enable      = SPI_D_CRC_DISABLE,
    .crc_polynomial  = 7,                      /* default; unused when CRC off */
    .timeout_ms      = 100
};

/*
 * SPI3 – W25Q64 Flash + LCD
 *   SPI Mode 0: CPOL=0 (LOW), CPHA=0 (1EDGE)
 *   APB1 @ 36 MHz / 8 = 4.5 MHz  (conservative; Flash supports up to 80 MHz)
 *   Both devices on this bus support Mode 0, so no bus re-configuration
 *   is needed between slave selections.
 */
static const SPI_BusConfig_t spi3_config = {
    .mode            = SPI_D_MODE_MASTER,
    .polarity        = SPI_D_CLK_POLARITY_LOW,   /* CPOL = 0 */
    .phase           = SPI_D_CLK_PHASE_1EDGE,    /* CPHA = 0 → Mode 0 */
    .prescaler       = SPI_D_PRESCALER_8,        /* 36 MHz / 8 = 4.5 MHz */
    .direction       = SPI_D_DIRECTION_2LINES,
    .data_size       = SPI_D_DATASIZE_8BIT,
    .first_bit       = SPI_D_FIRSTBIT_MSB,
    .nss             = SPI_D_NSS_SOFT,
    .ti_mode         = SPI_D_TIMODE_DISABLE,
    .crc_enable      = SPI_D_CRC_DISABLE,
    .crc_polynomial  = 7,
    .timeout_ms      = 100
};

/* ═══════════════════════════════════════════════════════════════════════════
 * SPI_Config_RegisterAll()
 *
 * Called by SPI_driver.c at the start of SPI_Init().
 * This function fills spi_bus[] directly via the extern declaration in
 * SPI_driver.h.  It does NOT call SPI_InitBus() – hardware init is done
 * by SPI_driver.c after this function returns.
 *
 * ── What it writes into each slot ─────────────────────────────────────────
 *   .descriptor   Human-readable bus label (e.g. "SPI1").
 *   .config       Copy of the static SPI_BusConfig_t above.
 *   .port         Address of the spi_port_stm32 vtable.
 *   .hw_handle    Address of the CubeMX-generated HAL handle (&hspi1 etc.).
 *   .slave[]      Array of SPI_Slave_t descriptors for this bus.
 *   .slave_count  Number of valid entries in slave[].
 *
 * ── Bounds checking ───────────────────────────────────────────────────────
 *   Before writing any slot the index is compared against SPI_MAX_BUS_COUNT
 *   and SPI_MAX_SLAVE_COUNT.  If either check fails the function returns
 *   SPI_ERROR immediately so SPI_Init() can report the misconfiguration.
 *
 * ── Unused slots ──────────────────────────────────────────────────────────
 *   Any spi_bus[] slot not written here keeps its zero-initialised state
 *   (port == NULL).  SPI_driver.c skips those slots.
 * ═══════════════════════════════════════════════════════════════════════════ */

SPI_Status_t SPI_Config_RegisterAll(void)
{
    /* ── Bounds check: ensure our index constants fit the driver limits ─── */
    /*
     * These checks catch configuration mistakes at runtime rather than
     * causing silent out-of-bounds writes.  If they fire, increase
     * SPI_MAX_BUS_COUNT or SPI_MAX_SLAVE_COUNT in SPI_driver.h.
     */
    if (BUS_SPI1_INDEX >= SPI_MAX_BUS_COUNT ||
        BUS_SPI3_INDEX >= SPI_MAX_BUS_COUNT) {
        return SPI_ERROR;   /* bus index exceeds driver limit */
    }

    if (SLAVE_ADS1220 >= SPI_MAX_SLAVE_COUNT ||
        SLAVE_FLASH   >= SPI_MAX_SLAVE_COUNT ||
        SLAVE_LCD     >= SPI_MAX_SLAVE_COUNT) {
        return SPI_ERROR;   /* slave index exceeds driver limit */
    }

    /* ── SPI1  ──────────────────────────────────────────────────────────── */

    /* Identity */
    /* strncpy would be safer for arbitrary strings but this is a known
     * constant that fits; the array is SPI_MAX_DESCRIPTOR_LEN bytes. */
    spi_bus[BUS_SPI1_INDEX].descriptor[0] = '\0';
    /* Use a literal assignment via compound literal trick – simplest in C99: */
    {
        const char lbl[] = "SPI1";
        uint8_t k;
        for (k = 0; k < sizeof(lbl) && k < SPI_MAX_DESCRIPTOR_LEN; k++) {
            spi_bus[BUS_SPI1_INDEX].descriptor[k] = lbl[k];
        }
    }

    /* Hardware configuration – copied by value */
    spi_bus[BUS_SPI1_INDEX].config = spi1_config;

    /* Platform binding */
    //spi_bus[BUS_SPI1_INDEX].port      = &spi_port_stm32;
    spi_bus[BUS_SPI1_INDEX].hw_handle = &hspi1;   /* CubeMX HAL handle */

    /* Slave 0: ADS1220 */
    {
        SPI_Slave_t *s = &spi_bus[BUS_SPI1_INDEX].slave[SLAVE_ADS1220];
        const char lbl[] = "ADS1220 24-bit ADC";
        uint8_t k;
        for (k = 0; k < sizeof(lbl) && k < SPI_MAX_DESCRIPTOR_LEN; k++) {
            s->descriptor[k] = lbl[k];
        }
        s->active    = 1;
        s->cs_handle = (void *)&cs_ads1220;  /* cast: port layer casts back */
    }

    spi_bus[BUS_SPI1_INDEX].slave_count = 1;

    /* ── SPI3  ──────────────────────────────────────────────────────────── */

    {
        const char lbl[] = "SPI3";
        uint8_t k;
        for (k = 0; k < sizeof(lbl) && k < SPI_MAX_DESCRIPTOR_LEN; k++) {
            spi_bus[BUS_SPI3_INDEX].descriptor[k] = lbl[k];
        }
    }

    spi_bus[BUS_SPI3_INDEX].config    = spi3_config;
    //spi_bus[BUS_SPI3_INDEX].port      = &spi_port_stm32;
    spi_bus[BUS_SPI3_INDEX].hw_handle = &hspi3;

    /* Slave 0: W25Q64 Flash */
    {
        SPI_Slave_t *s = &spi_bus[BUS_SPI3_INDEX].slave[SLAVE_FLASH];
        const char lbl[] = "W25Q64JVSSIQ 64Mbit Flash";
        uint8_t k;
        for (k = 0; k < sizeof(lbl) && k < SPI_MAX_DESCRIPTOR_LEN; k++) {
            s->descriptor[k] = lbl[k];
        }
        s->active    = 1;
        s->cs_handle = (void *)&cs_flash;
    }

    /* Slave 1: LCD (currently inactive – set active = 1 when populated) */
    {
        SPI_Slave_t *s = &spi_bus[BUS_SPI3_INDEX].slave[SLAVE_LCD];
        const char lbl[] = "LCD Display";
        uint8_t k;
        for (k = 0; k < sizeof(lbl) && k < SPI_MAX_DESCRIPTOR_LEN; k++) {
            s->descriptor[k] = lbl[k];
        }
        s->active    = 0;              /* inactive until LCD is on the PCB */
        s->cs_handle = (void *)&cs_lcd;
    }

    spi_bus[BUS_SPI3_INDEX].slave_count = 2;

    /* ── All slots filled, report success ───────────────────────────────── */
    return SPI_OK;
}
