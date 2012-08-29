/*
 *  linux/arch/arm/mach-ns2816/hotplug.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/completion.h>
#include <asm/unified.h>
#include <asm/cacheflush.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <mach/board-ns2816.h>
#include "scm.h"

//#define DEBUG
extern void __cortex_a9_save(int mode);
extern volatile int pen_release;
//extern volatile int __cpuinitdata pen_release;
extern void __ns2816_lp_reset(void);

static DECLARE_COMPLETION(cpu_killed);

static inline void cpu_enter_lowpower(void)
{
	unsigned int v;

	flush_cache_all();
	asm volatile(
	"	mcr	p15, 0, %1, c7, c5, 0\n"
	"	mcr	p15, 0, %1, c7, c10, 4\n"
	/*
	 * Turn off coherency
	 */
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	bic	%0, %0, #0x20\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	bic	%0, %0, #0x04\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	  : "=&r" (v)
	  : "r" (0)
	  : "cc");
}

static inline void cpu_leave_lowpower(void)
{
	unsigned int v;

	asm volatile(	"mrc	p15, 0, %0, c1, c0, 0\n"
	"	orr	%0, %0, #0x04\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	orr	%0, %0, #0x20\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	  : "=&r" (v)
	  :
	  : "cc");
}

static inline void platform_do_lowpower(unsigned int cpu)
{
	/*
	 * there is no power-control hardware on this platform, so all
	 * we can do is put the core into WFI; this is safe as the calling
	 * code will have already disabled interrupts
	 */
	for (;;) {
		/*
		 * here's the WFI
		 */
		asm(".word	0xe320f003\n"
		    :
		    :
		    : "memory", "cc");

		if (pen_release == cpu) {
			/*
			 * OK, proper wakeup, we're done
			 */
			break;
		}

		/*
		 * getting here, means that we have come out of WFI without
		 * having been woken up - this shouldn't happen
		 *
		 * The trouble is, letting people know about this is not really
		 * possible, since we are currently running incoherently, and
		 * therefore cannot safely call printk() or anything else
		 */
#ifdef DEBUG
		printk("CPU%u: spurious wakeup call\n", cpu);
#endif
	}
}

int platform_cpu_kill(unsigned int cpu)
{
	return wait_for_completion_timeout(&cpu_killed, 5000);
}

/*
 * platform-specific code to shutdown a CPU
 *
 * Called with IRQs disabled
 */
void __cpuinitdata platform_cpu_die(unsigned int cpu)
{
	unsigned int reg;
	void  __iomem * flow = __io_address(SCM_CPU1_FLOW_REG);
#ifdef DEBUG
	unsigned int this_cpu = hard_smp_processor_id();

	if (cpu != this_cpu) {
		printk(KERN_CRIT "Eek! platform_cpu_die running on %u, should be %u\n",
			   this_cpu, cpu);
		BUG();
	}
#endif

	reg = readl(flow);
	writel(reg | CPU1_ON, flow);
	printk(KERN_NOTICE "CPU%u: %s\n", cpu, __func__);
	complete(&cpu_killed);

	/*
	 * we're ready for shutdown now, so do it
	 */
//	cpu_enter_lowpower();
//	platform_do_lowpower(cpu);
	//__asm("b .");
	reg = __raw_readl((IO_ADDRESS(NS2816_PRCM_BASE)+0x14));	
	//reg |= (3<<12);
	reg |= (2<<12);
	__raw_writel(reg, (IO_ADDRESS(NS2816_PRCM_BASE)+0x14));	
	reg = __raw_readl((IO_ADDRESS(NS2816_SCU_BASE)+0x8));
	//reg |= 0x3;
	//set cpu1 shutdown
	//reg |= (0x3<<8);
	__raw_writel(reg, (IO_ADDRESS(NS2816_SCU_BASE)+0x8));

	__raw_writel(BSYM(virt_to_phys(__ns2816_lp_reset)),
		     __io_address(SCM_WFI_ENTRY_REG));

	flush_cache_all();
	barrier();
	__cortex_a9_save(2);
	
	barrier();
	/* note: must have the following code which clr power status*/
	reg = __raw_readl((IO_ADDRESS(NS2816_SCU_BASE)+0x8));
	reg &= ~(0x3<<8);
	__raw_writel(reg, (IO_ADDRESS(NS2816_SCU_BASE)+0x8));
	barrier();

	preempt_enable_no_resched();
	/*
	 * bring this CPU back into the world of cache
	 * coherency, and then restore interrupts
	 */
//	cpu_leave_lowpower();
}

int mach_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}
int platform_cpu_disable(unsigned int cpu)
{
        /*
         * we don't allow CPU 0 to be shutdown (it is still too special
         * e.g. clock tick interrupts)
         */
        return cpu == 0 ? -EPERM : 0;
}

