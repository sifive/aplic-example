/* Copyright 2020 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <metal/machine.h>

#include "interrupts.h"

/* external globals */
extern uint32_t external_isr_counter;
extern uint32_t timer_isr_counter;
extern uint32_t software_isr_counter;
extern uint32_t l2_isr_counter;
extern uint32_t beu_accrued_value;

/* PLIC software table */
extern void (*aplic_minor_func[])(int);

/*
 *
 *
 * APLIC major interrupt handlers are below
 *
 *
 */

// Software handler is major interrupt #3
void __attribute__((interrupt)) software_handler (void) {

    software_isr_counter++;
#if DEBUG_PRINT
    //printf ("Software Handler! Count: %d\n", software_isr_counter);
#endif
    /* clear software pending in msip */
    write_word(CLINT_MSIP_ADDR_HART(metal_cpu_get_current_hartid()), 0x0);

    // system IO release to sync the MSIP write. This prevents spurious interrupts
    // Different option would be to check that mip[3] is clear before exiting
    asm ("fence ow, ow");
}

// Timer handler is major interrupt #7
void __attribute__((interrupt)) timer_handler (void) {

    int hartid = metal_cpu_get_current_hartid();

    timer_isr_counter++;
#if DEBUG_PRINT
    //printf ("Timer Handler! Count: %d\n", timer_isr_counter);
#endif

    /* Generic - set to some time way in the future to clear timer pending interrupt */
    write_dword(CLINT_MTIMECMP_ADDR_HART(hartid), (read_dword(CLINT_MTIME_BASE_ADDR) + 0x00A00000));	// write msip to value in the future

    asm ("fence ow, ow");	// system IO release to sync the mtimecmp write. This prevents spurious interrupts
}

// External Interrupt is major interrupt #11 - handles all global interrupts from APLIC
void __attribute__((interrupt)) external_handler (void) {

    uintptr_t claimi, int_id, prio, mip, countdown, aplic_pending, topi = 0;
    uint32_t hartid = metal_cpu_get_current_hartid();

    do {
        // Read claimi
        claimi = read_word(APLIC_CLAIMI_ADDR(hartid));
        int_id = (claimi >> 16) & 0x3FF;		// ID is [25:16]
        prio = (claimi & 0x3F); 				// Priority is [7:0]

        if (int_id != 0) {
#if DEBUG_PRINT
            printf ("Calling minor function for interrupt ID %d\n", int_id);
#endif
            // Call minor function based on claimi [25:16] which is ID, and [7:0] is priority
            aplic_minor_func[int_id](int_id);

#if DEBUG_PRINT
            printf ("Returned from minor handler\n");
#endif
            // Optional step for level triggered interrupt to clear the source - just an example, not a real function
            // aplic_clear_source(int_id); 

            // Read topi to see if a different (higher priority) enabled & pending interrupt comes in
            topi = read_word(APLIC_TOPI_ADDR(hartid));
            asm ("fence ir, iorw"); 	// Optional: System IO acquire for the topi read synchronization before we check it

#if DEBUG_PRINT
            printf ("Just read topi: 0x%lx\n", topi);
#endif

            // --------------------------------- NOTE ----------------------------------------
            // No functional code past here. Just print's to communicate different scenarios.
            // -------------------------------------------------------------------------------
            if ((topi != 0) && (topi != claimi)) {

                // A new global interrupt has arrived. Print debug message and loop back around to handle it.
#if DEBUG_PRINT
                printf ("**** TOPI not zero! TOPI: 0x%lx. Another enabled & pending interrupt arrived in external handler.\n", topi);
#endif
            } else if (topi == claimi) {

                // simply print the message and handle it again. Might add a max count for the same interrupt condition
                printf ("???? TOPI MMIO register reads: 0x%lx. CLAIMI reads: 0x%lx. Was this interrupt triggered again?\n");
            }
        } else {

#if DEBUG_PRINT
            // possible spurious interrupt if interrupt ID was 0 while reading CLAIMI
            printf ("$$$$ Spurious APLIC Interrupt! CLAIMI: 0x%lx, Priority: 0x%lx, was IFORCE used?\n", claimi, prio);
#endif
        }

    } while (topi != 0);

#if DEBUG_PRINT
    printf ("Exiting minor handler\n");
#endif

    external_isr_counter++; /* global to track our isr hits */

    // Help prevent inadvertent spurious external interrupts
    // Not needed when we do the TOPI read after the CLAIMI step
    //countdown = 20;
    //while ((read_csr(mip) & (1 << CLINT_MACHINE_EXTERNAL_INT_ID)) && countdown--);
}

