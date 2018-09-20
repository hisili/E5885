/*lint -save -e26 -e42 -e64 --e{402} -e438 -e564 -e572 -e665 -e413 -e527 -e537 --e{529} --e{530} --e{533}-e539 -e548
-e613 -e648 -e666 -e701 -e702 -e713 -e718 -e732 -e734 -e737 -e746 --e{752} -e762 --e{830} -e958*/

/**
 * gadget.c - DesignWare USB3 DRD Controller Gadget Framework Link
 *
 * Copyright (C) 2010-2011 Texas Instruments Incorporated - http://www.ti.com
 * Authors: Felipe Balbi <balbi@ti.com>,
 *	    Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright (C) 2013 Huawei Technologies Co., Ltd.
 * 2013-7-19 Adopt&bugfix for hisi balong platform. Zhongshun Wang<foss@huawei.com>
 * 2015-10-15 Add USB Hibernation featrue. Dongyue Chen<foss@huawei.com>
 * 2015-12-06 Add USB Balong phy hardware bug work around. Dongyue Chen<foss@huawei.com>
 * 2016-06-06 Add USB 3.0 function remote wakeup support. Dongyue Chen<foss@huawei.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */




#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>

#include <linux/usb/otg.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/balong_usb_nv.h>

#if (FEATURE_ON == MBB_USB)
#include <product_config.h>
#endif

#include "product_config.h"
#include "core.h"
#include "gadget.h"
#include "io.h"
#include "bsp_slice.h"

#if (FEATURE_ON == MBB_USB)
#include "../mbb_usb_unitary/usb_vendor.h"
#define RECONNECT_DELAYTIME         (100)
#else
#include "../gadget/usb_vendor.h"
#endif


#ifdef CONFIG_GADGET_CORE_HIBERNATION
#include "bsp_pmu.h"
#include "dwc3_conn_ctrl_balong.h"

extern void syscrg_vcc_reset_for_lpm(void);
extern void syscrg_vcc_unreset_for_lpm(void);
extern void syscrg_usb_set_pmu_state(int value);
extern int syscrg_usb_get_pmu_state(unsigned int is_superspeed);

extern int syscrg_usb2_get_pmu_state(void);
extern int syscrg_usb3_get_pmu_state(void);
extern int dwc3_pmuctrl_set_usbphy_clk_mode(int is_superspeed);
extern int dwc3_pmuctrl_set_usbphy_pwctrl_mode(int is_support_wakeup);

static int dwc3_gadget_hibernation_irq_register(struct dwc3 *dwc);
static int dwc3_gadget_hibernation_init(struct dwc3 *dwc);
static void dwc3_gadget_hibernation_exit(struct dwc3 *dwc);

void dwc3_gadget_hibernation_disconnect(struct dwc3 * dwc);
void dwc3_gadget_hibernation_suspend(struct dwc3 * dwc);
int dwc3_gadget_hibernation_resume(struct dwc3 * dwc);
void dwc3_gadget_hibernation_stop_transfers(struct dwc3 *dwc);
int dwc3_gadget_hibernation_timeout_reset(struct dwc3 *dwc);
static int dwc3_gadget_hibernation_reinit(struct dwc3 *dwc);


#endif

#define USB_US_TO_32KHZ(us)         ((us)*10/305)

static void dwc3_disable_phy_by_speed(struct dwc3 *dwc, unsigned int speed);
static void dwc3_enable_both_phy(void);
void dwc3_intr_disable(struct dwc3 *dwc);
void dwc3_intr_enable(struct dwc3 *dwc);
static void dwc3_gadget_intr_bh(unsigned long _params);

extern void syssc_usb_powerdown_hsp(int value);
extern void syssc_usb_powerdown_ssp(int value);


static struct dwc3* the_dwc3 = NULL;

#if (FEATURE_ON == MBB_USB)

static int g_dwc_phy_auto_pwrdown = 0;
#endif

/**
 * dwc3_gadget_set_test_mode - Enables USB2 Test Modes
 * @dwc: pointer to our context structure
 * @mode: the mode to set (J, K SE0 NAK, Force Enable)
 *
 * Caller should take care of locking. This function will
 * return 0 on success or -EINVAL if wrong Test Selector
 * is passed
 */
int dwc3_gadget_set_test_mode(struct dwc3 *dwc, int mode)
{
	u32		reg;

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_TSTCTRL_MASK;

	switch (mode) {
	case TEST_J:
	case TEST_K:
	case TEST_SE0_NAK:
	case TEST_PACKET:
	case TEST_FORCE_EN:
		reg |= mode << 1;
		break;
	default:
		return -EINVAL;
	}

	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	return 0;
}

/**
 * dwc3_gadget_set_link_state - Sets USB Link to a particular State
 * @dwc: pointer to our context structure
 * @state: the state to put link into
 *
 * Caller should take care of locking. This function will
 * return 0 on success or -ETIMEDOUT.
 */
int dwc3_gadget_set_link_state(struct dwc3 *dwc, enum dwc3_link_state state)
{
	int		retries = 10000;
	u32		reg;

	/*
	 * Wait until device controller is ready. Only applies to 1.94a and
	 * later RTL.
	 */
	if (dwc->revision >= DWC3_REVISION_194A) {
		while (--retries) {
			reg = dwc3_readl(dwc->regs, DWC3_DSTS);
			if (reg & DWC3_DSTS_DCNRD)
				udelay(5);
			else
				break;
		}

		if (retries <= 0)
			return -ETIMEDOUT;
	}

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;

	/* set requested state */
	reg |= DWC3_DCTL_ULSTCHNGREQ(state);
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	/*
	 * The following code is racy when called from dwc3_gadget_wakeup,
	 * and is not needed, at least on newer versions
	 */
	if (dwc->revision >= DWC3_REVISION_194A)
		return 0;

	/* wait for a change in DSTS */
	retries = 10000;
	while (--retries) {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);

		if (DWC3_DSTS_USBLNKST(reg) == state)
			return 0;

		udelay(5);
	}

	dev_vdbg(dwc->dev, "link state change request timed out\n");

	return -ETIMEDOUT;
}
/**
 * dwc3_gadget_get_link_state - Gets USB Link State
 * @dwc: pointer to our context structure
 *
 * Return:
 * state: the state to put link into
 *
 */
static inline enum dwc3_link_state dwc3_gadget_get_link_state(struct dwc3 *dwc)
{
    enum dwc3_link_state	state;
	u32			reg;

	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	state = (enum dwc3_link_state)DWC3_DSTS_USBLNKST(reg);

    return state;
}
/**
 * dwc3_gadget_resize_tx_fifos - reallocate fifo spaces for current use-case
 * @dwc: pointer to our context structure
 *
 * This function will a best effort FIFO allocation in order
 * to improve FIFO usage and throughput, while still allowing
 * us to enable as many endpoints as possible.
 *
 * Keep in mind that this operation will be highly dependent
 * on the configured size for RAM1 - which contains TxFifo -,
 * the amount of endpoints enabled on coreConsultant tool, and
 * the width of the Master Bus.
 *
 * In the ideal world, we would always be able to satisfy the
 * following equation:
 *
 * ((512 + 2 * MDWIDTH-Bytes) + (Number of IN Endpoints - 1) * \
 * (3 * (1024 + MDWIDTH-Bytes) + MDWIDTH-Bytes)) / MDWIDTH-Bytes
 *
 * Unfortunately, due to many variables that's not always the case.
 */
int dwc3_gadget_resize_tx_fifos(struct dwc3 *dwc)
{
	int		last_fifo_depth = 0;
	int		ram1_depth;
	int		fifo_size;
	int		mdwidth;
	int		num;
	unsigned int fifo_size_cov;
		
	if (!dwc->needs_fifo_resize)
		return 0;

	ram1_depth = DWC3_RAM1_DEPTH(dwc->hwparams.hwparams7);
	mdwidth = DWC3_MDWIDTH(dwc->hwparams.hwparams0);

	/* MDWIDTH is represented in bits, we need it in bytes */
	mdwidth >>= 3;

	/*
	 * FIXME For now we will only allocate 1 wMaxPacketSize space
	 * for each enabled endpoint, later patches will come to
	 * improve this algorithm so that we better use the internal
	 * FIFO space
	 */
	for (num = 0; num < DWC3_ENDPOINTS_NUM; num++) {
		struct dwc3_ep	*dep = dwc->eps[num];
		int		fifo_number = dep->number >> 1;
		int		mult = 1;
		int		tmp;

		if (!(dep->number & 1))
			continue;

		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;

		if (usb_endpoint_xfer_bulk(dep->endpoint.desc)
				|| usb_endpoint_xfer_isoc(dep->endpoint.desc))
			mult = 3;

		/*
		 * REVISIT: the following assumes we will always have enough
		 * space available on the FIFO RAM for all possible use cases.
		 * Make sure that's true somehow and change FIFO allocation
		 * accordingly.
		 *
		 * If we have Bulk or Isochronous endpoints, we want
		 * them to be able to be very, very fast. So we're giving
		 * those endpoints a fifo_size which is enough for 3 full
		 * packets
		 */
		tmp = mult * (dep->endpoint.maxpacket + mdwidth);
		tmp += mdwidth;

		fifo_size = DIV_ROUND_UP(tmp, mdwidth);

		fifo_size |= (last_fifo_depth << 16);

		/*for coverity warning only. */
		fifo_size_cov = (unsigned int)fifo_size;

		dev_vdbg(dwc->dev, "%s: Fifo Addr %04x Size %d\n",
				dep->name, last_fifo_depth, fifo_size_cov & 0xffff);

		dwc3_writel(dwc->regs, DWC3_GTXFIFOSIZ(fifo_number),
				fifo_size);

		last_fifo_depth += (fifo_size & 0xffff);
	}

	return 0;
}

void dwc3_gadget_giveback(struct dwc3_ep *dep, struct dwc3_request *req,
		int status)
{
	struct dwc3			*dwc = dep->dwc;
	int				i;

	if (req->queued) {
		i = 0;
		do {
			dep->busy_slot++;
			/*
			 * Skip LINK TRB. We can't use req->trb and check for
			 * DWC3_TRBCTL_LINK_TRB because it points the TRB we
			 * just completed (not the LINK TRB).
			 */
			if (((dep->busy_slot & DWC3_TRB_MASK) ==
				DWC3_TRB_NUM- 1) &&
				usb_endpoint_xfer_isoc(dep->endpoint.desc))
				dep->busy_slot++;

            if(IS_ENABLED(CONFIG_USB_DWC3_NOLOOP_SGS)){
    			if (req->request.num_mapped_sgs){
                    i += req->request.num_mapped_sgs - 1;
                    dep->busy_slot += req->request.num_mapped_sgs - 1;
                }
            }
		} while(++i < req->request.num_mapped_sgs);

        /* BUGFIX: ZLP refresh trb on complete scene */
    	if (req->request.zero){
            dep->busy_slot++;
        }

		req->queued = false;
	}
	list_del(&req->list);
	req->trb = NULL;
	req->request.zero = 0;  /* let zero to default */

	if (req->request.status == -EINPROGRESS)
		req->request.status = status;

	if (dwc->ep0_bounced && dep->number == 0)
		dwc->ep0_bounced = false;
	else
		usb_gadget_unmap_request(&dwc->gadget, &req->request,
				req->direction);

	DWC3_TRACE("request %pK from %s completed %d/%d ===> %d\n",
			req, dep->name, req->request.actual,
			req->request.length, status);

	spin_unlock(&dwc->lock);
	req->request.complete(&dep->endpoint, &req->request);
	spin_lock(&dwc->lock);
}

static const char *dwc3_gadget_ep_cmd_string(u8 cmd)
{
	switch (cmd) {
	case DWC3_DEPCMD_DEPSTARTCFG:
		return "Start New Configuration";
	case DWC3_DEPCMD_ENDTRANSFER:
		return "End Transfer";
	case DWC3_DEPCMD_UPDATETRANSFER:
		return "Update Transfer";
	case DWC3_DEPCMD_STARTTRANSFER:
		return "Start Transfer";
	case DWC3_DEPCMD_CLEARSTALL:
		return "Clear Stall";
	case DWC3_DEPCMD_SETSTALL:
		return "Set Stall";
	case DWC3_DEPCMD_GETEPSTATE:
		return "Get Endpoint State";
	case DWC3_DEPCMD_SETTRANSFRESOURCE:
		return "Set Endpoint Transfer Resource";
	case DWC3_DEPCMD_SETEPCONFIG:
		return "Set Endpoint Configuration";
	default:
		return "UNKNOWN command";
	}
}

int dwc3_send_gadget_generic_command(struct dwc3 *dwc, int cmd, u32 param)
{
	u32		timeout = 500;
	u32		reg;
	u32		cmd_reg;
	
	cmd_reg = (unsigned int)cmd;
	
	dwc3_writel(dwc->regs, DWC3_DGCMDPAR, param);
	dwc3_writel(dwc->regs, DWC3_DGCMD, cmd_reg | DWC3_DGCMD_CMDACT);

	do {
		reg = dwc3_readl(dwc->regs, DWC3_DGCMD);
		if (!(reg & DWC3_DGCMD_CMDACT)) {
			DWC3_TRACE("Command Complete --> %d\n",
					DWC3_DGCMD_STATUS(reg));
			return 0;
		}

		/*
		 * We can't sleep here, because it's also called from
		 * interrupt context.
		 */
		timeout--;
		if (!timeout){
            dwc->cmd_err++;
            DWC3_ERR("Command Timeout --> %d\n",
					DWC3_DGCMD_STATUS(reg));
			return -ETIMEDOUT;
		}
		udelay(1);
	} while (1);
}

int dwc3_send_gadget_ep_cmd(struct dwc3 *dwc, unsigned ep,
		unsigned cmd, struct dwc3_gadget_ep_cmd_params *params)
{
    #ifdef DEBUG
	struct dwc3_ep		*dep = dwc->eps[ep];
    #endif
	u32			timeout = 500;
	u32			reg;

	DWC3_TRACE("%s: cmd '%s'(%08x) params %08x %08x %08x\n",
			dep->name,
			dwc3_gadget_ep_cmd_string(cmd), cmd, params->param0,
			params->param1, params->param2);

	dwc3_writel(dwc->regs, DWC3_DEPCMDPAR0(ep), params->param0);
	dwc3_writel(dwc->regs, DWC3_DEPCMDPAR1(ep), params->param1);
	dwc3_writel(dwc->regs, DWC3_DEPCMDPAR2(ep), params->param2);

	dwc3_writel(dwc->regs, DWC3_DEPCMD(ep), cmd | DWC3_DEPCMD_CMDACT);
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DEPCMD(ep));
		if (!(reg & DWC3_DEPCMD_CMDACT)) {
			DWC3_TRACE("Command Complete --> %d\n",
					DWC3_DEPCMD_STATUS(reg));
			return 0;
		}

		/*
		 * We can't sleep here, because it is also called from
		 * interrupt context.
		 */
		timeout--;
		if (!timeout)
			return -ETIMEDOUT;

		udelay(1);
	} while (1);
}

static dma_addr_t dwc3_trb_dma_offset(struct dwc3_ep *dep,
		struct dwc3_trb *trb)
{
	u32		offset = (char *) trb - (char *) dep->trb_pool;

	return dep->trb_pool_dma + offset;
}

static int dwc3_alloc_trb_pool(struct dwc3_ep *dep)
{
	struct dwc3		*dwc = dep->dwc;

	if (dep->trb_pool)
		return 0;

	if (dep->number == 0 || dep->number == 1)
		return 0;

    BUILD_BUG_ON_NOT_POWER_OF_2(DWC3_TRB_NUM);

	dep->trb_pool = dma_alloc_coherent(dwc->dev,
			sizeof(struct dwc3_trb) * DWC3_TRB_NUM,
			&dep->trb_pool_dma, GFP_KERNEL);
	if (!dep->trb_pool) {
		dev_err(dep->dwc->dev, "failed to allocate trb pool for %s\n",
				dep->name);
		return -ENOMEM;
	}

	return 0;
}

static void dwc3_free_trb_pool(struct dwc3_ep *dep)
{
	struct dwc3		*dwc = dep->dwc;

    /* ep0 trb_pool not in here, we must check the pointer */
    if (dep->trb_pool) {
    	dma_free_coherent(dwc->dev, sizeof(struct dwc3_trb) * DWC3_TRB_NUM,
    			dep->trb_pool, dep->trb_pool_dma);
    }
	dep->trb_pool = NULL;
	dep->trb_pool_dma = 0;
}

#ifdef CONFIG_USB_OTG_DWC_BALONG
static int dwc3_xmit_host_role_request(struct dwc3 *dwc, u32 param)
{
    return dwc3_send_gadget_generic_command(dwc,
                DWC3_DGCMD_XMIT_HOST_ROLE_REQUEST, param);
}
#endif

static int dwc3_gadget_start_config(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;
	u32			cmd;

	memset(&params, 0x00, sizeof(params));

	if (dep->number != 1) {
		cmd = DWC3_DEPCMD_DEPSTARTCFG;
		/* XferRscIdx == 0 for ep0 and 2 for the remaining */
		if (dep->number > 1) {
			if (dwc->start_config_issued)
				return 0;
			dwc->start_config_issued = true;
			cmd |= DWC3_DEPCMD_PARAM(2);
		}

		return dwc3_send_gadget_ep_cmd(dwc, 0, cmd, &params);
	}

	return 0;
}

static int dwc3_gadget_set_ep_config(struct dwc3 *dwc, struct dwc3_ep *dep,
		const struct usb_endpoint_descriptor *desc,
		const struct usb_ss_ep_comp_descriptor *comp_desc,
		bool ignore)
{
	struct dwc3_gadget_ep_cmd_params params;

	memset(&params, 0x00, sizeof(params));

	params.param0 = DWC3_DEPCFG_EP_TYPE(usb_endpoint_type(desc))
		| DWC3_DEPCFG_MAX_PACKET_SIZE(usb_endpoint_maxp(desc));

	/* Burst size is only needed in SuperSpeed mode */
	if (dwc->gadget.speed == USB_SPEED_SUPER) {
		u32 burst = dep->endpoint.maxburst - 1;

		params.param0 |= DWC3_DEPCFG_BURST_SIZE(burst);
	}

	if (ignore)
		params.param0 |= DWC3_DEPCFG_IGN_SEQ_NUM;/*lint !e648*/

	params.param1 = DWC3_DEPCFG_XFER_COMPLETE_EN
		| DWC3_DEPCFG_XFER_NOT_READY_EN;

    if(dep->endpoint.masknotready){
    	params.param1 &= ~DWC3_DEPCFG_XFER_NOT_READY_EN;
    }

	if (usb_ss_max_streams(comp_desc) && usb_endpoint_xfer_bulk(desc)) {
		params.param1 |= DWC3_DEPCFG_STREAM_CAPABLE
			| DWC3_DEPCFG_STREAM_EVENT_EN;
		dep->stream_capable = true;
	}

	if (usb_endpoint_xfer_isoc(desc))
		params.param1 |= DWC3_DEPCFG_XFER_IN_PROGRESS_EN;

    if(dep->endpoint.enable_xfer_in_progress){	/* enable the XFERINPROGRESS intr, such as out endpoint of rndis/ecm */
    	params.param1 |= DWC3_DEPCFG_XFER_IN_PROGRESS_EN;
    }

	/*
	 * We are doing 1:1 mapping for endpoints, meaning
	 * Physical Endpoints 2 maps to Logical Endpoint 2 and
	 * so on. We consider the direction bit as part of the physical
	 * endpoint number. So USB endpoint 0x81 is 0x03.
	 */
	params.param1 |= DWC3_DEPCFG_EP_NUMBER(dep->number);

	/*
	 * We must use the lower 16 TX FIFOs even though
	 * HW might have more
	 */
	if (dep->direction)
		params.param0 |= DWC3_DEPCFG_FIFO_NUMBER(dep->number >> 1);

	if (desc->bInterval) {
		params.param1 |= DWC3_DEPCFG_BINTERVAL_M1(desc->bInterval - 1);
		dep->interval = 1 << (desc->bInterval - 1);
	}

#ifdef CONFIG_GADGET_CORE_HIBERNATION

	if(dwc->restore_flag){
		params.param0 |= DWC3_DEPCFG_ACTION_RESTORE;
		params.param2 = dep->saved_state;
	}
#endif

	return dwc3_send_gadget_ep_cmd(dwc, dep->number,
			DWC3_DEPCMD_SETEPCONFIG, &params);
}

static int dwc3_gadget_set_xfer_resource(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;

	memset(&params, 0x00, sizeof(params));

	params.param0 = DWC3_DEPXFERCFG_NUM_XFER_RES(1);

	return dwc3_send_gadget_ep_cmd(dwc, dep->number,
			DWC3_DEPCMD_SETTRANSFRESOURCE, &params);
}

/**
 * __dwc3_gadget_ep_enable - Initializes a HW endpoint
 * @dep: endpoint to be initialized
 * @desc: USB Endpoint Descriptor
 *
 * Caller should take care of locking
 */
static int __dwc3_gadget_ep_enable(struct dwc3_ep *dep,
		const struct usb_endpoint_descriptor *desc,
		const struct usb_ss_ep_comp_descriptor *comp_desc,
		bool ignore)
{
	struct dwc3		*dwc = dep->dwc;
	u32			reg;
	int			ret = -ENOMEM;

	dep->stat.enable_cnt++;

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		ret = dwc3_gadget_start_config(dwc, dep);
		if (ret)
			return ret;
	}

	ret = dwc3_gadget_set_ep_config(dwc, dep, desc, comp_desc, ignore);
	if (ret)
		return ret;

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		struct dwc3_trb	*trb_st_hw;
		struct dwc3_trb	*trb_link;

		ret = dwc3_gadget_set_xfer_resource(dwc, dep);
		if (ret)
			return ret;

		dep->endpoint.desc = desc;
		dep->comp_desc = comp_desc;
		dep->type = usb_endpoint_type(desc);
		dep->flags |= DWC3_EP_ENABLED;

		reg = dwc3_readl(dwc->regs, DWC3_DALEPENA);
		reg |= DWC3_DALEPENA_EP(dep->number);
		dwc3_writel(dwc->regs, DWC3_DALEPENA, reg);

		if (!usb_endpoint_xfer_isoc(desc))
			return 0;

		memset(&trb_link, 0, sizeof(trb_link));

		/* Link TRB for ISOC. The HWO bit is never reset */
		trb_st_hw = &dep->trb_pool[0];

		trb_link = &dep->trb_pool[DWC3_TRB_NUM - 1];

		trb_link->bpl = lower_32_bits(dwc3_trb_dma_offset(dep, trb_st_hw));
		trb_link->bph = upper_32_bits(dwc3_trb_dma_offset(dep, trb_st_hw));
		trb_link->ctrl |= DWC3_TRBCTL_LINK_TRB;
		trb_link->ctrl |= DWC3_TRB_CTRL_HWO;
	}

	return 0;
}

static void dwc3_stop_active_transfer(struct dwc3 *dwc, u32 epnum, bool force);
static void dwc3_remove_requests(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_request		*req;

	if (!list_empty(&dep->req_queued)) {
		dwc3_stop_active_transfer(dwc, dep->number, true);

		/* - giveback all requests to gadget driver */
		while (!list_empty(&dep->req_queued)) {
			req = next_request(&dep->req_queued);
			if(!req){
				dev_dbg(dwc->dev, "next_request NULL\n");
				return;
			}
			dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
		}
	}

	while (!list_empty(&dep->request_list)) {
		req = next_request(&dep->request_list);
		if(!req){
			dev_dbg(dwc->dev, "next_request NULL\n");
			return;
		}

		dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
	}
}

/**
 * __dwc3_gadget_ep_disable - Disables a HW endpoint
 * @dep: the endpoint to disable
 *
 * This function also removes requests which are currently processed ny the
 * hardware and those which are not yet scheduled.
 * Caller should take care of locking.
 */
static int __dwc3_gadget_ep_disable(struct dwc3_ep *dep)
{
	struct dwc3		*dwc = dep->dwc;
	u32			reg;

    dep->stat.disable_cnt++;

	dwc3_remove_requests(dwc, dep);

	reg = dwc3_readl(dwc->regs, DWC3_DALEPENA);
	reg &= ~DWC3_DALEPENA_EP(dep->number);/*lint !e502*/
	dwc3_writel(dwc->regs, DWC3_DALEPENA, reg);

	dep->stream_capable = false;
	dep->endpoint.desc = NULL;
	dep->comp_desc = NULL;
	dep->type = 0;
	dep->flags = 0;

	return 0;
}

/* -------------------------------------------------------------------------- */

static int dwc3_gadget_ep0_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	return -EINVAL;
}

static int dwc3_gadget_ep0_disable(struct usb_ep *ep)
{
	return -EINVAL;
}

/* -------------------------------------------------------------------------- */

