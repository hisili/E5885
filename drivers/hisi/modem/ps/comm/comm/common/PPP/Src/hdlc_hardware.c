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


/******************************************************************************
   头文件包含
******************************************************************************/
#include "product_config.h"
#include "TTFComm.h"
#include "soc_sctrl_interface.h"

#if ((VOS_RTOSCK == VOS_OS_VER) || (VOS_WIN32 == VOS_OS_VER))
#include "pppc_ctrl.h"
#endif

#include "mdrv.h"
#include "hdlc_hardware.h"

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

/*****************************************************************************
   1 协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_HDLC_HARDWARE_C


/******************************************************************************
   2 外部函数变量声明
******************************************************************************/
#if (VOS_OS_VER == VOS_WIN32)
extern VOS_UINT8   g_ucScCtrlRegAddr[0xFFC];
#endif


/*****************************************************************************
   3 私有定义
*****************************************************************************/


/*****************************************************************************
   4 全局变量定义
*****************************************************************************/
/* 解封装输出的非完整帧信息 */
HDLC_DEF_UNCOMPLETED_INFO_STRU  g_stUncompletedInfo = {0};

/* 保存解封装使用的内存 */
HDLC_DEF_BUFF_INFO_STRU        *g_pstHdlcDefBufInfo = VOS_NULL_PTR;

/* 保存封装使用的内存 */
HDLC_FRM_BUFF_INFO_STRU        *g_pstHdlcFrmBufInfo = VOS_NULL_PTR;

/* HDLC配置相关信息 */
HDLC_CONFIG_INFO_STRU           g_stHdlcConfigInfo  =
{
    0,
    0,
    0,
    0,
    0,
    HDLC_DEF_INTERRUPT_LIMIT_DEFAULT,
    HDLC_FRM_INTERRUPT_LIMIT_DEFAULT,
    VOS_NULL_PTR,
    0
};

/* 统计信息 */
PPP_HDLC_HARD_DATA_PROC_STAT_ST g_PppHdlcHardStat   = {0};

/* 保留清原始中断时的RAW_INT和STATUS值 */
HDLC_REG_SAVE_INFO_STRU         g_stHdlcRegSaveInfo;

/* 系统控制器基地址 */
VOS_UINT_PTR                    g_ulHdlcScCtrlBaseAddr  = 0;

/******************************************************************************
   5 函数实现
******************************************************************************/



VOS_VOID PPP_HDLC_HARD_PeriphClkOpen(VOS_VOID)
{
    VOS_UINT32      ulValue = 0;

    ulValue |= (1 << HDLC_CRG_CLK_BITPOS);
    PPP_HDLC_WRITE_32REG(HDLC_CRG_CLKEN4_ADDR(g_ulHdlcScCtrlBaseAddr), ulValue);

    return;
}


VOS_VOID PPP_HDLC_HARD_PeriphClkClose(VOS_VOID)
{
    VOS_UINT32      ulValue = 0;

    ulValue |= (1 << HDLC_CRG_CLK_BITPOS);
    PPP_HDLC_WRITE_32REG(HDLC_CRG_CLKENDIS4_ADDR(g_ulHdlcScCtrlBaseAddr), ulValue);

    return;
}


VOS_UINT32 PPP_HDLC_HARD_MntnSetConfig(VOS_UINT32 ulConfig)
{
    g_stHdlcConfigInfo.ulHdlcMntnTraceCfg = ulConfig;

    return g_stHdlcConfigInfo.ulHdlcMntnTraceCfg;
}


VOS_UINT32 PPP_HDLC_HARD_MntnSetDefIntLimit(VOS_UINT32 ulIntLimit)
{
    g_stHdlcConfigInfo.ulHdlcDefIntLimit = ulIntLimit;

    return g_stHdlcConfigInfo.ulHdlcDefIntLimit;
}


VOS_UINT32 PPP_HDLC_HARD_MntnSetFrmIntLimit(VOS_UINT32 ulIntLimit)
{
    g_stHdlcConfigInfo.ulHdlcFrmIntLimit = ulIntLimit;

    return g_stHdlcConfigInfo.ulHdlcFrmIntLimit;
}


VOS_VOID PPP_HDLC_HARD_MntnTraceMsg
(
    HDLC_MNTN_TRACE_HEAD_STRU          *pstHead,
    HDLC_MNTN_EVENT_TYPE_ENUM_UINT32    ulMsgname,
    VOS_UINT32                          ulDataLen
)
{
    pstHead->ulReceiverCpuId = VOS_LOCAL_CPUID;
    pstHead->ulReceiverPid   = PS_PID_PPP_HDLC;
    pstHead->ulSenderCpuId   = VOS_LOCAL_CPUID;
    pstHead->ulSenderPid     = PS_PID_PPP_HDLC;
    pstHead->ulLength        = ulDataLen - VOS_MSG_HEAD_LENGTH;

    pstHead->ulMsgname       = ulMsgname;

    PPP_MNTN_TRACE_MSG(pstHead);

    return;
}


VOS_VOID PPP_HDLC_HARD_MntnDefTraceRegConfig
(
    VOS_UINT32      ulEnable,
    VOS_UINT32      ulValue,
    VOS_UINT32      ulEnableInterrupt
)
{
    HDLC_MNTN_DEF_REG_CONFIG_STRU           stRegConfig;
    HDLC_MNTN_DEF_REG_CONFIG_STRU          *pstRegConfig = &stRegConfig;
    VOS_UINT32                              ulDataLen;

    if ((g_stHdlcConfigInfo.ulHdlcMntnTraceCfg & PPP_HDLC_MNTN_TRACE_REG) != 0)
    {
        ulDataLen    = sizeof(HDLC_MNTN_DEF_REG_CONFIG_STRU);

        /* 拷贝全部寄存器内容 */
        pstRegConfig->ulStateSwRst             = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_STATE_SW_RST_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulPriorTimeoutCtrl       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_PRIROR_TIMEOUT_CTRL_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulRdErrCurrAddr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_RD_ERR_CURR_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulWrErrCurrAddr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_WR_ERR_CURR_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefEn              = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_EN_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefRawInt          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_RAW_INT_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefIntStatus       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_INT_STATUS_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefIntClr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_INT_CLR_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefCfg             = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_CFG_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefUncompletedLen      = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_LEN_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefUncompletedPro      = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_PRO_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefUncompletedAddr     = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefUncompleteStAgo     = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_ST_AGO_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefGoOn            = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_GO_ON_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefStatus          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_STATUS_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefUncompletStNow      = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_ST_NOW_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefInLliAddr           = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_IN_LLI_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefInPktAddr           = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_IN_PKT_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefInPktLen            = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_IN_PKT_LEN_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefInPktLenMax         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_IN_PKT_LEN_MAX_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefOutSpcAddr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_OUT_SPC_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefOutSpaceDep         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_OUT_SPACE_DEP_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefRptAddr             = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_RPT_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefRptDep              = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_RPT_DEP_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefErrInfor0       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_ERR_INFO_0_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefErrInfor1       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_ERR_INFO_1_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefErrInfor2       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_ERR_INFO_2_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcDefErrInfor3       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_ERR_INFO_3_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefInfoFr1CntAgo       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_INFO_FRL_CNT_AGO_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulDefInfoFr1CntNow       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_INFO_FRL_CNT_NOW_ADDR(HDLC_IP_BASE_ADDR));

        /* 使能前勾包，使能寄存器还没有配置，因为配置之后HDLC会开始工作，会改变其他寄存器的值 */
        if( VOS_FALSE == ulEnable)
        {
            pstRegConfig->ulHdlcDefEn   = ulValue;
            PPP_HDLC_HARD_MntnTraceMsg((HDLC_MNTN_TRACE_HEAD_STRU *)pstRegConfig,
                                       ID_HDLC_MNTN_DEF_REG_BEFORE_EN, ulDataLen);
        }
        else
        {
            /* 使能后勾包时，如果采用中断方式，则RawInt和Status取g_stHdlcRegSaveInfo保存的值 */
            if( VOS_TRUE == ulEnableInterrupt )
            {
                pstRegConfig->ulHdlcDefRawInt   = g_stHdlcRegSaveInfo.ulHdlcDefRawInt;
                pstRegConfig->ulHdlcDefStatus   = g_stHdlcRegSaveInfo.ulHdlcDefStatus;
            }
            PPP_HDLC_HARD_MntnTraceMsg((HDLC_MNTN_TRACE_HEAD_STRU *)pstRegConfig,
                                       ID_HDLC_MNTN_DEF_REG_AFTER_EN, ulDataLen);
        }
    }

    return;
}