// Major handler we are using to test the SETIP method of interrupt delivery
void __attribute__((interrupt)) set_mip_major_handler (void) {

    printf ("Set MIP major handler for interrupt ID: %d\n", INTERRUPT_ID_FOR_SET_MIP_TEST);

    // clear interrupt by writing MIP
    interrupt_local_pending_disable (INTERRUPT_ID_FOR_SET_MIP_TEST);
}

// Generic function where all interrupts will land that are not configured specifically to do something
// The default application will spin here because landing here would be considered unexpected
void __attribute__((interrupt)) default_vector_handler (void) {
    /* Add functionality if desired */
    while (1);
}

/*
 *
 *
 * Default Exception Handler. First entry of the vector table, and used
 * to handle all exception events in the system.
 *
 * Exceptions will always land at the base of the mtvec CSR, and when
 * configured for vectored mode of operation, the exception handler is
 * the first entry in the table. See handlers.S for more detail.
 *
 *
 */
void default_exception_handler(void) {

    /* Read mcause to understand the exception type */
    uint32_t mcause = read_csr(mcause);
    uint32_t mepc = read_csr(mepc);
    uint32_t mtval = read_csr(mtval);
    uint32_t code = MCAUSE_CODE(mcause);

    printf ("Exception Hit! mcause: 0x%08x, mepc: 0x%08x, mtval: 0x%08x\n", mcause, mepc, mtval);
    printf ("Mcause Exception Code: 0x%08x\n", code);
    printf("Now Exiting...\n");

    /* Exit here using non-zero return code */
    exit (0xEE);
    /* Note: You don't have to do this in your application, you may
     * choose to handle the exception and continue executing back to
     * your main application if the exit() is removed. Note that you
     * may need to manually bump the mepc CSR value to the next instruction
     * if an instruction exception occurred or else you will continue
     * to return here infinitely */
}

/*
 *
 *
 * APLIC minor interrupt handlers are below
 *
 *
 */
void aplic_default_handler() {
    /* Nothing here */
    printf ("Default APLIC handler!\n");
}

void aplic_setip_by_num_handler () {
    printf ("APLIC SETIPNUM Handler!\n");

    // clear our test interrupt
    write_word(APLIC_CLRIPNUM_0_ADDR, INTERRUPT_ID_FOR_SETIP_TEST); // clear by interrupt number
}

void aplic_l2_handler() {

#if DEBUG_PRINT
    printf ("APLIC Minor: L2 APLIC handler!\n");
#endif
    /* Clear L2 pending by writing your own code here .... */
    l2_isr_counter++;
}

void aplic_beu0_handler () {
#if BEU0_PRESENT
    /* Capture BEU code into global flag and clear BEU error, source of interrupt */
    beu_accrued_value = read_word (BEU0_ACCRUED_ADDR);

    /* Clear the interrupt */
    write_word(BEU0_ACCRUED_ADDR, 0);
#endif
}

void aplic_beu1_handler () {
#if BEU1_PRESENT
    /* Capture BEU code into global flag and clear BEU error, source of interrupt */
    beu_accrued_value = read_word (BEU1_ACCRUED_ADDR);

    /* Clear the interrupt */
    write_word(BEU1_ACCRUED_ADDR, 0);
#endif
}

void aplic_beu2_handler () {
#if BEU2_PRESENT
    /* Capture BEU code into global flag and clear BEU error, source of interrupt */
    beu_accrued_value = read_word (BEU2_ACCRUED_ADDR);

    /* Clear the interrupt */
    write_word(BEU2_ACCRUED_ADDR, 0);
#endif
}

void aplic_beu3_handler () {
#if BEU3_PRESENT
    /* Capture BEU code into global flag and clear BEU error, source of interrupt */
    beu_accrued_value = read_word (BEU3_ACCRUED_ADDR);

    /* Clear the interrupt */
    write_word(BEU3_ACCRUED_ADDR, 0);
#endif
}