static int dwc3_gadget_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct dwc3_ep			*dep;
	struct dwc3			*dwc;
	unsigned long			flags;
	int				ret;

	if (!ep || !desc || desc->bDescriptorType != USB_DT_ENDPOINT) {
		pr_debug("dwc3: invalid parameters\n");
		return -EINVAL;
	}

	if (!desc->wMaxPacketSize) {
		pr_debug("dwc3: missing wMaxPacketSize\n");
		return -EINVAL;
	}

	dep = to_dwc3_ep(ep);
	dwc = dep->dwc;

	if (dep->flags & DWC3_EP_ENABLED) {
		dev_WARN_ONCE(dwc->dev, true, "%s is already enabled\n",
				dep->name);
		return 0;
	}

	switch (usb_endpoint_type(desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		strlcat(dep->name, "-control", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_ISOC:
		strlcat(dep->name, "-isoc", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_BULK:
		strlcat(dep->name, "-bulk", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_INT:
		strlcat(dep->name, "-int", sizeof(dep->name));
		break;
	default:
		dev_err(dwc->dev, "invalid endpoint transfer type\n");
	}

	DWC3_TRACE("Enabling %s\n", dep->name);

	spin_lock_irqsave(&dwc->lock, flags);

#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(unlikely(1 == dwc->gadget.is_suspend && bsp_usb_is_support_hibernation())){
		dwc3_gadget_hibernation_soft_resume(dwc);
	}
#endif

	ret = __dwc3_gadget_ep_enable(dep, desc, ep->comp_desc, false);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_ep_disable(struct usb_ep *ep)
{
	struct dwc3_ep			*dep;
	struct dwc3			*dwc;
	unsigned long			flags;
	int				ret;

	if (!ep) {
		pr_debug("dwc3: invalid parameters\n");
		return -EINVAL;
	}

	dep = to_dwc3_ep(ep);
	dwc = dep->dwc;

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		dev_WARN_ONCE(dwc->dev, true, "%s is already disabled\n",
				dep->name);
		return 0;
	}

	snprintf(dep->name, sizeof(dep->name), "ep%d%s",
			dep->number >> 1,
			(dep->number & 1) ? "in" : "out");

	spin_lock_irqsave(&dwc->lock, flags);
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(unlikely(1 == dwc->gadget.is_suspend && bsp_usb_is_support_hibernation())){
		dwc3_gadget_hibernation_soft_resume(dwc);
	}
#endif

	ret = __dwc3_gadget_ep_disable(dep);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static struct usb_request *dwc3_gadget_ep_alloc_request(struct usb_ep *ep,
	gfp_t gfp_flags)
{
	struct dwc3_request		*req;
	struct dwc3_ep			*dep = to_dwc3_ep(ep);
	struct dwc3			*dwc = dep->dwc;

	req = kzalloc(sizeof(*req), gfp_flags);
	if (!req) {
		dev_err(dwc->dev, "not enough memory\n");
		return NULL;
	}

	req->epnum	= dep->number;
	req->dep	= dep;
	return &req->request;
}

static void dwc3_gadget_ep_free_request(struct usb_ep *ep,
		struct usb_request *request)
{
	struct dwc3_request		*req = to_dwc3_request(request);

	kfree(req);
}

static inline bool dwc3_ep_is_cleanup(struct dwc3_ep *dep)
{
    return (dep->free_slot == dep->busy_slot);
}

static inline bool dwc3_ep_is_complete(struct dwc3_ep *dep)
{
    struct dwc3_trb	*trb;
    u32 cur_slot = dep->free_slot;

    if(dwc3_ep_is_cleanup(dep))
        return true;

    trb = &dep->trb_pool[(cur_slot-1) & DWC3_TRB_MASK];
    DWC3_BUG(("this trb must be last!\n"), !(trb->ctrl & DWC3_TRB_CTRL_LST));/*lint !e548*/

    return (trb->ctrl & DWC3_TRB_CTRL_HWO)?false:true;
}

/**
 * dwc3_prepare_one_trb - setup one TRB from one request
 * @dep: endpoint for which this request is prepared
 * @req: dwc3_request pointer
 */
static void dwc3_prepare_one_trb(struct dwc3_ep *dep,
		struct dwc3_request *req, dma_addr_t dma,
		unsigned length, unsigned last, unsigned chain, unsigned node)
{
	struct dwc3_trb		*trb;

	DWC3_TRACE("%s: req %pK dma %08llx length %d%s%s\n",
			dep->name, req, (unsigned long long) dma,
			length, last ? " last" : "",
			chain ? " chain" : "");

	/* Skip the LINK-TRB on ISOC */
	if (((dep->free_slot & DWC3_TRB_MASK) == DWC3_TRB_NUM - 1) &&
			usb_endpoint_xfer_isoc(dep->endpoint.desc))
		dep->free_slot++;

	trb = &dep->trb_pool[dep->free_slot & DWC3_TRB_MASK];

	if (!req->trb) {
		dwc3_gadget_move_request_queued(req);
		req->trb = trb;
		req->trb_dma = dwc3_trb_dma_offset(dep, trb);
		req->start_slot = dep->free_slot & DWC3_TRB_MASK;
	}

	dep->free_slot++;

	trb->size = DWC3_TRB_SIZE_LENGTH(length);
	trb->bpl = lower_32_bits(dma);
	trb->bph = upper_32_bits(dma);

	switch (usb_endpoint_type(dep->endpoint.desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		trb->ctrl = DWC3_TRBCTL_CONTROL_SETUP;
		break;

	case USB_ENDPOINT_XFER_ISOC:
		if (!node)
			trb->ctrl = DWC3_TRBCTL_ISOCHRONOUS_FIRST;
		else
			trb->ctrl = DWC3_TRBCTL_ISOCHRONOUS;

		if (!req->request.no_interrupt && !chain)
			trb->ctrl |= DWC3_TRB_CTRL_IOC;
		break;

	case USB_ENDPOINT_XFER_BULK:
		trb->ctrl = DWC3_TRBCTL_NORMAL;
		if (!req->request.no_interrupt
            && dep->endpoint.enable_xfer_in_progress) {
            if (req->direction) /* in */ {
    			/* trb->ctrl |= DWC3_TRB_CTRL_IOC; */
            }
            else {
                trb->ctrl |= DWC3_TRB_CTRL_ISP_IMI;
                trb->ctrl |= DWC3_TRB_CTRL_CSP;
            }
		}
		break;
	case USB_ENDPOINT_XFER_INT:
		trb->ctrl = DWC3_TRBCTL_NORMAL;
		break;
	default:
		/*
		 * This is only possible with faulty memory because we
		 * checked it already :)
		 */
		BUG();
	}

	if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
		trb->ctrl |= DWC3_TRB_CTRL_ISP_IMI;
		trb->ctrl |= DWC3_TRB_CTRL_CSP;
	} else if (last) {
		trb->ctrl |= DWC3_TRB_CTRL_LST;
	}

	if (chain)
		trb->ctrl |= DWC3_TRB_CTRL_CHN;

	if (usb_endpoint_xfer_bulk(dep->endpoint.desc) && dep->stream_capable)
		trb->ctrl |= DWC3_TRB_CTRL_SID_SOFN(req->request.stream_id);

	trb->ctrl |= DWC3_TRB_CTRL_HWO;
}

/*
 * dwc3_prepare_trbs - setup TRBs from requests
 * @dep: endpoint for which requests are being prepared
 * @starting: true if the endpoint is idle and no requests are queued.
 *
 * The function goes through the requests list and sets up TRBs for the
 * transfers. The function returns once there are no more TRBs available or
 * it runs out of requests.
 */
static void dwc3_prepare_trbs(struct dwc3_ep *dep, bool starting)
{
	struct dwc3_request	*req, *n;
	struct dwc3			*dwc = dep->dwc;
	u32			trbs_left;
	u32			trbs_left_n;
	u32			max;
	u32			last_one = 0;

	BUILD_BUG_ON_NOT_POWER_OF_2(DWC3_TRB_NUM);

	/* the first request must not be queued */
	trbs_left = (dep->busy_slot - dep->free_slot) & DWC3_TRB_MASK;

	/* Can't wrap around on a non-isoc EP since there's no link TRB */
	if (!usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
		max = DWC3_TRB_NUM - (dep->free_slot & DWC3_TRB_MASK);
		if (trbs_left > max)
			trbs_left = max;
	}

	/*
	 * If busy & slot are equal than it is either full or empty. If we are
	 * starting to process requests then we are empty. Otherwise we are
	 * full and don't do anything
	 */
	if (!trbs_left) {
		if (!starting)
			return;
		trbs_left = DWC3_TRB_NUM;
		/*
		 * In case we start from scratch, we queue the ISOC requests
		 * starting from slot 1. This is done because we use ring
		 * buffer and have no LST bit to stop us. Instead, we place
		 * IOC bit every TRB_NUM/4. We try to avoid having an interrupt
		 * after the first request so we start at slot 1 and have
		 * 7 requests proceed before we hit the first IOC.
		 * Other transfer types don't use the ring buffer and are
		 * processed from the first TRB until the last one. Since we
		 * don't wrap around we have to start at the beginning.
		 */
		if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			dep->busy_slot = 1;
			dep->free_slot = 1;
		} else {
			dep->busy_slot = 0;
			dep->free_slot = 0;
		}
	}

	/* The last TRB is a link TRB, not used for xfer */
	if ((trbs_left <= 1) && usb_endpoint_xfer_isoc(dep->endpoint.desc))
		return;

	list_for_each_entry_safe(req, n, &dep->request_list, list) {/*lint !e413*/
		unsigned	length;
		dma_addr_t	dma;
		last_one = false;

		if (req->request.num_mapped_sgs > 0) {
			struct usb_request *request = &req->request;
			struct scatterlist *sg = request->sg;
			struct scatterlist *s;
			unsigned int		i;

#if (FEATURE_ON == MBB_USB)
            if (trbs_left & 0x80000000) 
            {
                printk("be care, trb fill full\n");
                trbs_left = 0;
            }
            if((int)request->num_mapped_sgs >= (int)(req->request.zero?(trbs_left-1):trbs_left))
#else
            if(request->num_mapped_sgs >= (req->request.zero?(trbs_left-1):(trbs_left)))
#endif
            {	/* no enough trbs for this request */
                dep->stat.trb_lack++;
                DWC3_INFO("num_mapped_sgs=%d,trbs_left=%d,request.zero=%d\n",
                    request->num_mapped_sgs,
                    trbs_left,
                    req->request.zero);
                break;
            }

            trbs_left_n = /* trbs left for next request */
				trbs_left - (req->request.zero?(request->num_mapped_sgs+1):request->num_mapped_sgs);

			for_each_sg(sg, s, request->num_mapped_sgs, i) {
				unsigned chain = true;

				length = sg_dma_len(s);
				dma = sg_dma_address(s);

				if (i == (request->num_mapped_sgs - 1) ||
						sg_is_last(s)) {
                    /*  multi-transfer quirk:if next request can't be queued, set last_one too. */
#if (FEATURE_ON == MBB_USB)
                    if (list_empty(&dep->request_list)
                        || list_is_last(&req->list, &dep->request_list)
                        || ((int)n->request.num_mapped_sgs >= (int)(n->request.zero?(trbs_left_n-1):trbs_left_n)))
#else
                    if (list_empty(&dep->request_list)
                        ||list_is_last(&req->list, &dep->request_list)
                        || (n->request.num_mapped_sgs >= (n->request.zero?(trbs_left_n-1):(trbs_left_n))))
#endif
                    {
                        last_one = true;
                    }

                    chain = false;
				}

				trbs_left--;
				if (!trbs_left)
					last_one = true;

                /* balong quirk:chain entry means transfer boundary */
                if(USB_REQUEST_M_TRANSFER == request->sg_mode
                    || sg_is_chain(s+1))
                {
                    chain = false;
                }

				if (last_one)
					chain = false;

                /* BUGFIX: ZLP on non-endpoint0 under sg case */
                if (unlikely(false == chain && req->request.zero)) {
    				dwc3_prepare_one_trb(dep, req, dma, length,
    						false, true, i);

                    trbs_left--;
                    if(!trbs_left)
                        dep->stat.trb_zero++;

                    /* NOTE: process ZLP */
    				dwc3_prepare_one_trb(dep, req, dwc->zlp_dma,
    				        dep->direction ? 0 : dep->endpoint.maxpacket,
    						last_one, chain, i);
                } else {
                    if(!trbs_left)
                        dep->stat.trb_zero++;

    				dwc3_prepare_one_trb(dep, req, dma, length,
    						last_one, chain, i);
                }

				if (last_one)
					break;
			}
			/* add the missed count as the break jump */
            dep->stat.trb_used += i + 1;
		} else {
            if(trbs_left < (req->request.zero?2:1))
            {
                dep->stat.trb_lack++;
                DWC3_INFO("trbs_left=%d,request.zero=%d\n",
                    trbs_left,req->request.zero);
                break;
            }

			dma = req->request.dma;
			length = req->request.length;
			trbs_left--;

			if (!trbs_left)
				last_one = 1;

			/* Is this the last request? */
			if (list_is_last(&req->list, &dep->request_list))
				last_one = 1;

            /* BUGFIX: ZLP on non-endpoint0 */
            if (likely(!req->request.zero)) {
                if(!trbs_left)
                    dep->stat.trb_zero++;
				dwc3_prepare_one_trb(dep, req, dma, length,
					last_one, false, 0);
			}else{
				dwc3_prepare_one_trb(dep, req, dma, length,
    					false, true, 0);

                trbs_left--;

                if(!trbs_left)
                    dep->stat.trb_zero++;

                /* NOTE: This is ZLP on non-endpoint0 */
    			dwc3_prepare_one_trb(dep, req, dwc->zlp_dma,
    			        dep->direction ? 0 : dep->endpoint.maxpacket,
    					last_one, false, 0);
			}

            dep->stat.trb_used++;

			if (last_one)
				break;
		}
	}
}

static int __dwc3_gadget_kick_transfer(struct dwc3_ep *dep, u16 cmd_param,
		int start_new)
{
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3_request		*req;
	struct dwc3			*dwc = dep->dwc;
	int				ret;
	u32				cmd;

	if (start_new && (dep->flags & DWC3_EP_BUSY)) {
		DWC3_DBG("%s: endpoint busy\n", dep->name);
		dep->stat.kick_busy++;
		return -EBUSY;
	}
	dep->flags &= ~DWC3_EP_PENDING_REQUEST;

	/*
	 * If we are getting here after a short-out-packet we don't enqueue any
	 * new requests as we try to set the IOC bit only on the last request.
	 */
	if (start_new) {
		if (list_empty(&dep->req_queued)){
            dwc3_prepare_trbs(dep, start_new);
        }

		/* req points to the first request which will be sent */
		req = next_request(&dep->req_queued);
	} else {
		dwc3_prepare_trbs(dep, start_new);

		/*
		 * req points to the first request where HWO changed from 0 to 1
		 */
		req = next_request(&dep->req_queued);
	}
	if (!req) {
		dep->flags |= DWC3_EP_PENDING_REQUEST;
		dep->stat.kick_none++;
		return 0;
	}

	if (start_new) {
	/*Use CONFIG_ARCH_DMA_ADDR_T_64BIT is purely for coverity miss warning. */
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
		params.param0 = upper_32_bits(req->trb_dma);
#else
		params.param0 = 0;
#endif
		params.param1 = lower_32_bits(req->trb_dma);
		params.param2 = 0;
		cmd = DWC3_DEPCMD_STARTTRANSFER | DWC3_DEPCMD_HIPRI_FORCERM;
	} else {
		memset(&params, 0, sizeof(params));
		cmd = DWC3_DEPCMD_UPDATETRANSFER;
	}

	cmd |= DWC3_DEPCMD_PARAM(cmd_param);
	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, cmd, &params);
	if (ret < 0) {
		dev_dbg(dwc->dev, "failed to send STARTTRANSFER command\n");

		/*
		 * FIXME we need to iterate over the list of requests
		 * here and stop, unmap, free and del each of the linked
		 * requests instead of what we do now.
		 */
		usb_gadget_unmap_request(&dwc->gadget, &req->request,
				req->direction);
        dep->stat.kick_cmderr++;
		list_del(&req->list);
		return ret;
	}

	dep->flags |= DWC3_EP_BUSY;

	if (start_new) {
		dep->resource_index = dwc3_gadget_ep_get_transfer_index(dwc,
				dep->number);
		WARN_ON_ONCE(!dep->resource_index);
	}

    dep->stat.kick_ok++;

	return 0;
}

static void __dwc3_gadget_start_isoc(struct dwc3 *dwc,
		struct dwc3_ep *dep, u32 cur_uf)
{
	u32 uf;
	int ret;

	if (list_empty(&dep->request_list)) {
		DWC3_DBG("ISOC ep %s run out for requests.\n",
			dep->name);
		dep->flags |= DWC3_EP_PENDING_REQUEST;
		return;
	}

	/* 4 micro frames in the future */
	uf = cur_uf + dep->interval * 4;
	ret = __dwc3_gadget_kick_transfer(dep, uf, 1);
	if (ret && ret != -EBUSY)	
		dev_dbg(dwc->dev, "%s: failed to kick transfers\n", dep->name);

}

static void dwc3_gadget_start_isoc(struct dwc3 *dwc,
		struct dwc3_ep *dep, const struct dwc3_event_depevt *event)
{
	u32 cur_uf, mask;

	mask = ~(dep->interval - 1);
	cur_uf = event->parameters & mask;

	__dwc3_gadget_start_isoc(dwc, dep, cur_uf);
}

int dwc3_event_process(struct dwc3 *dwc);

static inline bool __dwc3_gadget_ep_kick(struct dwc3_ep *dep, u32 kicksource)
{
    struct dwc3		*dwc = dep->dwc;
    int ret = 0;

    if(dwc3_ep_is_complete(dep))        /* td complete */
    {
        /* there are two cases:
           1.complete intr executed,but kicked nothing.
             in this case, dwc3_ep_is_cleanup matched.
           2.complete intr didn't executed.
             in this case, dwc3_ep_is_cleanup unmatch.
        */
        if(dwc3_ep_is_cleanup(dep))    /* complete events processed */
        {
            ret = __dwc3_gadget_kick_transfer(dep, 0, 1);
            if (ret && ret != -EBUSY)	
                dev_dbg(dwc->dev, "%s: failed to kick transfers\n", dep->name);
            dep->stat.dokick[kicksource]++;
            dep->stat.kick_pos = __LINE__;
        }
        else /* complete events not processed */
        {
            (void)dwc3_event_process(dwc);    /* processe events,including kick */
            dep->stat.eventkick[kicksource]++;
        }

        return true;
    }
    else    /* td not complete, waiting */
    {
        dep->stat.nokick[kicksource]++;
        dep->stat.kick_pos = __LINE__;
        return false;
    }
}

static int __dwc3_gadget_ep_queue(struct dwc3_ep *dep, struct dwc3_request *req)
{
	struct dwc3		*dwc = dep->dwc;
	int			ret;

	req->request.actual	= 0;
	req->request.status	= -EINPROGRESS;
	req->direction		= dep->direction;
	req->epnum		= dep->number;

	/*
	 * We only add to our list of requests now and
	 * start consuming the list once we get XferNotReady
	 * IRQ.
	 *
	 * That way, we avoid doing anything that we don't need
	 * to do now and defer it until the point we receive a
	 * particular token from the Host side.
	 *
	 * This will also avoid Host cancelling URBs due to too
	 * many NAKs.
	 */
	ret = usb_gadget_map_request(&dwc->gadget, &req->request,
			dep->direction);
	if (ret)
		return ret;

	list_add_tail(&req->list, &dep->request_list);

    if(dep->endpoint.masknotready) { /* mask notready intr */
        (void)__dwc3_gadget_ep_kick(dep, DWC3_EP_KICKSOURCE_QUEUE);
        return 0;
    }

    /*
	 * There are a few special cases:
	 *
	 * 1. XferNotReady with empty list of requests. We need to kick the
	 *    transfer here in that situation, otherwise we will be NAKing
	 *    forever. If we get XferNotReady before gadget driver has a
	 *    chance to queue a request, we will ACK the IRQ but won't be
	 *    able to receive the data until the next request is queued.
	 *    The following code is handling exactly that.
	 *
	 */
	if (dep->flags & DWC3_EP_PENDING_REQUEST) {
		/*
		 * If xfernotready is already elapsed and it is a case
		 * of isoc transfer, then issue END TRANSFER, so that
		 * you can receive xfernotready again and can have
		 * notion of current microframe.
		 */
		if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			if (list_empty(&dep->req_queued)) {
				dwc3_stop_active_transfer(dwc, dep->number, true);
				dep->flags = DWC3_EP_ENABLED;
			}
			return 0;
		}

		ret = __dwc3_gadget_kick_transfer(dep, 0, true);
		if (ret && ret != -EBUSY)
			DWC3_INFO("%s: failed to kick transfers\n",
					dep->name);
		return ret;
	}

	/*
	 * 2. XferInProgress on Isoc EP with an active transfer. We need to
	 *    kick the transfer here after queuing a request, otherwise the
	 *    core may not see the modified TRB(s).
	 */
	if (usb_endpoint_xfer_isoc(dep->endpoint.desc) &&
			(dep->flags & DWC3_EP_BUSY) &&
			!(dep->flags & DWC3_EP_MISSED_ISOC)) {
		WARN_ON_ONCE(!dep->resource_index);
		ret = __dwc3_gadget_kick_transfer(dep, dep->resource_index,
				false);
		if (ret && ret != -EBUSY)
			dev_dbg(dwc->dev, "%s: failed to kick transfers\n",
					dep->name);
		return ret;
	}

	return 0;
}

static int dwc3_gadget_ep_queue(struct usb_ep *ep, struct usb_request *request,
	gfp_t gfp_flags)
{
	struct dwc3_request		*req = to_dwc3_request(request);
	struct dwc3_ep			*dep = to_dwc3_ep(ep);
	struct dwc3			*dwc = dep->dwc;

	unsigned long			flags;

	int				ret;


	if (!dep->endpoint.desc) {
		dev_dbg(dwc->dev, "trying to queue request %pK to disabled %s\n",
				request, ep->name);
		return -ESHUTDOWN;
	}

	DWC3_TRACE("queing request %pK to %s length %d\n",
			request, ep->name, request->length);

	spin_lock_irqsave(&dwc->lock, flags);
	
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(unlikely(1 == dwc->gadget.is_suspend && bsp_usb_is_support_hibernation())){
		dwc->stat.ep_queue_in_supsned[dep->number]++;
		
		DWC3_ERR("Gadget ep%u Remote wakeup. \n", ep->address);
		ret = dwc3_gadget_hibernation_remote_wakeup(dwc);
		if(ret){
			spin_unlock_irqrestore(&dwc->lock, flags);
			DWC3_ERR("Gadget ep Remote wakeup fail. \n");
			return ret;
		}
 	}
#endif

	ret = __dwc3_gadget_ep_queue(dep, req);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_ep_dequeue(struct usb_ep *ep,
		struct usb_request *request)
{
	struct dwc3_request		*req = to_dwc3_request(request);
	struct dwc3_request		*r = NULL;

	struct dwc3_ep			*dep = to_dwc3_ep(ep);
	struct dwc3			*dwc = dep->dwc;

	unsigned long			flags;
	int				ret = 0;

    dep->stat.dequeue_cnt++;

	spin_lock_irqsave(&dwc->lock, flags);

#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(unlikely(1 == dwc->gadget.is_suspend && bsp_usb_is_support_hibernation())){
		dwc3_gadget_hibernation_soft_resume(dwc);
	}
#endif

	list_for_each_entry(r, &dep->request_list, list) {
		if (r == req)
			break;
	}

	if (r != req) {
		list_for_each_entry(r, &dep->req_queued, list) {
			if (r == req)
				break;
		}
		if (r == req) {
			/* wait until it is processed */
			dwc3_stop_active_transfer(dwc, dep->number, true);
			goto out1;
		}
		dev_err(dwc->dev, "request %pK was not queued to %s\n",
				request, ep->name);
		ret = -EINVAL;
		goto out0;
	}

out1:
	/* giveback the request */
	dwc3_gadget_giveback(dep, req, -ECONNRESET);

out0:
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

int __dwc3_gadget_ep_set_halt(struct dwc3_ep *dep, int value)
{
	struct dwc3_gadget_ep_cmd_params	params;
	struct dwc3				*dwc = dep->dwc;
	int					ret;

	dep->stat.sethalt_cnt++;

	memset(&params, 0x00, sizeof(params));

	if (value) {
		ret = dwc3_send_gadget_ep_cmd(dwc, dep->number,
			DWC3_DEPCMD_SETSTALL, &params);
		if (ret)
			dev_err(dwc->dev, "failed to %s STALL on %s\n",
					value ? "set" : "clear",
					dep->name);
		else
			dep->flags |= DWC3_EP_STALL;
	} else {
		if (dep->flags & DWC3_EP_WEDGE)
			return 0;

		ret = dwc3_send_gadget_ep_cmd(dwc, dep->number,
			DWC3_DEPCMD_CLEARSTALL, &params);
		if (ret)
			dev_err(dwc->dev, "failed to %s STALL on %s\n",
					value ? "set" : "clear",
					dep->name);
		else
			dep->flags &= ~DWC3_EP_STALL;
	}

	return ret;
}

static int dwc3_gadget_ep_set_halt(struct usb_ep *ep, int value)
{
	struct dwc3_ep			*dep = to_dwc3_ep(ep);
	struct dwc3			*dwc = dep->dwc;

	unsigned long			flags;

	int				ret;

	dep->stat.setwedge_cnt++;
	

	spin_lock_irqsave(&dwc->lock, flags);

#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(unlikely(1 == dwc->gadget.is_suspend && bsp_usb_is_support_hibernation())){
		dwc3_gadget_hibernation_soft_resume(dwc);
	}
#endif

	if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
		dev_err(dwc->dev, "%s is of Isochronous type\n", dep->name);
		ret = -EINVAL;
		goto out;
	}

	ret = __dwc3_gadget_ep_set_halt(dep, value);