VOS_VOID PPP_HDLC_HARD_DefCfgGoOnReg
(
    VOS_UINT32          ulDefStatus
)
{
    /*
    hdlc_def_go_on  (0x84)
     31                  17  16  15    9   8  7   1  0
    |----------------------|----|-------|----|-----|----|
    |         Rsv          |Goon|  Rsv  |Goon| Rsv |Goon|
    Reserved             [31:17] 15'b0   h/s R/W  保留位。读时返回0。写时无影响。
    def_rpt_ful_goon     [16]    1'b0    h/s WO   外部解封装有效帧信息上报空间存满暂停解除
    Reserved             [15:9]  7'b0    h/s R/W  保留位。读时返回0。写时无影响。
    def_outspc_ful_goon  [8]     1'b0    h/s WO   外部解封装输出数据存储空间存满暂停状态清除
    Reserved             [7:1]   7'b0    h/s R/W  保留位。读时返回0。写时无影响。
    def_lcp_goon         [0]     1'b0    h/s WO   解出一个合法LCP帧导致的硬件暂停状态清除。当解封装模块未处理完一组待解封装的数据（<=2KB(def_in_pkt_len_max)），解出一个合法LCP帧，则会暂停解帧，等待此软件向此寄存器写"1"，再继续处理剩余的数据。
    */

    /* GO_ON前清除上次解封装的原始中断 */
    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_DEF_INT_CLR_ADDR(HDLC_IP_BASE_ADDR), 0xFFFFFFFFU);

    if (HDLC_DEF_STATUS_PAUSE_RPT_SPACE_FULL == ulDefStatus )
    {
        PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_DEF_GO_ON_ADDR(HDLC_IP_BASE_ADDR),
                        (VOS_UINT32)0x10000);
    }
    else if (HDLC_DEF_STATUS_PAUSE_OUTPUT_SPACE_FULL == ulDefStatus )
    {
        PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_DEF_GO_ON_ADDR(HDLC_IP_BASE_ADDR),
                        (VOS_UINT32)0x100);
    }
    else if (HDLC_DEF_STATUS_PAUSE_LCP == ulDefStatus )
    {
        PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_DEF_GO_ON_ADDR(HDLC_IP_BASE_ADDR),
                        (VOS_UINT32)0x1);
    }
    else
    {
        PPP_HDLC_ERROR_LOG1("PPP_HDLC_HARD_DefCfgGoOnReg, ERROR, Wrong ulDefStatus %d!\r\n", ulDefStatus);
    }

    return;
}


VOS_VOID PPP_HDLC_HARD_MntnDefTraceUncompleteInfo
(
    HDLC_DEF_UNCOMPLETED_INFO_STRU     *pstUncompletedInfo
)
{
    HDLC_MNTN_DEF_UNCOMPLETED_INFO_STRU stMntnUncompletedInfo;


    VOS_MemCpy_s(&stMntnUncompletedInfo.stUncompletedInfo, sizeof(HDLC_DEF_UNCOMPLETED_INFO_STRU),
               pstUncompletedInfo, sizeof(HDLC_DEF_UNCOMPLETED_INFO_STRU));

    PPP_HDLC_HARD_MntnTraceMsg((HDLC_MNTN_TRACE_HEAD_STRU *)&stMntnUncompletedInfo,
                               ID_HDLC_MNTN_DEF_UNCOMPLETED_INFO,
                               sizeof(HDLC_MNTN_DEF_UNCOMPLETED_INFO_STRU));
    return;
}


VOS_VOID PPP_HDLC_HARD_MntnShowFrmReg(VOS_VOID)
{
    HDLC_MNTN_FRM_REG_CONFIG_STRU           stRegConfig;
    HDLC_MNTN_FRM_REG_CONFIG_STRU          *pstRegConfig = &stRegConfig;


    /* 拷贝全部寄存器内容 */
    pstRegConfig->ulStateSwRst          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_STATE_SW_RST_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulPriorTimeoutCtrl    = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_PRIROR_TIMEOUT_CTRL_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulRdErrCurrAddr       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_RD_ERR_CURR_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulWrErrCurrAddr       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_WR_ERR_CURR_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcFrmEn           = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_EN_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcFrmRawInt       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_RAW_INT_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcFrmIntStatus    = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_INT_STATUS_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcFrmIntClr       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_INT_CLR_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcFrmCfg          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_CFG_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcFrmAccm         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_ACCM_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcFrmStatus       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_STATUS_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmInLliAddr        = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_LLI_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmInSublliAddr     = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_SUBLLI_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmInPktLen         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_PKT_LEN_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmInBlkAddr        = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_BLK_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmInBlkLen         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_BLK_LEN_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmOutLliAddr       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_OUT_LLI_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmOutSpaceAddr     = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_OUT_SPACE_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmOutSpaceDep      = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_OUT_SPACE_DEP_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmRptAddr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_RPT_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulFrmRptDep           = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_RPT_DEP_ADDR(HDLC_IP_BASE_ADDR));

    vos_printf("\n================HDLC Hardware ShowFrmReg Begin==========================\n");

    vos_printf("ulStateSwRst                    = 0x%x\n", pstRegConfig->ulStateSwRst);
    vos_printf("ulPriorTimeoutCtrl              = 0x%x\n", pstRegConfig->ulPriorTimeoutCtrl);
    vos_printf("ulRdErrCurrAddr                 = 0x%x\n", pstRegConfig->ulRdErrCurrAddr);
    vos_printf("ulWrErrCurrAddr                 = 0x%x\n", pstRegConfig->ulWrErrCurrAddr);
    vos_printf("ulHdlcFrmEn                     = 0x%x\n", pstRegConfig->ulHdlcFrmEn);
    vos_printf("ulHdlcFrmRawInt                 = 0x%x\n", pstRegConfig->ulHdlcFrmRawInt);
    vos_printf("ulHdlcFrmIntStatus              = 0x%x\n", pstRegConfig->ulHdlcFrmIntStatus);
    vos_printf("ulHdlcFrmIntClr                 = 0x%x\n", pstRegConfig->ulHdlcFrmIntClr);
    vos_printf("ulHdlcFrmCfg                    = 0x%x\n", pstRegConfig->ulHdlcFrmCfg);
    vos_printf("ulHdlcFrmAccm                   = 0x%x\n", pstRegConfig->ulHdlcFrmAccm);
    vos_printf("ulHdlcFrmStatus                 = 0x%x\n", pstRegConfig->ulHdlcFrmStatus);
    vos_printf("ulFrmInLliAddr                  = 0x%x\n", pstRegConfig->ulFrmInLliAddr);
    vos_printf("ulFrmInSublliAddr               = 0x%x\n", pstRegConfig->ulFrmInSublliAddr);
    vos_printf("ulFrmInPktLen                   = 0x%x\n", pstRegConfig->ulFrmInPktLen);
    vos_printf("ulFrmInBlkAddr                  = 0x%x\n", pstRegConfig->ulFrmInBlkAddr);
    vos_printf("ulFrmInBlkLen                   = 0x%x\n", pstRegConfig->ulFrmInBlkLen);
    vos_printf("ulFrmOutLliAddr                 = 0x%x\n", pstRegConfig->ulFrmOutLliAddr);
    vos_printf("ulFrmOutSpaceAddr               = 0x%x\n", pstRegConfig->ulFrmOutSpaceAddr);
    vos_printf("ulFrmOutSpaceDep                = 0x%x\n", pstRegConfig->ulFrmOutSpaceDep);
    vos_printf("ulFrmRptAddr                    = 0x%x\n", pstRegConfig->ulFrmRptAddr);
    vos_printf("ulFrmRptDep                     = 0x%x\n", pstRegConfig->ulFrmRptDep);

    vos_printf("\n================HDLC Hardware ShowFrmReg End==========================\n");

    return;
}


