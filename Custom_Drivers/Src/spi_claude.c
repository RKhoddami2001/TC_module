/**
 * =============================================================================
 * FILE: spi_driver.c
 * =============================================================================
 *
 * This file contains the IMPLEMENTATION of the SPI driver.
 *
 * Remember:
 *
 *     spi_driver.h  -> WHAT exists
 *     spi_driver.c  -> HOW it works
 *
 * Every function declared in the header must be implemented here.
 *
 * =============================================================================
 */

#include "spi_driver.h"

#include <string.h>     // memset(), strncpy()

/* ============================================================================
 * GLOBAL DRIVER STORAGE
 * ============================================================================
 *
 * This allocates memory for all SPI buses.
 *
 * The header contained:
 *
 *     extern SPI_Bus_t spi_bus[SPI_BUS_COUNT];
 *
 * "extern" only declared it.
 *
 * Here we DEFINE it.
 *
 * Memory is actually allocated now.
 */
SPI_Bus_t spi_bus[SPI_BUS_COUNT];


/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================
 *
 * These functions are INTERNAL to this file.
 *
 * "static" means:
 *
 *   - visible only inside spi_driver.c
 *   - cannot be called from other files
 *
 * This is good encapsulation.
 * ============================================================================ */


/**
 * Convert our bus index into an STM32 HAL SPI handle.
 *
 * Example:
 *
 *     bus_index = 0  -> &hspi1
 *     bus_index = 1  -> &hspi3
 *
 * Change this mapping if your project uses different SPI peripherals.
 */
static SPI_HandleTypeDef* get_hal_handle(uint8_t bus_index)
{
    switch(bus_index)
    {
        case 0:
            return &hspi1;

        case 1:
            return &hspi3;

        default:
            return NULL;
    }
}


/**
 * Assert chip select.
 *
 * Active-low CS:
 *
 *     LOW  = selected
 *     HIGH = deselected
 */
static void cs_assert(uint16_t pin)
{
    HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_RESET);
}


/**
 * Release chip select.
 */
static void cs_deassert(uint16_t pin)
{
    HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_SET);
}


/* ============================================================================
 * SPI_InitBus()
 * ============================================================================
 */
SPI_Status_t SPI_InitBus(SPI_Bus_t *bus)
{
    uint8_t i;

    /* Null pointer protection */
    if(bus == NULL)
    {
        return SPI_ERROR;
    }

    /* Validate bus index */
    if(bus->bus_index >= SPI_BUS_COUNT)
    {
        return SPI_ERROR;
    }

    /* Verify HAL handle exists */
    if(get_hal_handle(bus->bus_index) == NULL)
    {
        return SPI_ERROR;
    }

    /*
     * Clear slave table.
     *
     * Mark every slot unused.
     */
    for(i = 0; i < SPI_MAX_SLAVES; i++)
    {
        bus->slaves[i].active = 0;
    }

    /* No slaves registered yet */
    bus->slave_count = 0;

    /* Bus is free */
    bus->busy = 0;

    /*
     * Store a copy into global array.
     *
     * This makes the configuration persistent.
     */
    spi_bus[bus->bus_index] = *bus;

    return SPI_OK;
}


/* ============================================================================
 * SPI_RegisterSlave()
 * ============================================================================
 */
SPI_Status_t SPI_RegisterSlave(
        SPI_Bus_t *bus,
        uint16_t cs_pin,
        const char *desc,
        uint8_t *slave_idx_out)
{
    SPI_Slave_t *slave;
    uint8_t index;

    if(bus == NULL)
        return SPI_ERROR;

    if(desc == NULL)
        return SPI_ERROR;

    if(slave_idx_out == NULL)
        return SPI_ERROR;

    /*
     * Check table isn't full.
     */
    if(bus->slave_count >= SPI_MAX_SLAVES)
        return SPI_ERROR;

    index = bus->slave_count;

    slave = &bus->slaves[index];

    /*
     * Copy human-readable description.
     */
    strncpy(
        slave->descriptor,
        desc,
        SPI_MAX_DESC_LEN - 1
    );

    /*
     * Guarantee string termination.
     */
    slave->descriptor[SPI_MAX_DESC_LEN - 1] = '\0';

    slave->cs_gpio_pin = cs_pin;
    slave->active = 1;

    /*
     * Ensure slave starts deselected.
     */
    cs_deassert(cs_pin);

    *slave_idx_out = index;

    bus->slave_count++;

    return SPI_OK;
}


