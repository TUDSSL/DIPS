/*
 * sbw_scan.c
 *
 *  Created on: Mar 23, 2022
 *      Author: jasper
 */

/*
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * This file provides device-level access to MSP430FR devices via SBW. The implementation
 * is based on code provided by TI (slau320 and slaa754).
 */

#include "target/sbw_jtag.h"
#include "target/sbw_transport.h"
#include "target/target_internal.h"
#include "gdb_packet.h"
#include "gdb_main.h"

#include "target/EEM_defs.h"

#include "general.h"
#include "target/sbw_scan.h"
#include "target.h"
#include "target/adiv5.h"
#include "target/jtag_devs.h"
#include "target/msp430.h"

#include "target/msp430/common.h"
#include "target/msp430/DisableLpmx5.h"
#include "target/msp430/memory.h"
#include "target/msp430/registers.h"
#include "target/msp430/pc.h"

#include "target/msp430/SyncJtagAssertPorSaveContext.h"
#include "spi.h"

/**
 * Resync the JTAG connection and execute a Power-On-Reset
 *
 * @returns SC_ERR_NONE if operation was successful, SC_ERR_GENERIC otherwise
 *
 */
static int SyncJtag_AssertPor(uint32_t* start_pc, uint8_t* start_wdt)
{
    return SyncJtagAssertPorSaveContext(start_pc, start_wdt);
}

/**
 * Determine & compare core identification info
 *
 * @param jtag_id pointer where jtag id is stored
 *
 * @returns SC_ERR_NONE if correct JTAG ID was returned, SC_ERR_GENERIC otherwise
 */
static int GetJtagID(uint16_t *jtag_id)
{
    // uint16_t JtagId = 0;  //initialize JtagId with an invalid value
    int i;
    for(i = 0; i < MAX_ENTRY_TRY; i++)
    {
        // release JTAG/TEST signals to safely reset the test logic
        StopJtag();
        // establish the physical connection to the JTAG interface
        ConnectJTAG();
        // Apply again 4wire/SBW entry Sequence.
        // set ResetPin =1

        EntrySequences_RstHigh_SBW();
        // reset TAP state machine -> Run-Test/Idle
        ResetTAP();
        // shift out JTAG ID
        *jtag_id = (uint16_t)IR_Shift(IR_CNTRL_SIG_CAPTURE);
        DWT_Delay(500);
        // break if a valid JTAG ID is being returned
        if((*jtag_id == JTAG_ID91) || (*jtag_id == JTAG_ID99) ||
           (*jtag_id == JTAG_ID98))  //****************************
        {
            break;
        }
    }

    if(i >= MAX_ENTRY_TRY)
    {
        // if connected device is MSP4305438 JTAG Mailbox is not usable
#ifdef ACTIVATE_MAGIC_PATTERN
        for(i = 0; i < MAX_ENTRY_TRY; i++)
        {
            // if no JTAG ID is returns -> apply magic pattern to stop user cd
            // excecution
            *jtag_id = magicPattern();
            if((*jtag_id == 1) || (i >= MAX_ENTRY_TRY))
            {
                // if magic pattern failed and 4 tries passed -> return status error
                return (SC_ERR_GENERIC);
            }
            else
            {
                break;
            }
        }
        // For MSP430F5438 family mailbox is not functional in reset state.
        // Because of this issue the magicPattern is not usable on MSP430F5438
        // family devices
#else
        return (SC_ERR_ET_DCDC_DEVID);
#endif
    }
    if((*jtag_id == JTAG_ID91) || (*jtag_id == JTAG_ID99) || (*jtag_id == JTAG_ID98))
    {
        return (SC_ERR_NONE);
    }
    else
    {
        return (SC_ERR_ET_DCDC_DEVID);
    }
}

/**
 * Determine & compare core identification info (Xv2)
 *
 * @param core_id pointer where core id gets stored
 * @param device_id_ptr pointer where device id pointer gets stored
 *
 * @returns STATUS_OK if correct JTAG ID was returned, STATUS_ERROR otherwise
 */
