/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Cambus driver nRF port.
 */
#include <string.h>
#include <stdbool.h>
#include "py/mphal.h"

#include "omv_boardconfig.h"
#include "cambus.h"
#include "common.h"

#define I2C_TIMEOUT             (1000)
#define I2C_SCAN_TIMEOUT        (100)

int cambus_init(cambus_t *bus, uint32_t bus_id, uint32_t speed)
{
    bus->id = bus_id;
    bus->speed = speed;
    bus->initialized = false;

    switch (bus_id) {
        case 0: {
            bus->scl_pin = TWI0_SCL_PIN;
            bus->sda_pin = TWI0_SDA_PIN;
            nrfx_twi_t _twi = NRFX_TWI_INSTANCE(0);
            memcpy(&bus->twi, &_twi, sizeof(nrfx_twi_t));
            break;
        }
        case 1: {
            bus->scl_pin = TWI1_SCL_PIN;
            bus->sda_pin = TWI1_SDA_PIN;
            nrfx_twi_t _twi = NRFX_TWI_INSTANCE(1);
            memcpy(&bus->twi, &_twi, sizeof(nrfx_twi_t));
            break;
        }
        default:
            return -1;
    }

    nrfx_twi_config_t config = {
       .scl                = bus->scl_pin,
       .sda                = bus->sda_pin,
       .frequency          = speed,
       .interrupt_priority = 4,
       .hold_bus_uninit    = false
    };

    if (nrfx_twi_init(&bus->twi, &config, NULL, NULL) != NRFX_SUCCESS) {
        return -1;
    }

    // This bus needs to be enabled for suspended transfers.
    nrfx_twi_enable(&bus->twi);

    bus->initialized = true;
    return 0;
}

int cambus_deinit(cambus_t *bus)
{
    if (bus->initialized) {
        nrfx_twi_disable(&bus->twi);
        nrfx_twi_uninit(&bus->twi);
        bus->initialized = false;
    }
    return 0;
}

int cambus_scan(cambus_t *bus)
{
    return 0;
}

int cambus_gencall(cambus_t *bus, uint8_t cmd)
{
    return 0;
}

int cambus_read_bytes(cambus_t *bus, uint8_t slv_addr, uint8_t *buf, int len, uint32_t flags)
{
    int ret = 0;
    slv_addr = slv_addr >> 1;
    uint32_t xfer_flags = 0;
    if (flags & CAMBUS_XFER_SUSPEND) {
        xfer_flags |= NRFX_TWI_FLAG_SUSPEND;
    }

    nrfx_twi_xfer_desc_t desc = NRFX_TWI_XFER_DESC_RX(slv_addr, buf, len);
    if (nrfx_twi_xfer(&bus->twi, &desc, xfer_flags) != NRFX_SUCCESS) {
        ret = -1;
    }
    return ret;
}

int cambus_write_bytes(cambus_t *bus, uint8_t slv_addr, uint8_t *buf, int len, uint32_t flags)
{
    int ret = 0;
    slv_addr = slv_addr >> 1;
    uint32_t xfer_flags = 0;
    if (flags & CAMBUS_XFER_NO_STOP) {
        xfer_flags |= NRFX_TWI_FLAG_TX_NO_STOP;
    } else if (flags & CAMBUS_XFER_SUSPEND) {
        xfer_flags |= NRFX_TWI_FLAG_SUSPEND;
    }

    nrfx_twi_xfer_desc_t desc = NRFX_TWI_XFER_DESC_TX(slv_addr, buf, len);
    if (nrfx_twi_xfer(&bus->twi, &desc, xfer_flags) != NRFX_SUCCESS) {
        ret = -1;
    }
    return ret;
}

int cambus_pulse_scl(cambus_t *bus)
{
    for (int i=0; i<10000; i++) {
        cambus_deinit(bus);
        nrfx_twi_bus_recover(bus->scl_pin, bus->sda_pin);
    }
    return 0;
}