VOS_VOID PPP_HDLC_HARD_MntnShowDefReg(VOS_VOID)
{
    HDLC_MNTN_DEF_REG_CONFIG_STRU           stRegConfig;
    HDLC_MNTN_DEF_REG_CONFIG_STRU          *pstRegConfig = &stRegConfig;


    /* 拷贝全部寄存器内容 */
    pstRegConfig->ulStateSwRst             = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_STATE_SW_RST_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulPriorTimeoutCtrl       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_PRIROR_TIMEOUT_CTRL_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulRdErrCurrAddr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_RD_ERR_CURR_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulWrErrCurrAddr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_WR_ERR_CURR_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefEn              = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_EN_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefRawInt          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_RAW_INT_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefIntStatus       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_INT_STATUS_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefIntClr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_INT_CLR_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefCfg             = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_CFG_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefUncompletedLen      = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_LEN_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefUncompletedPro      = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_PRO_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefUncompletedAddr     = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefUncompleteStAgo     = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_ST_AGO_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefGoOn            = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_GO_ON_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefStatus          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_STATUS_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefUncompletStNow      = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_UNCOMPLETED_ST_NOW_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefInLliAddr           = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_IN_LLI_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefInPktAddr           = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_IN_PKT_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefInPktLen            = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_IN_PKT_LEN_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefInPktLenMax         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_IN_PKT_LEN_MAX_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefOutSpcAddr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_OUT_SPC_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefOutSpaceDep         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_OUT_SPACE_DEP_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefRptAddr             = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_RPT_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefRptDep              = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_RPT_DEP_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefErrInfor0       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_ERR_INFO_0_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefErrInfor1       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_ERR_INFO_1_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefErrInfor2       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_ERR_INFO_2_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulHdlcDefErrInfor3       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_ERR_INFO_3_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefInfoFr1CntAgo       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_INFO_FRL_CNT_AGO_ADDR(HDLC_IP_BASE_ADDR));
    pstRegConfig->ulDefInfoFr1CntNow       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_INFO_FRL_CNT_NOW_ADDR(HDLC_IP_BASE_ADDR));

    vos_printf("\n================HDLC Hardware ShowDefReg Begin==========================\n");

    vos_printf("ulStateSwRst             = 0x%x\n", pstRegConfig->ulStateSwRst);
    vos_printf("ulPriorTimeoutCtrl       = 0x%x\n", pstRegConfig->ulPriorTimeoutCtrl);
    vos_printf("ulRdErrCurrAddr          = 0x%x\n", pstRegConfig->ulRdErrCurrAddr);
    vos_printf("ulWrErrCurrAddr          = 0x%x\n", pstRegConfig->ulWrErrCurrAddr);
    vos_printf("ulHdlcDefEn              = 0x%x\n", pstRegConfig->ulHdlcDefEn);
    vos_printf("ulHdlcDefRawInt          = 0x%x\n", pstRegConfig->ulHdlcDefRawInt);
    vos_printf("ulHdlcDefIntStatus       = 0x%x\n", pstRegConfig->ulHdlcDefIntStatus);
    vos_printf("ulHdlcDefIntClr          = 0x%x\n", pstRegConfig->ulHdlcDefIntClr);
    vos_printf("ulHdlcDefCfg             = 0x%x\n", pstRegConfig->ulHdlcDefCfg);
    vos_printf("ulDefUncompletedLen      = 0x%x\n", pstRegConfig->ulDefUncompletedLen);
    vos_printf("ulDefUncompletedPro      = 0x%x\n", pstRegConfig->ulDefUncompletedPro);
    vos_printf("ulDefUncompletedAddr     = 0x%x\n", pstRegConfig->ulDefUncompletedAddr);
    vos_printf("ulDefUncompleteStAgo     = 0x%x\n", pstRegConfig->ulDefUncompleteStAgo);
    vos_printf("ulHdlcDefGoOn            = 0x%x\n", pstRegConfig->ulHdlcDefGoOn);
    vos_printf("ulHdlcDefStatus          = 0x%x\n", pstRegConfig->ulHdlcDefStatus);
    vos_printf("ulDefUncompletStNow      = 0x%x\n", pstRegConfig->ulDefUncompletStNow);
    vos_printf("ulDefInLliAddr           = 0x%x\n", pstRegConfig->ulDefInLliAddr);
    vos_printf("ulDefInPktAddr           = 0x%x\n", pstRegConfig->ulDefInPktAddr);
    vos_printf("ulDefInPktLen            = 0x%x\n", pstRegConfig->ulDefInPktLen);
    vos_printf("ulDefInPktLenMax         = 0x%x\n", pstRegConfig->ulDefInPktLenMax);
    vos_printf("ulDefOutSpcAddr          = 0x%x\n", pstRegConfig->ulDefOutSpcAddr);
    vos_printf("ulDefOutSpaceDep         = 0x%x\n", pstRegConfig->ulDefOutSpaceDep);
    vos_printf("ulDefRptAddr             = 0x%x\n", pstRegConfig->ulDefRptAddr);
    vos_printf("ulDefRptDep              = 0x%x\n", pstRegConfig->ulDefRptDep);
    vos_printf("ulHdlcDefErrInfor0       = 0x%x\n", pstRegConfig->ulHdlcDefErrInfor0);
    vos_printf("ulHdlcDefErrInfor1       = 0x%x\n", pstRegConfig->ulHdlcDefErrInfor1);
    vos_printf("ulHdlcDefErrInfor2       = 0x%x\n", pstRegConfig->ulHdlcDefErrInfor2);
    vos_printf("ulHdlcDefErrInfor3       = 0x%x\n", pstRegConfig->ulHdlcDefErrInfor3);
    vos_printf("ulDefInfoFr1CntAgo       = 0x%x\n", pstRegConfig->ulDefInfoFr1CntAgo);
    vos_printf("ulDefInfoFr1CntNow       = 0x%x\n", pstRegConfig->ulDefInfoFr1CntNow);

    vos_printf("\n================HDLC Hardware ShowDefReg End==========================\n");

}


VOS_VOID PPP_HDLC_HARD_FrmProcException
(
    VOS_UINT32          ulStatus,
    VOS_UINT32          ulEnableInterrupt
)
{
    VOS_UINT32                          ulRawInt;


    if( VOS_TRUE == ulEnableInterrupt )
    {
        /* 由于在中断服务程序中进行了清中断操作，故此处取保存在g_stHdlcRegSaveInfo中的原始中断寄存器值 */
        ulRawInt                        =   g_stHdlcRegSaveInfo.ulHdlcFrmRawInt;
        g_PppHdlcHardStat.usFrmExpInfo |=   (1 << HDLC_INTERRUPT_IND_BITPOS);
    }
    else
    {
        ulRawInt  =   PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_RAW_INT_ADDR(HDLC_IP_BASE_ADDR));
    }

    PPP_HDLC_ERROR_LOG2("PPP_HDLC_HARD_FrmProcException, ERROR, Exception ocurr status 0x%x RAW_INT 0x%x\r\n", ulStatus, ulRawInt);
    PPP_HDLC_HARD_MntnShowFrmReg();
    PPP_HDLC_HARD_MntnShowDefReg();

    g_PppHdlcHardStat.usFrmExpInfo |=   (1 << HDLC_EXCEPTION_IND_BITPOS);

    /* 复位前先Delay 1s保证可维可测正常输出 */
    VOS_TaskDelay(1000);

    /* 如果HDLC出现异常，则单板异常重启 */
    mdrv_om_system_error(HDLC_FRM_SYSTEM_ERROR_ID, __LINE__, (VOS_INT)ulStatus,
                         (VOS_CHAR *)&g_stHdlcRegSaveInfo,
                         sizeof(HDLC_REG_SAVE_INFO_STRU));
    (VOS_VOID)ulRawInt;

    return;
}


VOS_VOID PPP_HDLC_HARD_MntnFrmTraceRegConfig
(
    VOS_UINT32      ulEnable,
    VOS_UINT32      ulValue,
    VOS_UINT32      ulEnableInterrupt
)
{
    HDLC_MNTN_FRM_REG_CONFIG_STRU           stRegConfig;
    HDLC_MNTN_FRM_REG_CONFIG_STRU          *pstRegConfig = &stRegConfig;
    VOS_UINT32                              ulDataLen;

    if ((g_stHdlcConfigInfo.ulHdlcMntnTraceCfg & PPP_HDLC_MNTN_TRACE_REG) != 0)
    {
        ulDataLen    = sizeof(HDLC_MNTN_FRM_REG_CONFIG_STRU);

        /* 拷贝全部寄存器内容 */
        pstRegConfig->ulStateSwRst          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_STATE_SW_RST_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulPriorTimeoutCtrl    = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_PRIROR_TIMEOUT_CTRL_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulRdErrCurrAddr       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_RD_ERR_CURR_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulWrErrCurrAddr       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_WR_ERR_CURR_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcFrmEn           = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_EN_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcFrmRawInt       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_RAW_INT_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcFrmIntStatus    = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_INT_STATUS_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcFrmIntClr       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_INT_CLR_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcFrmCfg          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_CFG_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcFrmAccm         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_ACCM_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulHdlcFrmStatus       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_STATUS_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmInLliAddr        = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_LLI_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmInSublliAddr     = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_SUBLLI_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmInPktLen         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_PKT_LEN_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmInBlkAddr        = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_BLK_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmInBlkLen         = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_IN_BLK_LEN_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmOutLliAddr       = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_OUT_LLI_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmOutSpaceAddr     = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_OUT_SPACE_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmOutSpaceDep      = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_OUT_SPACE_DEP_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmRptAddr          = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_RPT_ADDR(HDLC_IP_BASE_ADDR));
        pstRegConfig->ulFrmRptDep           = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_RPT_DEP_ADDR(HDLC_IP_BASE_ADDR));

        /* 使能前勾包，使能寄存器还没有配置，因为配置之后HDLC会开始工作，会改变其他寄存器的值 */
        if( VOS_FALSE == ulEnable )
        {
            pstRegConfig->ulHdlcFrmEn    = ulValue;
            PPP_HDLC_HARD_MntnTraceMsg((HDLC_MNTN_TRACE_HEAD_STRU *)pstRegConfig,
                                       ID_HDLC_MNTN_FRM_REG_BEFORE_EN, ulDataLen);
        }
        else
        {
            /* 使能后勾包时，如果采用中断方式，则RawInt和Status取g_stHdlcRegSaveInfo保存的值 */
            if( VOS_TRUE == ulEnableInterrupt )
            {
                pstRegConfig->ulHdlcFrmRawInt   = g_stHdlcRegSaveInfo.ulHdlcFrmRawInt;
                pstRegConfig->ulHdlcFrmStatus   = g_stHdlcRegSaveInfo.ulHdlcFrmStatus;
            }
            PPP_HDLC_HARD_MntnTraceMsg((HDLC_MNTN_TRACE_HEAD_STRU *)pstRegConfig,
                                       ID_HDLC_MNTN_FRM_REG_AFTER_EN, ulDataLen);
        }
    }

    return;
}


