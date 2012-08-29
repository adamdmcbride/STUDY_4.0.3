/*
 * arch/arm/mach-ns2816/include/mach/irqs-ns2816.h
 *
 * Copyright (C) 2010 NUFRONT Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __MACH_IRQS_NS2816_H
#define __MACH_IRQS_NS2816_H

/*
 * Irqs
 */
#define IRQ_NS2816_GIC_START			32

/*
 * NS2816 on-board gic irq sources
 */
#define IRQ_NS2816_AHB_SUB_AHBARBINT	(IRQ_NS2816_GIC_START + 0)
#define IRQ_NS2816_SDMMC		(IRQ_NS2816_GIC_START + 1)
#define IRQ_NS2816_L2_L2CCINTR		(IRQ_NS2816_GIC_START + 2)
#define IRQ_NS2816_L2_ECNTRINTR		(IRQ_NS2816_GIC_START + 3)
#define IRQ_NS2816_L2_PARRTINTR		(IRQ_NS2816_GIC_START + 4)
#define IRQ_NS2816_L2_PARRDINTR		(IRQ_NS2816_GIC_START + 5)
#define IRQ_NS2816_L2_ERRWTINTR		(IRQ_NS2816_GIC_START + 6)
#define IRQ_NS2816_L2_ERRWDINTR		(IRQ_NS2816_GIC_START + 7)
#define IRQ_NS2816_L2_ERRRTINTR		(IRQ_NS2816_GIC_START + 8)
#define IRQ_NS2816_L2_ERRRDINTR		(IRQ_NS2816_GIC_START + 9)
#define IRQ_NS2816_L2_SLVERRINTR	(IRQ_NS2816_GIC_START + 10)
#define IRQ_NS2816_L2_DECERRINTR	(IRQ_NS2816_GIC_START + 11)
#define IRQ_NS2816_DDR_ERR		(IRQ_NS2816_GIC_START + 12)
#define IRQ_NS2816_SATA_INTRQ		(IRQ_NS2816_GIC_START + 13)
#define IRQ_NS2816_GMAC_TOP_SBD		(IRQ_NS2816_GIC_START + 14)
#define IRQ_NS2816_GMAC_TOP_PMT		(IRQ_NS2816_GIC_START + 15)
#define IRQ_NS2816_USB_INTR_IRQ		(IRQ_NS2816_GIC_START + 16)
#define IRQ_NS2816_USB_OHCI_0_SMI	(IRQ_NS2816_GIC_START + 17)
#define IRQ_NS2816_USB_OHCI_0_INTR	(IRQ_NS2816_GIC_START + 18)
#define IRQ_NS2816_USB_OHCI_0_LGCY_IRQ1	(IRQ_NS2816_GIC_START + 19)
#define IRQ_NS2816_USB_OHCI_0_LGCY_IRQ12	(IRQ_NS2816_GIC_START + 20)
#define IRQ_NS2816_MALI_IRQGP		(IRQ_NS2816_GIC_START + 21)
#define IRQ_NS2816_MALI_IRQGPMMU	(IRQ_NS2816_GIC_START + 22)
#define IRQ_NS2816_MALI_IRQPP0		(IRQ_NS2816_GIC_START + 23)
#define IRQ_NS2816_MALI_IRQPPMMU0	(IRQ_NS2816_GIC_START + 24)
#define IRQ_NS2816_MALI_IRQPP1		(IRQ_NS2816_GIC_START + 25)
#define IRQ_NS2816_MALI_IRQPPMMU1	(IRQ_NS2816_GIC_START + 26)
#define IRQ_NS2816_MALI_IRQPMU		(IRQ_NS2816_GIC_START + 27)
#define IRQ_NS2816_VPU_XINTDEC		(IRQ_NS2816_GIC_START + 28)
#define IRQ_NS2816_DISPLAY_DERR		(IRQ_NS2816_GIC_START + 121)
#define IRQ_NS2816_DISPLAY_VERT		(IRQ_NS2816_GIC_START + 122)
#define IRQ_NS2816_DISPLAY_HORIZ	(IRQ_NS2816_GIC_START + 123)
#define IRQ_NS2816_PL111_VCOMP_INTR	(IRQ_NS2816_GIC_START + 112)
#define IRQ_NS2816_PL111_LNBU_INTR	(IRQ_NS2816_GIC_START + 113)
#define IRQ_NS2816_PL111_FUF_INTR	(IRQ_NS2816_GIC_START + 114)
#define IRQ_NS2816_PL111_MBE_INTR	(IRQ_NS2816_GIC_START + 115)
#define IRQ_NS2816_DMAC_FLAG4_IRQ	(IRQ_NS2816_GIC_START + 32)
#define IRQ_NS2816_DMAC_FLAG3_IRQ	(IRQ_NS2816_GIC_START + 33)
#define IRQ_NS2816_DMAC_FLAG2_IRQ	(IRQ_NS2816_GIC_START + 34)
#define IRQ_NS2816_DMAC_FLAG1_IRQ	(IRQ_NS2816_GIC_START + 35)
#define IRQ_NS2816_DMAC_FLAG0_IRQ	(IRQ_NS2816_GIC_START + 36)
#define IRQ_NS2816_DMAC_COMBINED	(IRQ_NS2816_GIC_START + 37)
#define IRQ_NS2816_TIMERS_INTR7		(IRQ_NS2816_GIC_START + 38)
#define IRQ_NS2816_TIMERS_INTR6		(IRQ_NS2816_GIC_START + 39)
#define IRQ_NS2816_TIMERS_INTR5		(IRQ_NS2816_GIC_START + 40)
#define IRQ_NS2816_TIMERS_INTR4		(IRQ_NS2816_GIC_START + 41)
#define IRQ_NS2816_TIMERS_INTR3		(IRQ_NS2816_GIC_START + 42)
#define IRQ_NS2816_TIMERS_INTR2		(IRQ_NS2816_GIC_START + 43)
#define IRQ_NS2816_TIMERS_INTR1		(IRQ_NS2816_GIC_START + 44)
#define IRQ_NS2816_TIMERS_INTR0		(IRQ_NS2816_GIC_START + 45)
#define IRQ_NS2816_GPIO_INTR7		(IRQ_NS2816_GIC_START + 46)
#define IRQ_NS2816_GPIO_INTR6		(IRQ_NS2816_GIC_START + 47)
#define IRQ_NS2816_GPIO_INTR5		(IRQ_NS2816_GIC_START + 48)
#define IRQ_NS2816_GPIO_INTR4		(IRQ_NS2816_GIC_START + 49)
#define IRQ_NS2816_GPIO_INTR3		(IRQ_NS2816_GIC_START + 50)
#define IRQ_NS2816_GPIO_INTR2		(IRQ_NS2816_GIC_START + 51)
#define IRQ_NS2816_GPIO_INTR1		(IRQ_NS2816_GIC_START + 52)
#define IRQ_NS2816_GPIO_INTR0		(IRQ_NS2816_GIC_START + 53)
#define IRQ_NS2816_UART_1_INTR		(IRQ_NS2816_GIC_START + 54)
#define IRQ_NS2816_UART_0_INTR		(IRQ_NS2816_GIC_START + 55)
#define IRQ_NS2816_I2C_2_INTR		(IRQ_NS2816_GIC_START + 56)
#define IRQ_NS2816_I2C_1_INTR		(IRQ_NS2816_GIC_START + 57)
#define IRQ_NS2816_I2C_0_INTR		(IRQ_NS2816_GIC_START + 58)
#define IRQ_NS2816_I2S_INTR		(IRQ_NS2816_GIC_START + 59)
#define IRQ_NS2816_SSI_1_INTR		(IRQ_NS2816_GIC_START + 60)
#define IRQ_NS2816_SSI_0_INTR		(IRQ_NS2816_GIC_START + 61)
#define IRQ_NS2816_DMA_IRQ_ABORT	(IRQ_NS2816_GIC_START + 62)
#define IRQ_NS2816_DMA_IRQ		(IRQ_NS2816_GIC_START + 63)
#define IRQ_NS2816_OSPREY_PMUIRQ	(IRQ_NS2816_GIC_START + 71)
#define IRQ_NS2816_OSPREY_PMUIRQ1	(IRQ_NS2816_GIC_START + 72)
#define IRQ_NS2816_OSPREY_DEFLAGS1	(IRQ_NS2816_GIC_START + 73)
#define IRQ_NS2816_OSPREY_DEFLAGS0	(IRQ_NS2816_GIC_START + 80)
#define IRQ_NS2816_OSPREY_COMMTX	(IRQ_NS2816_GIC_START + 87)
#define IRQ_NS2816_OSPREY_COMMRX	(IRQ_NS2816_GIC_START + 89)
#define IRQ_NS2816_RTC_INTR		(IRQ_NS2816_GIC_START + 91)
#define IRQ_NS2816_OSPREY_PARITYFAILSCU	(IRQ_NS2816_GIC_START + 92)
#define IRQ_NS2816_OSPREY_PARITYFAILL1	(IRQ_NS2816_GIC_START + 94)
#define IRQ_NS2816_OSPREY_PARITYFAILL0	(IRQ_NS2816_GIC_START + 102)
#define IRQ_NS2816_SCU_EV_ABOUT_INT	(IRQ_NS2816_GIC_START + 110)


#define NR_GIC_NS2816		1
/*
 * Only define NR_IRQS if less than NR_IRQS_NS2816
 */
#define NR_IRQS_NS2816		(IRQ_NS2816_GIC_START + 128)

#if !defined(NR_IRQS) || (NR_IRQS < NR_IRQS_NS2816)
#undef NR_IRQS
#define NR_IRQS			NR_IRQS_NS2816
#endif

#if !defined(MAX_GIC_NR) || (MAX_GIC_NR < NR_GIC_NS2816)
#undef MAX_GIC_NR
#define MAX_GIC_NR		NR_GIC_NS2816
#endif

#endif	/* __MACH_IRQS_NS2816_H */