static int GetCoreipIdXv2(uint16_t *core_id, uint32_t *device_id_ptr)
{
    IR_Shift(IR_COREIP_ID);
    *core_id = DR_Shift16(0);
    if(*core_id == 0)
    {
        return (SC_ERR_GENERIC);
    }
    IR_Shift(IR_DEVICE_ID);
    *device_id_ptr = DR_Shift20(0);
    // workaround
    // ti issue:
    // https://e2e.ti.com/support/microcontrollers/msp-low-power-microcontrollers-group/msp430/f/msp-low-power-microcontroller-forum/979284/msp430fr2422-access-to-device-descriptors-from-0x1a00-to-0x1a23-gives-0x3fff
    uint32_t DeviceIpPointer = *device_id_ptr;
    DeviceIpPointer          = ((DeviceIpPointer & 0xFFFF) << 4) + (DeviceIpPointer >> 16);
    *device_id_ptr           = DeviceIpPointer;
    // end of work around
    // The ID pointer is an un-scrambled 20bit value
    return (SC_ERR_NONE);
}

/**
 * Takes target device under JTAG control. Disables the target watchdog.
 * Reads device information.
 *
 * @param dsc pointer where device info gets stored
 *
 * @returns SC_ERR_GENERIC if fuse is blown, incorrect JTAG ID or synchronizing time-out; SC_ERR_NONE otherwise
 */
static int GetDevice_430Xv2(dev_dsc_t *dsc)
{
    if(GetJtagID(&dsc->jtag_id) != SC_ERR_NONE)
    {
        return SC_ERR_GENERIC;
    }
    
    // Enable supply when valid device is found in profile mode
    profile_check_for_boot();
    if(m_debugger_mode == MODE_MINIMUM_ENERGY_BUDGET){
        // We don't want to count the energy used to enable the device
        enable_supply();
    }


    if(IsLockKeyProgrammed() != SC_ERR_NONE)  // Stop here if fuse is already blown
    {
        return STATUS_FUSEBLOWN;
    }
    if(GetCoreipIdXv2(&dsc->core_id, &dsc->device_id_ptr) != SC_ERR_NONE)
    {
        return SC_ERR_GENERIC;
    }
    if(SyncJtag_AssertPor(&(dsc->start_pc), &(dsc->start_wdt)) != SC_ERR_NONE)
    {
        return SC_ERR_GENERIC;
    }
    // CPU is now in Full-Emulation-State
    // read DeviceId from memory
    ReadMemQuick_430Xv2(dsc->device_id_ptr + 4, 1, &dsc->device_id);

    return (SC_ERR_NONE);
}

/**
 * Release the target device from JTAG control
 *
 * @param Addr 0xFFFE: Perform Reset, means Load Reset Vector into PC, otherwise: Load Addr into PC
 */
static int ReleaseDevice_430Xv2(uint32_t Addr)
{
    uint16_t shiftResult = 0;
    switch(Addr)
    {
        case V_BOR:

            // perform a BOR via JTAG - we loose control of the device then...
            shiftResult = IR_Shift(IR_TEST_REG);
            DR_Shift16(0x0200);
            DWT_Delay(5000);  // wait some time before doing any other action
            // JTAG control is lost now - GetDevice() needs to be called again to gain
            // control.
            break;

        case V_RESET:

            IR_Shift(IR_CNTRL_SIG_16BIT);
            DR_Shift16(0x0C01);  // Perform a reset
            DR_Shift16(0x0401);
            shiftResult = IR_Shift(IR_CNTRL_SIG_RELEASE);
            break;

        default:

            SbwSetPC_430Xv2(NULL, Addr);  // Set target CPU's PC
            // prepare release & release
            set_tclk_sbw();
            IR_Shift(IR_CNTRL_SIG_16BIT);
            DR_Shift16(0x0401);
            IR_Shift(IR_ADDR_CAPTURE);
            shiftResult = IR_Shift(IR_CNTRL_SIG_RELEASE);
    }

    if((shiftResult == JTAG_ID91) || (shiftResult == JTAG_ID99) ||
       (shiftResult == JTAG_ID98))  //****************************
    {
        return (SC_ERR_NONE);
    }
    else
    {
        return (SC_ERR_GENERIC);
    }
}

/**
 * Disables the Memory Protection Unit (FRAM devices only)
 *
 * @returns SC_ERR_NONE if MPU was disabled successfully, SC_ERR_GENERIC otherwise
 */