VOS_UINT32 PPP_HDLC_HARD_FrmCfgEnReg(VOS_UINT32   ulTotalLen)
{
    /*
    1.hdlc_frm_en   (0x10)
      31   25 24  23 18 17  16  15  14  13  12  11  10   9   8  7    1  0
    |--------|---|-----|---|---|---|---|---|---|---|---|---|---|------|---|
    |   Rsv  |en | Rsv |en |en |en |en |en |en |en |en |en |en |  Rsv |en |

    Reserved            [31:25] 7'b0    h/s R/W  保留位。读时返回0。写时无影响。
    frm_over_int_en     [24]    1'b0    h/s R/W  一套链表封装结束中断使能;0：中断禁止;1：中断使能;
    Reserved            [23:18] 6'b0    h/s R/W  保留位。读时返回0。写时无影响。
    frm_rpt_dep_err_en  [17]    1'b0    h/s R/W  封装外部正确帧长度上报空间不足中断使能;0：中断禁止;1：中断使能;
    frm_out_spc_err_en  [16]    1'b0    h/s R/W  封装外部输出存储空间不足中断使能;0：中断禁止;1：中断使能
    frm_rpt_prm_err_en  [15]    1'b0    h/s R/W  封装上报空间相关参数错误中断使能;0：中断禁止;1：中断使能
    frm_out_prm_err_en  [14]    1'b0    h/s R/W  封装输出链表相关参数错误中断使能;0：中断禁止;1：中断使能
    frm_in_prm_err_en   [13]    1'b0    h/s R/W  封装输入链表相关参数错误中断使能;0：中断禁止;1：中断使能
    frm_cfg_err_en      [12]    1'b0    h/s R/W  封装协议及其压缩指示配置错误中断使能;0：中断禁止;1：中断使能
    frm_wr_timeout_en   [11]    1'b0    h/s R/W  封装时AXI总线写请求timeout中断使能;0：中断禁止;1：中断使能
    frm_rd_timeout_en   [10]    1'b0    h/s R/W  封装时AXI总线读请求timeout中断使能;0：中断禁止;1：中断使能
    frm_wr_err_en       [9]     1'b0    h/s R/W  封装时AXI总线写操作错误中断使能;0：中断禁止;1：中断使能
    frm_rd_err_en       [8]     1'b0    h/s R/W  封装时AXI总线读操作错误中断使能;0：中断禁止;1：中断使能
    Reserved            [7:1]   7'b0    h/s R/W  保留位。读时返回0。写时无影响。
    frm_en              [0]     1'b0    h/s R/W  一套链表封装使能，软件向frm_en写入1'b1启动封装工作;一套链表封装完成后，由硬件自动对frm_en清零；
                                                 封装过程出错时，硬件也会对frm_en自动清零，使内部状态机返回IDLE状态；
                                                 写时设置一套链表封装使能;0：不使能封装处理;1：使能封装处理;
                                                 读时返回一套链表封装处理状态;0：没在进行封装处理;1：正在进行封装处理。
    */

    VOS_UINT32          ulEnableInterrupt;
    VOS_UINT32          ulValue;
    const VOS_UINT32    ulInterruptValue    = 0x0103FF01;   /* 使用中断方式时配置使能寄存器的值 */
    const VOS_UINT32    ulPollValue         = 0x01;         /* 使用轮询方式时配置使能寄存器的值 */


    /* 判断待封装数据的总长度，若大于门限则使用中断方式，否则使用轮询方式 */
    if( ulTotalLen > HDLC_FRM_INTERRUPT_LIMIT )
    {
        /* 配置封装相关使能寄存器hdlc_frm_en的[31:0]位为0x0103FF01 */
        ulValue             = ulInterruptValue;
        ulEnableInterrupt   = VOS_TRUE;

        g_PppHdlcHardStat.ulFrmWaitIntCnt++;
    }
    else
    {
        /* 配置封装相关使能寄存器hdlc_frm_en的[31:0]位为0x01 */
        ulValue             = ulPollValue;
        ulEnableInterrupt   = VOS_FALSE;

        g_PppHdlcHardStat.ulFrmWaitQueryCnt++;
    }

    /* 使能前清除上次封装、解封装的原始中断 */
    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_DEF_INT_CLR_ADDR(HDLC_IP_BASE_ADDR), 0xFFFFFFFFU);
    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_FRM_INT_CLR_ADDR(HDLC_IP_BASE_ADDR), 0xFFFFFFFFU);

    /* 上报寄存器可维可测 */
    PPP_HDLC_HARD_MntnFrmTraceRegConfig(VOS_FALSE, ulValue, ulEnableInterrupt);

    /* 使能硬件之前先强制ARM顺序执行结束前面的指针 */
    TTF_FORCE_ARM_INSTUCTION();

    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_FRM_EN_ADDR(HDLC_IP_BASE_ADDR), ulValue);

    return ulEnableInterrupt;
}



VOS_VOID PPP_HDLC_HARD_DefProcException
(
    VOS_UINT32          ulStatus,
    VOS_UINT32          ulEnableInterrupt
)
{
    VOS_UINT32                          ulRawInt;


    if( VOS_TRUE == ulEnableInterrupt )
    {
        /* 由于在中断服务程序中进行了清中断操作，故此处取保存在g_stHdlcRegSaveInfo中的原始中断寄存器值 */
        ulRawInt                        =   g_stHdlcRegSaveInfo.ulHdlcDefRawInt;
        g_PppHdlcHardStat.usDefExpInfo |=   (1 << HDLC_INTERRUPT_IND_BITPOS);
    }
    else
    {
        ulRawInt  =   PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_RAW_INT_ADDR(HDLC_IP_BASE_ADDR));
    }

    PPP_HDLC_ERROR_LOG2("PPP_HDLC_HARD_DefProcException, ERROR, Exception ocurr status 0x%x RAW_INT 0x%x\r\n", ulStatus, ulRawInt);
    PPP_HDLC_HARD_MntnShowFrmReg();
    PPP_HDLC_HARD_MntnShowDefReg();

    g_PppHdlcHardStat.usDefExpInfo |=   (1 << HDLC_EXCEPTION_IND_BITPOS);

    /* 复位前先Delay 1s保证可维可测正常输出 */
    VOS_TaskDelay(1000);

    /* 如果HDLC出现异常，则单板异常重启 */
    mdrv_om_system_error(HDLC_DEF_SYSTEM_ERROR_ID, __LINE__, (VOS_INT)ulStatus,
                         (VOS_CHAR *)&g_stHdlcRegSaveInfo,
                         sizeof(HDLC_REG_SAVE_INFO_STRU));
    (VOS_VOID)ulRawInt;

    return;
}


