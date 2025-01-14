/* Copyright 2020 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <metal/machine.h>

#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

/* enable debug prints */
#define DEBUG_PRINT 			TRUE

/* Enable the demonstration of different interrupt delivery methods */
#define INTERRUPT_ID_FOR_SET_MIP_TEST		16			// Use first local external interrupt to test major interrupt handling
#define INTERRUPT_ID_FOR_SETIP_TEST			21			// test this major interrupt using SETIP by INT number. Make sure this exists in your design

/* Compile time options to determine which modules we have.
 * Assignments will resolve to 0 or 1, so be careful about using
 * "#if defined(CLINT_PRESENT)" for example, because that will always be true */
#define CLINT_PRESENT                           (METAL_MAX_CLINT_INTERRUPTS > 0)
#define CLIC_PRESENT                            (METAL_MAX_CLIC_INTERRUPTS > 0)
#define PLIC_PRESENT                            (METAL_MAX_PLIC_INTERRUPTS > 0)
#define BEU0_PRESENT							(METAL_SIFIVE_BUSERROR0_0_SIZE > 0)
#define BEU1_PRESENT							(METAL_SIFIVE_BUSERROR0_1_SIZE > 0)
#define BEU2_PRESENT							(METAL_SIFIVE_BUSERROR0_2_SIZE > 0)
#define BEU3_PRESENT							(METAL_SIFIVE_BUSERROR0_3_SIZE > 0)
#define APLIC_PRESENT							(METAL_SIFIVE_APLICS_0_BASE_ADDRESS > 0)
#define EC_PRESENT								(METAL_SIFIVE_EXTENSIBLECACHE0_CACHE_SIZE > 0)

/*****************************************************************************
 * This example assumes both APLIC and BEU are part of the design.
 * If one or the other doesn't exist, you may see errors.
 * The BEU is configured to trigger an APLIC interrupt, to demonstrate
 * how the interrupt is handled.
 *
 * This test demonstrates how to enable and handle a global interrupt that
 * is managed by the Advanced Platform Level Interrupt Controller (APLIC),
 * and routed into the CPU through the local external interrupt connection,
 * which has interrupt ID #11.
 *
 * At the CPU level, we configure CLINT vectored mode of operation, which
 * allows lower latency to handle any local interrupt into the CPU. This
 * approach requires a vector table which can be found in handlers.S
 *
 * Refer to the file metal.h in the BSP to get interrupt IDs for each BEU.
 * See the __metal_driver_sifive_buserror0_interrupt_id function return values.
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!! THIS HAS TO BE DONE MANUALLY FOR YOUR DESIGN !!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *****************************************************************************/
#if BEU0_PRESENT
#define BUSERR0_INT_NUM		130		/* This is unique to your design - check your manual! */
#endif
#if BEU1_PRESENT
#define BUSERR1_INT_NUM		131  	/* This is unique to your design - check your manual! */
#endif
#if BEU2_PRESENT
#define BUSERR2_INT_NUM		132  	/* This is unique to your design - check your manual! */
#endif
#if BEU3_PRESENT
#define BUSERR3_INT_NUM		133  	/* This is unique to your design - check your manual! */
#endif

#if !BEU0_PRESENT
//#error "BEU does not exist in this design - build fail!"
// removed so this example will support designs without BEU as well
#else
/* Define BEU Errors and Bit Positions */
#define BEU_ICACHE_TL_BUS_ERROR					(1 << 1) 	// Instruction cache TileLink bus error
#define BEU_ICACHE_ITIM_SINGLE_BIT_ERROR		(1 << 2) 	// Instruction cache or ITIM correctable ECC error
#define BEU_ICACHE_UNCORRECTABLE_ECC_ERROR		(1 << 3)	// Instruction cache uncorrectable ECC error
#define BEU_RESERVED							(1 << 4)	// Reserved
#define BEU_LD_ST_PTW_TL_BUS_ERROR				(1 << 5) 	// Load/Store/PTW TileLink bus error
#define BEU_DCACHE_SINGLE_BIT_ERROR				(1 << 6) 	// Data cache correctable ECC error
#define BEU_DCACHE_UNCORRECTABLE_ECC_ERROR		(1 << 7) 	// Data cache uncorrectable ECC error
#define UTLB_CORRECTABLE_PARITY_ERROR			(1 << 8)
#define L2_PRIVATE_TILELINK_BUS_ERROR			(1 << 9)
#define L2_CACHE_SINGLE_BIT_ERROR				(1 << 10)
#define L2_CACHE_UNCORRECTABLE_ERROR			(1 << 11)
#endif /* #if BEU0_PRESENT */