out:
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_ep_set_wedge(struct usb_ep *ep)
{
	struct dwc3_ep			*dep = to_dwc3_ep(ep);
	struct dwc3			*dwc = dep->dwc;
	unsigned long			flags;

	spin_lock_irqsave(&dwc->lock, flags);
	dep->flags |= DWC3_EP_WEDGE;
	spin_unlock_irqrestore(&dwc->lock, flags);

	if (dep->number == 0 || dep->number == 1)
		return dwc3_gadget_ep0_set_halt(ep, 1);
	else
		return dwc3_gadget_ep_set_halt(ep, 1);
}

/* -------------------------------------------------------------------------- */

static struct usb_endpoint_descriptor dwc3_gadget_ep0_desc = {
	.bLength	= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes	= USB_ENDPOINT_XFER_CONTROL,
};

static const struct usb_ep_ops dwc3_gadget_ep0_ops = {
	.enable		= dwc3_gadget_ep0_enable,
	.disable	= dwc3_gadget_ep0_disable,
	.alloc_request	= dwc3_gadget_ep_alloc_request,
	.free_request	= dwc3_gadget_ep_free_request,
	.queue		= dwc3_gadget_ep0_queue,
	.dequeue	= dwc3_gadget_ep_dequeue,
	.set_halt	= dwc3_gadget_ep0_set_halt,
	.set_wedge	= dwc3_gadget_ep_set_wedge,
};

static const struct usb_ep_ops dwc3_gadget_ep_ops = {
	.enable		= dwc3_gadget_ep_enable,
	.disable	= dwc3_gadget_ep_disable,
	.alloc_request	= dwc3_gadget_ep_alloc_request,
	.free_request	= dwc3_gadget_ep_free_request,
	.queue		= dwc3_gadget_ep_queue,
	.dequeue	= dwc3_gadget_ep_dequeue,
	.set_halt	= dwc3_gadget_ep_set_halt,
	.set_wedge	= dwc3_gadget_ep_set_wedge,
};

/* -------------------------------------------------------------------------- */

static int dwc3_gadget_get_frame(struct usb_gadget *g)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	u32			reg;

#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(unlikely(1 == dwc->gadget.is_suspend && bsp_usb_is_support_hibernation())){
		dwc3_gadget_hibernation_soft_resume(dwc);
	}
#endif

	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	return DWC3_DSTS_SOFFN(reg);
}

static void dwc3_gadget_remote_wake_notify(struct dwc3 *dwc, u32 intf)
{
    u32 param;

    if (dwc->speed == DWC3_DCFG_SUPERSPEED) {
        param = DWC3_DGCMD_DEV_NOTIFY_FUNC_WAKEUP |
            (intf << DWC3_DGCMD_DEV_NOTIFY_PARAM_SHIFT);
        dwc3_send_gadget_generic_command(dwc,
            DWC3_DGCMD_FUNCTION_WAKEUP_DEV_NOTIFY, param);
    }
}
static void dwc3_gadget_wakeup_interrupt(struct dwc3 *dwc);

/* should work in atomic context, as the function driver could move on
   ONLY after the gadget is resumed successfully */
static int dwc3_gadget_wakeup(struct usb_gadget *g)
{
	struct dwc3		*dwc = gadget_to_dwc(g);

	unsigned long		timeout;/*lint !e550*/
	unsigned long		flags;

	u32			reg;

	int			ret = 0;

	u8			link_state;
	u8			speed;
	u32		time_record;

#ifdef CONFIG_GADGET_CORE_HIBERNATION
 	if(bsp_usb_is_support_hibernation()){
		dwc->stat.resume_by_func_remote_wakeup++;
		DWC3_ERR("Func Remote wakeup. \n");
		ret = dwc3_gadget_hibernation_remote_wakeup(dwc);
		return ret;
 	}
#endif

	if(unlikely((DWC3_DCFG_SUPERSPEED != dwc->speed)&&(!g->hs_rwakeup))) {
		if (printk_ratelimit())
			dev_warn(dwc->dev,"gadget not enabled for remote wakeup\n");
		return -EOPNOTSUPP;
	}

	spin_lock_irqsave(&dwc->lock, flags);
	/*
	 * According to the Databook Remote wakeup request should
	 * be issued only when the device is in early suspend state.
	 *
	 * We can check that via USB Link State bits in DSTS register.
	 */
	reg = dwc3_readl(dwc->regs, DWC3_DSTS);

	speed = reg & DWC3_DSTS_CONNECTSPD;
	/*There is not reason for not allowed superspeed remote wakeup.*/
	/*

	if (speed == DWC3_DSTS_SUPERSPEED) {
		dev_dbg(dwc->dev, "no wakeup on SuperSpeed\n");
		ret = -EINVAL;
		goto out;
	}
	*/

	link_state = DWC3_DSTS_USBLNKST(reg);

	switch (link_state) {
	case DWC3_LINK_STATE_RX_DET:	/* in HS, means Early Suspend */
	case DWC3_LINK_STATE_U3:	/* in HS, means SUSPEND */
		break;
	default:
		dev_dbg(dwc->dev, "can't wakeup from link state %d\n",
				link_state);
		ret = -EINVAL;
		goto out;
	}

	ret = dwc3_gadget_set_link_state(dwc, DWC3_LINK_STATE_RECOV);
	if (ret) {
		dev_err(dwc->dev, "failed to put link in Recovery\n");
		goto out;
	}

	/* Recent versions do this automatically */
	if (dwc->revision < DWC3_REVISION_194A) {
		/* write zeroes to Link Change Request */
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	}

    /* poll until Link State changes to ON,
	 * we could not use jiffies in atomic context
	 */
	time_record = bsp_get_slice_value();
    do {
        timeout = get_timer_slice_delta(time_record, bsp_get_slice_value());
        reg = dwc3_readl(dwc->regs, DWC3_DSTS);

		/* in HS, means ON */
		if (DWC3_DSTS_USBLNKST(reg) == DWC3_LINK_STATE_U0)
			break;
    }while(timeout < USB_US_TO_32KHZ(1000*500));

	if (DWC3_DSTS_USBLNKST(reg) != DWC3_LINK_STATE_U0) {
		dev_err(dwc->dev, "failed to send remote wakeup\n");
		ret = -EINVAL;
		goto out;
	}

    /* send function remote wakeup notify */
    dwc3_gadget_remote_wake_notify(dwc,0);

    dwc3_gadget_wakeup_interrupt(dwc);

out:
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}/*lint !e550*/

static int dwc3_gadget_set_selfpowered(struct usb_gadget *g,
		int is_selfpowered)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	unsigned long		flags;

	spin_lock_irqsave(&dwc->lock, flags);
	dwc->is_selfpowered = !!is_selfpowered;
	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}

static int dwc3_gadget_run_stop(struct dwc3 *dwc, int is_on)
{
	u32			reg;
	u32			timeout = 500;

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	if (is_on) {
#if (FEATURE_ON == MBB_USB)
#ifdef CONFIG_USB_OTG_DWC_BALONG
        if((DWC3_MODE_DRD == bsp_usb_mode_support()) && (0 == dwc->udc_connect || 0 == dwc->softconnect))
        {
            pr_err("%s return,udc_connect = %d  softconnect = %d",__FUNCTION__,dwc->udc_connect,dwc->softconnect);
            return 0;
        }
#endif
#endif
		if (dwc->revision <= DWC3_REVISION_187A) {
			reg &= ~DWC3_DCTL_TRGTULST_MASK;
			reg |= DWC3_DCTL_TRGTULST_RX_DET;
		}

		if (dwc->revision >= DWC3_REVISION_194A)
			reg &= ~DWC3_DCTL_KEEP_CONNECT;
		reg |= DWC3_DCTL_RUN_STOP;

#ifdef CONFIG_GADGET_CORE_HIBERNATION
		if(bsp_usb_is_support_hibernation()){
			reg |= DWC3_DCTL_KEEP_CONNECT;
		}
#endif

		dwc->pullups_connected = true;
	} else {
		reg &= ~DWC3_DCTL_RUN_STOP;
		dwc->pullups_connected = false;
#ifdef CONFIG_GADGET_CORE_HIBERNATION
		if(bsp_usb_is_support_hibernation()){
			reg &= ~DWC3_DCTL_KEEP_CONNECT;
		}
#endif
	}

	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	do {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);
		if (is_on) {
			if (!(reg & DWC3_DSTS_DEVCTRLHLT))
				break;
		} else {
			if (reg & DWC3_DSTS_DEVCTRLHLT)
				break;
		}
		timeout--;
		if (!timeout)
			return -ETIMEDOUT;
		udelay(1);
	} while (1);

	dev_vdbg(dwc->dev, "gadget %s data soft-%s\n",
			dwc->gadget_driver
			? dwc->gadget_driver->function : "no-function",
			is_on ? "connect" : "disconnect");

	return 0;
}

static int dwc3_gadget_pullup(struct usb_gadget *g, int is_on)
{
    struct dwc3 *dwc = gadget_to_dwc(g);
    unsigned long flags;
    int ret = 0;
	
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(unlikely(1 == dwc->gadget.is_suspend && bsp_usb_is_support_hibernation())){
		dwc3_gadget_hibernation_soft_resume(dwc);
	}
#endif

    is_on = !!is_on;

#if (FEATURE_ON == MBB_USB)
#ifdef CONFIG_USB_OTG_DWC_BALONG
    dwc->softconnect = is_on;
#endif
#endif

    dwc3_enable_both_phy();

    spin_lock_irqsave(&dwc->lock, flags);

    if(DWC3_MODE_DRD == bsp_usb_mode_support()){
        if(is_on){
            if(!(g->ep0_ready && g->functions_ready)){
                DWC3_ERR("ep0_ready %d, functions_ready %d, is_on %d \n",
                    g->ep0_ready, g->functions_ready, is_on);
                spin_unlock_irqrestore(&dwc->lock, flags);
                return ret;
            }
        }
    }

    ret = dwc3_gadget_run_stop(dwc, is_on);
    spin_unlock_irqrestore(&dwc->lock, flags);

    return ret;
}

static void dwc3_gadget_enable_irq(struct dwc3 *dwc)
{
	u32			reg;

	/* Enable all but Start and End of Frame IRQs */
	reg = (DWC3_DEVTEN_VNDRDEVTSTRCVEDEN |
			DWC3_DEVTEN_EVNTOVERFLOWEN |
			DWC3_DEVTEN_CMDCMPLTEN |
#if (FEATURE_OFF == MBB_USB)
			DWC3_DEVTEN_ERRTICERREN |
#endif
//			DWC3_DEVTEN_ULSTCNGEN |
			DWC3_DEVTEN_CONNECTDONEEN |
			DWC3_DEVTEN_USBRSTEN);
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(bsp_usb_is_support_hibernation()){
		reg |= DWC3_DEVTEN_HIBERNATIONREQEVTEN;
	}else
#endif
	{
		reg |= DWC3_DEVTEN_U3L2L1SUSPEN | DWC3_DEVTEN_WKUPEVTEN;
	}
	if(bsp_usb_is_vbus_connect()){
		reg |= DWC3_DEVTEN_DISCONNEVTEN;
	}
	dwc3_writel(dwc->regs, DWC3_DEVTEN, reg);
}

static void dwc3_gadget_disable_irq(struct dwc3 *dwc)
{
	/* mask all interrupts */
	dwc3_writel(dwc->regs, DWC3_DEVTEN, 0x00);
}

static irqreturn_t dwc3_interrupt(int irq, void *_dwc);
static irqreturn_t dwc3_thread_interrupt(int irq, void *_dwc);

static int dwc3_gadget_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct dwc3		*dwc = gadget_to_dwc(g);/*lint !e413*/
	struct dwc3_ep		*dep;
	unsigned long		flags;
	int			ret = 0;
	int			irq;
	u32			reg;
	struct usb_phy *otg = NULL;

    if(DWC3_MODE_DRD == bsp_usb_mode_support()){
        otg = usb_get_phy(USB_PHY_TYPE_USB3);
    	if (!otg) {
    		dev_err(dwc->dev, "OTG driver not available!\n");
    		return -ENODEV;
    	}
    	usb_put_phy(otg);
    }
	tasklet_init(&dwc->intr_bh_tasklet, dwc3_gadget_intr_bh, (unsigned long) dwc);

#ifdef CONFIG_GADGET_CORE_HIBERNATION

	ret = dwc3_gadget_hibernation_irq_register(dwc);
	if (ret) {
 		dev_err(dwc->dev, "failed to request pme irq  \n");
		goto err0;
	}
#endif
	irq = platform_get_irq(to_platform_device(dwc->dev), 0);
	if(IS_ENABLED(CONFIG_USB_DWC3_IRQTHREAD)){
		ret = request_threaded_irq(irq, dwc3_interrupt, dwc3_thread_interrupt,
			IRQF_SHARED | IRQF_ONESHOT, "dwc3", dwc);
	}else{
		ret = request_irq(irq, dwc3_interrupt, IRQF_SHARED,
			"dwc3", dwc);
	}
	if (ret) {
		dev_err(dwc->dev, "failed to request irq #%d --> %d\n",
				irq, ret);
		goto err0;
	}
	dwc->irq = irq;

	spin_lock_irqsave(&dwc->lock, flags);

	if (dwc->gadget_driver) {
		dev_err(dwc->dev, "%s is already bound to %s\n",
				dwc->gadget.name,
				dwc->gadget_driver->driver.name);
		ret = -EBUSY;
		goto err1;
	}

	dwc->gadget_driver	= driver;

	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_SPEED_MASK);

	/**
	 * WORKAROUND: DWC3 revision < 2.20a have an issue
	 * which would cause metastability state on Run/Stop
	 * bit if we try to force the IP to USB2-only mode.
	 *
	 * Because of that, we cannot configure the IP to any
	 * speed other than the SuperSpeed
	 *
	 * Refers to:
	 *
	 * STAR#9000525659: Clock Domain Crossing on DCTL in
	 * USB 2.0 Mode
	 */
#ifndef CONFIG_USB_FORCE_HIGHSPEED
	if(!bsp_usb_is_force_highspeed()){
		if (dwc->revision < DWC3_REVISION_220A)
			reg |= DWC3_DCFG_SUPERSPEED;
		else
			reg |= dwc->maximum_speed;
	}
#endif
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	dwc->start_config_issued = false;

    if(DWC3_MODE_DRD != bsp_usb_mode_support()){
        /* Start with SuperSpeed Default */
    	dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err2;
	}

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err3;
	}

	/* begin to receive SETUP packets */
	dwc->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dwc);/*lint !e413*/

    	dwc3_gadget_enable_irq(dwc);
    }

	spin_unlock_irqrestore(&dwc->lock, flags);

    if(DWC3_MODE_DRD == bsp_usb_mode_support()){
    	otg = usb_get_phy(USB_PHY_TYPE_USB3);
    	otg->io_priv = (uint8_t *)dwc->regs - DWC3_GLOBALS_REGS_START;
    	otg_set_peripheral(otg->otg, &dwc->gadget);
    	usb_put_phy(otg);
    }

	return 0;


err3:
    if(DWC3_MODE_DRD != bsp_usb_mode_support()){
	    __dwc3_gadget_ep_disable(dwc->eps[0]);
    }

err2:
	dwc->gadget_driver = NULL;

err1:
	spin_unlock_irqrestore(&dwc->lock, flags);

	free_irq(irq, dwc);

err0:

	return ret;
}

static int dwc3_gadget_stop(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	unsigned long		flags;
	int			irq;
    struct usb_phy *otg;

#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(unlikely(1 == dwc->gadget.is_suspend && bsp_usb_is_support_hibernation())){
		dwc3_gadget_hibernation_soft_resume(dwc);
	}
#endif

    if(DWC3_MODE_DRD == bsp_usb_mode_support()){
    	otg = usb_get_phy(USB_PHY_TYPE_USB3);
    	otg_set_peripheral(otg->otg, NULL);
    	usb_put_phy(otg);
    }

	spin_lock_irqsave(&dwc->lock, flags);

	dwc3_gadget_disable_irq(dwc);
	irq = platform_get_irq(to_platform_device(dwc->dev), 0);
	free_irq(irq, dwc);

#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if(bsp_usb_is_support_hibernation()){
		irq = platform_get_irq_byname(to_platform_device(dwc->dev), "usb_pme_irq");
		free_irq(irq,dwc);
	}
#endif
	tasklet_kill(&dwc->intr_bh_tasklet);

	__dwc3_gadget_ep_disable(dwc->eps[0]);
	__dwc3_gadget_ep_disable(dwc->eps[1]);

	dwc->gadget_driver	= NULL;

	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}

#ifdef CONFIG_USB_OTG_DWC_BALONG
static void dwc3_gadget_usb2_phy_suspend(struct dwc3 *dwc, int suspend);
static void dwc3_gadget_usb3_phy_suspend(struct dwc3 *dwc, int suspend);
static void dwc3_gadget_disconnect_interrupt(struct dwc3 *dwc);

int pcd_otg_start_peripheral(struct usb_gadget *gadget)
{
	return gadget->ops->otg_start(gadget, NULL);
}

int pcd_otg_stop_peripheral(struct usb_gadget *gadget)
{
	return gadget->ops->otg_stop(gadget);
}

static int dwc3_otg_gadget_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	struct dwc3_ep		*dep;
	unsigned long		flags;
	u32					reg;
	int					ret;
	u32                 i;

	spin_lock_irqsave(&dwc->lock, flags);

	ret = dwc3_core_init(dwc);
	if (ret) {
		dev_err(dwc->dev, "failed to initialize core at otg start\n");
	}

	dwc3_event_buffers_setup(dwc);

	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg |= DWC3_DCFG_LPM_CAP;
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	dwc3_intr_disable(dwc);
	for (i = 0; i < dwc->num_event_buffers; i++) {
		reg = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(i));
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(i), reg);
	}
	dwc3_intr_enable(dwc);

	dwc3_gadget_enable_irq(dwc);

	/* Enable USB2 LPM and automatic phy suspend only on recent versions */
	if (dwc->revision >= DWC3_REVISION_194A) {
		reg = dwc3_readl(dwc->regs, DWC3_DCFG);
		reg |= DWC3_DCFG_LPM_CAP;
		dwc3_writel(dwc->regs, DWC3_DCFG, reg);

		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~(DWC3_DCTL_HIRD_THRES_MASK | DWC3_DCTL_L1_HIBER_EN);

		/* TODO: This should be configurable */
		reg |= DWC3_DCTL_HIRD_THRES(12);

		dwc3_writel(dwc->regs, DWC3_DCTL, reg);

		dwc3_gadget_usb2_phy_suspend(dwc, false);
		dwc3_gadget_usb3_phy_suspend(dwc, false);
	}

	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_SPEED_MASK);

	/**
	 * WORKAROUND: DWC3 revision < 2.20a have an issue
	 * which would cause metastability state on Run/Stop
	 * bit if we try to force the IP to USB2-only mode.
	 *
	 * Because of that, we cannot configure the IP to any
	 * speed other than the SuperSpeed
	 *
	 * Refers to:
	 *
	 * STAR#9000525659: Clock Domain Crossing on DCTL in
	 * USB 2.0 Mode
	 */
#ifndef CONFIG_USB_FORCE_HIGHSPEED
	if(!bsp_usb_is_force_highspeed()){
		if (dwc->revision < DWC3_REVISION_220A)
			reg |= DWC3_DCFG_SUPERSPEED;
		else
			reg |= dwc->maximum_speed;
	}
#endif
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);
	
	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_DEVADDR_MASK);
	reg |= DWC3_DCFG_DEVADDR(0);
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	dwc->start_config_issued = false;

	/* Start with SuperSpeed Default */
	dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err0;
	}

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err1;
	}

	/* begin to receive SETUP packets */
	dwc->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dwc);

	g->ep0_ready = 1;

#if (FEATURE_ON == MBB_USB)
#ifdef CONFIG_USB_OTG_DWC_BALONG
    dwc->udc_connect = 1 ;

#endif
    ret = dwc3_gadget_run_stop(dwc, 1);

#else
    ret = dwc3_gadget_pullup(g, 1);
#endif
	if(ret){
		dev_err(dwc->dev, "failed to run peripheral\n");
		goto err2;
	}

	dwc->wants_host = 0;
	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;

err2:
	__dwc3_gadget_ep_disable(dwc->eps[1]);

err1:
	__dwc3_gadget_ep_disable(dwc->eps[0]);

err0:
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_otg_gadget_stop(struct usb_gadget *g)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	unsigned long		flags;
#if (FEATURE_ON == MBB_USB)
    unsigned long    index = 0;
#endif
	u32             temp;

	spin_lock_irqsave(&dwc->lock, flags);

	/* Remove request and disconnect gadget */
	dwc3_gadget_disconnect_interrupt(dwc);

	__dwc3_gadget_ep_disable(dwc->eps[0]);
	__dwc3_gadget_ep_disable(dwc->eps[1]);

	g->ep0_ready = 0;

	/* Clear Run/Stop bit */
	dwc3_gadget_run_stop(dwc, 0);

	/* Disable device interrupt */
	dwc3_writel(dwc->regs, DWC3_DEVTEN, 0);

	spin_unlock_irqrestore(&dwc->lock, flags);

	do {
#if (FEATURE_ON == MBB_USB)
        index++;
        if (100 == index)
        {
            dev_err(dwc->dev, "read DSTS HALT failed\n");
            break;
        }
#endif
		msleep(1);
		temp = dwc3_readl(dwc->regs, DWC3_DSTS);
	} while (!(temp & DWC3_DSTS_DEVCTRLHLT));

    msleep(10);
#if (FEATURE_ON == MBB_USB)
#ifdef CONFIG_USB_OTG_DWC_BALONG
    dwc->udc_connect = 0;
#endif
#endif
    return 0;
}

static int dwc3_otg_gadget_send_hrr(struct usb_gadget *g, int is_init)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
    u32     param;

	param = is_init ? DWC3_DGCMDPAR_HOST_ROLE_REQ_INITIATE :
		DWC3_DGCMDPAR_HOST_ROLE_REQ_CONFIRM;

	return dwc3_xmit_host_role_request(dwc, param);
}
#endif

static const struct usb_gadget_ops dwc3_gadget_ops = {
	.get_frame		= dwc3_gadget_get_frame,
	.wakeup			= dwc3_gadget_wakeup,
	.set_selfpowered	= dwc3_gadget_set_selfpowered,
	.pullup			= dwc3_gadget_pullup,
	.udc_start		= dwc3_gadget_start,
	.udc_stop		= dwc3_gadget_stop,
#ifdef CONFIG_USB_OTG_DWC_BALONG
    .otg_start          = dwc3_otg_gadget_start,
    .otg_stop           = dwc3_otg_gadget_stop,
    .send_hrr       = dwc3_otg_gadget_send_hrr,
#endif
};

/*Start of usb dwc3_conn*/
struct dwc3_gadget_conn_ops dwc3_conn_ops ={
	.usb_conndone_cb = NULL,
	.usb_disconnect_cb = NULL,
	.usb_set_dwc_cb = NULL,
};

int dwc3_gadget_register_conn_ops(struct dwc3_gadget_conn_ops *reg_ops)
{
	if(NULL == reg_ops){
		printk("dwc3_gadget_register_conn_ops fail. \n");
		return -ENXIO;
	}
	dwc3_conn_ops.usb_conndone_cb = reg_ops->usb_conndone_cb;
	dwc3_conn_ops.usb_disconnect_cb = reg_ops->usb_disconnect_cb;
	dwc3_conn_ops.usb_set_dwc_cb = reg_ops->usb_set_dwc_cb;
	return 0;
}


/*End of usb dwc3_conn*/

static int dwc3_gadget_init_hw_endpoints(struct dwc3 *dwc,
		u8 num, u32 direction)
{
	struct dwc3_ep			*dep;
	u8				i;

	for (i = 0; i < num; i++) {
		u8 epnum = (i << 1) | (!!direction);

		dep = kzalloc(sizeof(*dep), GFP_KERNEL);
		if (!dep) {
			dev_err(dwc->dev, "can't allocate endpoint %d\n",
					epnum);
			return -ENOMEM;
		}

		dep->dwc = dwc;
		dep->number = epnum;
		dwc->eps[epnum] = dep;

		snprintf(dep->name, sizeof(dep->name), "ep%d%s", epnum >> 1,
				(epnum & 1) ? "in" : "out");

		dep->endpoint.name = dep->name;
		dep->direction = (epnum & 1);

		if (epnum == 0 || epnum == 1) {
			dep->endpoint.maxpacket = 512;
			dep->endpoint.maxburst = 1;
			dep->endpoint.ops = &dwc3_gadget_ep0_ops;
			if (!epnum)
				dwc->gadget.ep0 = &dep->endpoint;
		} else {
			int		ret;

			dep->endpoint.maxpacket = 1024;
			dep->endpoint.max_streams = 15;
			dep->endpoint.ops = &dwc3_gadget_ep_ops;
			list_add_tail(&dep->endpoint.ep_list,
					&dwc->gadget.ep_list);

			ret = dwc3_alloc_trb_pool(dep);
			if (ret)
				return ret;
		}

		INIT_LIST_HEAD(&dep->request_list);
		INIT_LIST_HEAD(&dep->req_queued);
	}

	return 0;
}