VOS_UINT32 PPP_HDLC_HARD_CommWaitSem
(
    VOS_UINT32          ulHdlcMasterSem,
    VOS_UINT32          ulSemTimeoutLen
)
{
    VOS_UINT32                          ulResult;

    /* 等待封装或解封装完成 */
    ulResult = VOS_SmP(ulHdlcMasterSem, ulSemTimeoutLen);

    if (VOS_OK != ulResult)
    {
        PPP_HDLC_WARNING_LOG2("PPP_HDLC_HARD_CommWaitSem, WARNING, VOS_SmP ulHdlcMasterSem 0x%x failed! ErrorNo = 0x%x\r\n",
                      ulHdlcMasterSem, ulResult);

        g_PppHdlcHardStat.usDefExpInfo |=   (1 << HDLC_SEM_TIMEOUT_IND_BITPOS);
        g_PppHdlcHardStat.usFrmExpInfo |=   (1 << HDLC_SEM_TIMEOUT_IND_BITPOS);

        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_UINT32 PPP_HDLC_HARD_DefIsr(unsigned int ulPara)
{
    g_stHdlcRegSaveInfo.ulHdlcDefRawInt = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_RAW_INT_ADDR(HDLC_IP_BASE_ADDR));
    g_stHdlcRegSaveInfo.ulHdlcDefStatus = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_STATUS_ADDR(HDLC_IP_BASE_ADDR));

    /* 收到一次中断后清除原始中断 */
    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_DEF_INT_CLR_ADDR(HDLC_IP_BASE_ADDR), 0xFFFFFFFFU);

    /* 释放封装完成信号量 */
    VOS_SmV(g_stHdlcConfigInfo.ulHdlcDefMasterSem);

    g_PppHdlcHardStat.ulDefIsrCnt++;


    /* drv提供的接口不关心返回值 */
    return 1; /* IRQ_HANDLED; */
}


VOS_UINT32 PPP_HDLC_HARD_FrmIsr(unsigned int ulPara)
{
    g_stHdlcRegSaveInfo.ulHdlcFrmRawInt = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_RAW_INT_ADDR(HDLC_IP_BASE_ADDR));
    g_stHdlcRegSaveInfo.ulHdlcFrmStatus = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_STATUS_ADDR(HDLC_IP_BASE_ADDR));

    /* 收到一次中断后清除原始中断 */
    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_FRM_INT_CLR_ADDR(HDLC_IP_BASE_ADDR), 0xFFFFFFFFU);

    /* 释放封装完成信号量 */
    VOS_SmV(g_stHdlcConfigInfo.ulHdlcFrmMasterSem);

    g_PppHdlcHardStat.ulFrmIsrCnt++;

    /* drv提供的接口不关心返回值 */
    return 1; /* IRQ_HANDLED; */
}


VOS_UINT32 PPP_HDLC_HARD_DefWaitStatusChange(VOS_VOID)
{
    /*
    hdlc_def_status  (0x88)
      31 30   24 23   8 7 6  5   4   3   2  1  0
    |---|-------|------|---|---|---|---|---|----|
    |Rsv|  Type |  Num |Rsv|Idx|Ful|Ful|Now|Stat|
    Reserved             [31]    1'b0    h/s R/W  保留位。读时返回0。写时无影响。
    def_err_type         [30:24] 7'b0    h/s RO   有帧上报时，错误帧类型，对应的bit位为1即表明发生该类型错误：
                                                  bit 30：错误类型6，转义字符0x7D后紧接一个Flag域；
                                                  bit 29：错误类型5，当AC域无压缩时，Address域值非0xFF；
                                                  bit 28：错误类型4，当AC域无压缩时，Control域值非0x03；
                                                  bit 27：错误类型3，当P域需剥离时，收到非法的Protocol域值；
                                                  bit 26：错误类型2，解封装后帧字节数小于4bites；
                                                  bit 25：错误类型1，解封装后帧字节数大于1502bytes（PPP帧的Information域不超过1500Bytes，加上协议域不超过1502Bytes）；
                                                  bit 24：错误类型0， CRC校验错误。
    def_valid_num        [23:8]  16'b0   h/s RO   有帧上报时，有效帧数目；（不包括最后一个可能的非完整帧）
    Reserved             [7:6]   2'b0    h/s R/W  保留位。读时返回0。写时无影响。
    def_error_index      [5]     1'b0    h/s RO   解封装发生错误指示
    def_rpt_ful          [4]     1'b0    h/s RO   解封装外部正确帧信息上报空间存满暂停指示
    def_out_spc_ful      [3]     1'b0    h/s RO   解封装外部输出存储空间存满暂停指示
    def_uncompleted_now  [2]     1'b0    h/s RO   用于指示当前链表是否有解出非完整帧，为了支持多个PPP/IP拨号而增加的配置：0，没有；1，有
    def_all_pkt_pro_stat [1:0]   2'b0    h/s RO   一套输入链表处理状态：00：未完成一套输入链表处理；01：未完成一套输入链表处理，已解出LCP帧，硬件处于暂停状态；
                                                  10：完成一套输入链表处理，但无帧上报；11: 完成一套输入链表处理，且有帧上报；
    */
    VOS_UINT32              ulRsltWaitNum;           /* 防止硬件异常的保护变量 */
    volatile VOS_UINT32     ulStatus;                /* 解封装状态 */

   /* 查询hdlc_frm_status (0x28)的第[0]位和第[1]位，任何一个为1或者超时则返回 */

    ulRsltWaitNum = 0UL;

    while (ulRsltWaitNum < HDLC_DEF_MAX_WAIT_RESULT_NUM)
    {
        /* 查询状态寄存器hdlc_def_status (0x88)的0-1和3-5位，任何一位变为1表示解封装模块暂停或停止 */
        ulStatus  =   PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_STATUS_ADDR(HDLC_IP_BASE_ADDR));

        if (HDLC_DEF_STATUS_DOING != (ulStatus & HDLC_DEF_STATUS_MASK))
        {
            break;
        }

        ulRsltWaitNum++;
    }

    if ( HDLC_DEF_MAX_WAIT_RESULT_NUM <= ulRsltWaitNum )
    {
        PPP_HDLC_WARNING_LOG2("PPP_HDLC_HARD_DefWaitStatusChange, WARNING, wait hdlc_def_status timeout %d status 0x%x!\r\n",
                      ulRsltWaitNum, ulStatus);

        g_PppHdlcHardStat.usDefExpInfo |=   (1 << HDLC_WAIT_STATUS_TIMEOUT_IND_BITPOS);

        return VOS_ERR;
    }

    g_PppHdlcHardStat.ulDefMaxQueryCnt = TTF_MAX(g_PppHdlcHardStat.ulDefMaxQueryCnt, ulRsltWaitNum);

    return VOS_OK;
}


VOS_UINT32 PPP_HDLC_HARD_FrmWaitStatusChange(VOS_VOID)
{
    VOS_UINT32              ulFrmRsltWaitNum;           /* 防止硬件异常的保护变量 */
    volatile VOS_UINT32     ulFrmStatus;                /* 封装状态 */


   /* 查询hdlc_frm_status (0x28)的第[0]位和第[1]位，任何一个为1或者超时则返回 */
    ulFrmRsltWaitNum = 0UL;

    while (ulFrmRsltWaitNum < HDLC_FRM_MAX_WAIT_RESULT_NUM)
    {
        /* 读取 hdlc_frm_status的[0][1]位 */
        ulFrmStatus  =   PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_STATUS_ADDR(HDLC_IP_BASE_ADDR));

        if (HDLC_FRM_ALL_PKT_DOING != (ulFrmStatus & HDLC_FRM_STATUS_MASK))
        {
            break;
        }

        ulFrmRsltWaitNum++;
    }

    if ( HDLC_FRM_MAX_WAIT_RESULT_NUM <= ulFrmRsltWaitNum )
    {
        PPP_HDLC_WARNING_LOG2("PPP_HDLC_HARD_FrmWaitStatusChange, WARNING, wait hdlc_frm_status timeout %d status 0x%x!\r\n",
                      ulFrmRsltWaitNum, ulFrmStatus);

        g_PppHdlcHardStat.usFrmExpInfo |=   (1 << HDLC_WAIT_STATUS_TIMEOUT_IND_BITPOS);

        return VOS_ERR;
    }

    g_PppHdlcHardStat.ulFrmMaxQueryCnt = TTF_MAX(g_PppHdlcHardStat.ulFrmMaxQueryCnt, ulFrmRsltWaitNum);

    return VOS_OK;
}