/* Bus Error Units */
#if BEU0_PRESENT
#define BEU0_BASE_ADDR							METAL_SIFIVE_BUSERROR0_0_BASE_ADDRESS
#define BEU0_CAUSE_ADDR							(BEU0_BASE_ADDR + METAL_SIFIVE_BUSERROR0_CAUSE)
#define BEU0_VALUE_ADDR							(BEU0_BASE_ADDR + METAL_SIFIVE_BUSERROR0_VALUE)
#define BEU0_ENABLE_ADDR						(BEU0_BASE_ADDR + METAL_SIFIVE_BUSERROR0_ENABLE)
#define BEU0_APLIC_INTERRUPT_ADDR				(BEU0_BASE_ADDR + METAL_SIFIVE_BUSERROR0_PLATFORM_INTERRUPT)
#define BEU0_ACCRUED_ADDR						(BEU0_BASE_ADDR + METAL_SIFIVE_BUSERROR0_ACCRUED)
#define BEU0_LOCAL_INTERRUPT_ADDR				(BEU0_BASE_ADDR + METAL_SIFIVE_BUSERROR0_LOCAL_INTERRUPT)
#endif

#if BEU1_PRESENT
#define BEU1_BASE_ADDR							METAL_SIFIVE_BUSERROR0_1_BASE_ADDRESS
#define BEU1_CAUSE_ADDR							(BEU1_BASE_ADDR + METAL_SIFIVE_BUSERROR0_CAUSE)
#define BEU1_VALUE_ADDR							(BEU1_BASE_ADDR + METAL_SIFIVE_BUSERROR0_VALUE)
#define BEU1_ENABLE_ADDR						(BEU1_BASE_ADDR + METAL_SIFIVE_BUSERROR0_ENABLE)
#define BEU1_APLIC_INTERRUPT_ADDR				(BEU1_BASE_ADDR + METAL_SIFIVE_BUSERROR0_PLATFORM_INTERRUPT)
#define BEU1_ACCRUED_ADDR						(BEU1_BASE_ADDR + METAL_SIFIVE_BUSERROR0_ACCRUED)
#define BEU1_LOCAL_INTERRUPT_ADDR				(BEU1_BASE_ADDR + METAL_SIFIVE_BUSERROR0_LOCAL_INTERRUPT)
#endif

#if BEU2_PRESENT
#define BEU2_BASE_ADDR							METAL_SIFIVE_BUSERROR0_2_BASE_ADDRESS
#define BEU2_CAUSE_ADDR							(BEU2_BASE_ADDR + METAL_SIFIVE_BUSERROR0_CAUSE)
#define BEU2_VALUE_ADDR							(BEU2_BASE_ADDR + METAL_SIFIVE_BUSERROR0_VALUE)
#define BEU2_ENABLE_ADDR						(BEU2_BASE_ADDR + METAL_SIFIVE_BUSERROR0_ENABLE)
#define BEU2_APLIC_INTERRUPT_ADDR				(BEU2_BASE_ADDR + METAL_SIFIVE_BUSERROR0_PLATFORM_INTERRUPT)
#define BEU2_ACCRUED_ADDR						(BEU2_BASE_ADDR + METAL_SIFIVE_BUSERROR0_ACCRUED)
#define BEU2_LOCAL_INTERRUPT_ADDR				(BEU2_BASE_ADDR + METAL_SIFIVE_BUSERROR0_LOCAL_INTERRUPT)
#endif

