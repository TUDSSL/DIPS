/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2015  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "command.h"
#include "general.h"
#include "target.h"
#include "target/target_internal.h"

#define CHIPPN_ADDR 0x40020000
#define VENDERID_ADDR 0x40020010

#define PN_M 0xFF000000
#define PN_S 0x18
#define FLASHSIZE_M 0xF00000
#define FLASHSIZE_S 0x14
#define SRAMSIZE_M 0xF0000
#define SRAMSIZE_S 0x10

#define VENDOR_APOLLO 0x414D4251

#define PN_APOLLO3 0x6
#define PN_APOLLO2 0x3
#define PN_APOLLO1 0x1

//static void apollo_add_flash(target *t, uint32_t addr, size_t length, size_t erasesize)
//{
//    struct target_flash *f = calloc(1, sizeof(*f));
//    if(!f)
//    { /* calloc failed: heap exhaustion */
//        DEBUG_WARN("calloc: failed in %s\n", __func__);
//        return;
//    }
//
//    f->start     = addr;
//    f->length    = length;
//    f->blocksize = erasesize;
//    f->erase     = apollo_flash_erase;
//    f->write     = apollo_flash_write;
//    f->erased    = 0xff;
//    target_add_flash(t, f);
//}

bool apollo_probe(target *t)
{
    uint32_t sdid  = target_mem_read32(t, CHIPPN_ADDR);
    uint32_t vendorID = target_mem_read32(t, VENDERID_ADDR);

    if (vendorID != VENDOR_APOLLO)
        return false;

    switch (sdid >> PN_S) {
        case PN_APOLLO3:
            t->driver = "Ambiq Apollo 3";
            break;
        case PN_APOLLO2:
            t->driver = "Ambiq Apollo 2";
            break;
        case PN_APOLLO1:
            t->driver = "Ambiq Apollo";
            break;
        default:
            return false;
    }

    uint32_t ram_size = sdid >> SRAMSIZE_S;
    target_add_ram(t, 0x10000000 , ram_size * 1024);

    uint32_t flash_size = sdid >> FLASHSIZE_S;

    return true;
}