VOS_UINT32 PPP_HDLC_HARD_FrmWaitResult
(
    VOS_UINT32          ulEnableInterrupt
)
{
    VOS_UINT32              ulFrmStatus;        /* 封装状态 */
    VOS_UINT32              ulResult;


    if (VOS_TRUE == ulEnableInterrupt)
    {
        /* 等待中断得到出错或完成状态 */
        ulResult = PPP_HDLC_HARD_CommWaitSem(g_stHdlcConfigInfo.ulHdlcFrmMasterSem, HDLC_FRM_MASTER_INT_TIMER_LEN);

        /* 由于在中断服务程序中进行了清中断操作，而Status指示是否出错的bit由原始中断寄存器
           决定，故此处取保存在g_stHdlcRegSaveInfo中的状态值 */
        ulFrmStatus = g_stHdlcRegSaveInfo.ulHdlcFrmStatus;
    }
    else
    {
        /* 轮询得到出错或完成 */
        ulResult = PPP_HDLC_HARD_FrmWaitStatusChange();

        /* 查询hdlc_frm_status (0x28)获取封装状态并将其返回 */
        ulFrmStatus  =  PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_STATUS_ADDR(HDLC_IP_BASE_ADDR));
    }

    /* 上报寄存器可维可测 */
    PPP_HDLC_HARD_MntnFrmTraceRegConfig(VOS_TRUE, 0, ulEnableInterrupt);

    /* 等不到说明HDLC还在工作 */
    if (VOS_OK != ulResult)
    {
        return HDLC_FRM_ALL_PKT_DOING;
    }

    ulFrmStatus &=  HDLC_FRM_STATUS_MASK;

    return ulFrmStatus;
}


VOS_UINT32 PPP_HDLC_HARD_DefWaitResult
(
    VOS_UINT32          ulEnableInterrupt
)
{
    VOS_UINT32                          ulStatus;                /* 解封装状态 */
    VOS_UINT32                          ulResult;


    if (VOS_TRUE == ulEnableInterrupt)
    {
        /* 等待中断得到暂停或完成状态 */
        ulResult = PPP_HDLC_HARD_CommWaitSem(g_stHdlcConfigInfo.ulHdlcDefMasterSem, HDLC_DEF_MASTER_INT_TIMER_LEN);

        /* 由于在中断服务程序中进行了清中断操作，而Status指示是否出错的bit由原始
           中断寄存器决定，故此处取保存在g_stHdlcRegSaveInfo中的状态值 */
        ulStatus = g_stHdlcRegSaveInfo.ulHdlcDefStatus;

    }
    else
    {
        /* 轮询得到暂停或完成 */
        ulResult = PPP_HDLC_HARD_DefWaitStatusChange();

        /* 查询hdlc_def_status (0x88)获取解封装状态并将其返回 */
        ulStatus  =  PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_STATUS_ADDR(HDLC_IP_BASE_ADDR));
    }

    /* 上报寄存器可维可测 */
    PPP_HDLC_HARD_MntnDefTraceRegConfig(VOS_TRUE, 0, ulEnableInterrupt);

    /* 等不到说明HDLC还在工作 */
    if (VOS_OK != ulResult)
    {
        return HDLC_DEF_STATUS_DOING;
    }

    ulStatus &=  HDLC_DEF_STATUS_MASK;

    return ulStatus;
}


VOS_UINT32 PPP_HDLC_HARD_DefCfgEnReg
(
    VOS_UINT32                      ulTotalLen
)
{
    /*
    hdlc_def_en   (0x60)
      31   25 24  23 19 18  17  16  15  14  13  12  11  10   9   8   7   1  0
    |--------|---|-----|---|---|---|---|---|---|---|---|---|---|---|------|---|
    |   Rsv  |en | Rsv |en |en |en |en |en |en |en |en |en |en |en |  Rsv |en |

    Reserved            [31:25] 7'b0    h/s R/W  保留位。读时返回0。写时无影响。
    def_over_int_en     [24]    1'b0    h/s R/W  一套链表解封装结束中断使能;0：中断禁止;1：中断使能;
    Reserved            [23:19] 5'b0    h/s R/W  保留位。读时返回0。写时无影响。
    def_rpt_ful_en      [18]    1'b0    h/s R/W  解封装外部正确帧信息上报空间存满暂停中断使能;0：中断禁止;1：中断使能;
    def_out_spc_ful_en  [17]    1'b0    h/s R/W  解封装外部输出存储空间存满暂停中断使能;0：中断禁止;1：中断使能
    def_lcp_int_en      [16]    1'b0    h/s R/W  解封装解出有效LCP帧暂停中断上报使能;0：中断禁止;1：中断使能
    def_rpt_prm_err_en  [15]    1'b0    h/s R/W  解封装上报空间相关参数错误中断使能;0：中断禁止;1：中断使能
    def_out_prm_err_en  [14]    1'b0    h/s R/W  解封装输出空间相关参数错误中断使能;0：中断禁止;1：中断使能
    def_in_prm_err_en   [13]    1'b0    h/s R/W  解封装输入链表相关参数错误中断使能;0：中断禁止;1：中断使能
    def_cfg_err_en      [12]    1'b0    h/s R/W  解封装协议压缩指示配置错误中断使能;0：中断禁止;1：中断使能
    def_wr_timeout_en   [11]    1'b0    h/s R/W  解封装时AXI总线写请求timeout中断使能;0：中断禁止;1：中断使能
    def_rd_timeout _en  [10]    1'b0    h/s R/W  解封装时AXI总线读请求timeout中断使能;0：中断禁止;1：中断使能
    def_wr_err_en       [9]     1'b0    h/s R/W  解封装时AXI总线写操作错误中断使能;0：中断禁止;1：中断使能
    def_rd_err_en       [8]     1'b0    h/s R/W  解封装时AXI总线读操作错误中断使能;0：中断禁止;1：中断使能
    Reserved            [7:1]   7'b0    h/s R/W  保留位。读时返回0。写时无影响。
    def_en              [0]     1'b0    h/s R/W  一套输入链表解封装使能，软件向def_en写入1'b1启动解封装工作；一套输入链表解封装完成后，由硬件自动对def_en清零；
                                                 解封装过程出错时，硬件也会对def_en自动清零，使内部状态机返回IDLE状态；读该寄存器返回解封装处理状态。
                                                 写时设置一套输入链表解封装使能：0：不使能解封装处理；1：使能解封装处理；
                                                 读时返回一套输入链表解封装处理状态：0：没在进行解封装处理；1：正在进行解封装处理。
    */
    VOS_UINT32          ulEnableInterrupt;
    VOS_UINT32          ulValue;
    const VOS_UINT32    ulInterruptValue    = 0x0107FF01;   /* 使用中断方式时配置使能寄存器的值 */
    const VOS_UINT32    ulPollValue         = 0x01;         /* 使用轮询方式时配置使能寄存器的值 */


    if( ulTotalLen > HDLC_DEF_INTERRUPT_LIMIT )
    {
        /* 配置封装相关使能寄存器hdlc_def_en的[31:0]位为0x0107FF01 */
        ulValue             = ulInterruptValue;
        ulEnableInterrupt   = VOS_TRUE;

        g_PppHdlcHardStat.ulDefWaitIntCnt++;
    }
    else
    {
        /* 配置封装相关使能寄存器hdlc_frm_en的[31:0]位为0x01 */
        ulValue             = ulPollValue;
        ulEnableInterrupt   = VOS_FALSE;

        g_PppHdlcHardStat.ulDefWaitQueryCnt++;
    }

    /* 使能前清除上次封装、解封装的原始中断 */
    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_DEF_INT_CLR_ADDR(HDLC_IP_BASE_ADDR), 0xFFFFFFFFU);
    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_FRM_INT_CLR_ADDR(HDLC_IP_BASE_ADDR), 0xFFFFFFFFU);

    /* 上报寄存器可维可测 */
    PPP_HDLC_HARD_MntnDefTraceRegConfig(VOS_FALSE, ulValue, ulEnableInterrupt);

    /* 使能硬件之前先强制ARM顺序执行结束前面的指针 */
    TTF_FORCE_ARM_INSTUCTION();

    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_DEF_EN_ADDR(HDLC_IP_BASE_ADDR), ulValue);

    return ulEnableInterrupt;
}


VOS_VOID PPP_HDLC_HARD_CommCfgReg(VOS_VOID)
{
    

    VOS_UINT32                          ulValue = 0x0;


    /* 使能AXI请求超时判断，debug时使用，由于HDLC设置超时时间过短，故正常功能模式下不开启 */
/*    SET_BIT_TO_DWORD(ulValue, 8); */

    /* 设置AXI请求超时时长，该值由SoC提供，并且保证无平台差异 */
    ulValue |= (HDLC_AXI_REQ_TIMEOUT_VALUE << 16);

    PPP_HDLC_WRITE_32REG(SOC_ARM_HDLC_PRIROR_TIMEOUT_CTRL_ADDR(HDLC_IP_BASE_ADDR), ulValue);

    return;
}