/* ============================================================================
 * SPI_Transfer()
 * ============================================================================
 */
SPI_Status_t SPI_Transfer(
        SPI_Bus_t *bus,
        SPI_Transfer_t *xfer)
{
    HAL_StatusTypeDef hal_status;
    SPI_HandleTypeDef *hspi;
    SPI_Slave_t *slave;

    if(bus == NULL)
        return SPI_ERROR;

    if(xfer == NULL)
        return SPI_ERROR;

    /*
     * Validate slave index.
     */
    if(xfer->slave_index >= bus->slave_count)
        return SPI_ERROR;

    slave = &bus->slaves[xfer->slave_index];

    if(slave->active == 0)
        return SPI_ERROR;

    /*
     * Prevent simultaneous access.
     */
    if(bus->busy)
        return SPI_BUSY;

    hspi = get_hal_handle(bus->bus_index);

    if(hspi == NULL)
        return SPI_ERROR;

    /*
     * Lock bus.
     */
    bus->busy = 1;

    /*
     * Select slave.
     */
    cs_assert(slave->cs_gpio_pin);

    switch(xfer->command)
    {
        case SPI_CMD_TX:

            if(xfer->tx_buf == NULL)
            {
                cs_deassert(slave->cs_gpio_pin);
                bus->busy = 0;
                return SPI_ERROR;
            }

            hal_status =
                HAL_SPI_Transmit(
                    hspi,
                    xfer->tx_buf,
                    xfer->length,
                    SPI_DEFAULT_TIMEOUT_MS);

            break;

        case SPI_CMD_RX:

            if(xfer->rx_buf == NULL)
            {
                cs_deassert(slave->cs_gpio_pin);
                bus->busy = 0;
                return SPI_ERROR;
            }

            hal_status =
                HAL_SPI_Receive(
                    hspi,
                    xfer->rx_buf,
                    xfer->length,
                    SPI_DEFAULT_TIMEOUT_MS);

            break;

        case SPI_CMD_TX_RX:

            if((xfer->tx_buf == NULL) ||
               (xfer->rx_buf == NULL))
            {
                cs_deassert(slave->cs_gpio_pin);
                bus->busy = 0;
                return SPI_ERROR;
            }

            hal_status =
                HAL_SPI_TransmitReceive(
                    hspi,
                    xfer->tx_buf,
                    xfer->rx_buf,
                    xfer->length,
                    SPI_DEFAULT_TIMEOUT_MS);

            break;

        default:

            cs_deassert(slave->cs_gpio_pin);
            bus->busy = 0;

            return SPI_ERROR;
    }

    /*
     * IMPORTANT:
     * Always release CS,
     * even if transfer failed.
     */
    cs_deassert(slave->cs_gpio_pin);

    /*
     * Unlock bus.
     */
    bus->busy = 0;

    switch(hal_status)
    {
        case HAL_OK:
            return SPI_OK;

        case HAL_TIMEOUT:
            return SPI_TIMEOUT;

        case HAL_BUSY:
            return SPI_BUSY;

        default:
            return SPI_ERROR;
    }
}


/* ============================================================================
 * SPI_TransferByte()
 * ============================================================================
 *
 * Convenience wrapper.
 *
 * Build a full transfer structure internally.
 */
uint8_t SPI_TransferByte(
        SPI_Bus_t *bus,
        uint8_t slave_index,
        uint8_t tx_byte)
{
    uint8_t rx_byte = 0;

    SPI_Transfer_t xfer;

    xfer.command = SPI_CMD_TX_RX;

    xfer.tx_buf = &tx_byte;

    xfer.rx_buf = &rx_byte;

    xfer.length = 1;

    xfer.slave_index = slave_index;

    if(SPI_Transfer(bus, &xfer) != SPI_OK)
    {
        return 0xFF;
    }

    return rx_byte;
}


/* ============================================================================
 * SPI_DeInit()
 * ============================================================================
 */
SPI_Status_t SPI_DeInit(SPI_Bus_t *bus)
{
    uint8_t i;

    if(bus == NULL)
        return SPI_ERROR;

    /*
     * Release all slave CS pins.
     */
    for(i = 0; i < bus->slave_count; i++)
    {
        if(bus->slaves[i].active)
        {
            cs_deassert(bus->slaves[i].cs_gpio_pin);
        }
    }

    /*
     * Clear entire structure.
     *
     * memset(destination,value,size)
     */
    memset(bus, 0, sizeof(SPI_Bus_t));

    return SPI_OK;
}