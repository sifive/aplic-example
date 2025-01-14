/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdlib.h>

/***************************************************************************
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * IMPORTANT: See comments in interrupt.h to implement this test correctly
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * This example configures and enables BEU global interrupts routed through
 * the APLIC and demonstrates a software method to handle multiple pending and
 * enabled interrupts within one interrupt handler call. Interrupts are
 * triggered through the BEU 'accrued' register which is typically used
 * to monitor errors but can be used to trigger specific events since it
 * has R/W properties.
 ****************************************************************************/

#include <metal/machine.h>
#include <metal/machine/platform.h>
#include <metal/machine/inline.h>

#include "interrupts.h"

/* !!!!! ALERT: This can be different per IP package, make sure it's right for you !!!!!!! */
#if defined(METAL_SIFIVE_EXTENSIBLECACHE0)
#define EC_INTERRUPT_ID		0x1
#endif

/* Globals */
volatile int harts_continue = 0;
volatile int checkin_count = 0;
uint32_t software_isr_counter = 0;
uint32_t timer_isr_counter = 0;
uint32_t external_isr_counter = 0;
uint32_t beu_interrupt_num = 0;
uint32_t l2_isr_counter = 0;
uint32_t beu_accrued_value = 0;

/* These two functions override the default metal init, and speeds up simulations.
 * If no freedom-metal API components are used, these can be mostly empty.
 * See freedom-metal/src/init.c for the original, weak functions.
 * !!! User beware !!!
 * You may need to add functionality here, or remove these in
 * order to use the default metal startup. 
 */
void metal_init_run() {

    /* There is extensible cache init, chicken bit, and tty init done here */
}

void metal_secondary_init_run() {
    
    /* L2 cache init and prefetcher init done here. See init.c if you need this */
}

/* Create array of function pointers for global minor (PLIC) interrupts */
void (*aplic_minor_func[TOTAL_EXT_INTERRUPTS])(void);

/* global to keep track of boot hart as defined by our linker script */
uintptr_t boot_hart;

