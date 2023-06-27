
#include "target/sbw_jtag.h"
#include "target/sbw_transport.h"
#include "target/target_internal.h"
#include "gdb_packet.h"

#include "target/EEM_defs.h"

#include "general.h"
#include "target/sbw_scan.h"
#include "target.h"
#include "target/adiv5.h"
#include "target/jtag_devs.h"
#include "target/msp430.h"

#include "target/msp430/pc.h"
#include "target/msp430/memory.h"

/**
 * Writes one byte/uint16_t at a given address ( <0xA00)
 *
 * @param Format F_BYTE or F_WORD
 * @param Addr Address of data to be written
 * @param Data Data to be written
 */
int WriteMem_430Xv2(uint16_t Format, uint32_t Addr, uint16_t Data)
{
    // Check Init State at the beginning
    IR_Shift(IR_CNTRL_SIG_CAPTURE);
    if(!(DR_Shift16(0) & 0x0301))
        return SC_ERR_GENERIC;

    clr_tclk_sbw();
    IR_Shift(IR_CNTRL_SIG_16BIT);
    if(Format == F_WORD)
    {
        DR_Shift16(0x0500);
    }
    else
    {
        DR_Shift16(0x0510);
    }
    IR_Shift(IR_ADDR_16BIT);
    DR_Shift20(Addr);

    set_tclk_sbw();
    // New style: Only apply data during clock high phase
    IR_Shift(IR_DATA_TO_ADDR);
    DR_Shift16(Data);  // Shift in 16 bits
    clr_tclk_sbw();
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x0501);
    set_tclk_sbw();
    // one or more cycle, so CPU is driving correct MAB
    clr_tclk_sbw();
    set_tclk_sbw();
    // Processor is now again in Init State

    return SC_ERR_NONE;
}

/**
 * Reads one byte/word from a given address in memory
 *
 * @param Format F_BYTE or F_WORD
 * @param Addr Address of data to be written
 *
 * @returns Data from device
 */
uint16_t ReadMem_430Xv2(uint16_t Format, uint32_t Addr)
{
    uint16_t TDOword = 0;
    DWT_Delay(1000);
    // Check Init State at the beginning
    IR_Shift(IR_CNTRL_SIG_CAPTURE);
    if(DR_Shift16(0) & 0x0301)
    {
        // Read Memory
        clr_tclk_sbw();
        IR_Shift(IR_CNTRL_SIG_16BIT);
        if(Format == F_WORD)
        {
            DR_Shift16(0x0501);  // Set uint16_t read
        }
        else
        {
            DR_Shift16(0x0511);  // Set byte read
        }
        IR_Shift(IR_ADDR_16BIT);
        DR_Shift20(Addr);  // Set address
        IR_Shift(IR_DATA_TO_ADDR);
        set_tclk_sbw();
        clr_tclk_sbw();
        TDOword = DR_Shift16(0x0000);  // Shift out 16 bits

        set_tclk_sbw();
        // one or more cycle, so CPU is driving correct MAB
        clr_tclk_sbw();
        set_tclk_sbw();
        // Processor is now again in Init State
    }
    return TDOword;
}

//----------------------------------------------------------------------------
//! \brief This function reads an array of words from the memory.
//! \param[in] word StartAddr (Start address of memory to be read)
//! \param[in] word Length (Number of words to be read)
//! \param[out] word *DataArray (Pointer to array for the data)
void ReadMemQuick_430Xv2(unsigned long StartAddr, unsigned long Length, uint16_t *DataArray)
{
    unsigned long i, lPc = 0;

    // Set PC to 'safe' address
    if((IR_Shift(IR_CNTRL_SIG_CAPTURE) == JTAG_ID99) || (IR_Shift(IR_CNTRL_SIG_CAPTURE) == JTAG_ID98))
    {
        lPc = 0x00000004;
    }

    SbwSetPC_430Xv2(NULL, StartAddr);
    set_tclk_sbw();
    IR_Shift(IR_CNTRL_SIG_16BIT);
    DR_Shift16(0x0501);
    IR_Shift(IR_ADDR_CAPTURE);

    IR_Shift(IR_DATA_QUICK);

    for(i = 0; i < Length; i++)
    {
        set_tclk_sbw();
        clr_tclk_sbw();
        *DataArray++ = DR_Shift16(0);  // Read data from memory.
    }

    if(lPc)
    {
        SbwSetPC_430Xv2(NULL, lPc);
    }
    set_tclk_sbw();
}

/**
 * Reads a word from the specified address in memory.
 */
void sbw_mem_read(target *t, void *dest, target_addr_t src, size_t len)
{
    *((uint32_t *)dest) = (uint32_t)ReadMem_430Xv2(F_WORD, (uint16_t)src);
}

/**
 * Writes a word to the specified address in memory.
 */
void sbw_mem_write(target *t, target_addr_t dest, const void *src, size_t len)
{
#if DISABLE_JTAG_SIGNATURE_WRITE
    /* Prevent write to JTAG signature region -> this would disable JTAG access */
    if((dest >= JTAG_SIGNATURE_LOW) && (dest < JTAG_SIGNATURE_HIGH))
    {
        return DRV_ERR_PROTECTED;
    }
#endif

    // TODO:check this
    if(WriteMem_430Xv2(F_WORD, (uint16_t)dest, ((uint16_t) * (uint16_t *)src)) != 0)
        return DRV_ERR_GENERIC;
    return DRV_ERR_OK;
}