/******************************************************************************
 * Enable or disable an APLIC interrupt on a hart (hardware thread).
 * Also configure the method of delivery (edge, level, disconnect, inactive).
 * Recall with APLIC, that a minor interrupt can only be mapped to a single
 * hart in the system.
 *
 * possible values of source_mode
 *    #define APLIC_SOURCECFG_MODE_INACTIVE		0
 *    #define APLIC_SOURCECFG_MODE_DETATCHED		1
 *    #define APLIC_SOURCECFG_MODE_RISE_EDGE		4
 *    #define APLIC_SOURCECFG_MODE_FALL_EDGE		5
 *    #define APLIC_SOURCECFG_MODE_HIGH_LEVEL		6
 *    #define APLIC_SOURCECFG_MODE_LOW_LEVEL		7
 *
 *****************************************************************************/
uint32_t aplic_int_enable_disable (uint32_t target_hart, uint32_t int_id, uint32_t source_mode, uint32_t priority, uint32_t m_or_s) {

    uint32_t delegate_to_s_mode;

    if (source_mode > 7) {
        printf ("APLIC SourceCfg source mode (%d) value not valid, exiting APLIC set up\n");
        return 0x8;
    }

    if (int_id > TOTAL_EXT_INTERRUPTS) {
        printf ("Interrupt ID: %d does not exist. Max interrupts is: %d\n", int_id, TOTAL_EXT_INTERRUPTS);
        return 0x8000;
    }

    // write APLIC domainCfg.globalInterrupt=1 to enable APLIC interrupts globally
    //write_word(APLIC_DOMAINCFG_0_ADDR, APLIC_DOMAIN_CONFIG_GLOBAL_ENABLE);
    // Move this global enable outside of this function

    // write APLIC sourceCfg to set edge, level, detached, or inactive, and the delegation setting
    delegate_to_s_mode = ((m_or_s == MACHINE_INTS) ? APLIC_SOURCECFG_NO_DELEGATION : APLIC_SOURCECFG_DELEGATION_TO_S);
    write_word(APLIC_SOURCECFG_0_ADDR(int_id), (source_mode | delegate_to_s_mode));

    // write APLIC target register to set the interrupt's priority - lower number is higher priority
    // Also, this register holds the bitfield(s) to specify which hart the interrupt will be sent to
    write_word(APLIC_TARGET_0_0_ADDR(int_id), ((target_hart << APLIC_TARGET_HART_BIT_POSITION) | priority));

    // Set the enable bit for this interrupt using SETIENUM
    write_word(APLIC_SETIENUM_0_ADDR, int_id);

    // Check this interrupt is properly enabled using the register and bit position in SETIE register
    if (!check_setie_by_int_num(int_id)) {
        printf ("Interrupt %d not enabled in SETIE register - check configuration!\n", int_id);
        return 0x5; 	// non-zero
    } else {
        return 0; 		// OK
    }

    return 0;
}

/* Check the enable bit for a given APLIC minor interrupt.
 * Return TRUE if enabled; FALSE if not  */
uint32_t check_setie_by_int_num(uint32_t int_id) {

    // We need to figure out which SETIE_X register, and the bit position in that register for this interrupt
    uint32_t bitshift = (int_id) % 32;	// SETIE0[1] is interrupt ID #1 for example; remainder of modulo is bit position
    uint32_t enabled;

    // The macro will return the proper base address based on interrupt ID,
    // and then a bitwise AND is done on the return value to check the SETIE bit
    // The result here will be 0 or 1
    enabled = ((read_word(APLIC_SETIE_0_ADDR(int_id)) & (1 << bitshift)) >> bitshift);

    return enabled;
}

/* Check the pending bit for a given APLIC minor interrupt.
   Return TRUE if enabled; FALSE if not						*/
