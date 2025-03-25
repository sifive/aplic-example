/*
 * handlers.S
 *
 * This file supports the population of the mtvec vector table,
 * and it contains opcodes of jump instructions to a specific handler.
 *
 */

#include <metal/machine/platform.h>

// Add support for *_asm handler
#if __riscv_xlen == 32
    #define REG_SIZE 4
    #define REG_SIZE_LOG2 2
    #define PTR_SIZE .word
    #define STORE    sw
    #define LOAD     lw
#elif __riscv_xlen == 64
    #define REG_SIZE 8
    #define REG_SIZE_LOG2 3
    #define PTR_SIZE .dword
    #define STORE    sd
    #define LOAD     ld
#else // __riscv_xlen == 64
    #error Unsupported XLEN
#endif // __riscv_xlen != 64

// for software ASM handler
#define CLINT_MSIP_BASE_ADDR        METAL_RISCV_CLINT0_0_BASE_ADDRESS

// alignment and globals
.balign 256, 0
.global __mtvec_clint_vector_table

.global default_exception_handler
.global software_handler
.global timer_handler
.global external_handler
.global set_mip_major_handler
.global software_handler_asm

// for ASM handler
.extern software_isr_counter

// do not generate compressed code
.option norvc

// interrupt vector table starts here
__mtvec_clint_vector_table:

IRQ_0:
        j default_exception_handler
IRQ_1:
        j default_vector_handler
IRQ_2:
        j default_vector_handler
IRQ_3:
        //j software_handler
        j software_handler_asm
IRQ_4:
        j default_vector_handler
IRQ_5:
        j default_vector_handler
IRQ_6:
        j default_vector_handler
IRQ_7:
        j timer_handler
IRQ_8:
        j default_vector_handler
IRQ_9:
        j default_vector_handler
IRQ_10:
        j default_vector_handler
IRQ_11:
        j external_handler
IRQ_12:
        j default_vector_handler
IRQ_13:
        j default_vector_handler
IRQ_14:
        j default_vector_handler
IRQ_15:
        j default_vector_handler
IRQ_16:
        j set_mip_major_handler
IRQ_17:
        j default_vector_handler
IRQ_18:
        j default_vector_handler
IRQ_19:
        j default_vector_handler
IRQ_20:
        j default_vector_handler
IRQ_21:
        j default_vector_handler
IRQ_22:
        j default_vector_handler
IRQ_23:
        j default_vector_handler
IRQ_24:
        j default_vector_handler
IRQ_25:
        j default_vector_handler
IRQ_26:
        j default_vector_handler
IRQ_27:
        j default_vector_handler
IRQ_28:
        j default_vector_handler
IRQ_29:
        j default_vector_handler
IRQ_30:
        j default_vector_handler
IRQ_31:
        j default_vector_handler
IRQ_32:
        j default_vector_handler
IRQ_33:
        j default_vector_handler
IRQ_34:
        j default_vector_handler
IRQ_35:
        j default_vector_handler
IRQ_36:
        j default_vector_handler
IRQ_37:
        j default_vector_handler
IRQ_38:
        j default_vector_handler
IRQ_39:
        j default_vector_handler
IRQ_40:
        j default_vector_handler
IRQ_41:
        j default_vector_handler
IRQ_42:
        j default_vector_handler
IRQ_43:
        j default_vector_handler
IRQ_44:
        j default_vector_handler
IRQ_45:
        j default_vector_handler
IRQ_46:
        j default_vector_handler
IRQ_47:
        j default_vector_handler
IRQ_48:
        j default_vector_handler
IRQ_49:
        j default_vector_handler
IRQ_50:
        j default_vector_handler
IRQ_51:
        j default_vector_handler
IRQ_52:
        j default_vector_handler
IRQ_53:
        j default_vector_handler
IRQ_54:
        j default_vector_handler
IRQ_55:
        j default_vector_handler
IRQ_56:
        j default_vector_handler
IRQ_57:
        j default_vector_handler
IRQ_58:
        j default_vector_handler
IRQ_59:
        j default_vector_handler
IRQ_60:
        j default_vector_handler
IRQ_61:
        j default_vector_handler
IRQ_62:
        j default_vector_handler
IRQ_63:
        j default_vector_handler

// ----------------------------------------------------------------------
// ASM implementation for clearing MSIP per hart for vectored interrupts
// ----------------------------------------------------------------------
software_handler_asm:

#if __riscv_xlen == 32
    add     sp, sp, -8
    STORE   t4, 0(sp)
    STORE   t5, 4(sp)
#else
    add     sp, sp, -16
    STORE   t4, 0(sp)
    STORE   t5, 8(sp)
#endif

    // clear msip for this hart
    li      t4, CLINT_MSIP_BASE_ADDR    // base address of global CLINT MMIO region
    csrr    t5, mhartid             // get hartid
    slli    t5, t5, 2               // multiply by 4 to get mspi reg offset for this hart
    add     t5, t5, t4              // address of msip for this hart now in t5
    sw      x0, 0(t5)               // clear msip for this hart

    // increment global counter
    la      t4, software_isr_counter
    LOAD    t5, 0(t4)
    addi    t5, t5, 1
    STORE   t5, 0(t4)

1:
    // do not exit until mip[3] clears, or we would get spurious s/w interrupts
    csrr    t4, mip
    andi    t5, t4, 8
    bne     t5, x0, 1b      // branch back to the 1: label if t4 != 0

    // pop stack
#if __riscv_xlen == 32
    LOAD    t4, 0(sp)
    LOAD    t5, 4(sp)
    add     sp, sp, 8
#else
    LOAD    t4, 0(sp)
    LOAD    t5, 8(sp)
    add     sp, sp, 16
#endif

    mret
// -------------------------------------------------------
// end of software_handler_asm
// -------------------------------------------------------