static int dwc3_gadget_init_endpoints(struct dwc3 *dwc)
{
	int				ret;

	INIT_LIST_HEAD(&dwc->gadget.ep_list);

	ret = dwc3_gadget_init_hw_endpoints(dwc, dwc->num_out_eps, 0);
	if (ret < 0) {
		dev_vdbg(dwc->dev, "failed to allocate OUT endpoints\n");
		return ret;
	}

	ret = dwc3_gadget_init_hw_endpoints(dwc, dwc->num_in_eps, 1);
	if (ret < 0) {
		dev_vdbg(dwc->dev, "failed to allocate IN endpoints\n");
		return ret;
	}

	return 0;
}

static void dwc3_gadget_free_endpoints(struct dwc3 *dwc)
{
	struct dwc3_ep			*dep;
	u8				epnum;

	for (epnum = 0; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		dep = dwc->eps[epnum];
		if (!dep)
			continue;
		/*
		 * Physical endpoints 0 and 1 are special; they form the
		 * bi-directional USB endpoint 0.
		 *
		 * For those two physical endpoints, we don't allocate a TRB
		 * pool nor do we add them the endpoints list. Due to that, we
		 * shouldn't do these two operations otherwise we would end up
		 * with all sorts of bugs when removing dwc3.ko.
		 */
		if (epnum != 0 && epnum != 1) {
			dwc3_free_trb_pool(dep);
			list_del(&dep->endpoint.ep_list);
		}

		kfree(dep);
	}
}

/* -------------------------------------------------------------------------- */

static int __dwc3_cleanup_done_trbs(struct dwc3 *dwc, struct dwc3_ep *dep,
		struct dwc3_request *req, struct dwc3_trb *trb,
		const struct dwc3_event_depevt *event, int status, int is_zlp)
{
	unsigned int		count;
	unsigned int		s_pkt = 0;
	unsigned int		trb_status;

	if ((trb->ctrl & DWC3_TRB_CTRL_HWO) && status != -ESHUTDOWN)
		/*
		 * We continue despite the error. There is not much we
		 * can do. If we don't clean it up we loop forever. If
		 * we skip the TRB then it gets overwritten after a
		 * while since we use them in a ring buffer. A BUG()
		 * would help. Lets hope that if this occurs, someone
		 * fixes the root cause instead of looking away :)
		 */
		dev_err(dwc->dev, "%s's TRB (%pK) still owned by HW\n",
				dep->name, trb);
	count = trb->size & DWC3_TRB_SIZE_MASK;

	if (dep->direction) {
		if (count) {
			trb_status = DWC3_TRB_SIZE_TRBSTS(trb->size);
			if (trb_status == DWC3_TRBSTS_MISSED_ISOC) {
				dev_dbg(dwc->dev, "incomplete IN transfer %s\n",
						dep->name);
				/*
				 * If missed isoc occurred and there is
				 * no request queued then issue END
				 * TRANSFER, so that core generates
				 * next xfernotready and we will issue
				 * a fresh START TRANSFER.
				 * If there are still queued request
				 * then wait, do not issue either END
				 * or UPDATE TRANSFER, just attach next
				 * request in request_list during
				 * giveback.If any future queued request
				 * is successfully transferred then we
				 * will issue UPDATE TRANSFER for all
				 * request in the request_list.
				 */
				dep->flags |= DWC3_EP_MISSED_ISOC;
			} else {
				dev_err(dwc->dev, "incomplete IN transfer %s\n",
						dep->name);
				status = -ECONNRESET;
			}
		} else {
			dep->flags &= ~DWC3_EP_MISSED_ISOC;
		}
	} else {
		if (count && (event->status & DEPEVT_STATUS_SHORT))
			s_pkt = 1;
	}

	/*
	 * We assume here we will always receive the entire data block
	 * which we should receive. Meaning, if we program RX to
	 * receive 4K but we receive only 2K, we assume that's all we
	 * should receive and we simply bounce the request back to the
	 * gadget driver for further processing.
	 */
	if (!is_zlp)
	    req->request.actual += req->request.length - count;
	if (s_pkt)
		return 1;
	if ((event->status & DEPEVT_STATUS_LST) &&
			(trb->ctrl & (DWC3_TRB_CTRL_LST |
				DWC3_TRB_CTRL_HWO)))
		return 1;
	if ((event->status & DEPEVT_STATUS_IOC) &&
			(trb->ctrl & DWC3_TRB_CTRL_IOC))
		return 1;
	return 0;
}

static int dwc3_cleanup_done_reqs(struct dwc3 *dwc, struct dwc3_ep *dep,
		const struct dwc3_event_depevt *event, int status)
{
	struct dwc3_request	*req;
	struct dwc3_trb		*trb = NULL;
	unsigned int		slot;
	unsigned int		i;
	int			ret;

	do {
		req = next_request(&dep->req_queued);
		if (!req) {
            dep->stat.cleanup_warn++;
            dev_err(dwc->dev, "warning on:ep %s ctrl %x status %x\n",
							dep->name,trb?trb->ctrl:0,event->status);
			WARN_ON_ONCE(1);
			return 1;
		}
		i = 0;
		do {
			slot = req->start_slot + i;
			if ((slot == DWC3_TRB_NUM - 1) &&
				usb_endpoint_xfer_isoc(dep->endpoint.desc))
				slot++;

            /* move slot to the last one directly, it's faster but not maturity.
               it's depend on your choice. */
            if (IS_ENABLED(CONFIG_USB_DWC3_NOLOOP_SGS)
                && req->request.num_mapped_sgs) {
                i += req->request.num_mapped_sgs - 1;
                slot += req->request.num_mapped_sgs - 1;
            }

			slot %= DWC3_TRB_NUM;
			trb = &dep->trb_pool[slot];

			ret = __dwc3_cleanup_done_trbs(dwc, dep, req, trb,
					event, status, 0);

			if (ret)
				break;
		}while (++i < req->request.num_mapped_sgs);

        /* now process zlp trb */
		if (req->request.zero){
            slot++;
            slot %= DWC3_TRB_NUM;
			trb = &dep->trb_pool[slot];
			ret = __dwc3_cleanup_done_trbs(dwc, dep, req, trb,
					event, status, 1);
		}
		dwc3_gadget_giveback(dep, req, status);

		if (ret)
			break;
	} while (1);

	if (usb_endpoint_xfer_isoc(dep->endpoint.desc) &&
			list_empty(&dep->req_queued)) {
		if (list_empty(&dep->request_list)) {
			/*
			 * If there is no entry in request list then do
			 * not issue END TRANSFER now. Just set PENDING
			 * flag, so that END TRANSFER is issued when an
			 * entry is added into request list.
			 */
			dep->flags = DWC3_EP_PENDING_REQUEST;
		} else {
			dwc3_stop_active_transfer(dwc, dep->number, true);
			dep->flags = DWC3_EP_ENABLED;
		}
		return 1;
	}

	if ((event->status & DEPEVT_STATUS_IOC) &&
			(trb->ctrl & DWC3_TRB_CTRL_IOC))
		return 0;
	return 1;
}

static void dwc3_endpoint_transfer_complete(struct dwc3 *dwc,
		struct dwc3_ep *dep, const struct dwc3_event_depevt *event,
		int start_new)
{
	int		status = 0;
	int			clean_busy;

	if (event->status & DEPEVT_STATUS_BUSERR)
		status = -ECONNRESET;

	clean_busy = dwc3_cleanup_done_reqs(dwc, dep, event, status);
	if (clean_busy && start_new){
		dep->flags &= ~DWC3_EP_BUSY;
	}

	/*
	 * WORKAROUND: This is the 2nd half of U1/U2 -> U0 workaround.
	 * See dwc3_gadget_linksts_change_interrupt() for 1st half.
	 */
	if (dwc->revision < DWC3_REVISION_183A) {
		u32		reg;
		int		i;

		for (i = 0; i < DWC3_ENDPOINTS_NUM; i++) {
			dep = dwc->eps[i];

			if (!(dep->flags & DWC3_EP_ENABLED))
				continue;

			if (!list_empty(&dep->req_queued))
				return;
		}

		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg |= dwc->u1u2;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);

		dwc->u1u2 = 0;
	}
}

static void dwc3_endpoint_interrupt(struct dwc3 *dwc,
		const struct dwc3_event_depevt *event)
{
	struct dwc3_ep		*dep;
	struct dwc3_event_stat_t *stat = &dwc->event_stat;
	u8			epnum = event->endpoint_number;
	u32 event_type = event->endpoint_event;
	int ret;

	dep = dwc->eps[epnum];

	if (!(dep->flags & DWC3_EP_ENABLED))
		return;

	DWC3_TRACE("%s: %s\n", dep->name,
		dwc3_ep_event_string(event->endpoint_event));

	if (epnum == 0 || epnum == 1) {
		dwc3_ep0_interrupt(dwc, event);
		stat->ep0_event[event_type]++;
		return;
	}

	stat->ep_event[epnum-2][event_type]++;

	switch (event->endpoint_event) {
	case DWC3_DEPEVT_XFERCOMPLETE:
		dep->resource_index = 0;

		if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			dev_dbg(dwc->dev, "%s is an Isochronous endpoint\n",
					dep->name);
			return;
		}

		dwc3_endpoint_transfer_complete(dwc, dep, event, 1);

		if (dep->endpoint.masknotready&& !list_empty(&dep->request_list)){
			dep->stat.dokick[DWC3_EP_KICKSOURCE_COMPLETE]++;
			dep->stat.kick_pos = __LINE__;
			ret = __dwc3_gadget_kick_transfer(dep, 0, 1);
			if (ret && ret != -EBUSY)
				dev_dbg(dwc->dev, "%s: failed to kick transfers\n", dep->name);
		}
		break;
	case DWC3_DEPEVT_XFERINPROGRESS:
        /* process the xferinprogress if the xferinprogress intr of
         * this endpoint is enabled.
         */
		if (!dep->endpoint.enable_xfer_in_progress
            && !usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			dev_dbg(dwc->dev, "%s is not an Isochronous endpoint\n",
					dep->name);
			return;
		}

		dwc3_endpoint_transfer_complete(dwc, dep, event, 0);
		break;
	case DWC3_DEPEVT_XFERNOTREADY:
		if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			dwc3_gadget_start_isoc(dwc, dep, event);
		} else {

			DWC3_TRACE("%s: reason %s\n",
					dep->name, event->status &
					DEPEVT_STATUS_TRANSFER_ACTIVE
					? "Transfer Active"
					: "Transfer Not Active");
			ret = __dwc3_gadget_kick_transfer(dep, 0, 1);
			if (!ret || ret == -EBUSY)
				return;

			dev_dbg(dwc->dev, "%s: failed to kick transfers\n",
					dep->name);
		}

		break;
	case DWC3_DEPEVT_STREAMEVT:
		if (!usb_endpoint_xfer_bulk(dep->endpoint.desc)) {
			dev_err(dwc->dev, "Stream event for non-Bulk %s\n",
					dep->name);
			return;
		}

		switch (event->status) {
		case DEPEVT_STREAMEVT_FOUND:
			dev_vdbg(dwc->dev, "Stream %d found and started\n",
					event->parameters);

			break;
		case DEPEVT_STREAMEVT_NOTFOUND:
			/* FALLTHROUGH */
		default:
			dev_dbg(dwc->dev, "Couldn't find suitable stream\n");
		}
		break;
	case DWC3_DEPEVT_RXTXFIFOEVT:
		dev_dbg(dwc->dev, "%s FIFO Overrun\n", dep->name);
		break;
	case DWC3_DEPEVT_EPCMDCMPLT:
		dev_vdbg(dwc->dev, "Endpoint Command Complete\n");

		dep->flags &= ~DWC3_EP_BUSY;
		if(!list_empty(&dep->request_list)){
			(void)__dwc3_gadget_ep_kick(dep, DWC3_EP_KICKSOURCE_EPCMDCMPLT);
		}
		break;
	}
}

static void dwc3_disconnect_gadget(struct dwc3 *dwc)
{
	if (dwc->gadget_driver && dwc->gadget_driver->disconnect) {
		spin_unlock(&dwc->lock);
		dwc->gadget_driver->disconnect(&dwc->gadget);
		spin_lock(&dwc->lock);
	}
}

#ifndef CONFIG_USB_DWC3_BALONG_PM
static void dwc3_suspend_gadget(struct dwc3 *dwc)
{
	if (dwc->gadget_driver && dwc->gadget_driver->suspend) {
		spin_unlock(&dwc->lock);
		dwc->gadget_driver->suspend(&dwc->gadget);
		spin_lock(&dwc->lock);
	}
}

static void dwc3_resume_gadget(struct dwc3 *dwc)
{
	if (dwc->gadget_driver && dwc->gadget_driver->resume) {
		spin_unlock(&dwc->lock);
		dwc->gadget_driver->resume(&dwc->gadget);
		spin_lock(&dwc->lock);
	}
}

#endif



static void dwc3_stop_active_transfer(struct dwc3 *dwc, u32 epnum, bool force)
{
	struct dwc3_ep *dep;
	struct dwc3_gadget_ep_cmd_params params;
	u32 cmd;
	int ret;

	dep = dwc->eps[epnum];

	if (!dep->resource_index)
		return;

	/*
	 * NOTICE: We are violating what the Databook says about the
	 * EndTransfer command. Ideally we would _always_ wait for the
	 * EndTransfer Command Completion IRQ, but that's causing too
	 * much trouble synchronizing between us and gadget driver.
	 *
	 * We have discussed this with the IP Provider and it was
	 * suggested to giveback all requests here, but give HW some
	 * extra time to synchronize with the interconnect. We're using
	 * an arbitraty 100us delay for that.
	 *
	 * Note also that a similar handling was tested by Synopsys
	 * (thanks a lot Paul) and nothing bad has come out of it.
	 * In short, what we're doing is:
	 *
	 * - Issue EndTransfer WITH CMDIOC bit set
	 * - Wait 100us
	 */

	cmd = DWC3_DEPCMD_ENDTRANSFER;
	cmd |= force ? DWC3_DEPCMD_HIPRI_FORCERM : 0;
	cmd |= DWC3_DEPCMD_CMDIOC;
	cmd |= DWC3_DEPCMD_PARAM(dep->resource_index);
	memset(&params, 0, sizeof(params));
	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, cmd, &params);
	WARN_ON_ONCE(ret);
	dep->resource_index = 0;

	/* dep->flags &= ~DWC3_EP_BUSY; */

	
	if(!force)
		dep->flags &= ~DWC3_EP_BUSY; 

	udelay(100);
}/*lint !e550*/

static void dwc3_stop_active_transfers(struct dwc3 *dwc)
{
	u32 epnum;

	for (epnum = 2; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		struct dwc3_ep *dep;

		dep = dwc->eps[epnum];
		if (!dep)
			continue;
		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;
		dwc3_remove_requests(dwc, dep);
	}
}

static void dwc3_clear_stall_all_ep(struct dwc3 *dwc)
{
	u32 epnum;

	for (epnum = 1; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		struct dwc3_ep *dep;
		struct dwc3_gadget_ep_cmd_params params;
		int ret;

		dep = dwc->eps[epnum];
		if (!dep)
			continue;

		if (!(dep->flags & DWC3_EP_STALL))
			continue;

		dep->flags &= ~DWC3_EP_STALL;

		memset(&params, 0, sizeof(params));
		ret = dwc3_send_gadget_ep_cmd(dwc, dep->number,
				DWC3_DEPCMD_CLEARSTALL, &params);
		WARN_ON_ONCE(ret);
	}/*lint !e550*/
}

static void dwc3_gadget_disconnect_interrupt(struct dwc3 *dwc)
{
	int			reg;
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	unsigned long flags;
#endif

	dev_vdbg(dwc->dev, "%s\n", __func__);
		
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_INITU1ENA;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	reg &= ~DWC3_DCTL_INITU2ENA;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	dwc3_disconnect_gadget(dwc);
	dwc->start_config_issued = false;

	dwc->gadget.speed = USB_SPEED_UNKNOWN;
	dwc->setup_packet_pending = false;

	/*set dctl 8:5 to 5 as required by databook 3.00a 8.1.7*/
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;
	reg |= DWC3_DCTL_ULSTCHNGREQ(5);
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	
	if(dwc3_conn_ops.usb_disconnect_cb){
		dwc3_conn_ops.usb_disconnect_cb(dwc);
	}
	
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	spin_lock_irqsave(&dwc->lpm_lock, flags);
	dwc3_gadget_hibernation_disconnect(dwc);
	spin_unlock_irqrestore(&dwc->lpm_lock, flags);
#endif

}

/*USB device initiated disconnect databook 3.00a 8.1.8 .
 1.Call this when usb disconnected and ready for power down;
 2.Set DCTL
*/

void dwc3_gadget_dev_init_disconnect(struct dwc3 *dwc)
{
	int ret;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return;
    }
#endif

	if(dwc->usb_core_powerdown){
		return;
	}
	/*If a control transfer is still in progress, complete it.*/


	/*Get the core into the "Setup a Control-Setup TRB / Start Transfer"*/


	/*Issue a DEPENDXFER command for any active transfers 
	(except for the default control endpoint 0)*/
	dwc3_stop_active_transfers(dwc);


	/*Set DCTL.RunStop to'0' to disconnect from the host*/
	
	ret = dwc3_gadget_run_stop(dwc, false);
	if(ret){
		ret = dwc3_core_init(dwc);
		if (ret) {
			dev_err(dwc->dev, "failed to initialize core at dev init disconn \n");
		}
		printk("dwc3_gadget_dev_init_disconnect set run stop fail. \n");
	}

	dwc->usb_core_powerdown = true;
}


int dwc3_gadget_soft_reset(struct dwc3 *dwc)
{
	struct dwc3_event_buffer *evt;
	struct dwc3_ep *dep;
	unsigned int reg;
	unsigned int i;
	int ret;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return -EPERM;
    }
#endif

	if(dwc->usb_core_powerdown){
		DWC3_ERR("usb core powerdown, soft reset fail.\n");
		return -ENODEV;
	}
	
	ret = dwc3_core_init(dwc);
	if(ret){
		DWC3_ERR("dwc3_gadget_soft_reset reset core fail.\n");
		goto err;
	}

	for (i = 0; i < dwc->num_event_buffers; i++) {
		evt = dwc->ev_buffs[i];
		DWC3_INFO("Event buf %pK dma %08llx length %d\n",
				evt->buf, (unsigned long long) evt->dma,
				evt->length);

		evt->lpos = 0;

		dwc3_writel(dwc->regs, DWC3_GEVNTADRLO(i),lower_32_bits(evt->dma));
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
		dwc3_writel(dwc->regs, DWC3_GEVNTADRHI(i),upper_32_bits(evt->dma));
#else
		dwc3_writel(dwc->regs, DWC3_GEVNTADRHI(i),0);
#endif
		dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(i),evt->length & 0xffff);
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(i), 0);
	}

	switch(dwc->mode){
	case DWC3_MODE_DEVICE:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_DEVICE);
		break;
	case DWC3_MODE_HOST:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_HOST);
		break;
	case DWC3_MODE_DRD:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_OTG);
		break;
	default:
		dev_err(dwc->dev, "Unsupported mode of operation %d\n", dwc->mode);
	}

	
	/*Set usb speed by write dcfg*/
	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_SPEED_MASK);
	
#ifndef CONFIG_USB_FORCE_HIGHSPEED
	if(!bsp_usb_is_force_highspeed()){
		reg |= dwc->maximum_speed;
	}
#endif
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	dwc->start_config_issued = false;

	/*disable gadget*/
	dwc3_gadget_disable_irq(dwc);
	__dwc3_gadget_ep_disable(dwc->eps[0]);
	__dwc3_gadget_ep_disable(dwc->eps[1]);
	
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	ret = dwc3_gadget_hibernation_reinit(dwc);
	if (ret) {
		dev_err(dwc->dev, "failed to enable hibernation \n");
		goto err;
	}
#endif

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err;
	}

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err1;
	}

	/* begin to receive SETUP packets */
	dwc->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dwc);

	dwc3_gadget_enable_irq(dwc);

	ret = dwc3_gadget_run_stop(dwc, true);
	
	return ret;
	
err1:
	__dwc3_gadget_ep_disable(dwc->eps[0]);

err:
	return ret;
}



static void dwc3_gadget_usb3_phy_suspend(struct dwc3 *dwc, int suspend)
{
	u32			reg;

	reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));

	if (suspend)
		reg |= DWC3_GUSB3PIPECTL_SUSPHY;
	else
		reg &= ~DWC3_GUSB3PIPECTL_SUSPHY;

	dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);
}

static void dwc3_gadget_usb2_phy_suspend(struct dwc3 *dwc, int suspend)
{
	u32			reg;

	reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));

	if (suspend)
		reg |= DWC3_GUSB2PHYCFG_SUSPHY;
	else
		reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;

	dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);
}

static void dwc3_gadget_reset_interrupt(struct dwc3 *dwc)
{
	u32			reg;

	dwc3_enable_both_phy();

	DWC3_NOTICE("dwc3 reset intr \n");

	/*
	 * WORKAROUND: DWC3 revisions <1.88a have an issue which
	 * would cause a missing Disconnect Event if there's a
	 * pending Setup Packet in the FIFO.
	 *
	 * There's no suggested workaround on the official Bug
	 * report, which states that "unless the driver/application
	 * is doing any special handling of a disconnect event,
	 * there is no functional issue".
	 *
	 * Unfortunately, it turns out that we _do_ some special
	 * handling of a disconnect event, namely complete all
	 * pending transfers, notify gadget driver of the
	 * disconnection, and so on.
	 *
	 * Our suggested workaround is to follow the Disconnect
	 * Event steps here, instead, based on a setup_packet_pending
	 * flag. Such flag gets set whenever we have a XferNotReady
	 * event on EP0 and gets cleared on XferComplete for the
	 * same endpoint.
	 *
	 * Refers to:
	 *
	 * STAR#9000466709: RTL: Device : Disconnect event not
	 * generated if setup packet pending in FIFO
	 */
	if (dwc->revision < DWC3_REVISION_188A) {
		if (dwc->setup_packet_pending)
			dwc3_gadget_disconnect_interrupt(dwc);
	}

	/* after reset -> Default State */
	usb_gadget_set_state(&dwc->gadget, USB_STATE_DEFAULT);

	/* clear the suspend & remote wakeup state */
	dwc->gadget.is_suspend = 0;
	dwc->gadget.hs_rwakeup = 0;

#if (FEATURE_ON == MBB_USB)
    huawei_set_usb_enum_state(USB_ENUM_NONE);
#endif

	/* Recent versions support automatic phy suspend and don't need this */
	if (dwc->revision < DWC3_REVISION_194A) {
		/* Resume PHYs */
		dwc3_gadget_usb2_phy_suspend(dwc, false);
		dwc3_gadget_usb3_phy_suspend(dwc, false);
	}

	if (dwc->gadget.speed != USB_SPEED_UNKNOWN)
		dwc3_disconnect_gadget(dwc);

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_TSTCTRL_MASK;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	dwc->test_mode = false;

	dwc3_stop_active_transfers(dwc);
	dwc3_clear_stall_all_ep(dwc);
	dwc->start_config_issued = false;

	/* Reset device address to zero */
	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_DEVADDR_MASK);
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

}
#ifdef BSP_CONFIG_BOARD_TELEMATIC
/*===========================================================================
FUNCTION 
    dwc3_gadget_disconnect_cb
DESCRIPTION
    手动触发USBdisconnect中断
DEPENDENCIES
    USB未断开时时触发
RETURN VALUE
    NO
SIDE EFFECTS
    None
===========================================================================*/
void dwc3_gadget_disconnect_cb(void)
{
    if(NULL == the_dwc3)
    {
        printk("[tbox dbg]dwc3_gadget_disconnect_cb:NULL \n");
        return;
    }
    dwc3_gadget_disconnect_interrupt(the_dwc3);
    return ;
}
#endif
static void dwc3_update_ram_clk_sel(struct dwc3 *dwc, u32 speed)/* [false alarm]:Disable fortify false alarm */
{
	u32 reg;/* [false alarm]:Disable fortify false alarm */
	u32 usb30_clock = DWC3_GCTL_CLK_BUS;

	/*
	 * We change the clock only at SS but I dunno why I would want to do
	 * this. Maybe it becomes part of the power saving plan.
	 */

	if (speed != DWC3_DSTS_SUPERSPEED)
		return;

	/*
	 * RAMClkSel is reset to 0 after USB reset, so it must be reprogrammed
	 * each time on Connect Done.
	 */
	if (!usb30_clock)/* [false alarm]:Disable fortify false alarm */
		return;
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);/* [false alarm]:Disable fortify false alarm */
	reg |= DWC3_GCTL_RAMCLKSEL(usb30_clock);/* [false alarm]:Disable fortify false alarm */
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);/* [false alarm]:Disable fortify false alarm */
}

