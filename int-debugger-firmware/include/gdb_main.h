/*
* This file is part of the Black Magic Debug project.
*
* Copyright (C) 2011  Black Sphere Technologies Ltd.
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

#ifndef INCLUDE_GDB_MAIN_H
#define INCLUDE_GDB_MAIN_H

void gdb_main(void);

typedef enum {
    UNKNOWN_ARCH,
    ARM_ARCH,
    MSP_ARCH,
} arch_mode_t;

typedef enum {
    MODE_REGULAR,
    MODE_PROFILE,
    MODE_DETACHED,
    MODE_MINIMUM_ENERGY_BUDGET,
    MODE_PASSIVE
} operation_mode_t;

extern bool m_interrupt_on_power_failure;
extern arch_mode_t m_current_architecture;
extern operation_mode_t m_debugger_mode;
extern bool disable_supply_after_next_continue;

#endif /* INCLUDE_GDB_MAIN_H */
