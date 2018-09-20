/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <product_config.h>
#include <linux/notifier.h>
#include <linux/kdebug.h>
#include <asm/traps.h>
#include <asm/thread_notify.h>
#include "drv_nv_id.h"
#include "drv_nv_def.h"
#include "bsp_om_enum.h"
#include "bsp_trace.h"
#include "bsp_nvim.h"
#include "bsp_adump.h"
#include "adump_baseinfo.h"
#include "adump_core.h"
#include "adump_exception.h"
#include "adump_debug.h"



struct adump_exception_ctrl_s g_adump_exception_ctrl;

 /*****************************************************************************
* 函 数 名  : adump_set_exc_vec
* 功能描述  : 设置复位的异常向量
*             该函数在内核中使用
*
* 输入参数  : vec 异常向量
*
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void adump_set_exc_vec(u32 vec)
{
    g_adump_exception_ctrl.exc_vec = vec;
}


 /*****************************************************************************
* 函 数 名  : adump_exc_hook
* 功能描述  : 系统异常钩子函数
*
* 输入参数  : currentTaskId  当前任务id
*             vec 复位向量
*             pReg 寄存器
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
static void adump_exc_hook(void * currentTaskId, s32 vec, u32* pReg)
{
    adump_fill_exc_vec((u32)vec);
    ap_system_error(RDR_AP_DUMP_ARM_MOD_ID_START+g_adump_exception_ctrl.exc_vec, AP_DUMP_REASON_ARM, 0, 0, 0);
    return;
}

extern void print_modules(void);
extern void show_regs(struct pt_regs * regs);
/* dump die notifier callback function */
int adump_die_callback(struct notifier_block *nb, unsigned long action, void *data)
{
    struct die_args* pargs = (struct die_args*)data;

    adump_err("str=%s, err=%ld, trapnr=%d, signr=%d\n",
        pargs->str, pargs->err, pargs->trapnr, pargs->signr);
    memcpy(&g_adump_exception_ctrl.pt_regs, pargs->regs, sizeof(struct pt_regs));

    if ((!in_interrupt())&&(!panic_on_oops))
    {
    	print_modules();
    	show_regs(pargs->regs);
        adump_exc_hook(current, g_adump_exception_ctrl.exc_vec, (u32*)&g_adump_exception_ctrl.pt_regs);
        return NOTIFY_STOP;
    }

    return NOTIFY_OK;
}

/* dump panic notifier callback function */
int adump_panic_callback(struct notifier_block *nb, unsigned long action, void *data)
{
    adump_err(" %s\n", (char*)data);

    adump_set_panic_flag();

    adump_exc_hook(current, g_adump_exception_ctrl.exc_vec, (u32*)&g_adump_exception_ctrl.pt_regs);
    return NOTIFY_OK;
}

/* dump die notifier */
static struct notifier_block adump_die_notifier =
{
    .notifier_call = adump_die_callback,
    .priority      = 0,
};

/* dump panic notifier */
static struct notifier_block adump_panic_notifier =
{
    .notifier_call = adump_panic_callback,
    .priority      = 0,
};


/* dump register notifier into kernel */
static void adump_register_exc_notifier(void)
{
    s32 ret = ADUMP_OK;
    NV_DUMP_STRU  dump_cfg = {{0}};

    ret = bsp_nvm_read(NV_ID_DRV_DUMP, (u8*)&dump_cfg, sizeof(NV_DUMP_STRU));
    if(ret != ADUMP_OK)
    {
        return;
    }
    /* register exception notifier */
    if(1 == dump_cfg.dump_cfg.Bits.ARMexc)
    {
        register_die_notifier(&adump_die_notifier);
        atomic_notifier_chain_register(&panic_notifier_list,&adump_panic_notifier);
    }
}


s32 adump_exception_init(void)
{
    adump_register_exc_notifier();

    adump_err("ok!\n");
    return 0;
}