VOS_VOID PPP_HDLC_HARD_MntnShowConfigInfo(VOS_VOID)
{
    /* 输出IP相关信息 */
    PPP_HDLC_WARNING_LOG3("HDLC Hardware Info: g_ulHDLCIPBaseAddr 0x%x g_slHdlcISRDef %d g_slHdlcISRFrm %d\r\n",
                  HDLC_IP_BASE_ADDR, g_stHdlcConfigInfo.slHdlcISRDef, g_stHdlcConfigInfo.slHdlcISRFrm);
    PPP_HDLC_WARNING_LOG2("HDLC Hardware Info: g_ulHdlcDefMasterSem 0x%x g_ulHdlcFrmMasterSem 0x%x\r\n",
                  g_stHdlcConfigInfo.ulHdlcDefMasterSem, g_stHdlcConfigInfo.ulHdlcFrmMasterSem);

    /* 输出内存相关信息 */
    PPP_HDLC_WARNING_LOG2("HDLC Memory Info: g_pstHdlcDefBufInfo 0x%x TTF_HDLC_MASTER_DEF_BUF_LEN %d\r\n",
                  g_pstHdlcDefBufInfo, TTF_HDLC_MASTER_DEF_BUF_LEN);
    PPP_HDLC_WARNING_LOG2("HDLC Memory Info: g_pstHdlcFrmBufInfo 0x%x TTF_HDLC_MASTER_FRM_BUF_LEN %d\r\n",
                  g_pstHdlcFrmBufInfo, TTF_HDLC_MASTER_FRM_BUF_LEN);

    return;
}


VOS_VOID PPP_HDLC_HARD_MntnShowStatInfo(VOS_VOID)
{
    vos_printf("\n================HDLC Hardware STAT INFO Begin==========================\n");

    vos_printf("解封装处理IP类型包个数            = %d\n", g_PppHdlcHardStat.ulDefIpDataProcCnt);
    vos_printf("解封装处理PPP类型包个数           = %d\n", g_PppHdlcHardStat.ulDefPppDataProcCnt);
    vos_printf("解封装输出非完整帧个数            = %d\n", g_PppHdlcHardStat.ulDefUncompleteCnt);
    vos_printf("解封装等待中断次数                = %d\n", g_PppHdlcHardStat.ulDefWaitIntCnt);
    vos_printf("解封装轮询次数                    = %d\n", g_PppHdlcHardStat.ulDefWaitQueryCnt);
    vos_printf("解封装中断次数                    = %d\n", g_PppHdlcHardStat.ulDefIsrCnt);
    vos_printf("解封装LCP帧暂停次数               = %d\n", g_PppHdlcHardStat.ulDefLcpPauseCnt);
    vos_printf("解封装空间满暂停次数              = %d\n", g_PppHdlcHardStat.ulDefFullPauseCnt);
    vos_printf("解封装丢弃错误数据包个数          = %d\n", g_PppHdlcHardStat.ulDefInputDiscardCnt);

    vos_printf("封装处理IP类型包个数              = %d\n", g_PppHdlcHardStat.ulFrmIpDataProcCnt);
    vos_printf("封装处理PPP类型包个数             = %d\n", g_PppHdlcHardStat.ulFrmPppDataProcCnt);
    vos_printf("封装等待中断次数                  = %d\n", g_PppHdlcHardStat.ulFrmWaitIntCnt);
    vos_printf("封装轮询次数                      = %d\n", g_PppHdlcHardStat.ulFrmWaitQueryCnt);
    vos_printf("封装中断次数                      = %d\n", g_PppHdlcHardStat.ulFrmIsrCnt);
    vos_printf("封装申请目的空间内存失败次数      = %d\n", g_PppHdlcHardStat.ulFrmAllocOutputMemFailCnt);
    vos_printf("封装申请第一个目的空间内存失败次数= %d\n", g_PppHdlcHardStat.ulFrmAllocFirstMemFailCnt);
    vos_printf("封装输出参数链表满次数            = %d\n", g_PppHdlcHardStat.ulFrmOutputLinkFullCnt);
    vos_printf("封装丢弃错误数据包个数            = %d\n", g_PppHdlcHardStat.ulFrmInputDiscardCnt);

    vos_printf("解封装输入链表最大节点数          = %d\n", g_PppHdlcHardStat.ulDefMaxInputCntOnce);
    vos_printf("解封装输入链表最大总长度          = %d\n", g_PppHdlcHardStat.ulDefMaxInputSizeOnce);
    vos_printf("解封装输出有效帧最大个数          = %d\n", g_PppHdlcHardStat.ulDefMaxValidCntOnce);
    vos_printf("解封装轮询最大次数                = %d\n", g_PppHdlcHardStat.ulDefMaxQueryCnt);

    vos_printf("封装输入链表最大节点数            = %d\n", g_PppHdlcHardStat.ulFrmMaxInputCntOnce);
    vos_printf("封装输入链表最大总长度            = %d\n", g_PppHdlcHardStat.ulFrmMaxInputSizeOnce);
    vos_printf("封装输出使用最大节点个数          = %d\n", g_PppHdlcHardStat.ulFrmMaxOutputCntOnce);
    vos_printf("封装输出使用最大节点总长度        = %d\n", g_PppHdlcHardStat.ulFrmMaxOutputCntOnce);
    vos_printf("封装轮询最大次数                  = %d\n", g_PppHdlcHardStat.ulFrmMaxQueryCnt);

    vos_printf("单次处理最大节点数                = %d\n", g_PppHdlcHardStat.ulMaxCntOnce);
    vos_printf("处理总节点数                      = %d\n", g_PppHdlcHardStat.ulHdlcHardProcCnt);
    vos_printf("continue次数                      = %d\n", g_PppHdlcHardStat.ulContinueCnt);
    vos_printf("usDefExpInfo标识                  = %d\n", g_PppHdlcHardStat.usDefExpInfo);
    vos_printf("usFrmExpInfo标识                  = %d\n", g_PppHdlcHardStat.usFrmExpInfo);

    vos_printf("规避HDLC BUG不拷贝数据次数        = %d\n", g_PppHdlcHardStat.ulForbidHdlcBugNoCpy);
    vos_printf("规避HDLC BUG拷贝数据次数          = %d\n", g_PppHdlcHardStat.ulForbidHdlcBugCpy);


    vos_printf("================HDLC Hardware STAT INFO End==========================\n");

    return;
}


VOS_VOID PPP_HDLC_HARD_MntnTraceSingleData
(
    VOS_UINT16                          usDataLen,
    VOS_UINT8                          *pucDataAddr,
    HDLC_MNTN_EVENT_TYPE_ENUM_UINT32    ulEventType,
    VOS_UINT32                          ulNodeIndex
)
{
    VOS_UINT32                          ulDataLen;
    HDLC_MNTN_NODE_DATA_STRU           *pstNodeData;
    VOS_UINT32                          ulAllocDataLen;


    ulAllocDataLen = TTF_MIN(usDataLen, HDLC_MNTN_ALLOC_MEM_MAX_SIZE);

    /* 消息长度等于消息结构大小加数据内容长度 */
    ulDataLen   = ulAllocDataLen + sizeof(HDLC_MNTN_NODE_DATA_STRU);

    pstNodeData = (HDLC_MNTN_NODE_DATA_STRU *)PS_MEM_ALLOC(PS_PID_PPP_HDLC, ulDataLen);

    if (VOS_NULL_PTR == pstNodeData)
    {
        PPP_HDLC_NORMAL_LOG1("PPP_HDLC_HARD_MntnTraceSingleData, NORMAL, Alloc mem failed, ulEventType %d!\r\n",
                      ulEventType);
        return;
    }

    /* 用于标识这是一组输入链表中的第几个IP包 */
    pstNodeData->usNodeIndex = (VOS_UINT16)ulNodeIndex;
    pstNodeData->usLen       = usDataLen;

    PPP_MemSingleCopy((VOS_UINT8 *)(pstNodeData + 1), pucDataAddr, ulAllocDataLen);

    PPP_HDLC_HARD_MntnTraceMsg((HDLC_MNTN_TRACE_HEAD_STRU *)pstNodeData,
                               ulEventType, ulDataLen);

    PS_MEM_FREE(PS_PID_PPP_HDLC, pstNodeData);

    return;
}