static void dwc3_gadget_phy_suspend(struct dwc3 *dwc, u8 speed)
{
	switch (speed) {
	case USB_SPEED_SUPER:
		dwc3_gadget_usb2_phy_suspend(dwc, true);
		break;
	case USB_SPEED_HIGH:
	case USB_SPEED_FULL:
	case USB_SPEED_LOW:
		dwc3_gadget_usb3_phy_suspend(dwc, true);
		break;
	}
}

static void dwc3_gadget_conndone_interrupt(struct dwc3 *dwc)
{
	struct dwc3_ep		*dep;
	int			ret;
	u32			reg;
	u8			speed;

	dev_vdbg(dwc->dev, "%s\n", __func__);

	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	speed = reg & DWC3_DSTS_CONNECTSPD;
	dwc->speed = speed;

	dwc3_update_ram_clk_sel(dwc, speed);

	switch (speed) {
	case DWC3_DCFG_SUPERSPEED:
		/*
		 * WORKAROUND: DWC3 revisions <1.90a have an issue which
		 * would cause a missing USB3 Reset event.
		 *
		 * In such situations, we should force a USB3 Reset
		 * event by calling our dwc3_gadget_reset_interrupt()
		 * routine.
		 *
		 * Refers to:
		 *
		 * STAR#9000483510: RTL: SS : USB3 reset event may
		 * not be generated always when the link enters poll
		 */
		if (dwc->revision < DWC3_REVISION_190A)
			dwc3_gadget_reset_interrupt(dwc);

		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);
		dwc->gadget.ep0->maxpacket = 512;
		dwc->gadget.speed = USB_SPEED_SUPER;
		break;
	case DWC3_DCFG_HIGHSPEED:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		dwc->gadget.ep0->maxpacket = 64;
		dwc->gadget.speed = USB_SPEED_HIGH;
		break;
	case DWC3_DCFG_FULLSPEED2:
	case DWC3_DCFG_FULLSPEED1:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		dwc->gadget.ep0->maxpacket = 64;
		dwc->gadget.speed = USB_SPEED_FULL;
		break;
	case DWC3_DCFG_LOWSPEED:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(8);
		dwc->gadget.ep0->maxpacket = 8;
		dwc->gadget.speed = USB_SPEED_LOW;
		break;
	}

	/* Enable USB2 LPM Capability */

	if ((dwc->revision > DWC3_REVISION_194A)
			&& (speed != DWC3_DCFG_SUPERSPEED)) {
		reg = dwc3_readl(dwc->regs, DWC3_DCFG);
		reg |= DWC3_DCFG_LPM_CAP;
		dwc3_writel(dwc->regs, DWC3_DCFG, reg);

		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~(DWC3_DCTL_HIRD_THRES_MASK | DWC3_DCTL_L1_HIBER_EN);

		/*
		 * TODO: This should be configurable. For now using
		 * maximum allowed HIRD threshold value of 0b1100
		 */
		reg |= DWC3_DCTL_HIRD_THRES(12);

		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	}

	/* Recent versions support automatic phy suspend and don't need this */
	if (dwc->revision < DWC3_REVISION_194A) {
		/* Suspend unneeded PHY */
		dwc3_gadget_phy_suspend(dwc, dwc->gadget.speed);
	}

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, true);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		return;
	}

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, true);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		return;
	}

	dwc->link_state = DWC3_LINK_STATE_RESET;

	/*
	 * Configure PHY via GUSB3PIPECTLn if required.
	 *
	 * Update GTXFIFOSIZn
	 *
	 * In both cases reset values should be sufficient.
	 */


	/*Disable unused usb phy*/
	dwc3_disable_phy_by_speed(dwc, speed);
	
	if(DWC3_DCFG_SUPERSPEED == speed){		
		DWC3_ERR_INTR("USB conn at SuperSpeed. \n");		
	}else{		
		DWC3_ERR_INTR("USB conn at HighSpeed. \n");		
	 }

	/*Call balong usb connect module. */

	if(dwc3_conn_ops.usb_conndone_cb){
		dwc3_conn_ops.usb_conndone_cb(dwc);
	}
	
}
static void dwc3_gadget_erratic_error_interrupt(struct dwc3 *dwc)
{
	int reg;

	/*For some unknow reason hibernation will trigger this interrupt, 
		which will cause usb unable to work normally*/
	if(bsp_usb_is_support_hibernation()){
		return;
	}
	
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_RUN_STOP;
	reg &= ~DWC3_DCTL_KEEP_CONNECT;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	reg = dwc3_readl(dwc->regs, DWC3_DEVTEN);
	reg &= ~DWC3_DEVTEN_ERRTICERREN;
	dwc3_writel(dwc->regs, DWC3_DEVTEN, reg);
	DWC3_ERR_INTR("USB DEVICE PHY ERROR. \n");
			
	dwc->intr_bh_event |= DWC3_EVENT_BH_SOFT_RESET;
	tasklet_schedule(&dwc->intr_bh_tasklet);/*call dwc3_gadget_intr_bh*/
}
static void dwc3_gadget_hibernation_interrupt(struct dwc3 *dwc)
{
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	unsigned long flags;
	unsigned int reg;
       /*loop 500 times*/
	unsigned int timeout=500;

	/*This is for support usb plug&unplug when vbus connected to pmu.*/
	if(unlikely(USB_PMU_DETECT == bsp_usb_vbus_detect_mode())){
		if(unlikely (!bsp_pmu_usb_state_get())) {
			/*if usb not connected, disable usb and wait for device remove*/
			dwc3_gadget_hibernation_stop_transfers(dwc);
			do {
				reg = dwc3_readl(dwc->regs, DWC3_DSTS);
				if (DWC3_LINK_STATE_SS_DIS == DWC3_DSTS_USBLNKST(reg)) {
					break;
				}

				timeout--;
				if (!timeout){
					dwc->stat.suspend_wait_disconnect_fail ++;
					DWC3_ERR_INTR("USB DEV DISCONN TIMEOUT. \n");
					return;
				}
				udelay(10);
			} while (1);
			reg = dwc3_readl(dwc->regs, DWC3_DCTL);
			reg &= ~DWC3_DCTL_RUN_STOP;
			reg &= ~DWC3_DCTL_KEEP_CONNECT;
			dwc3_writel(dwc->regs, DWC3_DCTL, reg);
			return;
		}
	}

	spin_lock_irqsave(&dwc->lpm_lock, flags);
	dwc3_gadget_hibernation_suspend(dwc);
	spin_unlock_irqrestore(&dwc->lpm_lock, flags);

#endif
	return;
}




#ifdef CONFIG_USB_DWC3_BALONG_PM
static inline void dwc3_gadget_resume_balong(struct dwc3 *dwc)
{
    dev_vdbg(dwc->dev, "%s\n", __func__);

    if (dwc->gadget_driver->resume) {
        dwc->gadget_driver->resume(&dwc->gadget);
    }

    /* clear stall all the eps */
    dwc3_clear_stall_all_ep(dwc);
}
#endif

static void dwc3_gadget_wakeup_interrupt(struct dwc3 *dwc)
{
#ifdef CONFIG_USB_DWC3_BALONG_PM
    enum dwc3_link_state state = DWC3_LINK_STATE_U0;
    unsigned long		timeout;
    u32		time_record;

    /* do the resume job really when the gadget is configured */
    if (dwc->gadget.state != USB_STATE_CONFIGURED) {
        dev_vdbg(dwc->dev, "%s dev_state = %d,not DWC3_CONFIGURED_STATE \n",
            __func__, (int)dwc->gadget.state);
        return;
    }

	/*
	 * TODO take core out of low power mode when that's
	 * implemented.
	 */
	 /* poll until Link State changes to U0,
	  * we could not use jiffies in atomic context
	  */
	time_record = bsp_get_slice_value();
    do {
        timeout = get_timer_slice_delta(time_record, bsp_get_slice_value());
        state = dwc3_gadget_get_link_state(dwc);

        if (state == DWC3_LINK_STATE_U0)
			break;
    }while(timeout < USB_US_TO_32KHZ(1000*100));

	if (state != DWC3_LINK_STATE_U0) {
		dev_err(dwc->dev, "timeout to wait for the link status reg update\n");
		state = DWC3_LINK_STATE_U0;
	}

    dev_vdbg(dwc->dev, "%s: link state:%d -> %d\n", __func__,
        (int)dwc->link_state,(int)state);

    dwc->link_state = state;

    if (dwc->gadget.is_suspend) {
        dwc3_gadget_resume_balong(dwc);
    }

    dwc->gadget.is_suspend = 0;
#else
	dwc->gadget_driver->resume(&dwc->gadget);
#endif
}/*lint !e550*/

static void dwc3_gadget_linksts_change_interrupt(struct dwc3 *dwc,
		unsigned int evtinfo)
{
	enum dwc3_link_state	next = (enum dwc3_link_state)(evtinfo & DWC3_LINK_STATE_MASK);
	unsigned int		pwropt;

	/*
	 * WORKAROUND: DWC3 < 2.50a have an issue when configured without
	 * Hibernation mode enabled which would show up when device detects
	 * host-initiated U3 exit.
	 *
	 * In that case, device will generate a Link State Change Interrupt
	 * from U3 to RESUME which is only necessary if Hibernation is
	 * configured in.
	 *
	 * There are no functional changes due to such spurious event and we
	 * just need to ignore it.
	 *
	 * Refers to:
	 *
	 * STAR#9000570034 RTL: SS Resume event generated in non-Hibernation
	 * operational mode
	 */
	pwropt = DWC3_GHWPARAMS1_EN_PWROPT(dwc->hwparams.hwparams1);
	if ((dwc->revision < DWC3_REVISION_250A) &&
			(pwropt != DWC3_GHWPARAMS1_EN_PWROPT_HIB)) {
		if ((dwc->link_state == DWC3_LINK_STATE_U3) &&
				(next == DWC3_LINK_STATE_RESUME)) {
			dev_vdbg(dwc->dev, "ignoring transition U3 -> Resume\n");
			return;
		}
	}

	/*
	 * WORKAROUND: DWC3 Revisions <1.83a have an issue which, depending
	 * on the link partner, the USB session might do multiple entry/exit
	 * of low power states before a transfer takes place.
	 *
	 * Due to this problem, we might experience lower throughput. The
	 * suggested workaround is to disable DCTL[12:9] bits if we're
	 * transitioning from U1/U2 to U0 and enable those bits again
	 * after a transfer completes and there are no pending transfers
	 * on any of the enabled endpoints.
	 *
	 * This is the first half of that workaround.
	 *
	 * Refers to:
	 *
	 * STAR#9000446952: RTL: Device SS : if U1/U2 ->U0 takes >128us
	 * core send LGO_Ux entering U0
	 */
	if (dwc->revision < DWC3_REVISION_183A) {
		if (next == DWC3_LINK_STATE_U0) {
			u32	u1u2;
			u32	reg;

			switch (dwc->link_state) {
			case DWC3_LINK_STATE_U1:
			case DWC3_LINK_STATE_U2:
				reg = dwc3_readl(dwc->regs, DWC3_DCTL);
				u1u2 = reg & (DWC3_DCTL_INITU2ENA
						| DWC3_DCTL_ACCEPTU2ENA
						| DWC3_DCTL_INITU1ENA
						| DWC3_DCTL_ACCEPTU1ENA);

				if (!dwc->u1u2)
					dwc->u1u2 = reg & u1u2;

				reg &= ~u1u2;

				dwc3_writel(dwc->regs, DWC3_DCTL, reg);
				break;
			default:
				/* do nothing */
				break;
			}
		}
	}

#ifndef CONFIG_USB_DWC3_BALONG_PM
	switch (next) {
	case DWC3_LINK_STATE_U1:
		if (dwc->speed == USB_SPEED_SUPER)
			dwc3_suspend_gadget(dwc);
		break;
	case DWC3_LINK_STATE_U2:
	case DWC3_LINK_STATE_U3:
		dwc3_suspend_gadget(dwc);
		break;
	case DWC3_LINK_STATE_RESUME:
		dwc3_resume_gadget(dwc);
		break;
	default:
		/* do nothing */
		break;
	}
#endif
	dwc->link_state = next;

	DWC3_INFO("%s link %d\n", __func__, dwc->link_state);
}

#ifdef CONFIG_USB_DWC3_BALONG_PM
static inline void dwc3_gadget_suspend_balong(struct dwc3 *dwc)
{
/* 收到suspend 后发现收到set_addr没有set_config,拉低拉高规避卡死问题*/
#if (FEATURE_ON == MBB_USB)
    if(IS_SET_ADD == huawei_get_adress_flag())
    {
        printk(KERN_INFO "dwc3_gadget_suspend for pc bios!!!! \n");
        huawei_set_adress_flag(SET_ADD_NONE);
        /* Clear Run/Stop bit */
        dwc3_gadget_run_stop(dwc, 0);
        mdelay(RECONNECT_DELAYTIME);
        dwc3_gadget_run_stop(dwc, 1);
    }
#endif
    /* We can't rely on the link state machine to decide
    whether to do the suspend. As link state change intr
    would modify the dwc->link_state if enabled */
    if (dwc->gadget.is_suspend) {
        dev_warn(dwc->dev, "usb gadget device already suspended\n");
        return;
    }

    dev_vdbg(dwc->dev, "%s\n", __func__);

    if (dwc->gadget_driver->suspend) {
        dwc->gadget_driver->suspend(&dwc->gadget);
    }

    /* stop all transfers */
    dwc3_stop_active_transfers(dwc);

    dwc->gadget.is_suspend = 1;
}

static void dwc3_gadget_suspend_interrupt(struct dwc3 *dwc)
{
    enum dwc3_link_state state;

    /* check the link state */
    state = dwc3_gadget_get_link_state(dwc);

    dev_vdbg(dwc->dev, "%s: link state:%d -> %d\n", __func__,
        (int)dwc->link_state,(int)state);

    if (state != DWC3_LINK_STATE_U3 || dwc->link_state == DWC3_LINK_STATE_U3) {
        return;
    }

    dwc->link_state = state;

    dwc3_gadget_suspend_balong(dwc);
}
#endif



static void dwc3_gadget_interrupt(struct dwc3 *dwc,
		const struct dwc3_event_devt *event)
{
	struct dwc3_event_stat_t *stat = &dwc->event_stat;

	switch (event->type) {
	case DWC3_DEVICE_EVENT_DISCONNECT:
		dwc3_gadget_disconnect_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_RESET:
		dwc3_gadget_reset_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_CONNECT_DONE:
		dwc3_gadget_conndone_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_WAKEUP:
		dev_vdbg(dwc->dev, "resume event received\n");
		if (dwc->revision != DWC3_REVISION_270A) {
			dwc3_gadget_wakeup_interrupt(dwc);
		}
		break;
	case DWC3_DEVICE_EVENT_HIBER_REQ:
		dwc3_gadget_hibernation_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_LINK_STATUS_CHANGE:
		dwc3_gadget_linksts_change_interrupt(dwc, event->event_info);
		break;
#ifdef CONFIG_USB_DWC3_BALONG_PM
	case DWC3_DEVICE_EVENT_SUSPEND:
		dev_vdbg(dwc->dev, "suspend event received\n");
		if (dwc->revision != DWC3_REVISION_270A) {
			dwc3_gadget_suspend_interrupt(dwc);
		}
		break;
#endif
	case DWC3_DEVICE_EVENT_SOF:
		dev_vdbg(dwc->dev, "Start of Periodic Frame\n");
		break;
	case DWC3_DEVICE_EVENT_ERRATIC_ERROR:
#if (FEATURE_OFF == MBB_USB)
		dev_vdbg(dwc->dev, "Erratic Error\n");
		dwc3_gadget_erratic_error_interrupt(dwc);

#endif
		break;
	case DWC3_DEVICE_EVENT_CMD_CMPL:
		dev_vdbg(dwc->dev, "Command Complete\n");
		break;
	case DWC3_DEVICE_EVENT_OVERFLOW:
		dev_vdbg(dwc->dev, "Overflow\n");
		break;
	default:
		dev_dbg(dwc->dev, "UNKNOWN IRQ %d\n", event->type);
        stat->device_event[DWC3_DEVICE_EVENT_UNKNOWNN]++;
        return;
	}

    stat->device_event[event->type]++;
}

static void dwc3_process_event_entry(struct dwc3 *dwc,
		const union dwc3_event *event)
{
	/* Endpoint IRQ, handle it and return early */
	if (event->type.is_devspec == 0) {
		/* depevt */
		dwc3_endpoint_interrupt(dwc, &event->depevt);
		return ;
	}

	switch (event->type.type) {
	case DWC3_EVENT_TYPE_DEV:
		dwc3_gadget_interrupt(dwc, &event->devt);
		break;
	/* REVISIT what to do with Carkit and I2C events ? */
	default:
		dev_err(dwc->dev, "UNKNOWN IRQ type %d\n", event->raw);
	}
}

void dwc3_ep_show_index(struct dwc3 *dwc)
{
    u32 cnt = 0;
    struct dwc3_ep *ep;

    for(cnt=0; cnt<DWC3_ENDPOINTS_NUM; cnt++)
    {
        ep = dwc->eps[cnt];
        if(NULL != ep){
            printk("| |--ep %u direction:%u typ:%u name:%s \n",
                ep->number, ep->direction, ep->type, ep->name);
				}
    }
}



void dwc3_ep_show(struct dwc3 *dwc, u32 ep_number)
{
    u32 cnt = 0;
    unsigned long flags = 0;
    u32 request_list = 0;
    u32 req_queued = 0;
    struct dwc3_ep *ep;
    struct list_head *pos;
    int i;

    printk("|-+dwc3 ep dump    :\n");

    spin_lock_irqsave(&dwc->lock, flags);

    if(!dwc->usb_core_powerdown){
        printk("| |--ep enable reg :0x%08x\n",dwc3_readl(dwc->regs, DWC3_DALEPENA));
    }else{
        printk("| |--ep reg info unavable, core power down. \n ");
    }
    for(cnt=0; cnt<DWC3_ENDPOINTS_NUM; cnt++)
    {
        if(ep_number < DWC3_ENDPOINTS_NUM)
        {
            cnt = ep_number;
        }

        ep = dwc->eps[cnt];

        request_list = 0;
        list_for_each(pos, &ep->request_list)
        {
            request_list++;
        }

        req_queued= 0;
        list_for_each(pos, &ep->req_queued)
        {
            req_queued++;
        }

        printk("| |--ep %d dump             :\n",ep->number);
        printk("| |--name                   :%s\n",ep->name);
        printk("| |--direction              :%u\n",ep->direction);
        printk("| |--flags                  :%x\n",ep->flags);
        printk("| |--type                   :%u\n",ep->type);
        printk("| |--request_list           :%u\n",request_list);
        printk("| |--req_queued             :%u\n",req_queued);
        printk("| |--trb_pool               :0x%08x\n",(u32)ep->trb_pool);
        printk("| |--current_trb            :%u\n",ep->current_trb);
        printk("| |--busy_slot              :%u\n",ep->busy_slot);
        printk("| |--free_slot              :%u\n",ep->free_slot);
        printk("| |--resource_index         :%u\n",ep->resource_index);
        printk("| |--stream_capable         :%u\n",ep->stream_capable);
		
        if(!dwc->usb_core_powerdown){
            printk("| |--DEPCMDPAR2             :0x%08x\n",(u32)dwc3_readl(dwc->regs, DWC3_DEPCMDPAR2(cnt)));
            printk("| |--DEPCMDPAR1             :0x%08x\n",(u32)dwc3_readl(dwc->regs, DWC3_DEPCMDPAR1(cnt)));
            printk("| |--DEPCMDPAR0             :0x%08x\n",(u32)dwc3_readl(dwc->regs, DWC3_DEPCMDPAR0(cnt)));
            printk("| |--DEPCMD                 :0x%08x\n",(u32)dwc3_readl(dwc->regs, DWC3_DEPCMD(cnt)));
        }
		
        printk("| |--stat.kick_busy         :%d\n",ep->stat.kick_busy);
        printk("| |--stat.kick_none         :%d\n",ep->stat.kick_none);
        printk("| |--stat.kick_cmderr       :%d\n",ep->stat.kick_cmderr);
        printk("| |--stat.kick_ok           :%d\n",ep->stat.kick_ok);
        printk("| |--stat.trb_zero          :%d\n",ep->stat.trb_zero);
        printk("| |--stat.trb_used          :%d\n",ep->stat.trb_used);
        printk("| |--stat.trb_lack          :%d\n",ep->stat.trb_lack);
        printk("| |--stat.cleanup_warn      :%d\n",ep->stat.cleanup_warn);
        printk("| |--stat.kick_pos          :%d\n",ep->stat.kick_pos);
        printk("| |--stat.enable_cnt        :%d\n",ep->stat.enable_cnt);
        printk("| |--stat.disable_cnt       :%d\n",ep->stat.disable_cnt);
        printk("| |--stat.dequeue_cnt       :%d\n",ep->stat.dequeue_cnt);
        printk("| |--stat.sethalt_cnt       :%d\n",ep->stat.sethalt_cnt);
        printk("| |--stat.setwedge_cnt      :%d\n",ep->stat.setwedge_cnt);
        printk("| |--stat.monitor_timeout   :%d\n",ep->stat.monitor_timeout);
        printk("| |--stat.monitor_start     :%d\n",ep->stat.monitor_start);
        printk("| |--stat.monitor_stop      :%d\n",ep->stat.monitor_stop);
        printk("| |--stat.monitor_enable    :%d\n",ep->stat.monitor_enable);
        printk("| |--stat.monitor_disable   :%d\n",ep->stat.monitor_disable);

        for (i=0;i<DWC3_EP_KICKSOURCE_BOTTOM;i++)
        {
            printk("| |--stat.dokick[%d]        :%d\n",i,ep->stat.dokick[i]);
            printk("| |--stat.nokick[%d]        :%d\n",i,ep->stat.nokick[i]);
            printk("| |--stat.eventkick[%d]     :%d\n",i,ep->stat.eventkick[i]);
            printk("| |--stat.zerokick[%d]      :%d\n",i,ep->stat.zerokick[i]);
        }

        printk("\n");

        if(ep_number < DWC3_ENDPOINTS_NUM)
        {
            break;
        }
    }
    spin_unlock_irqrestore(&dwc->lock, flags);
}

void dwc3_event_stat_show(struct dwc3 *dwc)
{
    struct dwc3_event_stat_t *stat = &dwc->event_stat;
    u32 cnt,epnum;

    printk("dwc3 event stat info:\n");
    printk("  intr_total: %d, ehandle_total: %d, loops_total: %d, events_total: %d\n",
        stat->intr_total,stat->ehandle_total, stat->loops_total,stat->events_total);

    printk("  device envent statis as below:\n");
    for (cnt=0; cnt<(DWC3_DEVICE_EVENT_BOTTOM); cnt++)
    {
        printk("[%u %u] ",cnt, stat->device_event[cnt]);
    }
    printk("\n");

    printk("  ep0 envent statis as below:\n");
    for (cnt=0; cnt<(DWC3_DEPEVT_BOTTOM); cnt++)
    {
        printk("[%u %u] ",cnt, stat->ep0_event[cnt]);
    }
    printk("\n");

    printk("  ep envent statis as below:\n");
    for (epnum=0; epnum < (DWC3_ENDPOINTS_NUM-2); epnum++)
    {
        printk("ep%d:",(epnum+2));
        for (cnt=0; cnt<(DWC3_DEPEVT_BOTTOM); cnt++)
        {
            printk("[%u %u] ", cnt, stat->ep_event[epnum][cnt]);
        }
        printk("\n");
    }
    printk("\n");

    return ;
}