uint32_t check_setip_by_int_num (uint32_t int_id) {

    // We need to figure out which SETIP_X register, and the bit position in that register for this interrupt
    uint32_t bitshift = (int_id) % 32; 	// SETIP0[1] is interrupt ID #1 for example; remainder of modulo is bit position
    uint32_t pending;

    // The macro will return the proper base address based on interrupt ID,
    // and then a bitwise AND is done on the return value to check the SETIE bit
    // The result here will be 0 or 1
    pending = ((read_word(APLIC_SETIP_0_ADDR(int_id)) & (1 << bitshift)) >> bitshift);

    return pending;
}

/* Set up all BEUs to capture errors and enable an APLIC interrupt, not local (RNMI) interrupt */
uint32_t beu_aplic_config(uint32_t error_enable) {

#if BEU0_PRESENT
    /* Set up BEU0 so we can use it to trigger PLIC interrupt */
    write_word(BEU0_ACCRUED_ADDR, 0);						/* ensure no errors exist */
    write_word(BEU0_ENABLE_ADDR, error_enable); 		/* enable event to trigger interrupt */
    write_word(BEU0_LOCAL_INTERRUPT_ADDR, 0); 				/* do not trigger any local interrupts or NMI events */
    write_word(BEU0_APLIC_INTERRUPT_ADDR, error_enable);	/* Enable global interrupt reporting */
#endif

#if BEU1_PRESENT
    /* Set up BEU1 so we can use it to trigger PLIC interrupt */
    write_word(BEU1_ACCRUED_ADDR, 0);						/* ensure no errors exist */
    write_word(BEU1_ENABLE_ADDR, error_enable); 		/* enable event to trigger interrupt */
    write_word(BEU1_LOCAL_INTERRUPT_ADDR, 0); 				/* do not trigger any local interrupts or NMI events */
    write_word(BEU1_APLIC_INTERRUPT_ADDR, error_enable);	/* Enable global interrupt reporting */
#endif

#if BEU2_PRESENT
    /* Set up BEU1 so we can use it to trigger PLIC interrupt */
    write_word(BEU2_ACCRUED_ADDR, 0);						/* ensure no errors exist */
    write_word(BEU2_ENABLE_ADDR, error_enable); 		/* enable event to trigger interrupt */
    write_word(BEU2_LOCAL_INTERRUPT_ADDR, 0); 				/* do not trigger any local interrupts or NMI events */
    write_word(BEU2_APLIC_INTERRUPT_ADDR, error_enable);	/* Enable global interrupt reporting */
#endif

#if BEU3_PRESENT
    /* Set up BEU1 so we can use it to trigger PLIC interrupt */
    write_word(BEU3_ACCRUED_ADDR, 0);						/* ensure no errors exist */
    write_word(BEU3_ENABLE_ADDR, error_enable); 		/* enable event to trigger interrupt */
    write_word(BEU3_LOCAL_INTERRUPT_ADDR, 0); 				/* do not trigger any local interrupts or NMI events */
    write_word(BEU3_APLIC_INTERRUPT_ADDR, error_enable);	/* Enable global interrupt reporting */
#endif

}

void interrupt_global_enable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mstatus, %1" : "=r"(m) : "r"(METAL_MIE_INTERRUPT));
}

void interrupt_global_disable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mstatus, %1" : "=r"(m) : "r"(METAL_MIE_INTERRUPT));
}

void interrupt_software_enable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_SW));
}

void interrupt_software_disable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_SW));
}

void interrupt_timer_enable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_TMR));
}

void interrupt_timer_disable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_TMR));
}

void interrupt_external_enable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_EXT));
}

void interrupt_external_disable (void) {
    unsigned long m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_EXT));
}

// write MIE bit to 1
void interrupt_local_enable (int id) {
    uintptr_t b = 1 << id;
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(b));
}

// clear MIE bit to 0
void interrupt_local_disable (int id) {
    uintptr_t b = 1 << id;
    uintptr_t m;
    __asm__ volatile("csrrc %0, mie, %1" : "=r"(m) : "r"(b));
}

// write MIP bit to 1
void interrupt_local_pending_enable (int id) {
    uintptr_t b = 1 << id;
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mip, %1" : "=r"(m) : "r"(b));
}

// clear MIP bit to 0
void interrupt_local_pending_disable (int id) {
    uintptr_t b = 1 << id;
    uintptr_t m;
    __asm__ volatile("csrrc %0, mip, %1" : "=r"(m) : "r"(b));
}
