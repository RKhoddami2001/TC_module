/**
 * @file    SPI_driver.c
 * @brief   Platform-agnostic SPI driver implementation.
 *
 * This file contains ZERO platform-specific code.  The only external symbol
 * it references outside the standard library is SPI_Config_RegisterAll(),
 * which is declared extern in SPI_driver.h and resolved by the linker from
 * whichever port file is included in the build.
 *
 * All hardware interaction happens through the SPI_Port_t vtable that the
 * port file writes into spi_bus[i].port during SPI_Config_RegisterAll().
 *
 * ─── Thread safety note ───────────────────────────────────────────────────
 * The busy flag is a simple re-entrancy guard for single-threaded bare-metal.
 * For RTOS use: replace the busy flag in SPI_Bus_t with an osMutexId_t and
 * replace the flag check/set/clear in SPI_Transfer() with mutex acquire/release.
 * Search for "RTOS NOTE" to find the exact lines.
 */

#include "SPI_driver.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Global bus table  – THE single definition of spi_bus[].
 *
 * Zero-initialised by the C runtime before main() runs.
 * • port == NULL in every slot until SPI_Config_RegisterAll() fills them.
 * • SPI_Init() skips any slot where port is still NULL.
 * ═══════════════════════════════════════════════════════════════════════════ */

SPI_Bus_t spi_bus[SPI_MAX_BUS_COUNT];   /* extern-declared in SPI_driver.h  */

/* ═══════════════════════════════════════════════════════════════════════════
 * File-scope helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Validate a bus index and return a pointer to the record.
 * @return Pointer to spi_bus[bus_index], or NULL if index is out of range.
 *
 * Using this helper avoids repeating the bounds check in every function and
 * keeps the NULL-return convention consistent: callers simply check for NULL.
 */
static SPI_Bus_t *get_bus(uint8_t bus_index)
{
    if (bus_index >= SPI_MAX_BUS_COUNT) {
        return NULL;
    }
    return &spi_bus[bus_index];
}

/**
 * @brief  Return 1 if a bus slot has been populated by SPI_Config_RegisterAll().
 *
 * A slot is considered registered when both port and hw_handle are non-NULL.
 * Slots the port file did not touch remain zero-initialised (both NULL).
 */
static int bus_is_registered(const SPI_Bus_t *bus)
{
    return (bus->port != NULL && bus->hw_handle != NULL);
}

/**
 * @brief  Validate the vtable: every function pointer must be non-NULL.
 *
 * A missing pointer would cause a hard fault when the driver tries to call
 * through it.  Catching it here during init gives a clear failure point.
 *
 * @return 1 if all pointers are valid, 0 if any is NULL.
 */