static irqreturn_t dwc3_thread_interrupt(int irq, void *_dwc)
{
	struct dwc3 *dwc = _dwc;
	unsigned long flags;
	irqreturn_t ret = IRQ_NONE;
	int i;

	spin_lock_irqsave(&dwc->lock, flags);

	for (i = 0; i < dwc->num_event_buffers; i++) {
		struct dwc3_event_buffer *evt;
		int			left;

		evt = dwc->ev_buffs[i];
		left = evt->count;

		if (!(evt->flags & DWC3_EVENT_PENDING))
			continue;

		while (left > 0) {
			union dwc3_event event;

			event.raw = *(u32 *) (evt->buf + evt->lpos);

			dwc3_process_event_entry(dwc, &event);

			/*
			 * FIXME we wrap around correctly to the next entry as
			 * almost all entries are 4 bytes in size. There is one
			 * entry which has 12 bytes which is a regular entry
			 * followed by 8 bytes data. ATM I don't know how
			 * things are organized if we get next to the a
			 * boundary so I worry about that once we try to handle
			 * that.
			 */
			evt->lpos = (evt->lpos + 4) % DWC3_EVENT_BUFFERS_SIZE;
			left -= 4;

			dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(i), 4);
		}

		evt->count = 0;
		evt->flags &= ~DWC3_EVENT_PENDING;
		ret = IRQ_HANDLED;
	}

	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static irqreturn_t dwc3_process_event_buf_direct(struct dwc3 *dwc, u32 buf)
{
	struct dwc3_event_buffer *evt;
	int left;
	u32 count;
	irqreturn_t ret = IRQ_NONE;

	dwc3_intr_disable(dwc);
	
	/*Add this goto finish for usb suspend functions.
	*In certain occasion like usb hibernation intr or disconnect intr, 
	*dwc3 core will be powered down in event process function and reqire
	*software no loger try to access dwc3 core register until we power it up.
	*Thereof we must end event process here to avoid futher register access.
	*/
	if(dwc->usb_core_powerdown){
		ret = IRQ_HANDLED;
		goto finish;
	}

	while(1)
	{

		count = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(buf));
		count &= DWC3_GEVNTCOUNT_MASK;
		if (!count)
			break;

		dwc->event_stat.loops_total++;
		dwc->event_stat.events_total += count>>2;

		evt = dwc->ev_buffs[buf];
		left = count;

		while (left > 0) {
			union dwc3_event event;

			event.raw = *(u32 *) ((u32)evt->buf + evt->lpos);

			dwc3_process_event_entry(dwc, &event);
			/*
			* XXX we wrap around correctly to the next entry as almost all
			* entries are 4 bytes in size. There is one entry which has 12
			* bytes which is a regular entry followed by 8 bytes data. ATM
			* I don't know how things are organized if were get next to the
			* a boundary so I worry about that once we try to handle that.
			*/

			/*Add this goto finish for usb suspend functions.
			*In certain occasion like usb hibernation intr or disconnect intr, 
			*dwc3 core will be powered down in event process function and reqire
			*software no loger try to access dwc3 core register until we power it up.
			*Thereof we must end event process here to avoid futher register access.
			*/
			if(dwc->usb_core_powerdown){
				ret = IRQ_HANDLED;
				goto finish;
			}
			
			evt->lpos = (evt->lpos + 4) % DWC3_EVENT_BUFFERS_SIZE;/*lint !e123*/
			left -= 4;
		}

		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(buf), count);

		ret = IRQ_HANDLED;

		if(dwc->event_stat.single_intr)
		{
			break;
		}
	}
	finish:
	dwc3_intr_enable(dwc);

	return ret;
}

static irqreturn_t dwc3_process_event_buf(struct dwc3 *dwc, u32 buf)
{
	struct dwc3_event_buffer *evt;
	u32 count;

    if(!IS_ENABLED(CONFIG_USB_DWC3_IRQTHREAD)){
        return dwc3_process_event_buf_direct(dwc, buf);
    }

	evt = dwc->ev_buffs[buf];

	count = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(buf));
	count &= DWC3_GEVNTCOUNT_MASK;
	if (!count)
		return IRQ_NONE;

	evt->count = count;
	evt->flags |= DWC3_EVENT_PENDING;

	return IRQ_WAKE_THREAD;
}

/* called in intr context */
void dwc3_intr_enable(struct dwc3 *dwc)
{
    /*USB ip version 3.00a have a bug prevent intr mask bit in
    DWC3_GEVNTSIZ(i) from working properly.*/
    enable_irq(dwc->irq);
}

void dwc3_intr_disable(struct dwc3 *dwc)
{
    /*USB ip version 3.00a have a bug prevent intr mask bit in
    DWC3_GEVNTSIZ(i) from working properly.*/
    disable_irq_nosync(dwc->irq);
}

int dwc3_event_process(struct dwc3 * dwc)
{
    u32				i;
    irqreturn_t			ret = IRQ_NONE;

    dwc->event_stat.ehandle_total++;

	for (i = 0; i < dwc->num_event_buffers; i++) {
		irqreturn_t status;

		status = dwc3_process_event_buf(dwc, i);
		if (status == IRQ_HANDLED)
			ret = status;
	}

    return ret;
}


extern void dwc3_sysctrl_exit(void);
extern void dwc3_sysctrl_init(void);

int dwc3_gadget_force_reset(struct dwc3 *dwc)
{
	int ret = 0;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return -EPERM;
    }
#endif

	if (dwc->gadget_driver->disconnect) {
		dwc->gadget_driver->disconnect(&dwc->gadget);
	}
	
	dwc3_sysctrl_exit();
	dwc3_sysctrl_init();
	ret = dwc3_gadget_soft_reset(dwc);

	return ret;
}

static void dwc3_gadget_intr_bh(unsigned long _params)
{
    struct dwc3 *dwc = (struct dwc3 *)_params;
    unsigned long flags;

    spin_lock_irqsave(&dwc->lock, flags);
    if(unlikely(!dwc)){
        DWC3_ERR("dwc3_gadget_intr_bh with dwc NULL.\n");
        spin_unlock_irqrestore(&dwc->lock, flags);
        return;
    }

    if(dwc->usb_core_powerdown){
        DWC3_ERR("dwc3_gadget_intr_bh with Core Powerdown.\n");
        spin_unlock_irqrestore(&dwc->lock, flags);
        return;
    }

    if(DWC3_EVENT_BH_SOFT_RESET & dwc->intr_bh_event){
	//need lock?
        dwc->intr_bh_event &= ~DWC3_EVENT_BH_SOFT_RESET;
        DWC3_ERR("USB soft reset. \n");
        if(dwc3_gadget_force_reset(dwc))
            DWC3_ERR("Reset USB Core on timeout fail\n");
    }

    spin_unlock_irqrestore(&dwc->lock, flags);

    if(unlikely(dwc->intr_bh_event)){
        DWC3_ERR("Undefined bottom half\n");
    }

}

#ifdef CONFIG_USB_OTG_DWC_BALONG
void dwc3_start_hnp(struct dwc3 *dwc)
{
	struct usb_phy *transceiver = usb_get_phy(USB_PHY_TYPE_USB3);

	if (!transceiver || !transceiver->otg)
		return;

	otg_start_hnp(transceiver->otg);
	usb_put_phy(transceiver);
}

void dwc3_host_release(struct dwc3 *dwc)
{
	struct usb_phy *transceiver = usb_get_phy(USB_PHY_TYPE_USB3);
	struct usb_otg *otg;

	if (!transceiver || !transceiver->otg)
		return;

	otg = transceiver->otg;
	otg->host_release(otg);
	usb_put_phy(transceiver);
}

static ssize_t store_srp(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct usb_phy *transceiver = usb_get_phy(USB_PHY_TYPE_USB3);

	if (!transceiver || !transceiver->otg)
		return count;

	otg_start_srp(transceiver->otg);
	usb_put_phy(transceiver);
	return count;
}
static DEVICE_ATTR(srp, 0222, NULL, store_srp);

static ssize_t store_end(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct usb_phy *transceiver = usb_get_phy(USB_PHY_TYPE_USB3);
    struct usb_otg *otg;

	if (!transceiver || !transceiver->otg)
		return count;

    otg = transceiver->otg;
	otg->end_session(otg);
	usb_put_phy(transceiver);
	return count;
}
static DEVICE_ATTR(end, 0222, NULL, store_end);

static ssize_t store_hnp(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
    struct dwc3 *dwc = container_of(dev, struct dwc3, gadget.dev);

	if (dwc->b_hnp_enable) {
		dwc->b_hnp_enable = 0;
		dwc->wants_host = 0;
		dwc3_start_hnp(dwc);
	} else {
		dwc->wants_host = 1;
		/* TODO if we don't receive the SET_FEATURE within 4 secs,
		 * reset this value
		 */
	}
	return count;
}
static DEVICE_ATTR(hnp, 0222, NULL, store_hnp);

static ssize_t store_rsp(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct usb_phy *transceiver = usb_get_phy(USB_PHY_TYPE_USB3);
    struct usb_otg *otg;

	if (!transceiver || !transceiver->otg)
		return count;

    otg = transceiver->otg;
	otg->start_rsp(otg);
	usb_put_phy(transceiver);
	return count;
}
static DEVICE_ATTR(rsp, 0222, NULL, store_rsp);

#endif

static ssize_t
dwc3_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	int count;
	struct dwc3 *dwc = the_dwc3;

	count = sprintf(buf, "|-+dwc3 info:\n");
	count += sprintf(buf + count, "| |--dwc3_msg_level                  :0x%08x\n",(u32)dwc3_msg_level);
	count += sprintf(buf + count, "| |--cmd_err                         :%u\n",dwc->cmd_err);
	count += sprintf(buf + count, "| |--is_suspend                      :%u\n",dwc->gadget.is_suspend);
	count += sprintf(buf + count, "| |--speed                           :%d\n",dwc->gadget.speed);
	count += sprintf(buf + count, "| |--mode                            :%u\n",dwc->mode);
	count += sprintf(buf + count, "| |--gadget_rwakeup                  :%u\n",dwc->gadget.hs_rwakeup);
	count += sprintf(buf + count, "| |--gadget_rwakeup_cap               :%u\n",dwc->gadget.hs_rwakeup_cap);
 
#ifdef CONFIG_GADGET_CORE_HIBERNATION

	if(bsp_usb_is_support_hibernation()){
		int i,j;

		count += sprintf(buf + count, "| |--hiber_suspend_entry               :%u\n",dwc->stat.suspend_entry);
		count += sprintf(buf + count, "| |--hiber_suspend_done                :%u\n",dwc->stat.suspend_done);
		count += sprintf(buf + count, "| |--hiber_suspend_while_connected     :%u\n",dwc->stat.suspend_while_connected);
		count += sprintf(buf + count, "| |--hiber_suspend_while_not_connected :%u\n",dwc->stat.suspend_while_not_connected);
		count += sprintf(buf + count, "| |--hiber_suspend_conn < 1ms          :%u\n",dwc->stat.suspend_conn_time[DWC3_TIME_1MS]);
		count += sprintf(buf + count, "| |--hiber_suspend_conn < 5ms          :%u\n",dwc->stat.suspend_conn_time[DWC3_TIME_5MS]);
		count += sprintf(buf + count, "| |--hiber_suspend_conn < 10ms         :%u\n",dwc->stat.suspend_conn_time[DWC3_TIME_10MS]);
		count += sprintf(buf + count, "| |--hiber_suspend_conn < 30ms         :%u\n",dwc->stat.suspend_conn_time[DWC3_TIME_30MS]);
		count += sprintf(buf + count, "| |--hiber_suspend_conn Other          :%u\n",dwc->stat.suspend_conn_time[DWC3_TIME_OTHER]);
		count += sprintf(buf + count, "| |--hiber_suspend_noconn < 1ms        :%u\n",dwc->stat.suspend_not_conn_time[DWC3_TIME_1MS]);
		count += sprintf(buf + count, "| |--hiber_suspend_noconn < 5ms        :%u\n",dwc->stat.suspend_not_conn_time[DWC3_TIME_5MS]);
		count += sprintf(buf + count, "| |--hiber_suspend_noconn < 10ms       :%u\n",dwc->stat.suspend_not_conn_time[DWC3_TIME_10MS]);
		count += sprintf(buf + count, "| |--hiber_suspend_noconn < 30ms       :%u\n",dwc->stat.suspend_not_conn_time[DWC3_TIME_30MS]);
		count += sprintf(buf + count, "| |--hiber_suspend_noconn Other        :%u\n",dwc->stat.suspend_not_conn_time[DWC3_TIME_OTHER]);
		count += sprintf(buf + count, "| |--hiber_suspend_ep_backup_count: \n");
		for(i = 0, j = 0; i < DWC3_ENDPOINTS_NUM; i++){
			if(0 == dwc->stat.suspend_ep_saved[i])
				continue;
			count += sprintf(buf + count, " [%d %u]", i, dwc->stat.suspend_ep_saved[i]);
			j++;
			if(0 == j%8)
			count += sprintf(buf + count, " \n");
		}
		count += sprintf(buf + count, "\n");

		count += sprintf(buf + count, "| |--hiber_suspend_reentry             :%u\n",dwc->stat.suspend_reentry_fail);
		count += sprintf(buf + count, "| |--hiber_suspend_wait_halt_fail      :%u\n",dwc->stat.suspend_wait_halt_fail);
		count += sprintf(buf + count, "| |--hiber_suspend_save_state_fail     :%u\n",dwc->stat.suspend_save_state_fail);
		count += sprintf(buf + count, "| |--hiber_suspend_switch_d3_fail      :%u\n",dwc->stat.suspend_switch_d3_fail);
		count += sprintf(buf + count, "| |--hiber_suspend_power_down_fail     :%u\n",dwc->stat.suspend_power_down_fail);

		count += sprintf(buf + count, "| | \n");

		count += sprintf(buf + count, "| |--hiber_resume_entry                :%u\n",dwc->stat.resume_entry);
		count += sprintf(buf + count, "| |--hiber_resume_done                 :%u\n",dwc->stat.resume_done);
		count += sprintf(buf + count, "| |--hiber_resume_while_connected      :%u\n",dwc->stat.resume_while_connected);
		count += sprintf(buf + count, "| |--hiber_resume_while_not_connected  :%u\n",dwc->stat.resume_while_not_connected);
		count += sprintf(buf + count, "| |--hiber_resume_by_host              :%u\n",dwc->stat.resume_by_host);
		count += sprintf(buf + count, "| |--hiber_resume_by_func_remote_wakeup:%u\n",dwc->stat.resume_by_func_remote_wakeup);
		count += sprintf(buf + count, "| |--hiber_resume_by_software          :%u\n",dwc->stat.resume_by_software_wakeup);
		count += sprintf(buf + count, "| |--hiber_resume_by_ep_count: ");
		for(i = 0, j = 0; i < DWC3_ENDPOINTS_NUM; i++){
			if(0 == dwc->stat.ep_queue_in_supsned[i])
				continue;
			count += sprintf(buf + count, " [%d %u]", i, dwc->stat.ep_queue_in_supsned[i]);
			j++;
			if(0 == j%8)
			count += sprintf(buf + count, " \n");
		}
		count += sprintf(buf + count, "\n");
		count += sprintf(buf + count, "| |--hiber_resume_conn < 1ms           :%u\n",dwc->stat.resume_conn_time[DWC3_TIME_1MS]);
		count += sprintf(buf + count, "| |--hiber_resume_conn < 5ms           :%u\n",dwc->stat.resume_conn_time[DWC3_TIME_5MS]);
		count += sprintf(buf + count, "| |--hiber_resume_conn < 10ms          :%u\n",dwc->stat.resume_conn_time[DWC3_TIME_10MS]);
		count += sprintf(buf + count, "| |--hiber_resume_conn < 30ms          :%u\n",dwc->stat.resume_conn_time[DWC3_TIME_30MS]);
		count += sprintf(buf + count, "| |--hiber_resume_conn Other           :%u\n",dwc->stat.resume_conn_time[DWC3_TIME_OTHER]);
		count += sprintf(buf + count, "| |--hiber_resume_noconn < 1ms         :%u\n",dwc->stat.resume_not_conn_time[DWC3_TIME_1MS]);
		count += sprintf(buf + count, "| |--hiber_resume_noconn < 5ms         :%u\n",dwc->stat.resume_not_conn_time[DWC3_TIME_5MS]);
		count += sprintf(buf + count, "| |--hiber_resume_noconn < 10ms        :%u\n",dwc->stat.resume_not_conn_time[DWC3_TIME_10MS]);
		count += sprintf(buf + count, "| |--hiber_resume_noconn < 30ms        :%u\n",dwc->stat.resume_not_conn_time[DWC3_TIME_30MS]);
		count += sprintf(buf + count, "| |--hiber_resume_noconn Other         :%u\n",dwc->stat.resume_not_conn_time[DWC3_TIME_OTHER]);
		count += sprintf(buf + count, "| |--hiber_resume_noconn Other         :%u\n",dwc->stat.resume_not_conn_time[DWC3_TIME_OTHER]);
		
		count += sprintf(buf + count, "| |--hiber_resume_link_state_count:");
		for(i = 0; i < DWC3_LINK_STATE_BUTTOM; i++){
			if(0 == dwc->stat.resume_link_state[i])
				continue;
			count += sprintf(buf + count, " [%d %u]", i, dwc->stat.resume_link_state[i]);
		}
		count += sprintf(buf + count, "\n");

		count += sprintf(buf + count, "| |--hiber_resume_reentry              :%u\n",dwc->stat.resume_reentry_fail);
		count += sprintf(buf + count, "| |--hiber_resume_reentry_by_host      :%u\n",dwc->stat.resume_reentry_by_host_fail);
		count += sprintf(buf + count, "| |--hiber_resume_reentry_by_remote    :%u\n",dwc->stat.resume_reentry_by_remote_fail);
		count += sprintf(buf + count, "| |--hiber_resume_reentry_by_software  :%u\n",dwc->stat.resume_reentry_by_software_fail);
		count += sprintf(buf + count, "| |--hiber_resume_power_up_fail        :%u\n",dwc->stat.resume_power_up_fail);
		count += sprintf(buf + count, "| |--hiber_resume_switch_d0_fail       :%u\n",dwc->stat.resume_switch_d0_fail);
		count += sprintf(buf + count, "| |--hiber_resume_reinit_fail          :%u\n",dwc->stat.resume_reinit_fail);
		count += sprintf(buf + count, "| |--hiber_resume_devready_fail        :%u\n",dwc->stat.resume_devready_fail);
		count += sprintf(buf + count, "| |--hiber_resume_unconn_reset_fail    :%u\n",dwc->stat.resume_unconn_reset_fail);
		count += sprintf(buf + count, "| |--hiber_resume_set_link_state_fail  :%u\n",dwc->stat.resume_set_link_state_fail);
	}

#endif

    return count;
}

static ssize_t
dwc3_store(struct device *pdev, struct device_attribute *attr, const char *buf, size_t size)
{
    unsigned int msg_level;
    (void)sscanf(buf, "%x", &msg_level);

    dwc3_msg_level = (unsigned long)msg_level;

    return size;
}

static DEVICE_ATTR(dwc3_common, S_IRUGO | S_IWUSR, dwc3_show, dwc3_store);

#if (FEATURE_ON == MBB_USB)
int dwc3_phy_auto_powerdown(int enable)
{
    g_dwc_phy_auto_pwrdown = enable;
    return 0;
}
#endif

static void dwc3_disable_phy_by_speed(struct dwc3 *dwc, unsigned int speed)
{
    if (bsp_usb_is_support_phy_apd()) {

        /* usb3.0 powerdown hsp phy, usb2.0 powerdown ssp phy */
        if (DWC3_DCFG_SUPERSPEED == speed) {
            syssc_usb_powerdown_hsp(1);
        }
        else {
            syssc_usb_powerdown_ssp(1);
        }
    }
    return;
}

static void dwc3_enable_both_phy()
{
    /* enable both usb2.0 and usb3.0 phy */
    syssc_usb_powerdown_hsp(0);
    syssc_usb_powerdown_ssp(0);
}


#if defined(CONFIG_GADGET_SUPPORT_REMOTE_WAKEUP)
static ssize_t
gadget_rwakeup_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
    ssize_t count;
    struct dwc3 *dwc = container_of(pdev, struct dwc3, gadget.dev);

    count = snprintf(buf, sizeof(int),"%d\n",dwc->gadget.hs_rwakeup_cap);

    return count;
}
static ssize_t
gadget_rwakeup_store(struct device *pdev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct dwc3 *dwc = container_of(pdev, struct dwc3, gadget.dev);
    int gadget_wakeup_en;

    (void)sscanf(buf, "%d", &gadget_wakeup_en);

    dwc->gadget.hs_rwakeup_cap = gadget_wakeup_en;

    return size;
}
static DEVICE_ATTR(remote_wakeup_enable, S_IRUGO | S_IWUSR, \
    gadget_rwakeup_show, gadget_rwakeup_store);
#endif
static ssize_t
dwc3_wakeup_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
    ssize_t count;
    struct usb_gadget *gadget = container_of(pdev, struct usb_gadget, dev);
    count = sprintf(buf, "%d\n",gadget->hs_rwakeup);

    return count;
}
static ssize_t
dwc3_wakeup_store(struct device *pdev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct dwc3 *dwc = container_of(pdev, struct dwc3, gadget.dev);

#if defined(CONFIG_GADGET_SUPPORT_REMOTE_WAKEUP)
    int wakeup;

    (void)sscanf(buf, "%d", &wakeup);

    if (!wakeup) {
        dev_warn(dwc->dev,"Attention:Write 1 pls, if you want to try to wakeup \
            the host\n");
        return size;
    }

    if (dwc->gadget.hs_rwakeup) {
        int ret;
        dev_dbg(dwc->dev, "try to wakeup the host\n");

        ret = dwc3_gadget_wakeup(&dwc->gadget);

        if (ret) {
            dev_dbg(dwc->dev,"dwc3_gadget_wakeup failed:%d\n",ret);
        } else {
            dev_dbg(dwc->dev,"succeed to invoke dwc3_gadget_wakeup\n");
        }
    }
    else {
        dev_warn(dwc->dev, "remote wakeup disabled by the host\n");
    }
#else
    dev_warn(dwc->dev, "GADGET_SUPPORT_REMOTE_WAKEUP is not defined\n");
#endif

    return size;
}

static DEVICE_ATTR(wakeup, S_IRUGO | S_IWUSR, dwc3_wakeup_show, dwc3_wakeup_store);

static irqreturn_t dwc3_interrupt(int irq, void *_dwc)
{
	struct dwc3			*dwc = _dwc;
	unsigned int		i;
	irqreturn_t			ret = IRQ_NONE;

	dwc->event_stat.intr_total++;

	spin_lock(&dwc->lock);

	for (i = 0; i < dwc->num_event_buffers; i++) {
		irqreturn_t status;

		status = dwc3_process_event_buf(dwc, i);
        if(IRQ_NONE != status){
            ret = status;
        }
	}

	spin_unlock(&dwc->lock);

	return ret;
}

static struct device_attribute *dwc3_gadget_attributes[] = {
    &dev_attr_dwc3_common,
    &dev_attr_wakeup,
#ifdef CONFIG_GADGET_SUPPORT_REMOTE_WAKEUP
    &dev_attr_remote_wakeup_enable,
#endif
#ifdef CONFIG_USB_OTG_DWC_BALONG
    &dev_attr_hnp,
    &dev_attr_rsp,
    &dev_attr_srp,
    &dev_attr_end,
#endif
    NULL
};


int dwc3_gadget_reinit(struct dwc3 *dwc)
{
    int ret;
    unsigned long flags;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return -EPERM;
    }
#endif

    spin_lock_irqsave(&dwc->lock, flags);
    ret = dwc3_gadget_soft_reset(dwc);
    spin_unlock_irqrestore(&dwc->lock, flags);

    if(ret)
        DWC3_ERR("Soft ware reset usb fail. \n");
	
    return ret;
}

/**
 * dwc3_gadget_init - Initializes gadget related registers
 * @dwc: pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int dwc3_gadget_init(struct dwc3 *dwc)
{
	u32					reg;
	int					ret;
	struct device_attribute **attrs = dwc3_gadget_attributes;
	struct device_attribute *attr;

	dwc->ctrl_req = dma_alloc_coherent(dwc->dev, sizeof(*dwc->ctrl_req),
			&dwc->ctrl_req_addr, GFP_KERNEL);
	if (!dwc->ctrl_req) {
		dev_err(dwc->dev, "failed to allocate ctrl request\n");
		ret = -ENOMEM;
		goto err0;
	}

	dwc->ep0_trb = dma_alloc_coherent(dwc->dev, sizeof(*dwc->ep0_trb),
			&dwc->ep0_trb_addr, GFP_KERNEL);
	if (!dwc->ep0_trb) {
		dev_err(dwc->dev, "failed to allocate ep0 trb\n");
		ret = -ENOMEM;
		goto err1;
	}

    /* BUGFIX: ZLP on non-endpoint0 */
	dwc->zlp = dma_alloc_coherent(dwc->dev, ZLP_MAX_PACKET_SIZE,
			&dwc->zlp_dma, GFP_KERNEL);
	if (!dwc->zlp) {
		dev_err(dwc->dev, "failed to allocate zlp\n");
		ret = -ENOMEM;
		goto err2;
	}

	dwc->setup_buf = kzalloc(DWC3_EP0_BOUNCE_SIZE, GFP_KERNEL);
	if (!dwc->setup_buf) {
		dev_err(dwc->dev, "failed to allocate setup buffer\n");
		ret = -ENOMEM;
		goto err_append2;
	}

	dwc->ep0_bounce = dma_alloc_coherent(dwc->dev,
			DWC3_EP0_BOUNCE_SIZE, &dwc->ep0_bounce_addr,
			GFP_KERNEL);
	if (!dwc->ep0_bounce) {
		dev_err(dwc->dev, "failed to allocate ep0 bounce buffer\n");
		ret = -ENOMEM;
		goto err3;
	}

	dwc->gadget.ops			= &dwc3_gadget_ops;
	dwc->gadget.max_speed		= USB_SPEED_SUPER;
    if(DWC3_MODE_DRD == bsp_usb_mode_support()){
        dwc->gadget.is_otg = 1;
    }

	dwc->gadget.speed		= USB_SPEED_UNKNOWN;
	dwc->gadget.sg_supported	= true;
	dwc->gadget.name		= "dwc3-gadget";
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	dwc->restore_flag = 0;
	dwc->gadget.is_suspend = 0;