#if BEU3_PRESENT
#define BEU3_BASE_ADDR							METAL_SIFIVE_BUSERROR0_3_BASE_ADDRESS
#define BEU3_CAUSE_ADDR							(BEU3_BASE_ADDR + METAL_SIFIVE_BUSERROR0_CAUSE)
#define BEU3_VALUE_ADDR							(BEU3_BASE_ADDR + METAL_SIFIVE_BUSERROR0_VALUE)
#define BEU3_ENABLE_ADDR						(BEU3_BASE_ADDR + METAL_SIFIVE_BUSERROR0_ENABLE)
#define BEU3_APLIC_INTERRUPT_ADDR				(BEU3_BASE_ADDR + METAL_SIFIVE_BUSERROR0_PLATFORM_INTERRUPT)
#define BEU3_ACCRUED_ADDR						(BEU3_BASE_ADDR + METAL_SIFIVE_BUSERROR0_ACCRUED)
#define BEU3_LOCAL_INTERRUPT_ADDR				(BEU3_BASE_ADDR + METAL_SIFIVE_BUSERROR0_LOCAL_INTERRUPT)
#endif

#if APLIC_PRESENT
// NOTE: !!!!! This should be updated for your design based on how many internal interrupts exist !!!!!
// Total APLIC interrupts. Currently the BSP only describes local external interrupts, not internal ones.
// This shows a buffer of 12, allowing up to 12 additional internal interrupt (like BEU, cache, etc)
// More complex designs might have more than 40! Ensure you check your product manual to adjust this if needed.
#define TOTAL_EXT_INTERRUPTS					METAL_MAX_GLOBAL_EXT_INTERRUPTS + 12 
#else
#error "No APLIC Present in this design! Check your configuration!"
#endif /* #if APLIC_PRESENT */

#if __riscv_xlen == 32
#define MCAUSE_INTR                         0x80000000UL
#define MCAUSE_CAUSE                        0x000003FFUL
#else
#define MCAUSE_INTR                         0x8000000000000000UL
#define MCAUSE_CAUSE                        0x00000000000003FFUL
#endif
#define MCAUSE_CODE(cause)                  (cause & MCAUSE_CAUSE)

#define DISABLE             0
#define ENABLE              1
#define RTC_FREQ            32768			/* Generic - this would need to change based on your implementation */
#define FALSE				0
#define TRUE				1

/* Interrupt Specific defines - used for mtvec.mode field, which is bit[0] for
 * designs with CLINT, or [1:0] for designs with a CLIC */
#define MTVEC_MODE_CLINT_DIRECT                 0x00
#define MTVEC_MODE_CLINT_VECTORED               0x01

/* Core Local Interrupter (CLINT) defines */
#define CLINT_BASE_ADDRESS \
	METAL_RISCV_CLINT0_0_BASE_ADDRESS

#define CLINT_MSIP_BASE_ADDR \
	(CLINT_BASE_ADDRESS + METAL_RISCV_CLINT0_MSIP_BASE)

#define CLINT_MSIP_ADDR_HART(hartid) \
	(CLINT_MSIP_BASE_ADDR + 4*hartid)

#define CLINT_MTIMECMP_BASE_ADDR \
    (CLINT_BASE_ADDRESS + METAL_RISCV_CLINT0_MTIMECMP_BASE)

#define CLINT_MTIMECMP_ADDR_HART(hartid) \
    (CLINT_MTIMECMP_BASE_ADDR + 8*hartid)

#define CLINT_MTIME_BASE_ADDR \
    (CLINT_BASE_ADDRESS + METAL_RISCV_CLINT0_MTIME)

#define CLINT_MACHINE_SOFTWARE_INT_ID			3
#define CLINT_MACHINE_TIMER_INT_ID				7
#define CLINT_MACHINE_EXTERNAL_INT_ID			11

/* Define priority thresholds */
#define PRIO_THRESH_0		0x0		// enable all interrupts
#define PRIO_THRESH_1		0x1
#define PRIO_THRESH_2		0x2
#define PRIO_THRESH_3		0x3
#define PRIO_THRESH_4		0x4
#define PRIO_THRESH_5		0x5
#define PRIO_THRESH_6		0x6
#define PRIO_THRESH_7		0x7

#define MACHINE_EXERNAL_ID			11
#define SUPERVISOR_EXTERNAL_ID		9

#if APLIC_PRESENT