int main(void) {

	uint32_t i, mode = MTVEC_MODE_CLINT_VECTORED, retry;
    uint32_t simulate_beu_error, countdown, mtvec_base, context_id, return_code = 0;
    uintptr_t mip_csr;

    /* Get local hartid for every hart running this code */
	int hartid = metal_cpu_get_current_hartid();

	/* assign our boot hart location to a variable. It's OK if all harts run this code */
	boot_hart = (uintptr_t)&__metal_boot_hart;

    /* Write mstatus.mie = 0 to disable all machine interrupts prior to setup */
	interrupt_global_disable();

	/* Ensure external interrupt count is 0 before starting */
	external_isr_counter = 0;

    /* Every hart can set up their own mtvec to point to the primary
     * exception handler table using mtvec.base, and assign
     * mtvec.mode = 1 for CLINT vectored mode of operation.
     * Refer to handlers.S for the vector table. The mtvec.mode
     * field is bit[0] for designs with CLINT.
     */
    mtvec_base = (uint32_t)&__mtvec_clint_vector_table;
    write_csr (mtvec, (mtvec_base | mode));

	/* Control the flow of the harts for this application */
    if (hartid == boot_hart) {

    	/* flag to release other cores from spinning */
    	harts_continue = 1;

    } else {

        /* Other cores wait till they are told to continue */
        while(!harts_continue);

        /* All other harts write their own mie CSR to enable interrupts from APLIC */
        interrupt_external_enable();

        /* enable interrupts globally for this hart */
        interrupt_global_enable();

        /* Other harts can optionally enable a particular APLIC interrupt for itself here */

        // if (hartid == 1)
        //		Global APLIC interrupt config for this hart
        //		MIE, MSTATUS config, etc
        // else if (hartid == 2)
        //		do different work
        // ... etc ...

		/* or just spin here */
        while (1);
    }

    /* Make a default software handler for global interrupts */
    for (i = 0; i < TOTAL_EXT_INTERRUPTS; i++ ){
    	aplic_minor_func[i] = aplic_default_handler;
    }

    /************************************************/
    /*	 		Set up APLIC interrupts here		*/
    /************************************************/
    // domainCfg.globalInterrupt=1 to enable APLIC interrupts globally
    write_word(APLIC_DOMAINCFG_0_ADDR, APLIC_DOMAIN_CONFIG_GLOBAL_ENABLE);
    
    // per hart IDELIVERY set to 1 to enable
    write_word(APLIC_IDELIVERY_ADDR(hartid), ENABLE);

    // Write the threshold value for this hart. 0 means enable all interrupts to be delivered
    // A different example, if 0x4 is written here, then interrupts with priority 0-3 are allowed on this hart
    // Lower the number, the higher the priority
    write_word(APLIC_ITHRESHOLD_ADDR(hartid), PRIO_THRESH_0);	// 0=enable all interrupts.

    /****************************************/
    /* 			CSR Enables					*/
    /****************************************/
    interrupt_external_enable();	// machine external interrupt #11 enable in mie CSR
    interrupt_global_enable();		// write mstatus.mie = 1 to enable all machine interrupts globally

#if BEU0_PRESENT
    
    /* Set up PLIC "minor" software ISR functions */
    aplic_minor_func[BUSERR0_INT_NUM] = aplic_beu0_handler;
    aplic_minor_func[BUSERR1_INT_NUM] = aplic_beu1_handler;
    aplic_minor_func[BUSERR2_INT_NUM] = aplic_beu2_handler;
    aplic_minor_func[BUSERR3_INT_NUM] = aplic_beu3_handler;

    /* Enable BEU for interrupt handling for certain events. We only enable these for testing for now,
     * and this function will enable all available BEUs in the subsystem */
    beu_aplic_config(BEU_DCACHE_SINGLE_BIT_ERROR | BEU_ICACHE_ITIM_SINGLE_BIT_ERROR);

    /* This is the error we will trigger via the BEU Accrued register to simulate APLIC error handling */
    simulate_beu_error = BEU_DCACHE_SINGLE_BIT_ERROR;

       // Enable APLIC BEU interrupts and configure delivery method, priority, and whether it's a machine or supervisor interrupt
    aplic_int_enable_disable (hartid, BUSERR0_INT_NUM, APLIC_SOURCECFG_MODE_RISE_EDGE, PRIO_THRESH_2, MACHINE_INTS);
    aplic_int_enable_disable (hartid, BUSERR1_INT_NUM, APLIC_SOURCECFG_MODE_RISE_EDGE, PRIO_THRESH_2, MACHINE_INTS);
    aplic_int_enable_disable (hartid, BUSERR2_INT_NUM, APLIC_SOURCECFG_MODE_RISE_EDGE, PRIO_THRESH_2, MACHINE_INTS);
    aplic_int_enable_disable (hartid, BUSERR3_INT_NUM, APLIC_SOURCECFG_MODE_RISE_EDGE, PRIO_THRESH_2, MACHINE_INTS);
    
    printf ("Testing APLIC interrupts for hart %d\n", hartid);

    /****************************************************************************/
    /*		trigger BEU interrupt that is configured to route through APLIC		*/
    /****************************************************************************/
    switch (hartid)
    {
    	case 0:
    		write_word(BEU0_ACCRUED_ADDR, simulate_beu_error);
    		break;
    	case 1:
    		write_word(BEU1_ACCRUED_ADDR, simulate_beu_error);
    		break;
    	case 2:
    		write_word(BEU2_ACCRUED_ADDR, simulate_beu_error);
    		break;
    	case 3:
    		write_word(BEU3_ACCRUED_ADDR, simulate_beu_error);
    		break;
    }

    /* Spin here momentarily to allow the external isr to hit */
    countdown = 0xfffff;
    while ((external_isr_counter == 0) && (countdown > 0)) {
    	countdown--; asm ("nop"); 
    }

    /* Make sure we didn't count down all the way, which indicates we did not hit the external ISR */
    if ((countdown == 0) && (external_isr_counter == 0)) {
    	printf ("External handler did not get triggered! Check the setup.\n");
    	return 0xEE;
    } else {
        /* Describe what happened for this BEU error */
        printf ("Hart %d reporting BEU error: 0x%x\n", hartid, beu_accrued_value);		// BEU handler will update this value
        printf ("Total global interrupts triggered: %d\n", external_isr_counter);		// External handler will update this value
    }
    
#endif /* #if BEU0_PRESENT */

    /********************************************/
    /* 		trigger software interrupt			*/
    /********************************************/
    interrupt_software_enable();
    printf ("Testing software interrupts...\n");

    for (i = 0; i < 5; i++) {

    	software_isr_counter = 0;  // reset flag to check for spurious interrupts
    	countdown = 0xffff;

    	// trigger s/w interrupt here
    	write_word(CLINT_MSIP_ADDR_HART(hartid), 1);

    	// wait for software interrupt to fire
		while ((software_isr_counter == 0) && (countdown--)){ asm ("nop"); }

		// if the s/w interrupt did not occur and we timeout, exit with fail code
		if ((countdown == 0) && (software_isr_counter == 0)) {
			printf ("Hart %d could not trigger software interrupt - check your config!\n", hartid);
			return 0x75;
		}

		// if the s/w isr gets hit a second time without resetting the flag, we might have a spurious interrupt
		if (software_isr_counter > 1) {
			printf ("Spurious s/w interrupt detected, exiting!\n");
			return 0xFA;	// spurious interrupt occurred
		}
    }
    printf("software interrupts - OK\n");


    /****************************************/
    /* 		trigger timer interrupt 		*/
    /****************************************/
    interrupt_timer_enable();
    printf ("Testing timer interrupts....\n");

    for (i = 0; i < 5; i++) {

    	timer_isr_counter = 0;	// reset the flag to make sure we don't see spurious interrupts
    	countdown = 0xffff;

        // figure out how fast the timer is ticking so we can set the number of ticks for timer to fire in the future
        int time_a = read_word(CLINT_MTIME_BASE_ADDR);
        asm ("nop"); asm ("nop"); asm ("nop"); asm ("nop"); asm ("nop");
        int time_b = read_word(CLINT_MTIME_BASE_ADDR);

        // write mtimecmp to allow the next timer interrupt to fire
    	write_dword(CLINT_MTIMECMP_ADDR_HART(hartid), (read_word(CLINT_MTIME_BASE_ADDR) + (time_b - time_a)*20 ));

    	// wait for timer interrupt to fire
		while ((timer_isr_counter == 0) && (countdown--));

		if ((countdown == 0) && (timer_isr_counter == 0)) {
			printf ("Hart %d could not trigger timer interrupt - check your config!\n", hartid);
			return 0x77;	// timeout, return with non zero error
		}

		// if the timer isr gets hit a second time without resetting the flag, we might have a spurious interrupt, flag and exit
		if (timer_isr_counter > 1) {
			printf ("Spurious s/w interrupt detected, exiting!\n");
			return 0xFB;	// spurious interrupt occurred
		}
    }
    printf("timer interrupts - OK\n");


    /************************************************************/
    /* 		trigger APLIC external interrupt via IFORCE 		*/
    /************************************************************/
    // read topi first to make sure it's 0 before starting the test
    uint32_t read_topi = read_word(APLIC_TOPI_ADDR(hartid));
    printf ("Testing IFORCE method for APLIC interrupt...\n");

    // Only trigger via IFORCE if TOPI currently reads as 0
    if (read_topi == 0) {
    	// Create spurious external interrupt, #11
    	write_word(APLIC_IFORCE_ADDR(hartid), TRUE);	// only 0 or 1 here
    } else {
    	printf ("TOPI is reading 0x%lx, no interrupt triggered using IFORCE\n", read_topi);
    	return 0xA1;
    }
    printf("IFORCE - OK\n");


    /********************************************************************/
    /* 		trigger APLIC interrupt using SETIPNUM MMIO register		*/
    /********************************************************************/
    // Make sure this interrupt is not already pending
    if (check_setip_by_int_num(INTERRUPT_ID_FOR_SETIP_TEST) == 0) {

    	printf ("Testing SETIPNUM method for APLIC interrupt %d.\n", INTERRUPT_ID_FOR_SETIP_TEST);

    	// install the handler
    	aplic_minor_func[INTERRUPT_ID_FOR_SETIP_TEST] = aplic_setip_by_num_handler;

    	// enable this interrupt
    	aplic_int_enable_disable (hartid, INTERRUPT_ID_FOR_SETIP_TEST, APLIC_SOURCECFG_MODE_RISE_EDGE, PRIO_THRESH_2, MACHINE_INTS);

    	// trigger interrupt
    	write_word(APLIC_SETIPNUM_0_ADDR, INTERRUPT_ID_FOR_SETIP_TEST);

    	// make sure external interrupt is not pending
    	if (read_csr(mip) & (1 << CLINT_MACHINE_EXTERNAL_INT_ID))  {
    		printf ("External interrupt still pending! Check your setup!\n");
    	}
    }
    printf("SETIPNUM - OK\n");

    /************************************************/
    /* 		We are done, thank you 					*/
    /************************************************/
    return_code = 0;	// if we get here we have passed. we return non-zero as we test things above
    printf ("Exiting test with code: %d\n", return_code);

    return (return_code); /* 0=pass */
}

/* This function overrides the "secondary_main" label in freedom-metal/gloss/crt0.S,
 * since this is defined as ".weak" by default.
 * From here, we can control which harts do the work in your application.
 */
int secondary_main(void) {

	/* Don't do anything fancy, let all harts enter main() from here */
	return main();
}