#endif

	/*
	 * REVISIT: Here we should clear all pending IRQs to be
	 * sure we're starting from a well known location.
	 */

	ret = dwc3_gadget_init_endpoints(dwc);
	if (ret)
		goto err4;
	
	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg |= DWC3_DCFG_LPM_CAP;
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	/* Enable USB2 LPM and automatic phy suspend only on recent versions */
	if (dwc->revision >= DWC3_REVISION_194A) {
		dwc3_gadget_usb2_phy_suspend(dwc, false);
		dwc3_gadget_usb3_phy_suspend(dwc, false);
	}
	
#ifdef CONFIG_GADGET_CORE_HIBERNATION	
	ret = dwc3_gadget_hibernation_init(dwc);
	if (ret) {
		dev_err(dwc->dev, "failed to init hibernation\n");
		goto err5;
	}

#endif

	ret = usb_add_gadget_udc(dwc->dev, &dwc->gadget);
	if (ret) {
		dev_err(dwc->dev, "failed to register udc\n");
		goto err5;
	}

    while ((attr = *attrs++)) {
		ret = device_create_file(dwc->dev, attr);
		if (ret) {
			goto err6;
		}
	}
    /* stick hilink 为降低整机功耗，修改USB phy 
       自适应，枚举为2.0则关闭3.0 否则相反  */
#if (FEATURE_ON == MBB_USB)
    dwc3_phy_auto_powerdown(1);
#endif

	the_dwc3 = dwc;
		
	if(dwc3_conn_ops.usb_set_dwc_cb){
		dwc3_conn_ops.usb_set_dwc_cb(dwc);
	}

	return 0;

err6:
	usb_del_gadget_udc(&dwc->gadget);

err5:
	dwc3_gadget_free_endpoints(dwc);

err4:
	dma_free_coherent(dwc->dev, DWC3_EP0_BOUNCE_SIZE,
			dwc->ep0_bounce, dwc->ep0_bounce_addr);

err3:
	kfree(dwc->setup_buf);

    /* BUGFIX: ZLP on non-endpoint0 */
err_append2:
	dma_free_coherent(dwc->dev, ZLP_MAX_PACKET_SIZE,
			dwc->zlp, dwc->zlp_dma);

err2:
	dma_free_coherent(dwc->dev, sizeof(*dwc->ep0_trb),
			dwc->ep0_trb, dwc->ep0_trb_addr);

err1:
	dma_free_coherent(dwc->dev, sizeof(*dwc->ctrl_req),
			dwc->ctrl_req, dwc->ctrl_req_addr);

err0:
	return ret;
}

/* -------------------------------------------------------------------------- */

void dwc3_gadget_exit(struct dwc3 *dwc)
{
#ifdef CONFIG_GADGET_CORE_HIBERNATION
	if((1 == dwc->gadget.is_suspend) && bsp_usb_is_support_hibernation()){
		dwc3_gadget_hibernation_soft_resume(dwc);
	}
#endif
	usb_del_gadget_udc(&dwc->gadget);

	dwc3_gadget_free_endpoints(dwc);

	dma_free_coherent(dwc->dev, DWC3_EP0_BOUNCE_SIZE,
			dwc->ep0_bounce, dwc->ep0_bounce_addr);

	kfree(dwc->setup_buf);

    /* BUGFIX: ZLP on non-endpoint0 */
	dma_free_coherent(dwc->dev, ZLP_MAX_PACKET_SIZE,
			dwc->zlp, dwc->zlp_dma);

	dma_free_coherent(dwc->dev, sizeof(*dwc->ep0_trb),
			dwc->ep0_trb, dwc->ep0_trb_addr);

	dma_free_coherent(dwc->dev, sizeof(*dwc->ctrl_req),
			dwc->ctrl_req, dwc->ctrl_req_addr);

#ifdef CONFIG_GADGET_CORE_HIBERNATION
	dwc3_gadget_hibernation_exit(dwc);
#endif

	the_dwc3 = NULL;

}

int dwc3_gadget_prepare(struct dwc3 *dwc)
{
	if (dwc->pullups_connected)
		dwc3_gadget_disable_irq(dwc);

	return 0;
}

void dwc3_gadget_complete(struct dwc3 *dwc)
{
	if (dwc->pullups_connected) {
		dwc3_gadget_enable_irq(dwc);
		dwc3_gadget_run_stop(dwc, true);
	}
}

#ifdef CONFIG_GADGET_CORE_HIBERNATION
static inline void dwc3_gadget_config_phyclk(struct dwc3 *dwc)
{
	unsigned int is_superspeed = true;
	int ret;
	if(bsp_usb_is_force_highspeed()){
		is_superspeed = false;		
	}
	ret = dwc3_pmuctrl_set_usbphy_clk_mode(is_superspeed);
	if(ret)
		DWC3_ERR_INTR("Config phyclk fail\n");
	return;
}


static inline void dwc3_gadget_vcc_reset(void)
{
	syscrg_vcc_reset_for_lpm();
}

static inline void dwc3_gadget_vcc_unreset(void)
{
	syscrg_vcc_unreset_for_lpm();
}

static inline void dwc3_gadget_set_pmu_to_d3(void)
{
	syscrg_usb_set_pmu_state(0x03);
}

static inline void dwc3_gadget_set_pmu_to_d0(void)
{
	syscrg_usb_set_pmu_state(0x00);
}

static inline int dwc3_gadget_get_pmu_dstatus(struct dwc3 *dwc)
{
	unsigned int is_superspeed = 0;
	if(USB_SPEED_SUPER == dwc->gadget.speed){
		is_superspeed = 1;
	}
	return syscrg_usb_get_pmu_state(is_superspeed);
}


static inline int dwc3_gadget_get_pmu2_dstatus(void)
{
	return syscrg_usb2_get_pmu_state();
}

static inline int dwc3_gadget_get_pmu3_dstatus(void)
{
	return syscrg_usb3_get_pmu_state();
}

static inline void dwc3_gadget_pwc_down_phy(struct dwc3 *dwc)
{
#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return;
    }
#endif

	if(USB_SPEED_SUPER == dwc->gadget.speed){
		syssc_usb_powerdown_hsp(1);
	}else{
		syssc_usb_powerdown_ssp(1);
	}
}


static irqreturn_t dwc3_pme_interrupt(int irq, void *_dwc)
{
	struct dwc3			*dwc = (struct dwc3 *)_dwc;
	unsigned long flags;
	int ret;
	
	if(!bsp_usb_is_support_hibernation()){
		return IRQ_HANDLED;
	}

	spin_lock_irqsave(&dwc->lpm_lock, flags);
	dwc->stat.resume_by_host ++;
	if(dwc->gadget.is_suspend)
		dwc->stat.resume_reentry_by_host_fail++;
	ret = dwc3_gadget_hibernation_resume(dwc);
	spin_unlock_irqrestore(&dwc->lpm_lock, flags);

	if(ret){
		DWC3_ERR_INTR("Host wakeup fail.\n");
	}else{
		DWC3_ERR_INTR("Host wakeup succ!\n");
	}
	dwc->restore_flag = 0;
	return IRQ_HANDLED;
}

static void dwc3_gadget_hibernation_set_scratchpad_buf_array(struct dwc3 *dwc)
{
	u32 param;
	s32 ret;
/*This step have been described in data book but enable it cause usb
device can not be identify by host after hibernation resume.

	u32 reg;
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	if(!(reg & DWC3_DCTL_RUN_STOP)){
		reg = dwc3_readl(dwc->regs, DWC3_DGCMD);
		reg &= ~DWC3_DGCMD_CMDIOC;
		dwc3_writel(dwc->regs, DWC3_DGCMD, reg);
	}
*/
	param = lower_32_bits(dwc->hiber_scratchpad_array_dma_addr);
	ret = dwc3_send_gadget_generic_command(dwc,
		DWC3_DGCMD_SET_SCRATCHPAD_ADDR_LO, param);
	WARN_ON_ONCE(ret);
	dwc->stat.set_scratchpad_buf_array_low_fail++;
	
	/*Use CONFIG_ARCH_DMA_ADDR_T_64BIT is purely for coverity miss warning. */
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	param = upper_32_bits(dwc->hiber_scratchpad_array_dma_addr);
#else
	param = 0;
#endif
	ret = dwc3_send_gadget_generic_command(dwc,
		DWC3_DGCMD_SET_SCRATCHPAD_ADDR_HI, param);
	WARN_ON_ONCE(ret);
	dwc->stat.set_scratchpad_buf_array_high_fail++;

	return;
}


inline void dwc3_gadget_hibernation_soft_resume(struct dwc3 *dwc)
{
	unsigned long flags;
	int ret;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return -EPERM;
    }
#endif

	DWC3_ERR("Software resume.\n");

	spin_lock_irqsave(&dwc->lpm_lock, flags);
	dwc->stat.resume_by_software_wakeup++;
	
	if(dwc->gadget.is_suspend)
		dwc->stat.resume_reentry_by_software_fail++;

	ret = dwc3_gadget_hibernation_resume(dwc);
	spin_unlock_irqrestore(&dwc->lpm_lock, flags);

	if(ret){
		DWC3_ERR("Software resume fail.\n");
	}else{
		DWC3_ERR("Software resume succ.\n");
	}

}

/*Check support hibernation before call this*/
int dwc3_gadget_hibernation_remote_wakeup(struct dwc3 *dwc)
{
	unsigned long			flags;
	int ret;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return -EOPNOTSUPP;
    }
#endif

	/*USB Highspeed remote wakeup enable check here. Superspeed check this function. */
	if(unlikely((DWC3_DCFG_SUPERSPEED != dwc->speed)&&(!dwc->gadget.hs_rwakeup))){
		DWC3_ERR("Highspeed remote wakeup disable.\n");
		return -EOPNOTSUPP;
	}
	spin_lock_irqsave(&dwc->lpm_lock, flags);
	ret = dwc3_gadget_hibernation_resume(dwc);
	dwc->restore_flag = 0;
	spin_unlock_irqrestore(&dwc->lpm_lock, flags);
	if(ret){
		DWC3_ERR("Remote wakeup fail.\n");
	}else{
		DWC3_ERR("Remote wakeup succ.\n");
	}
	return ret;
}


static int dwc3_gadget_hibernation_set_usbphy_ao(void)
{
	int ret = 0;

	ret = dwc3_pmuctrl_set_usbphy_pwctrl_mode(true);
	return ret;
}

static int dwc3_gadget_hibernation_start_ep0(struct dwc3 *dwc)
{

	struct dwc3_ep		*dep;
	int ret;


	dep = dwc->eps[0];
	dep->flags &= ~(DWC3_EP_ENABLED | DWC3_EP_BUSY);
	dep->resource_index = 0;

	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err0;
	}

	dep = dwc->eps[1];
	dep->flags &= ~DWC3_EP_ENABLED;

	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err1;
	}

	/* begin to receive SETUP packets */
	dwc->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dwc);/*lint !e413*/

	return 0;

err1:
	__dwc3_gadget_ep_disable(dwc->eps[0]);

err0:
	return ret;
}


static int dwc3_gadget_hibernation_irq_register(struct dwc3 *dwc)
{
	int ret;
	int pme_irq;

	if(!bsp_usb_is_support_hibernation()){
		return 0;
	}

	pme_irq = platform_get_irq_byname(to_platform_device(dwc->dev), "usb_pme_irq");

	if(pme_irq == 0){
		DWC3_ERR("acquire irq number fail.\n");
		return -ENXIO;
	}

	ret = request_irq(pme_irq, dwc3_pme_interrupt,
		IRQF_NO_SUSPEND|IRQF_TRIGGER_HIGH, "usb_pme", dwc);
	if (ret) {
		DWC3_ERR("failed to request irq #%d --> %d\n",pme_irq, ret);
		return -ENXIO;
	}

	return ret;
}

static int dwc3_gadget_hibernation_reinit(struct dwc3 *dwc)
{
	u32 scratch_buffer_num;
	u32 reg;
	int ret;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return -EPERM;
    }
#endif

	if(!bsp_usb_is_support_hibernation())
		return 0;

	if (dwc->revision != DWC3_REVISION_300A){
		//bsp_pmu_usbphy_ldo_set();
	}
	else{
		ret = dwc3_gadget_hibernation_set_usbphy_ao();
		/*set usb phy 3.3v power input to always on.*/
		if(ret){
			dev_err(dwc->dev, "Set usbphy ao in deep sleep fail. \n");
			return -EIO;
		}
	}

	reg = dwc3_readl(dwc->regs, DWC3_GHWPARAMS1);
	if(DWC3_GHWPARAMS1_EN_PWROPT_HIB != DWC3_GHWPARAMS1_EN_PWROPT(reg)){
		dev_err(dwc->dev, "dwc3 core does not support hibernation. \n");
		return -ESRCH;
	}

	//step2
	reg = dwc3_readl(dwc->regs, DWC3_GHWPARAMS4);
	scratch_buffer_num = DWC3_GHWPARAMS4_HIBER_SCRATCHBUFS(reg);

	if(DWC3_MAX_HIBER_SCRATCHBUFS < scratch_buffer_num){
		dev_err(dwc->dev, "dwc3 core scratch lager than max. \n");
		return -ENXIO;
	}

	//step 4
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg |= DWC3_GCTL_GBLHIBERNATIONEN;
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	//step 5-6:We do not support L1 hiberntion do nothing./*fixme read l1 en to be sure*/
	//step 7
	dwc3_gadget_hibernation_set_scratchpad_buf_array(dwc);

	dwc3_gadget_usb2_phy_suspend(dwc, true);
	dwc3_gadget_usb3_phy_suspend(dwc, true);
	return 0;
}

static int dwc3_gadget_hibernation_init(struct dwc3 *dwc)
{
	u32 scratch_buffer_num;
	u32 reg;
	u32 i;
	dma_addr_t dma_addr = 0;
	int ret;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return -EPERM;
    }
#endif

	if(!bsp_usb_is_support_hibernation()){
		return 0;
	}
	memset(&(dwc->stat), 0, sizeof(struct dwc3_stat_t));

	if (dwc->revision != DWC3_REVISION_300A){
		//bsp_pmu_usbphy_ldo_set();
	}
	else{
		ret = dwc3_gadget_hibernation_set_usbphy_ao();
		/*set usb phy 3.3v power input to always on.*/
		if(ret){
			dev_err(dwc->dev, "Set usbphy ao in deep sleep fail. \n");
			return -EIO;
		}
	}

	reg = dwc3_readl(dwc->regs, DWC3_GHWPARAMS1);
	if(DWC3_GHWPARAMS1_EN_PWROPT_HIB != DWC3_GHWPARAMS1_EN_PWROPT(reg)){
		dev_err(dwc->dev, "dwc3 core does not support hibernation. \n");
		return -ESRCH;
	}

	//step2
	reg = dwc3_readl(dwc->regs, DWC3_GHWPARAMS4);
	scratch_buffer_num = DWC3_GHWPARAMS4_HIBER_SCRATCHBUFS(reg);

	if(DWC3_MAX_HIBER_SCRATCHBUFS < scratch_buffer_num){
		dev_err(dwc->dev, "dwc3 core scratch lager than max. \n");
		return -ENXIO;
	}
	dwc->scratch_buffer_num = scratch_buffer_num;

	//step3
	/*Allocate scratch buffer head */
	dwc->hiber_scratchpad_array =
		dma_alloc_coherent(NULL, sizeof(struct dwc_hiber_scratchpad_array),
			&dwc->hiber_scratchpad_array_dma_addr, GFP_KERNEL);

	if(!dwc->hiber_scratchpad_array){
	 	DWC3_ERR(" hibernation hiber_scratchpad_array allocation fail\n");
		return -ENOMEM;
	}
	memset((void *)dwc->hiber_scratchpad_array,
		0, sizeof(struct dwc_hiber_scratchpad_array));

	/* Allocate scratch buffers */
	for (i = 0; i < scratch_buffer_num; i++) {
		dwc->hiber_scratchpad_virt_addr[i] =
			dma_alloc_coherent(NULL, 0x1000, &dma_addr, GFP_KERNEL);

		if(!dwc->hiber_scratchpad_virt_addr[i]){
		 	DWC3_ERR(" hibernation buffers allocation fail\n");
			goto err;
		}
		dwc->hiber_scratchpad_array->dma_addr[i] = (u64)dma_addr;
		memset(dwc->hiber_scratchpad_virt_addr[i], 0, 0x1000);
	}

	//step 4
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg |= DWC3_GCTL_GBLHIBERNATIONEN;
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	//step 5-6:We do not support L1 hiberntion do nothing./*fixme read l1 en to be sure*/
	//step 7
	dwc3_gadget_hibernation_set_scratchpad_buf_array(dwc);

	dwc3_gadget_usb2_phy_suspend(dwc, true);
	dwc3_gadget_usb3_phy_suspend(dwc, true);

	return 0;

err:
	for (i = 0; i < scratch_buffer_num; i++) {
		if(dwc->hiber_scratchpad_virt_addr[i]){
			dma_addr = dwc->hiber_scratchpad_array->dma_addr[i];
 			dma_free_coherent(NULL, 0x1000, dwc->hiber_scratchpad_virt_addr[i], dma_addr);
			dwc->hiber_scratchpad_virt_addr[i] = NULL;
		}
	}

	dma_free_coherent(NULL, 0x1000,
		dwc->hiber_scratchpad_array, dwc->hiber_scratchpad_array_dma_addr);
	return -ENOMEM;

}

static void dwc3_gadget_hibernation_exit(struct dwc3 *dwc)
{
	u32		i;
	dma_addr_t dma_addr;

	if(!bsp_usb_is_support_hibernation()){
		return;
	}

	for (i = 0; i < dwc->scratch_buffer_num; i++) {
		if(dwc->hiber_scratchpad_virt_addr[i]){
			dma_addr = dwc->hiber_scratchpad_array->dma_addr[i];
 			dma_free_coherent(NULL, 0x1000, dwc->hiber_scratchpad_virt_addr[i], dma_addr);
			dwc->hiber_scratchpad_virt_addr[i] = NULL;
		}
	}

	dma_free_coherent(NULL, 0x1000,
		dwc->hiber_scratchpad_array, dwc->hiber_scratchpad_array_dma_addr);
}

static inline enum dwc3_lpm_time dwc3_gadget_hibernation_get_time_cost(unsigned int time_start)
{
	unsigned int time_end=0;
	unsigned int time_cost=0;

	time_end = bsp_get_slice_value_hrt();

	time_cost = get_timer_slice_delta(time_start, time_end);
		
	if(time_cost <= HRT_1MS){
		return DWC3_TIME_1MS;
	}else if(time_cost <= HRT_5MS){
		return DWC3_TIME_5MS;
	}else if(time_cost <= HRT_10MS){
		return DWC3_TIME_10MS;
	}else if(time_cost <= HRT_30MS){
		DWC3_ERR_INTR("SR Time larger than 10ms\n");
		return DWC3_TIME_30MS;
	}else{
		DWC3_ERR_INTR("SR Time larger than 30ms\n");
		return DWC3_TIME_OTHER;
	}
}

void dwc3_gadget_hibernation_print_d3_time(unsigned int time_start)
{
	unsigned int time_end=0;
	unsigned int time_cost=0;
	unsigned int time_cost_ms = 0;
	unsigned int time_cost_ns = 0;

	time_end = bsp_get_slice_value_hrt();
	time_cost = get_timer_slice_delta(time_start, time_end);
	time_cost = time_cost/HRT_10NS;
	time_cost_ms = time_cost/(HRT_1MS/HRT_10NS);
	time_cost_ns = time_cost%(HRT_1MS/HRT_10NS);
	DWC3_ERR("D3 Time cost %u.%u ms", time_cost_ms, time_cost_ns);
	
	return ;	
}

static void dwc3_gadget_hibernation_save_ep_state(struct dwc3 * dwc, struct dwc3_ep *dep)
{
	u32		ret;
	u32     cmd;
	struct dwc3_gadget_ep_cmd_params params;

	cmd = DWC3_DEPCMD_GETEPSTATE;
	memset(&params, 0, sizeof(params));

	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, cmd, &params);
	WARN_ON_ONCE(ret);
	//read and asve BEPCMDPAR2n
	dep->saved_state = dwc3_readl(dwc->regs, DWC3_DEPCMDPAR2(dep->number));

}

static void	dwc3_gadget_hibernation_save_reg(struct dwc3 *dwc)
{
	struct dwc3_ep	*dep;
	u32		reg;
	u32		i;
	int		epnum;

	dwc->hiber_backup_reg.dctl = dwc3_readl(dwc->regs, DWC3_DCTL);
	dwc->hiber_backup_reg.dcfg = dwc3_readl(dwc->regs, DWC3_DCFG);
	dwc->hiber_backup_reg.devten = dwc3_readl(dwc->regs, DWC3_DEVTEN);

	dwc->hiber_backup_reg.gctl = dwc3_readl(dwc->regs, DWC3_GCTL);
	dwc->hiber_backup_reg.gusb3pipectl0 = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
	dwc->hiber_backup_reg.gusb2phycfg0 = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
	for (i = 0; i < dwc->num_event_buffers; i++) {
		dwc->hiber_backup_reg.gevntaddrlo[i] = dwc3_readl(dwc->regs, DWC3_GEVNTADRLO(i));
		dwc->hiber_backup_reg.gevntaddrhi[i] = dwc3_readl(dwc->regs, DWC3_GEVNTADRHI(i));
		dwc->hiber_backup_reg.gevntsiz[i] = dwc3_readl(dwc->regs, DWC3_GEVNTSIZ(i));
   }

	dwc->hiber_backup_reg.dalepena = dwc3_readl(dwc->regs, DWC3_DALEPENA);

	for (epnum = 0; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		dep = dwc->eps[epnum];
		if (!(epnum & 1))
			continue;
		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;
		reg = dwc3_readl(dwc->regs, DWC3_GTXFIFOSIZ(epnum >> 1));
		dep->txfifosz_save = reg;
	}

	dep = dwc->eps[0];
	dep->rxfifosz_save = dwc3_readl(dwc->regs, DWC3_GRXFIFOSIZ(0));
}

static int	dwc3_gadget_hibernation_restore_state(struct dwc3 *dwc)
{
	u32		timeout = 5000;
	u32		reg;

	//step 4
	dwc3_gadget_hibernation_set_scratchpad_buf_array(dwc);

	//step 5
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg |= DWC3_DCTL_CRS;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	do {
		udelay(100);
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);
		if(!(reg & DWC3_DSTS_RSS)){
			break;
		}
		timeout--;
		if(0 == timeout){
			DWC3_ERR_INTR("Restore scratchpad Timeout \n");
			return -ETIMEDOUT;
		}
	} while (1);

	/*Remove restore txfifo operation for readl result in backup process have some fass.
		The backuped txfifo value is different from the value in normal operation.
		And this maybe the reason why USB turn into unknow device after dial.
	*/
	/*
	for (epnum = 0; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		dep = dwc->eps[epnum];
		if (!(epnum & 1))
			continue;
		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;
		reg = dep->txfifosz_save;
		dwc3_writel(dwc->regs, DWC3_GTXFIFOSIZ(epnum >> 1), reg);
	}
	dep = dwc->eps[0];
	dwc3_writel(dwc->regs, DWC3_GRXFIFOSIZ(0), dep->rxfifosz_save);
	*/

	return 0;
}



void dwc3_gadget_hibernation_stop_transfers(struct dwc3 *dwc)
{
	u32 epnum;
	struct dwc3_request		*req;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return;
    }
#endif

	for (epnum = 0; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		struct dwc3_ep *dep;

		dep = dwc->eps[epnum];
		if (!dep)
			continue;
		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;

		/*same as dwc3_remove_requests, with ForceRM=0*/
		if (!list_empty(&dep->req_queued)) {
			dwc3_stop_active_transfer(dwc, dep->number, false);

			/* - giveback all requests to gadget driver */
			while (!list_empty(&dep->req_queued)) {
				req = next_request(&dep->req_queued);
				if(NULL != req){
					dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
				}
			}
		}

		while (!list_empty(&dep->request_list)) {
			req = next_request(&dep->request_list);
			if(NULL != req){
				dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
			}
		}
	}
}


/*
 * Configure the core as described in databook Section 9.1.1 "Device
 * Power-On or Soft Reset," excluding the first step (Soft Reset).
 */
static int dwc3_gadget_hibernation_reinit_ctrl(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	u32		i;
	s32		ret;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return -EPERM;
    }