#define APLIC_BASE_ADDR						METAL_SIFIVE_APLICS_0_BASE_ADDRESS
#define APLIC_CHICKEN_CSR_ADDR				APLIC_BASE_ADDR		// No offset
// The methodology below follows this idea:
//uint32_t reg = int_id_bit / 32; 		// get register index
//uint32_t bitshift = int_id_bit % 32;	// remainder is bit position
#define APLIC_DOMAINCFG_0_ADDR				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_DOMAINCFG_BASE)
#define APLIC_SOURCECFG_0_ADDR(iid)			(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_SOURCECFG_BASE + (0x4 * (iid - 1)))		// [0] is first interrupt, ID #1. 32b per interrupt ID
#define APLIC_SETIP_0_ADDR(iid)				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_SETIP_BASE + (0x4 * ((iid - 1) >> 5))) 	// div by 32, to get reg index to calculate offset
#define APLIC_SETIPNUM_0_ADDR				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_SETIPNUM_BASE)
#define APLIC_CLRIP_0_ADDR(iid)				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_IN_CLRIP_BASE + (0x4 * ((iid - 1) >> 5)))
#define APLIC_CLRIPNUM_0_ADDR				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_CLRIPNUM_BASE)
#define APLIC_SETIE_0_ADDR(iid)				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_SETIE_BASE + (0x4 * ((iid - 1) >> 5)))
#define APLIC_SETIENUM_0_ADDR				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_SETIENUM_BASE)
#define APLIC_CLRIE_0_ADDR(iid)				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_CLRIE_BASE + (0x4 * ((iid - 1) >> 5)))
#define APLIC_CLRIENUM_0_ADDR				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_CLRIENUM_BASE)
#define APLIC_SETIPNUMLE_0_ADDR				(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_SETIPNUM_LE_BASE)
#define APLIC_TARGET_0_0_ADDR(iid)			(APLIC_BASE_ADDR + METAL_SIFIVE_APLICS_TARGET_BASE + (0x4 * (iid - 1)))

// Interrupt Delivery Control structure for each hart
#define HART_IDC_BASE						0x8000
#define HART_IDC_OFFSET						0x20

// create index for IDC structures below
#define APLIC_IDELIVERY_ADDR(hartid)		(APLIC_IDELIVERY_0_0 + (hartid * HART_IDC_OFFSET))
#define APLIC_IFORCE_ADDR(hartid)			(APLIC_IFORCE_0_0 + (hartid * HART_IDC_OFFSET))
#define APLIC_ITHRESHOLD_ADDR(hartid)		(APLIC_ITHRESHOLD_0_0 + (hartid * HART_IDC_OFFSET))
#define APLIC_TOPI_ADDR(hartid)				(APLIC_TOPI_0_0 + (hartid * HART_IDC_OFFSET))
#define APLIC_CLAIMI_ADDR(hartid)			(APLIC_CLAIMI_0_0 + (hartid * HART_IDC_OFFSET))

// hart 0
#define APLIC_IDELIVERY_0_0					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x00)
#define APLIC_IFORCE_0_0					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x04)
#define APLIC_ITHRESHOLD_0_0				(APLIC_BASE_ADDR + HART_IDC_BASE + 0x08)
#define APLIC_TOPI_0_0						(APLIC_BASE_ADDR + HART_IDC_BASE + 0x18)
#define APLIC_CLAIMI_0_0					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x1C)
// hart 1
#define APLIC_IDELIVERY_0_1					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x20)
#define APLIC_IFORCE_0_1					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x24)
#define APLIC_ITHRESHOLD_0_1				(APLIC_BASE_ADDR + HART_IDC_BASE + 0x28)
#define APLIC_TOPI_0_1						(APLIC_BASE_ADDR + HART_IDC_BASE + 0x38)
#define APLIC_CLAIMI_0_1					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x3C)
// hart 2
#define APLIC_IDELIVERY_0_2					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x40)
#define APLIC_IFORCE_0_2					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x44)
#define APLIC_ITHRESHOLD_0_2				(APLIC_BASE_ADDR + HART_IDC_BASE + 0x48)
#define APLIC_TOPI_0_2						(APLIC_BASE_ADDR + HART_IDC_BASE + 0x58)
#define APLIC_CLAIMI_0_2					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x5C)
// hart 3
#define APLIC_IDELIVERY_0_3					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x60)
#define APLIC_IFORCE_0_3					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x64)
#define APLIC_ITHRESHOLD_0_3				(APLIC_BASE_ADDR + HART_IDC_BASE + 0x68)
#define APLIC_TOPI_0_3						(APLIC_BASE_ADDR + HART_IDC_BASE + 0x78)
#define APLIC_CLAIMI_0_3					(APLIC_BASE_ADDR + HART_IDC_BASE + 0x7C)

