/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <asm/errno.h>
#include <linux/list.h>
#include <malloc.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/tieoff.h>
#include <asm/arch/reset.h>

#include "nexell_udc_otg_regs.h"
#include "dwc2_udc_otg_priv.h"
#include <usb/lin_gadget_compat.h>
#include <usb/dwc2_udc.h>

void otg_phy_init(struct dwc2_udc *dev)
{
	/* USB PHY0 Enable */
	debug("USB PHY0 Enable\n");

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_ACAENB, 0);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_DCDENB, 0);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_VDATSRCENB, 0);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_VDATDETENB, 0);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_CHRGSEL, 0);


	nx_rstcon_setrst(RESET_ID_USB20OTG, RSTCON_ASSERT);
	udelay(10);
	nx_rstcon_setrst(RESET_ID_USB20OTG, RSTCON_NEGATE);
	udelay(10);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_OTGTUNE, 0);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_ss_scaledown_mode, 0);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_WORDINTERFACE, 1);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_WORDINTERFACE_ENB, 1);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_VBUSVLDEXT, 0);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_VBUSVLDEXTSEL, 0);
	udelay(10);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_POR, 0);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_POR_ENB, 1);
	udelay(10);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_POR, 1);
	udelay(10);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_POR, 0);
	udelay(40);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_nResetSync, 1);
	udelay(10);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_nUtmiResetSync, 1);
	udelay(10);
}

void otg_phy_off(struct dwc2_udc *dev)
{
	/* USB PHY0 Disable */
	debug("USB PHY0 Disable\n");

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_VBUSVLDEXT, 1);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_VBUSVLDEXTSEL, 1);
	udelay(10);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_nResetSync, 0);
	udelay(10);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_nUtmiResetSync, 0);
	udelay(10);

	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_POR, 1);
	nx_tieoff_set(NX_TIEOFF_USB20OTG0_i_POR_ENB, 1);
	udelay(10);

	nx_rstcon_setrst(RESET_ID_USB20OTG, RSTCON_ASSERT);
	udelay(10);
}