#endif
	dwc3_writel(dwc->regs, DWC3_GCTL, dwc->hiber_backup_reg.gctl);
	dwc3_writel(dwc->regs, DWC3_DCFG, dwc->hiber_backup_reg.dcfg);
	dwc3_writel(dwc->regs, DWC3_DCTL, dwc->hiber_backup_reg.dctl);

	for (i = 0; i < dwc->num_event_buffers; i++) {
		evt = dwc->ev_buffs[i];
		DWC3_INFO("Event buf %pK dma %08llx length %d\n",
				evt->buf, (unsigned long long) evt->dma,
				evt->length);

		evt->lpos = 0;

		dwc3_writel(dwc->regs, DWC3_GEVNTADRLO(i),lower_32_bits(evt->dma));
		dwc3_writel(dwc->regs, DWC3_GEVNTADRHI(i),upper_32_bits(evt->dma));
		dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(i),evt->length & 0xffff);
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(i), 0);
	}

	dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), dwc->hiber_backup_reg.gusb3pipectl0);
	dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), dwc->hiber_backup_reg.gusb2phycfg0);

	/* Enable IRQs */
	dwc3_gadget_enable_irq(dwc);

	/*Enable EPs*/

	ret = dwc3_gadget_hibernation_start_ep0(dwc);
	if(ret){
		DWC3_ERR_INTR("Dwc3 gadget start ep0 fail.\n");
		return ret;
	}

	return 0;
}

static void dwc3_gadget_hibernation_restart_xfer(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params	params;
	struct dwc3_trb			*trb;
	dma_addr_t				trb_dma;
	u32					ctrl, size, stat, cmd;
	int					i, owned, ret;

	dev_vdbg(dwc->dev, "%s()\n", __func__);
	trb = dep->trb_pool;

	/* Find the first non-hw-owned TRB */
	for (i = 0; i < DWC3_TRB_NUM; i++, trb++) {
		ctrl = le32_to_cpu(trb->ctrl);
		size = le32_to_cpu(trb->size);
		stat = (size >> 28) & 0xf;
		if (!(ctrl & 1) && !(stat & DWC3_TRB_STS_XFER_IN_PROG)) {
			dev_vdbg(dwc->dev, "Found non-hw-owned TRB at %d\n", i);
			break;
		}
	}

	if (i == DWC3_TRB_NUM)
		trb = dep->trb_pool;

	/* Find the first following hw-owned TRB */
	for (i = 0, owned = -1; i < DWC3_TRB_NUM; i++) {
		/*
		 * If status == 4, this TRB's xfer was in progress prior to
		 * entering hibernation
		 */
		ctrl = le32_to_cpu(trb->ctrl);
		size = le32_to_cpu(trb->size);
		stat = (size >> 28) & 0xf;
		if (stat & DWC3_TRB_STS_XFER_IN_PROG) {
			dev_vdbg(dwc->dev, "Found in-progress TRB at %d\n", i);

			/* Set HWO back to 1 so the xfer can continue */
			ctrl |= 1;
			trb->ctrl = cpu_to_le32(ctrl);
			owned = trb - dep->trb_pool;
			break;
		}

		/* Save the index of the first TRB with the HWO bit set */
		if (ctrl & 1) {
			dev_vdbg(dwc->dev, "Found hw-owned TRB at %d\n", i);
			owned = trb - dep->trb_pool;
			break;
		}

		trb++;
		if (trb == dep->trb_pool + DWC3_TRB_NUM)
			trb = dep->trb_pool;
	}

	wmb();
	dep->hiber_trb_idx = 0;

	if (owned < 0)
		/* No TRB had HWO bit set, fine */
		return;

	dev_vdbg(dwc->dev, "idx=%d trb=%pK\n", owned, trb);

	/* Restart of Isoc EPs is deferred until the host polls the EP */
	if (usb_endpoint_type(dep->endpoint.desc) == USB_ENDPOINT_XFER_ISOC) {
		dev_vdbg(dwc->dev, "Deferring restart until host polls\n");
		dep->hiber_trb_idx = owned + 1;
		return;
	}

	dev_vdbg(dwc->dev, "%08x %08x %08x %08x\n",
			*((unsigned *)trb), *((unsigned *)trb + 1),
			*((unsigned *)trb + 2), *((unsigned *)trb + 3));

	/* Now restart at the first TRB found with HWO set */
	trb_dma = dep->trb_pool_dma + owned * 16;

	memset(&params, 0, sizeof(params));

	/*Use CONFIG_ARCH_DMA_ADDR_T_64BIT is purely for coverity miss warning. */
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	params.param0 = upper_32_bits(trb_dma);
#else
	params.param0 = 0;
#endif
	params.param1 = lower_32_bits(trb_dma);

	cmd = DWC3_DEPCMD_STARTTRANSFER;

	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, cmd, &params);
	if (ret < 0) {
		dev_dbg(dwc->dev, "failed to send STARTTRANSFER command\n");
		/*
		 * FIXME what to do here?
		 */
		dep->resource_index = 0;
		dep->flags &= ~DWC3_EP_BUSY;
		return;
	}

	dep->resource_index = dwc3_gadget_ep_get_transfer_index(dwc,
			dep->number);
	WARN_ON_ONCE(!dep->resource_index);
}


void dwc3_gadget_hibernation_exit_after_connect(struct dwc3 *dwc)
{
	struct dwc3_gadget_ep_cmd_params	params;
	struct dwc3_ep				*dep;
	int					epnum, ret;

	dev_vdbg(dwc->dev, "%s \n", __func__);

	/*
	 * Perform the steps in Section 9.1.5 "Initialization on
	 * SetConfiguration or SetInterface Command" in databook.
	 *
	 * While issuing the DEPCFG commands, set each endpoint's sequence
	 * number and flow control state to the value read during the save.
	 *
	 * If the endpoint was in the Halted state prior to entering
	 * hibernation, software must issue the "Set STALL" endpoint command
	 * to put the endpoint back into the Halted state.
	 */
	memset(&params, 0, sizeof(params));

	for (epnum = 2; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		dep = dwc->eps[epnum];
		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;
		if (!dep->endpoint.desc)
			continue;
		dev_vdbg(dwc->dev, "Enabling phys EP%d\n", epnum);

		dep->flags &= ~DWC3_EP_ENABLED;
		ret = __dwc3_gadget_ep_enable(dep, dep->endpoint.desc, dep->comp_desc, false);
		if (ret)
			dev_err(dwc->dev, "failed to enable %s\n", dep->name);

		if (dep->flags & DWC3_EP_STALL) {
			ret = dwc3_send_gadget_ep_cmd(dwc, epnum,
						DWC3_DEPCMD_SETSTALL, &params);
			if (ret)
				dev_err(dwc->dev, "failed to set STALL on %s\n",
						dep->name);
		}
	}

	/*
	 * (In this step, software re-configures the existing endpoints and
	 * starts their transfers).
	 *
	 * When software issued the EndXfer command with the ForceRM field set
	 * to '0' prior to entering hibernation, the core may have written back
	 * an active TRB for the transfer, setting the HWO bit to '0'. Software
	 * must ensure that the TRB is valid and set the HWO bit back to '1'
	 * prior to re-starting the transfer in this step.
	 */

	for (epnum = 2; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		dep = dwc->eps[epnum];
		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;
		if (!(dep->flags & DWC3_EP_BUSY)|| !dep->resource_index)
			continue;
		dwc3_gadget_hibernation_restart_xfer(dwc, dep);
	}

}


#define D3_MAX_TIME 100000
#define D3_NORMAL_TIME 500

void dwc3_gadget_hibernation_suspend(struct dwc3 * dwc)
{
	u32		timeout = 500;
	u32		suspend_start = 0;
	u32		reg;
	u32		event_buf_num = 0;
	u32		ep_num = 0;
	u32		d3_enter = 0;
	u32		u2_d3_time = 0;
	u32		u3_d3_time = 0;
	int		u2_status = 0;
	int		u3_status = 0;

	struct dwc3_ep *dep;
	struct dwc3_event_buffer	*evt;
	enum dwc3_lpm_time suspend_time_zone;
	enum dwc3_link_state state;


#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return;
    }
#endif

	if(!bsp_usb_is_support_hibernation()){
		return;
	}
	
	suspend_start = bsp_get_slice_value_hrt();

	/* We can't rely on the link state machine to decide
	whether to do the suspend. As link state change intr
	would modify the dwc->link_state if enabled */
	if (dwc->gadget.is_suspend) {
		dwc->stat.suspend_reentry_fail++;
		DWC3_ERR_INTR("USB gadget already suspended\n");
		return;
	}

	dwc->stat.suspend_entry ++;

	if (dwc->gadget_driver->suspend) {
		dwc->gadget_driver->suspend(&dwc->gadget);
	}

	/* step 3 stop all transfers including ep0*/
	dwc3_gadget_hibernation_stop_transfers(dwc);

	//step4
	dwc->stat.suspend_while_connected ++;
	for(ep_num = 0; ep_num < DWC3_ENDPOINTS_NUM; ep_num++){
		dep = dwc->eps[ep_num];
		if(!(dep->flags & DWC3_EP_ENABLED))
			continue;
		dwc3_gadget_hibernation_save_ep_state(dwc, dep);
		dwc->stat.suspend_ep_saved[ep_num] ++;
	}

	//step5

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_RUN_STOP;
	reg |= DWC3_DCTL_KEEP_CONNECT;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	timeout = 500;
	do {
		for (event_buf_num = 0; event_buf_num < dwc->num_event_buffers; event_buf_num++) {
			evt = dwc->ev_buffs[event_buf_num];
			evt->lpos = 0;
			reg = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(event_buf_num));
			reg = reg&DWC3_GEVNTCOUNT_MASK;
	    	if (reg)
			dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(event_buf_num), reg);
		}

		reg = dwc3_readl(dwc->regs, DWC3_DSTS);
		if (reg & DWC3_DSTS_DEVCTRLHLT) {
			break;
		}

		timeout--;
		if (!timeout){
			dwc->stat.suspend_wait_halt_fail ++;
			DWC3_ERR_INTR("USB DEVICE HALT TIME OUT. \n");
			/*Disable usb interrupt and disable usb hibernation interrupt
			to stop hibernation interrupt*/
			reg = dwc3_readl(dwc->regs, DWC3_DCTL);
			reg &= ~DWC3_DCTL_KEEP_CONNECT;
			dwc3_writel(dwc->regs, DWC3_DCTL, reg);

			reg = dwc3_readl(dwc->regs, DWC3_DEVTEN);
			reg &= ~DWC3_DEVTEN_HIBERNATIONREQEVTEN;
			dwc3_writel(dwc->regs, DWC3_DEVTEN, reg);
			
			dwc->intr_bh_event |= DWC3_EVENT_BH_SOFT_RESET;
			tasklet_schedule(&dwc->intr_bh_tasklet);/*call dwc3_gadget_intr_bh*/
			return;
		}
		udelay(10);
	} while (1);

	state = dwc3_gadget_get_link_state(dwc);
	if(state == DWC3_LINK_STATE_SS_DIS){
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~DWC3_DCTL_KEEP_CONNECT;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	}
	//End of step5
	
	//step6
	dwc3_gadget_hibernation_save_reg(dwc);

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg |= DWC3_DCTL_CSS;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	udelay(10);

	//step6
	timeout = 500;
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);

		if (!(reg & DWC3_DSTS_SSS)) {
			break;
		}

		timeout--;
		if (!timeout){
			dwc->stat.suspend_save_state_fail ++;
			DWC3_ERR_INTR("USB SAVE STATE TIMEOUT. \n");
#if (FEATURE_ON == MBB_USB)
            spin_unlock_irqrestore(&dwc->lock, flags);
#endif
			return;
		}
		udelay(1);
	} while (1);

	//step 8 turn sys to D3 and wait until all go to D3,turn off the regulator
	dwc3_gadget_set_pmu_to_d3();
	timeout = D3_MAX_TIME;
	d3_enter = bsp_get_slice_value_hrt();
	do {
		if(0x3 != u2_status){
			u2_status = dwc3_gadget_get_pmu2_dstatus();
			if(0x3 == u2_status){
				u2_d3_time = D3_MAX_TIME-timeout;
			}
		}

		if(0x3 != u3_status){
			u3_status = dwc3_gadget_get_pmu3_dstatus();
			if(0x3 == u3_status){
				u3_d3_time = D3_MAX_TIME-timeout;
			}
		}

		if(u2_status == u3_status && 0x3 == u3_status){
			if((D3_MAX_TIME - timeout) > D3_NORMAL_TIME){
				dwc3_gadget_hibernation_print_d3_time(d3_enter);
				DWC3_ERR_INTR("D3 TIME u2 %u u3 %u. \n", u2_d3_time, u3_d3_time);
			}
			break;
		}
		
		timeout--;
		if (!timeout){
			dwc->stat.suspend_switch_d3_fail ++;
			DWC3_ERR_INTR("USB SET PMU TO D3 TIMEOUT. \n");
			return;
		}
		udelay(1);
	} while (1);

	/*step 9*/
	/*assert vcc_reset_n because power controller because last step may not power down vcc*/
	dwc3_gadget_vcc_reset();

	dwc3_gadget_config_phyclk(dwc);
	
	dwc->usb_core_powerdown = true;
	dwc->gadget.is_suspend = 1;
	dwc->restore_flag = 1;
	dwc->stat.suspend_done ++;
	dwc3_gadget_pwc_down_phy(dwc);

	suspend_time_zone = dwc3_gadget_hibernation_get_time_cost(suspend_start);
	dwc->stat.suspend_conn_time[suspend_time_zone]++;
	DWC3_ERR_INTR("USB connect hibernation done. \n");

	bsp_usb_unlock_wakelock();

}


void dwc3_gadget_hibernation_disconnect(struct dwc3 * dwc)
{
	u32		timeout = 500;
	u32		suspend_start = 0;
	u32		reg;
	u32		event_buf_num = 0;
	u32		u2_status = 0;
	u32		u3_status = 0;
	u32		d3_enter = 0;
	u32		u2_d3_time = 0;
	u32		u3_d3_time = 0;
	struct dwc3_event_buffer	*evt;
	enum dwc3_lpm_time suspend_time_zone;

#if (FEATURE_ON == MBB_USB)
    if (NULL == dwc)
    {
        return;
    }
#endif
	
	if(!bsp_usb_is_support_hibernation()){
		return;
	}

	suspend_start = bsp_get_slice_value_hrt();

	/* We can't rely on the link state machine to decide
	whether to do the suspend. As link state change intr
	would modify the dwc->link_state if enabled */
	if (dwc->gadget.is_suspend) {
		dwc->stat.suspend_reentry_fail++;
		DWC3_ERR_INTR("USB gadget already suspended\n");
		return;
	}

	dwc->stat.suspend_entry ++;

	/*call disconnect callback.*/
	if (dwc->gadget_driver->disconnect) {
		dwc->gadget_driver->disconnect(&dwc->gadget);
	}

	/* step 1 stop all transfers including ep0*/
	dwc3_gadget_hibernation_stop_transfers(dwc);

	dwc->stat.suspend_while_not_connected ++;
	//step 2
	//pull dsts usblnkst to 4 as step2 of device initiated disconnect said.
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);
		if (DWC3_LINK_STATE_SS_DIS == DWC3_DSTS_USBLNKST(reg)) {
			break;
		}

		timeout--;
		if (!timeout){
			dwc->stat.suspend_wait_disconnect_fail ++;
			DWC3_ERR_INTR("USB DEVICE DISCONNECT LINK %x. \n",DWC3_DSTS_USBLNKST(reg));
			return;
		}
		udelay(10);
	} while (1);

	//step 3
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_RUN_STOP;
	reg &= ~DWC3_DCTL_KEEP_CONNECT;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	timeout = 500;
	do {
		for (event_buf_num = 0; event_buf_num < dwc->num_event_buffers; event_buf_num++) {
			evt = dwc->ev_buffs[event_buf_num];
			evt->lpos = 0;
			reg = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(event_buf_num));
			reg = reg&DWC3_GEVNTCOUNT_MASK;
	    	if (reg)
			dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(event_buf_num), reg);
		}

		reg = dwc3_readl(dwc->regs, DWC3_DSTS);
		if (reg & DWC3_DSTS_DEVCTRLHLT) {
			break;
		}

		timeout--;
		if (!timeout){
			dwc->stat.suspend_wait_halt_fail ++;
			DWC3_ERR_INTR("USB DEVICE HALT TIME OUT. \n");
			
			/*Disable usb interrupt and disable usb hibernation interrupt
			to stop hibernation interrupt*/
			reg = dwc3_readl(dwc->regs, DWC3_DEVTEN);
			reg &= ~DWC3_DEVTEN_HIBERNATIONREQEVTEN;
			dwc3_writel(dwc->regs, DWC3_DEVTEN, reg);
			
			dwc->intr_bh_event |= DWC3_EVENT_BH_SOFT_RESET;
			tasklet_schedule(&dwc->intr_bh_tasklet);/*call dwc3_gadget_intr_bh*/
			return;
		}
		udelay(10);
	} while (1);

	//step 4 turn sys to D3 and wait until all go to D3,turn off the regulator
	dwc3_gadget_set_pmu_to_d3();
	timeout = D3_MAX_TIME;
	d3_enter = bsp_get_slice_value_hrt();
	do {
		if(0x3 != u2_status){
			u2_status = dwc3_gadget_get_pmu2_dstatus();
			if(0x3 == u2_status){
				u2_d3_time = D3_MAX_TIME-timeout;
			}
		}

		if(0x3 != u3_status){
			u3_status = dwc3_gadget_get_pmu3_dstatus();
			if(0x3 == u3_status){
				u3_d3_time = D3_MAX_TIME-timeout;
			}
		}

		if(u2_status == u3_status && 0x3 == u3_status){
			if((D3_MAX_TIME - timeout) > D3_NORMAL_TIME){
				dwc3_gadget_hibernation_print_d3_time(d3_enter);
				DWC3_ERR_INTR("D3 TIME u2 %u u3 %u. \n", u2_d3_time, u3_d3_time);
			}
			break;
		}
		
		timeout--;
		if (!timeout){
			dwc->stat.suspend_switch_d3_fail ++;
			DWC3_ERR_INTR("USB SET PMU TO D3 TIMEOUT. \n");
			return;
		}
		udelay(1);
	} while (1);
	
	/*step 5*/
	/*assert vcc_reset_n because power controller because last step may not power down vcc*/
	dwc3_gadget_vcc_reset();

	dwc3_gadget_config_phyclk(dwc);
	
	dwc->usb_core_powerdown = true;
	dwc->gadget.is_suspend = 1;
	dwc->restore_flag = 0;
	dwc->stat.suspend_done ++;

	suspend_time_zone = dwc3_gadget_hibernation_get_time_cost(suspend_start);
	dwc->stat.suspend_not_conn_time[suspend_time_zone]++;
	
	DWC3_ERR_INTR("USB disconnect hibernation done. \n");
	bsp_usb_unlock_wakelock();

}


int dwc3_gadget_hibernation_resume(struct dwc3 * dwc)
{
	u32		reg;
	u32		speed;
	u32		timeout = 500;
	s32		state;
	s32		ret;
	u32		resume_start;

	enum dwc3_lpm_time resume_time_zone;

	resume_start = bsp_get_slice_value_hrt();

	if(0 == dwc->gadget.is_suspend)
	{
		dwc->stat.resume_reentry_fail++;
		return 0;
	}
	dwc3_enable_both_phy();

	bsp_usb_lock_wakelock();
	dwc->stat.resume_entry ++;

	/*step 1 de-assert vcc_reset_n because power controller dosen't auto reset as
	the document said, it needs a kick. */
	dwc3_gadget_vcc_unreset();

	udelay(100);

	//step 2
	dwc3_gadget_set_pmu_to_d0();
	do {
		ret = dwc3_gadget_get_pmu_dstatus(dwc);

		if(0 == ret){
			break;
		}

		udelay(1);
		timeout --;
		if(0 == timeout){
			dwc->stat.resume_switch_d0_fail ++;
			if(-1 == ret){
				DWC3_ERR("u2 u3 uneuqal. \n");
			}

			DWC3_ERR("dwc3_gadget_hibernation_resume switch usb to D0 timeout. \n");
			return -ETIMEDOUT;
		}
	}while(1);
	dwc->usb_core_powerdown = false;
	
	/*step 3 how to judge if GSBUSCFG0/1 is correct or not?
	It a fact that those register does not change before&after L2 suspend resume */


	//step 4,5,
	if(dwc->restore_flag){
		dwc->stat.resume_while_connected ++;
		ret = dwc3_gadget_hibernation_restore_state(dwc);
		if(ret){
			DWC3_ERR_INTR("Restore scratchpad fail.\n");
			return ret;
		}
	}else{
		dwc->stat.resume_while_not_connected ++;

		/*make the acture reset*/
		timeout = 500;
		dwc3_writel(dwc->regs, DWC3_DCTL, DWC3_DCTL_CSFTRST);
		do {
			reg = dwc3_readl(dwc->regs, DWC3_DCTL);
			if (!(reg & DWC3_DCTL_CSFTRST))
				break;

			udelay(2);
			timeout --;
			if(0 == timeout){
				dwc->stat.resume_unconn_reset_fail ++;
				DWC3_ERR_INTR("Reset timeout. \n");
				return -ETIMEDOUT;
			}
		} while (1);
	}

	//step 6 fix me: may need more work
	ret = dwc3_gadget_hibernation_reinit_ctrl(dwc);
	if(ret){
		dwc->stat.resume_reinit_fail ++;
		DWC3_ERR_INTR("Restore state fail.\n");
		return ret;
	}

	//step 7
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg |=DWC3_DCTL_RUN_STOP | DWC3_DCTL_KEEP_CONNECT;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	//step 8
	timeout = 500;
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);
		if (!(reg & DWC3_DSTS_DCNRD))
			break;

		udelay(1);
		timeout--;
		if (0 == timeout){
			dwc->stat.resume_devready_fail ++;
			DWC3_ERR_INTR("Device ready timeout. \n");
			return -ETIMEDOUT;
		}
	} while (1);

	state = dwc3_gadget_get_link_state(dwc);
	dwc->stat.resume_link_state[state]++;
	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	speed = reg & DWC3_DSTS_CONNECTSPD;

	/*
	 * If the core is not connected to the host, wait for a Connect Done
	 * event.
	 */

	if (DWC3_LINK_STATE_RESUME == state || DWC3_LINK_STATE_U3 == state 
		|| (DWC3_LINK_STATE_U2 == state && USB_SPEED_HIGH == dwc->gadget.speed )){

		/*Is 3,15 to step 8 a(3),b(15)*/
		//step 10 erform the steps in Section 9.1.3 "Initialization on Connect Done" in databook.
		dwc3_gadget_conndone_interrupt(dwc);

		//step 11
		dwc3_gadget_hibernation_exit_after_connect(dwc);

		//step 12
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;
			
		reg |= DWC3_DCTL_ULSTCHNG_RECOVERY;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
		
	}else if (state == DWC3_LINK_STATE_RESET) {

		/*
		 * If the DSTS.USBLnkSt equals 14, it means a USB reset was received while the core
		 * was entering or exiting hibernation.Prior to performing the steps in sections 9.1.2 and 9.1.3,
		 * software must write Resume (8) into the DCTL.ULStChngReq field. */

		ret = dwc3_gadget_set_link_state(dwc, DWC3_LINK_STATE_RECOV);
		if(ret){
			dwc->stat.resume_set_link_state_fail++;
		}
		reg = dwc3_readl(dwc->regs, DWC3_DCFG);
		reg &= ~(DWC3_DCFG_DEVADDR_MASK);
		reg |= DWC3_DCFG_DEVADDR(0);
		dwc3_writel(dwc->regs, DWC3_DCFG, reg);
	}

	//step 13 does not acturely do anything

	dwc->gadget.is_suspend = 0;


	resume_time_zone = dwc3_gadget_hibernation_get_time_cost(resume_start);

	if(dwc->restore_flag){
		dwc->stat.resume_conn_time[resume_time_zone]++;
	}else{
		dwc->stat.resume_not_conn_time[resume_time_zone]++;
	}

	/*if backup flag is 0, then there will be disconnect process,
		func layer should goto disconnet process*/
	if(dwc->restore_flag){
		if (dwc->gadget_driver->resume) {
			dwc->gadget_driver->resume(&dwc->gadget);
		}
	}
	/*link state and usb speed*/
	DWC3_ERR_INTR("LS %d,SP %u \n", state, speed);

	dwc->stat.resume_done++;
	return 0;
}


#endif


int dwc3_gadget_suspend(struct dwc3 *dwc)
{
	__dwc3_gadget_ep_disable(dwc->eps[0]);
	__dwc3_gadget_ep_disable(dwc->eps[1]);

	dwc->dcfg = dwc3_readl(dwc->regs, DWC3_DCFG);

	return 0;
}

int dwc3_gadget_resume(struct dwc3 *dwc)
{
	struct dwc3_ep		*dep;
	int			ret;

	/* Start with SuperSpeed Default */
	dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret)
		goto err0;

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false);
	if (ret)
		goto err1;

	/* begin to receive SETUP packets */
	dwc->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dwc);

	dwc3_writel(dwc->regs, DWC3_DCFG, dwc->dcfg);

	return 0;

err1:
	__dwc3_gadget_ep_disable(dwc->eps[0]);

err0:
	return ret;
}
/*lint -restore*/