#define APLIC_DOMAIN_CONFIG_GLOBAL_ENABLE	0x100
#define APLIC_DOMAIN_CONFIG_GLOBAL_DISABLE	0x0

#define APLIC_SOURCECFG_MODE_INACTIVE		0
#define APLIC_SOURCECFG_MODE_DETATCHED		1
#define APLIC_SOURCECFG_MODE_RISE_EDGE		4
#define APLIC_SOURCECFG_MODE_FALL_EDGE		5
#define APLIC_SOURCECFG_MODE_HIGH_LEVEL		6
#define APLIC_SOURCECFG_MODE_LOW_LEVEL		7
#define APLIC_SOURCECFG_DELEGATION_TO_S		(1 << 10)
#define APLIC_SOURCECFG_NO_DELEGATION		(0 << 10)

#define APLIC_TARGET_HART_BIT_POSITION		18

#endif /* #if APLIC_PRESENT */

/* different interrupt types for enables - these are just random identifiers */
#define MACHINE_INTS                            0x31
#define SUPERVISOR_INTS                         0x51

/* Defines to access CSR registers within C code */
#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define write_csr(reg, val) ({ \
  asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

/* Defines to access GPRs within C code */
#define read_gpr(reg) ({ unsigned long __tmp; \
  asm volatile ("mv %0, " #reg : "=r"(__tmp)); \
  __tmp; })

/* r/w defines */
#define write_dword(addr, data)                 ((*(uint64_t *)(addr)) = data)
#define read_dword(addr)                        (*(uint64_t *)(addr))
#define write_word(addr, data)                  ((*(uint32_t *)(addr)) = data)
#define read_word(addr)                         (*(uint32_t *)(addr))
#define write_byte(addr, data)                  ((*(uint8_t *)(addr)) = data)
#define read_byte(addr)                         (*(uint8_t *)(addr))

// General helpers
#if __riscv_xlen == 64
    #define STORE    sd
    #define LOAD     ld
    #define REGBYTES 8
    #define PTR_SIZE .dword
#else
    #define STORE    sw
    #define LOAD     lw
    #define REGBYTES 4
    #define REGBYTES 4
    #define PTR_SIZE .word
#endif

#ifdef __ASSEMBLY__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

/* Linker generated symbol that lets us know the boot hart */
extern int __metal_boot_hart;

/* Prototypes */
int main(void);
int other_main();
uint32_t aplic_int_enable_disable (uint32_t target_hart, uint32_t int_id, uint32_t source_mode, uint32_t priority, uint32_t m_or_s);
uint32_t check_setie_by_int_num(uint32_t int_id);
uint32_t check_setip_by_int_num (uint32_t int_id);
uint32_t beu_aplic_config(uint32_t error_enable);
void interrupt_global_enable (void);
void interrupt_global_disable (void);
void interrupt_software_enable (void);
void interrupt_software_disable (void);
void interrupt_timer_enable (void);
void interrupt_timer_disable (void);
void interrupt_external_enable (void);
void interrupt_external_disable (void);
void interrupt_local_enable (int id);
void interrupt_local_disable (int id);
void interrupt_local_pending_enable (int id);
void interrupt_local_pending_disable (int id);
void default_exception_handler(void);
int secondary_main(void);
int main(void);

/* Exception handler base, and vector table base address */
/* Alignment defined in handlers.S, and default_exception_handler() lives here */
void __attribute__((interrupt)) __attribute__ ((aligned(256))) __mtvec_clint_vector_table(void);

/* Major interrupts */
void __attribute__((interrupt)) software_handler (void);
void __attribute__((interrupt)) timer_handler (void);
void __attribute__((interrupt)) external_handler (void);
void __attribute__((interrupt)) set_mip_major_handler (void);
void __attribute__((interrupt)) default_vector_handler (void);

/* Minor handlers that are called via software in the external handler */
void aplic_beu0_handler (void);
void aplic_beu1_handler (void);
void aplic_beu2_handler (void);
void aplic_beu3_handler (void);
void aplic_l2_handler(void);
void aplic_setip_by_num_handler (void);
void aplic_default_handler(void);

#endif /* _INTERRUPTS_H_ */