static int DisableMpu_430Xv2(void)
{
    if(IR_Shift(IR_CNTRL_SIG_CAPTURE) == JTAG_ID98)
    {
        uint16_t newRegisterVal = ReadMem_430Xv2(F_WORD, FR4xx_LOCKREGISTER);
        newRegisterVal &= ~0xFF03;
        newRegisterVal |= 0xA500;
        // unlock MPU for FR4xx/FR2xx
        WriteMem_430Xv2(F_WORD, FR4xx_LOCKREGISTER, newRegisterVal);
        if((ReadMem_430Xv2(F_WORD, FR4xx_LOCKREGISTER) & 0x3) == 0x0)
        {
            return SC_ERR_NONE;
        }
        return SC_ERR_GENERIC;
    }
    else
    {
        uint16_t MPUCTL0    = 0x0000;
        uint16_t FramCtlKey = 0xA500;

        // first read out the MPU control register 0
        MPUCTL0 = ReadMem_430Xv2(F_WORD, 0x05A0);

        // check MPUENA bit: if MPU is not enabled just return no error
        if((MPUCTL0 & 0x1) == 0)
        {
            return (SC_ERR_NONE);
        }
        // check MPULOCK bit: if MPULOCK is set write access to all MPU
        // registers is disabled until a POR/BOR occurs
        if((MPUCTL0 & 0x3) != 0x1)
        {
            // feed in magic pattern to stop code execution after BOR
            if(i_WriteJmbIn16(STOP_DEVICE) == SC_ERR_GENERIC)
            {
                return (SC_ERR_GENERIC);
            }
            // Apply BOR to reset the device
            set_sbwtck(true);
            DWT_Delay(5000);
            set_sbwtck(false);

            set_sbwtdio(true);
            DWT_Delay(5000);
            set_sbwtdio(false);
            DWT_Delay(5000);

            // connect to device again, apply entry sequence
            ConnectJTAG();

            // Apply again 4wire/SBW entry Sequence.

            EntrySequences_RstHigh_SBW();

            // reset TAP state machine -> Run-Test/Idle
            ResetTAP();
            // get jtag control back
            uint32_t start_pc = 0;
            uint8_t start_wdt = 0;
            if(SC_ERR_GENERIC == SyncJtag_AssertPor(&start_pc, &start_wdt))
            {
                return (SC_ERR_GENERIC);
            }
            {
                return (SC_ERR_GENERIC);
            }
        }
        // MPU Registers are unlocked. MPU can now be disabled.
        // Set MPUENA = 0, write Fram MPUCTL0 key
        WriteMem_430Xv2(F_WORD, 0x05A0, FramCtlKey);

        MPUCTL0 = ReadMem_430Xv2(F_WORD, 0x05A0);
        // now check if MPU is disabled
        if((MPUCTL0 & 0x1) == 0)
        {
            return SC_ERR_NONE;
        }
        return SC_ERR_GENERIC;
    }
}

/* Disables access to and communication with the MSP430. After this, the core should be reset and running */
int sbw_close(void)
{
    ReleaseDevice_430Xv2(V_RESET);
    sbw_transport_disconnect();
    return DRV_ERR_OK;
}

uint32_t sbw_scan(uint32_t targetid)
{
    target_list_free();

    jtag_dev_count = 0;
    memset(&jtag_devs, 0, sizeof(jtag_devs));

    dev_dsc_t dsc;
    sbw_transport_init();
    sbw_transport_connect();

    if(GetDevice_430Xv2(&dsc) != SC_ERR_NONE)
        return DRV_ERR_GENERIC;

    gdb_outf("Found SBW Device coreid: %x, deviceid: %x  jtagid: %x\n", dsc.core_id, dsc.device_id, dsc.jtag_id);

    /* Disables FRAM write protection */
    if(DisableMpu_430Xv2() != SC_ERR_NONE)
    {
        sbw_close();
        return DRV_ERR_GENERIC;
    }

    // Device is fully in debug mode, we can safely enable it to run
    if(m_debugger_mode == MODE_PROFILE){
        profile_start();
    }
    if(m_debugger_mode == MODE_MINIMUM_ENERGY_BUDGET){
        // We don't want to count the energy used to enable the device
        disable_supply();
    }

    msp430_probe(&dsc);

    // TODO: support mulitple sbw devices
    return target_list ? 1 : 0;
}