static int port_is_complete(const SPI_Port_t *port)
{
    return (port->init             != NULL &&
            port->deinit           != NULL &&
            port->transmit         != NULL &&
            port->receive          != NULL &&
            port->transmit_receive != NULL &&
            port->cs_assert        != NULL &&
            port->cs_deassert      != NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API implementation
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Full system initialisation.
 *
 * Step 1 – Registration:
 *   Calls SPI_Config_RegisterAll() which is implemented in the port/config
 *   file.  That function writes directly into spi_bus[] (it has access via
 *   the extern declaration in SPI_driver.h) to fill each bus slot it wants
 *   to activate with config data, vtable pointer, hw_handle, and slaves.
 *
 * Step 2 – Hardware init:
 *   Iterates spi_bus[].  Any slot still NULL-ported (not filled by the port
 *   file) is silently skipped.  SPI_InitBus() is called on every other slot.
 */
SPI_Status_t SPI_Init(void)
{
    /* ── Step 1: let the port file populate spi_bus[] ───────────────────── */
    SPI_Status_t rc = SPI_Config_RegisterAll();
    if (rc != SPI_OK) {
        /*
         * Registration failed – the port file reported an error.
         * We stop here rather than trying to init with broken config data.
         */
        return SPI_ERROR;
    }

    /* ── Step 2: initialise every slot that was registered ──────────────── */
    SPI_Status_t result = SPI_OK;

    for (uint8_t i = 0; i < SPI_MAX_BUS_COUNT; i++) {

        if (!bus_is_registered(&spi_bus[i])) {
            continue;   /* slot was not filled by port file – skip silently  */
        }

        if (SPI_InitBus(i) != SPI_OK) {
            result = SPI_ERROR;   /* record failure but keep trying the rest  */
        }
    }

    return result;
}

/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise one bus slot by calling its vtable init() function.
 *
 * The vtable init() receives hw_handle and the config struct and is
 * responsible for translating the logical SPI_BusConfig_t parameters into
 * whatever the hardware SDK (HAL, registers, etc.) needs.
 */
SPI_Status_t SPI_InitBus(uint8_t bus_index)
{
    SPI_Bus_t *bus = get_bus(bus_index);
    if (bus == NULL) {
        return SPI_INVALID_ARG;
    }

    if (!bus_is_registered(bus)) {
        /* Slot was never populated by SPI_Config_RegisterAll(). */
        return SPI_ERROR;
    }

    /* Validate the vtable before making any calls through it. */
    if (!port_is_complete(bus->port)) {
        return SPI_ERROR;
    }

    /* Delegate hardware initialisation entirely to the port layer. */
    SPI_Status_t rc = bus->port->init(bus->hw_handle, &bus->config);

    if (rc == SPI_OK) {
        bus->initialised = 1;
        bus->busy        = 0;
    }

    return rc;
}

/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Execute a blocking SPI transfer.
 *
 * This function orchestrates the transfer sequence but touches no hardware
 * directly.  Every hardware action goes through the vtable.
 */
SPI_Status_t SPI_Transfer(uint8_t bus_index, const SPI_Transfer_t *xfer)
{
    /* ── 1. Basic argument validation ───────────────────────────────────── */
    if (xfer == NULL) {
        return SPI_INVALID_ARG;
    }

    SPI_Bus_t *bus = get_bus(bus_index);
    if (bus == NULL) {
        return SPI_INVALID_ARG;
    }

    if (!bus->initialised) {
        /* SPI_InitBus() has not been called successfully for this slot. */
        return SPI_ERROR;
    }

    if (xfer->slave_index >= bus->slave_count) {
        return SPI_INVALID_ARG;
    }

    SPI_Slave_t *slave = &bus->slave[xfer->slave_index];

    if (!slave->active) {
        /* Slave is marked inactive in the config – refuse the transfer. */
        return SPI_ERROR;
    }

    if (slave->cs_handle == NULL) {
        /* No CS context was set – the slave was misconfigured. */
        return SPI_INVALID_ARG;
    }

    /* Direction-specific buffer validation. */
    if ((xfer->direction == SPI_D_DIR_TX || xfer->direction == SPI_D_DIR_TXRX)
        && xfer->tx_buf == NULL) {
        return SPI_INVALID_ARG;
    }
    if ((xfer->direction == SPI_D_DIR_RX || xfer->direction == SPI_D_DIR_TXRX)
        && xfer->rx_buf == NULL) {
        return SPI_INVALID_ARG;
    }
    if (xfer->length == 0) {
        return SPI_INVALID_ARG;
    }

    /* ── 2. Acquire bus ─────────────────────────────────────────────────── */
    /*
     * RTOS NOTE: replace these two lines with a mutex acquire, e.g.:
     *   if (osMutexAcquire(bus->mutex, bus->config.timeout_ms) != osOK)
     *       return SPI_BUSY;
     */
    if (bus->busy) {
        return SPI_BUSY;
    }
    bus->busy = 1;

    /* ── 3. Assert CS (select slave) ────────────────────────────────────── */
    //bus->port->cs_assert(slave->cs_handle);/*Use standard function*/

    /* ── 4. Transfer through vtable ─────────────────────────────────────── */
    SPI_Status_t rc;

    switch (xfer->direction) {

        case SPI_D_DIR_TX:
            /*rc = bus->port->transmit(bus->hw_handle,
                                     xfer->tx_buf,
                                     xfer->length,
                                     bus->config.timeout_ms);/*Use standard function*/
            break;

        case SPI_D_DIR_RX:
           /* rc = bus->port->receive(bus->hw_handle,
                                    xfer->rx_buf,
                                    xfer->length,
                                    bus->config.timeout_ms);/*Use standard function*/
            break;

        case SPI_D_DIR_TXRX:
           /* rc = bus->port->transmit_receive(bus->hw_handle,
                                             xfer->tx_buf,
                                             xfer->rx_buf,
                                             xfer->length,
                                             bus->config.timeout_ms);/*Use standard function*/
            break;

        default:
            rc = SPI_INVALID_ARG;
            break;
    }

    /* ── 5. De-assert CS – ALWAYS, even if the transfer failed ──────────── */
    /*
     * This is unconditional.  Leaving CS asserted on error would lock the
     * slave and corrupt any subsequent transaction on the same bus.
     */
   // bus->port->cs_deassert(slave->cs_handle);/*Use standard function*/

    /* ── 6. Release bus ─────────────────────────────────────────────────── */
    /*
     * RTOS NOTE: replace with mutex release, e.g.:
     *   osMutexRelease(bus->mutex);
     */
    bus->busy = 0;

    return rc;
}

/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief  De-initialise one bus slot.
 *
 * Calls port->deinit() and clears the runtime flags.
 * The slot remains registered (port, hw_handle, config intact) so a
 * subsequent SPI_InitBus() call can bring it back without re-registering.
 */
SPI_Status_t SPI_DeInitBus(uint8_t bus_index)
{
    SPI_Bus_t *bus = get_bus(bus_index);
    if (bus == NULL) {
        return SPI_INVALID_ARG;
    }

    if (!bus_is_registered(bus)) {
        return SPI_ERROR;
    }

    //SPI_Status_t rc = bus->port->deinit(bus->hw_handle);/*Use standard function*/

    /* Clear runtime flags regardless of whether deinit succeeded,
     * so the slot is not left in an inconsistent state. */
    bus->initialised = 0;
    bus->busy        = 0;

    return rc;
}

/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief  De-initialise all registered bus slots.
 */
SPI_Status_t SPI_DeInit(void)
{
    SPI_Status_t result = SPI_OK;

    for (uint8_t i = 0; i < SPI_MAX_BUS_COUNT; i++) {
        if (!bus_is_registered(&spi_bus[i])) {
            continue;
        }
        if (SPI_DeInitBus(i) != SPI_OK) {
            result = SPI_ERROR;   /* keep going – deinit the rest anyway     */
        }
    }

    return result;
}

/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Return a read-only pointer to a bus record.
 *
 * Useful for device drivers or logging code that needs to inspect bus state
 * (e.g. check initialised flag, read the descriptor) without being able to
 * mutate the record.
 */
const SPI_Bus_t *SPI_GetBus(uint8_t bus_index)
{
    if (bus_index >= SPI_MAX_BUS_COUNT) {
        return NULL;
    }
    return &spi_bus[bus_index];
}