VOS_VOID PPP_HDLC_HARD_MntnTraceInputParaLink
(
    HDLC_MNTN_EVENT_TYPE_ENUM_UINT32    ulEventType,
    VOS_UINT32                          ulLinkNodeCnt,
    VOS_UINT32                          ulLinkTotalSize,
    HDLC_PARA_LINK_NODE_STRU           *pastLinkNodeBuf
)
{
    HDLC_MNTN_INPUT_PARA_LINK_STRU      stInputPara;
    HDLC_MNTN_INPUT_PARA_LINK_STRU     *pstInputPara = &stInputPara;
    VOS_UINT32                          ulDataLen;


    ulDataLen = sizeof(HDLC_MNTN_INPUT_PARA_LINK_STRU);

    /* 记录并上报参数链表所有节点的信息*/
    pstInputPara->ulInputLinkNodeCnt   = ulLinkNodeCnt;
    pstInputPara->ulInputLinkTotalSize = ulLinkTotalSize;

    /* 参数链表每个节点的内容 */
    VOS_MemCpy_s((VOS_UINT8 *)(&pstInputPara->astInputParaLinkNodeBuf[0]),
                ulLinkNodeCnt * sizeof(HDLC_PARA_LINK_NODE_STRU),
               (VOS_UINT8 *)(pastLinkNodeBuf),
               ulLinkNodeCnt * sizeof(HDLC_PARA_LINK_NODE_STRU));

    PPP_HDLC_HARD_MntnTraceMsg((HDLC_MNTN_TRACE_HEAD_STRU *)pstInputPara,
                               ulEventType, ulDataLen);

    return;
}


VOS_UINT32 PPP_HDLC_HARD_DefIsEnabled(VOS_VOID)
{
    VOS_UINT32                          ulValue;


    /* SoC会在处理完一套输入链表的时候自动将使能位清零 */
    ulValue = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_DEF_EN_ADDR(HDLC_IP_BASE_ADDR));

    if (0x01 == (ulValue & 0x01))
    {
        return VOS_TRUE;
    }
    else
    {
        return VOS_FALSE;
    }
}


VOS_UINT32 PPP_HDLC_HARD_FrmIsEnabled(VOS_VOID)
{
    VOS_UINT32                          ulValue;


    /* SoC会在处理完一套输入链表的时候自动将使能位清零 */
    ulValue = PPP_HDLC_READ_32REG(SOC_ARM_HDLC_FRM_EN_ADDR(HDLC_IP_BASE_ADDR));

    if (0x01 == (ulValue & 0x01))
    {
        return VOS_TRUE;
    }
    else
    {
        return VOS_FALSE;
    }
}


VOS_VOID PPP_HDLC_HARD_SetUp(PPP_ID usPppId)
{
    HDLC_DEF_UNCOMPLETED_INFO_STRU      *pstUncompletedInfo;


    pstUncompletedInfo = HDLC_DEF_GET_UNCOMPLETED_INFO(usPppId);

    VOS_MemSet_s(pstUncompletedInfo, sizeof(HDLC_DEF_UNCOMPLETED_INFO_STRU),
                 0, sizeof(HDLC_DEF_UNCOMPLETED_INFO_STRU));

    return;
}


VOS_VOID PPP_HDLC_HARD_Disable(VOS_VOID)
{
    /* 暂无操作，因为HDLC优化后，一套链表封装或解封装完成时，由硬件自动对frm_en或def_en清零；
       封装或解封装过程出错时，硬件也会自动清零，使内部状态机返回IDLE状态；*/
}


VOS_UINT32 PPP_HDLC_HARD_Init(VOS_VOID)
{
    VOS_UINT_PTR                          ulBaseAddr;

    /* 获取HDLC基地址 */
    ulBaseAddr              = (VOS_UINT_PTR)mdrv_misc_get_ip_baseaddr(BSP_IP_TYPE_HDLC);

    HDLC_IP_BASE_ADDR       = (VOS_UINT_PTR)PPP_IO_ADDRESS(ulBaseAddr);

    if (VOS_NULL_PTR == HDLC_IP_BASE_ADDR)
    {
        PPP_HDLC_ERROR_LOG1("PPP_HDLC_HARD_Init HDLC base addr is null,0x%x\r\n",
                      ulBaseAddr);
        return VOS_ERR;
    }

#if (VOS_OS_VER == VOS_WIN32)
    g_ulHdlcScCtrlBaseAddr  = g_ucScCtrlRegAddr;
#else

#if ((SC_CTRL_MOD_P532 == SC_CTRL_MOD) || (SC_CTRL_MOD_6950_SFT == SC_CTRL_MOD) || (SC_CTRL_MOD_6932_SFT == SC_CTRL_MOD))
    ulBaseAddr              = (VOS_UINT_PTR)mdrv_misc_get_ip_baseaddr(BSP_IP_TYPE_SYSCTRL_PD);
#elif ((SC_CTRL_MOD_6250_SFT == SC_CTRL_MOD) || (SC_CTRL_MOD_3660_SFT == SC_CTRL_MOD))
    /* =========dallas/chicago中使用: HDLC寄存器基地址位置======== */
    ulBaseAddr              = (VOS_UINT_PTR)mdrv_misc_get_ip_baseaddr(BSP_IP_TYPE_SYSCTRL_MDM);
#else
    ulBaseAddr              = (VOS_UINT_PTR)mdrv_misc_get_ip_baseaddr(BSP_IP_TYPE_SYSCTRL);
#endif

    g_ulHdlcScCtrlBaseAddr  = (VOS_UINT_PTR)PPP_IO_ADDRESS(ulBaseAddr);

#endif
    if (VOS_NULL_PTR == g_ulHdlcScCtrlBaseAddr)
    {
        PPP_HDLC_ERROR_LOG1("PPP_HDLC_HARD_Init HDLC SCCTRL base addr is null,0x%x\r\n",
                      ulBaseAddr);
        return VOS_ERR;
    }

    /* 关闭HDLC时钟 */
    PPP_HDLC_HARD_PeriphClkClose();

    /*获取HDLC解封装中断号*/
    g_stHdlcConfigInfo.slHdlcISRDef   = mdrv_int_get_num(BSP_INT_TYPE_HDLC_DEF);

    /*获取HDLC封装中断号*/
    g_stHdlcConfigInfo.slHdlcISRFrm   = mdrv_int_get_num(BSP_INT_TYPE_HDLC_FRM);

    /* 初始化内存 */
    if (VOS_OK != PPP_HDLC_HARD_InitBuf())
    {
        return VOS_ERR;
    }

    if ( VOS_OK != VOS_SmBCreate("HdlcDefMasterSem", 0, VOS_SEMA4_FIFO, (VOS_SEM *)&g_stHdlcConfigInfo.ulHdlcDefMasterSem) )
    {
        PPP_HDLC_ERROR_LOG("PPP_HDLC_HARD_Init, ERROR, Create g_ulHdlcDefMasterSem failed!\r\n");
        return VOS_ERR;
    }

    if ( VOS_OK != VOS_SmBCreate("HdlcFrmMasterSem", 0, VOS_SEMA4_FIFO, (VOS_SEM *)&g_stHdlcConfigInfo.ulHdlcFrmMasterSem) )
    {
        PPP_HDLC_ERROR_LOG("PPP_HDLC_HARD_Init, ERROR, Create g_ulHdlcFrmMasterSem failed!\r\n");
        return VOS_ERR;
    }

    /* 中断挂接 */
    if (VOS_OK != mdrv_int_connect(g_stHdlcConfigInfo.slHdlcISRDef, (VOIDFUNCPTR)PPP_HDLC_HARD_DefIsr, 0))
    {
        PPP_HDLC_ERROR_LOG1("PPP_HDLC_HARD_Init, ERROR, Connect slHdlcISRDef %d to PPP_HDLC_HARD_DefIsr failed!\r\n",
                      g_stHdlcConfigInfo.slHdlcISRDef);
        return VOS_ERR;
    }

    /* 中断使能 */
    if (VOS_OK != mdrv_int_enable(g_stHdlcConfigInfo.slHdlcISRDef))
    {
        PPP_HDLC_ERROR_LOG1("PPP_HDLC_HARD_Init, ERROR, Enable slHdlcISRDef %d failed!\r\n",
                      g_stHdlcConfigInfo.slHdlcISRDef);
        return VOS_ERR;
    }

    /* 中断挂接 */
    if (VOS_OK != mdrv_int_connect(g_stHdlcConfigInfo.slHdlcISRFrm, (VOIDFUNCPTR)PPP_HDLC_HARD_FrmIsr, 0))
    {
        PPP_HDLC_ERROR_LOG1("PPP_HDLC_HARD_Init, ERROR, Connect slHdlcISRFrm %d to PPP_HDLC_HARD_FrmIsr failed!\r\n",
                      g_stHdlcConfigInfo.slHdlcISRFrm);
        return VOS_ERR;
    }

    if (VOS_OK != mdrv_int_enable(g_stHdlcConfigInfo.slHdlcISRFrm))
    {
        PPP_HDLC_ERROR_LOG1("PPP_HDLC_HARD_Init, ERROR, Enable slHdlcISRFrm %d failed!\r\n",
                      g_stHdlcConfigInfo.slHdlcISRFrm);
        return VOS_ERR;
    }

    return VOS_OK;
}    /* link_HDLCInit */



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


