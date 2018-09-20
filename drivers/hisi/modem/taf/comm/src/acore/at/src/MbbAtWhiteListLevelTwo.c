/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2013-2015. All rights reserved.
 *
 * mobile@huawei.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*************************************************************************
CAUTION : This file is Auto Generated by VBA based on 《MBB_AT_whitelist_auto_generator.xlsm》.
          So, don't modify this file manually!
****************************************************************************/


/******************************* 引用头文件 *******************************/
#include "MbbAtCmdFilter.h"





/****************************************************************************/
/*************************** MBB AT 二级白名单列表开始 **********************/
/****************************************************************************/
const VOS_CHAR *gszAllowCmdTable[] = 
{
    "^TMODE",    /* 操作模式设置命令^TMODE */
    "^SD",    /* SD卡操作命令^SD */
    "^BSN",    /* 单板条码号命令^BSN */
    "^GPIOPL",    /* 连接器测试命令^GPIOPL */
    "^INFORBU",    /* 信息备份命令^INFORBU */
    "^DATALOCK",    /* 设置数据锁命令^DATALOCK */
    "^CSVER",    /* 定制版本号查询命令^CSVER */
    "^PORTLOCK",    /* 端口锁命令^PORTLOCK */
    "^SIMLOCK",    /* SIMLOCK生产定制解锁和查询命令^SIMLOCK */
    "^FTXON",    /* 非信令下打开发射机^FTXON */
    "^FRXON",    /* 非信令下打开接收机^FRXON */
    "^FLNA",    /* 设置接收机LNA的等级^FLNA */
    "^FRSSI",    /* 获得当前信道RSSI值^FRSSI */
    "^PLATFORM",    /* 获取方案平台信息^PLATFORM */
    "^VERSION",    /* 版本号指令^VERSION */
    "^MAXLCKTMS",    /* 最大锁卡次数^MAXLCKTMS */
    "^WIENABLE",    /* WiFi开关指令^WIENABLE */
    "^WIMODE",    /* 设置WiFi模式指令^WIMODE */
    "^WIBAND",    /* 设置WiFi带宽指令^WIBAND */
    "^WIPOW",    /* 设置WiFi功率指令^WIPOW */
    "^WITX",    /* 设置WiFi发射机开关^WITX */
    "^WIRX",    /* 设置WiFi接收机开关^WIRX */
    "^WIRPCKG",    /* WiFi接收包命令^WIRPCKG */
    "^TMMI",    /* MMI测试结果指令^TMMI */
    "^ANTENNA",    /* 查询设置内外置天线切换^ANTENNA */
    "^TCHRENABLE",    /* 充电使能指令^TCHRENABLE */
    "^SSID",    /* SSID指令^SSID */
    "^WIKEY",    /* WiFi Key指令^WIKEY */
    "^WUPWD",    /* Web UI登陆密码指令^WUPWD */
    "^EQVER",    /* 查询装备归一化AT命令版本号命令^EQVER */
    "^TBAT",    /* 测试电池指令^TBAT */
    "^WIINFO",    /* 查询WiFi信息^WIINFO */
    "^WIPARANGE",    /* WIFI增益等级命令^WIPARANGE */
    "^WIWEP",    /* WiFi WEP指令^WIWEP */
    "^TBATDATA",    /* 设置查询电池校准参数^TBATDATA */
    "^WIPLATFORM",    /* 获取WiFi方案平台信息^WIPLATFORM */
    "^WUSITE",    /* WEB UI登录网址指令^WUSITE */
    "^WIPIN",    /* WPS PIN指令^WIPIN */
    "^TNETPORT",    /* 网口测试指令^TNETPORT */
    "^WUUSER",    /* WEB UI登陆用户名配置指令^WUUSER */
    "^BODYSARON",    /* 开关BODY SAR功能^BODYSARON */
    "^RSIM",    /* 查询外置SIM/USIM/UIM卡接触状态命令^RSIM */
    "^FCHAN",    /* 设置非信令的信道^FCHAN */
    "^SECUBOOT",    /* Secure Boot 命令 ^SECUBOOT */
    "^SECUBOOTFEATURE",    /* Secure Boot 命令 ^SECUBOOTFEATURE */
    "^SKU",    /* SKU Type Control Switch ^SKU */
    "^WIDATARATE",    /* 设置WiFi速率指令^WIDATARATE */
    "^TSELRF",    /* 选择射频通路指令^TSELRF */
    "^SFM",    /* 生产模式查询命令^SFM */
    "^PHYNUM",    /* 设置查询物理号命令^PHYNUM */
    "^CSDFLT",    /* 清空生产定制^CSDFLT */
    "^AUDIO",    /* 音频测试指令^AUDIO */
    "^WIFREQ",    /* 设置WiFi点指令^WIFREQ */
    "^PRODTYPE",    /* 产品形态指令^PRODTYPE */
    "^SFEATURE",    /* 查询产品特性查询指令^SFEATURE */
    "^PSTANDBY",    /* 设置进入待机状态指令^PSTANDBY */
    "^EXTCHARGE",    /* 对外充电电路测试指令^EXTCHARGE */
    "^FACINFO",    /* 制造信息指令^FACINFO */
    "^AUTHVER",    /* GetAuthVer AT命令接口 ^AUTHVER */
    "^FWAVE",    /* 非信令下波形设置 */
    "^USBSPEED",    /* USB数率操作 */
    "E",    /* 回显命令E */
    "S0",    /* Ring before automatic answer S1 */
    "S3",    /* 命令行结束符S2 */
    "S4",    /* 响应格式字符S3 */
    "S5",    /* 退格字符S4 */
    "V",    /* ME响应格式命令V */
    "I",    /* 查询MS的所有ID信息 I */
    "+GCAP",    /* 查询MS当前所支持的传输能力域+GCAP */
    "+CGMI",    /* 制造商信息查询命令+CGMI/+GMI */
    "+GMI",    /* 制造商信息查询命令+CGMI/+GMI */
    "+CGMM",    /* 产品名称查询命令+CGMM/+GMM */
    "+GMM",    /* 产品名称查询命令+CGMM/+GMM */
    "+CGMR",    /* 软件版本号查询命令+CGMR/+GMR */
    "+GMR",    /* 软件版本号查询命令+CGMR/+GMR */
    "+CGSN",    /* IMEI查询命令+CGSN/+GSN */
    "+GSN",    /* IMEI查询命令+CGSN/+GSN */
    "+CSCS",    /* 设置TE字符集+CSCS */
    "+CIMI",    /* IMSI查询命令+CIMI */
    "Z",    /* 恢复AT命令出厂设置 Z */
    "&F",    /* 恢复AT命令及相关设置出厂设置命令&F */
    "A/",    /* Repeat previous command line A/ */
    "Q",    /* Set result code presentation mode Q */
    "&W",    /* Store user settings &W */
    "&V",    /* Query current configuration &V */
    "+CCLK",    /* Return current time of the module +CCLK */
    "X",    /* Result code selection and call progress monitoring control X */
    "+CRC",    /* Cellular result codes +CRC */
    "\\S",    /* Get at command settings \S */
    "D",    /* 呼叫发起命令D */
    "A",    /* 接听命令A */
    "+CHUP",    /* 呼叫挂断命令+CHUP */
    "+VTS",    /* 发送DTMF音+VTS */
    "+CSTA",    /* Select type of address +CSTA */
    "+CBST",    /* Select bearer service type +CBST */
    "O",    /* Return to Data State O */
    "+++",    /* Switch data mode between command mode +++ */
    "+COPS",    /* 运营商选择命令+COPS */
    "+CREG",    /* 网络注册+CREG */
    "+CLCK",    /* 设备锁命令+CLCK */
    "+CPWD",    /* 用于修改"+CLCK 定义的设备锁定密码 */
    "+CLIR",    /* 主叫号码限制+CLIR */
    "+CLIP",    /* 主叫号码显示+CLIP */
    "+CLCC",    /* 呼叫状态查询命令+CLCC */
    "+CCFC",    /* 呼叫转移+CCFC */
    "+CCWA",    /* 呼叫等待命令+CCWA */
    "+CHLD",    /* 呼叫保持命令+CHLD */
    "+CSSN",    /* 补充业务指示上报设置+CSSN */
    "+CUSD",    /* USSD命令+CUSD */
    "+CNUM",    /* 用户号码+CNUM */
    "+CPOL",    /* 优先网络列表+CPOL */
    "+CPLS",    /* 选择优先网络列表+CPLS */
    "+CCED",    /* Query cell environment +CCED */
    "^NWINFO",    /* 1 Query network information ^NWINFO */
    "&C",    /* Set carrier detection (DCD) line mode &C */
    "&D",    /* Set DTE ready (DTR) line mode &D */
    "&S",    /* Set data set ready (DSR) line mode &S */
    "+ICF",    /* Set character framing +ICF */
    "+IPR",    /* Set fixed data rate +IPR */
    "+IFC",    /* Control local flow +IFC */
    "+CFUN",    /* 操作模式设置命令+CFUN */
    "+CPIN",    /* PIN管理命令+CPIN */
    "+CBC",    /* 查询电池电量+CBC */
    "+CSQ",    /* 信号质量命令+CSQ */
    "+CLVL",    /* 耳机音量设置命令+CLVL */
    "+CMUT",    /* 麦克静音设置命令+CMUT */
    "+CPBS",    /* 电话本内存选择命令+CPBS */
    "+CPBF",    /* 电话本查找命令+CPBF */
    "+CPBR",    /* 电话本读取命令+CPBR */
    "+CPBW",    /* 电话本写命令+CPBW */
    "+CRSM",    /* 受限SIM卡访问命令+CRSM */
    "+CPROT",    /* 通讯协议切换命令+CPROT */
    "+CMEE",    /* 终端报错命令+CMEE */
    "+CGDCONT",    /* 定义PDP上下文+CGDCONT */
    "+CGACT",    /* PDP上下文激活或去激活+CGACT */
    "+CGATT",    /* PS域附着或分离+CGATT */
    "+CGEQNEG",    /* 2G协商服务质量查询命令+CGEQNEG */
    "H",    /* 挂机命令H */
    "+CGREG",    /* PS域注册状态+CGREG */
    "+CGSMS",    /* 短信承载域命令+CGSMS */
    "+CGDATA",    /* Enter data state +CGDATA */
    "+CGDSCONT",    /* 定义Secondary PDP上下文+CGDSCONT */
    "+CGPADDR",    /* Show PDP address(es) +CGPADDR */
    "+CGEQREQ",    /* 设置QoS 参数：+CGEQREQ */
    "+CGEQMIN",    /* 设置最小QoS：+CGEQMIN */
    "+CGQMIN",    /* Quality of Service Profile (Minimum acceptable) +CGQMIN */
    "+CGQREQ",    /* Quality of Service Profile (Requested) +CGQREQ */
    "+CSMS",    /* 选择短信服务类型命令+CSMS */
    "+CPMS",    /* 短信存储介质选择命令+CPMS */
    "+CMGF",    /* 设置短信格式命令+CMGF */
    "+CSCA",    /* 短信服务中心号码命令+CSCA */
    "+CSMP",    /* 设置Text Mode参数+CSMP */
    "+CSDH",    /* 显示Text Mode的参数+CSDH */
    "+CSCB",    /* 选择小区消息类型+CSCB */
    "+CNMI",    /* 新短信通知设置命令+CNMI */
    "+CMGD",    /* 删除短信命令+CMGD */
    "+CMMS",    /* 短信链路控制命令+CMMS */
    "+CNMA",    /* 新短信确认命令+CNMA */
    "+CMGL",    /* 短信列表命令+CMGL */
    "+CMGR",    /* 读取一条短信命令+CMGR */
    "+CMGS",    /* 短信发送命令+CMGS */
    "+CMGW",    /* 短信存储命令+CMGW */
    "+CMSS",    /* Send Message from Storage +CMSS */
    "+TCOM",    /* 通迅协议切换命令+TCOM */
    "^HS",    /* TE与MT握手功能命令^HS */
    "^SPN",    /* SPN读取功能命令^SPN */
    "^HWVER",    /* 硬件版本号查询命令^HWVER */
    "^HVER",    /* 硬件版本号查询命令^HVER */
    "^CARDMODE",    /* SIM/USIM卡模式识别^CARDMODE */
    "^SN",    /* 查询产品SN号^SN */
    "^CPIN",    /* PIN管理命令^CPIN */
    "^CARDLOCK",    /* 运营商锁^CARDLOCK */
    "^GLASTERR",    /* 查询指定功能项最后的错误码^GLASTERR */
    "^ADCTEMP",    /* 热保护门限设定^ADCTEMP */
    "^SETPID",    /* Linux下端口切换^SETPID */
    "^RFSWITCH",    /* W_DISABLE#管脚状态查询以及关闭射频命令^RFSWITCH */
    "^DIALMODE",    /* Modem/NDIS支持形态查询接口^DIALMODE */
    "^PAD",    /* 射频PAD器件查询^PAD */
    "+CPLG",    /* 文件夹名称语言种类切换命令+CPLG */
    "^PLID",    /* 检测和新建播放列表命令^PLID */
    "^FPLOCK",    /* 指纹设备锁状态查询命令^FPLOCK */
    "^FPVERIFYIND",    /* 指纹验证结果上报命令^FPVERIFYIND */
    "^RSTRIGGER",    /* 业务平台激活状态标志^RSTRIGGER */
    "^RSFR",    /* 读数据文件^RSFR */
    "^RSFW",    /* 写数据文件^RSFW */
    "^SDRWCFG",    /* 查询及设置SD分区读写属性^SDRWCFG */
    "^DATACLASS",    /* UE支持的最高data class ^DATACLASS */
    "^RRCVer",    /* 3GPP RRC 版本号设置命令 ^RRCVer */
    "^CMDLEN",    /* AT命令长度查询设置命令^CMDLEN */
    "^DEVERRQRY",    /* 单板错误状态查询命令^DEVERRQRY */
    "@BCSMODE",    /* 小区广播维护模式切换命令AT@BCSMODE */
    "^BCSSWITCH",    /* 小区广播开关命令^BCSSWITCH */
    "^CURC",    /* 模块主动上报控制命令：^CURC */
    "^SIQ",    /* 信号源信息查询命令^SIQ */
    "^NSI",    /* 小区信息查询命令^NSI */
    "^BSINFO",    /* 基站ID和协议版本号查询命令^BSINFO */
    "^MDN",    /* 本机号码设置命令^MDN */
    "^NTCT",    /* 网络侧时间主动上报使能控制命令^NTCT */
    "^NWTIME",    /* 网络侧系统时间查询和主动上报命令^NWTIME */
    "^VTD",    /* DTMF音占空比设置命令AT^VTD */
    "+CTA",    /* 数据业务进入休眠模式等待时长的设置命令+CTA */
    "^SIMPWR",    /* SIM卡下电命令^ SIMPWR */
    "^ANTDTCT",    /* 查询单板的天线开路状态及主动上报命令 */
    "^FASTDORM",    /* Fast Dormancy设置^FASTDORM */
    "^VMSET",    /* 设置音频场景：^VMSET */
    "^IMEISV",    /* IMEISV查询命令^IMEISV */
    "^UDSTART",    /* 升级启动命令^UDSTART */
    "^THERMFUN",    /* 设置温度保护功能开启及关闭命令^THERMFUN */
    "^BODYSARON",    /* 开关BODY SAR功能 ^BODYSARON */
    "^BODYSARWCDMA",    /* 设定WCDMA最大发射功率门限值 ^BODYSARWCDMA */
    "^BODYSARGSM",    /* 设定GSM最大发射功率门限值 ^BODYSARGSM */
    "^BODYSARCDMA",    /* 设定CDMA最大发射功率门限值 ^BODYSARCDMA */
    "^BODYSARLTE",    /* 设定LTE最大发射功率门限值 ^BODYSARLTE */
    "^BODYSARTDSCDMA",    /* Set the Maximum Tx Power Limit of TD-SCDMA ^BODYSARTDSCDMA */
    "^CHIPTEMP",    /* 查询PA/SIM/电池/晶振温度：^CHIPTEMP */
    "^ADCREAD",    /* 查询ADC读数命令^ADCREAD */
    "^MTCARRIER",    /* Mulit Carrier特性运营商查询及设置命令 ^ MTCARRIER */
    "^MTCDEFAULT",    /* Mulit Carrier 默认运营商查询、设置及使能命令 ^MTCDEFAULT */
    "^MTCAUTOSEL",    /* Mulit Carrier SIM卡自动切换运营商使能命令 ^MTCAUTOSEL */
    "^PWROFFCFG",    /* 关机延迟时间配置命令AT^ PWROFFCFG */
    "^HGMR",    /* 查询版本号：^HGMR */
    "^ANTMODE",    /* 主分集天线工作方式设置命令 ^ANTMODE */
    "^REALNAME",    /* 产品名称功能查询命令^REALNAME */
    "^BIPSTATUS",    /* BIP状态查询命令^BIPSTATUS */
    "^HWCUST",    /* Enable custom hardware function ^HWCUST */
    "^CUSTCAP",    /* 研发可配置支持能力查询AT：^CUSTCAP */
    "^SUPINFO",    /* 发送当前时间和软件版本号命令 ^ SUPINFO */
    "^CHKUPINFO",    /* 查询升级信息记录命令^ CHKUPINFO */
    "^PORTPW",    /* SD卡端口锁定制：^PORTPW */
    "^MDON",    /* LOG功能开关命令^MDON */
    "^BODYSARCFG",    /* Set the bodysar type to limit the power of ^BODYSARCFG */
    "^JDETEX",    /* AT^JDETEX–Command for Jammer Detection */
    "^JDET",    /* AT^JDET–Command for Jammer Detection */
    "^SIGNVER",    /* 软件版本签名校验命令^SIGNVER */
    "^DRCINDRPT",    /* DRC期望速率查询^DRCINDRPT */
    "^SYSPARMIND",    /* 查询Field Test参数:^SYSPARMIND */
    "^EMERGCFG",    /* AT^EMERGCFG–紧急呼叫参数配置命令 */
    "^HFDOR",    /* Fast Dormancy模式控制命令 ^HFDOR */
    "^DTMF",    /* 二次拨号命令^DTMF */
    "^CDUR",    /* 通话时长查询命令^CDUR */
    "^AUTOANSWER",    /* Auto Answer ^AUTOANSWER */
    "^CVOICE",    /* 语音模式切换命令^CVOICE */
    "^DDSETEX",    /* 设置语音输出端口命令^DDSETEX */
    "^SWSPATH",    /* Switch sound path ^SWSPATH */
    "+CMIC",    /* Tune microphone gain level +CMIC */
    "^ECHOPARA",    /* 设置抑制回音的参数值 ^ECHOPARA */
    "^MWIMSG",    /* Query and report voice mail message ^MWIMSG */
    "^HSMF",    /* 3GPP/3GPP1多模终端应用WebSDK短信存储状态命令：^HSMF */
    "^HCPMS",    /* 短信存储选择命令^ HCPMS */
    "^HCMGF",    /* 设置短信格式命令^ HCMGF */
    "^HCMGD",    /* 删除短信命令^ HCMGD */
    "^DSFLOWCLR",    /* DS流量清零命令^DSFLOWCLR */
    "^DSFLOWQRY",    /* DS流量查询命令^DSFLOWQRY */
    "^DSFLOWRPT",    /* DS流量上报^DSFLOWRPT */
    "^CPBR",    /* 电话本读取命令^CPBR */
    "^CPBW",    /* 电话本写命令^CPBW */
    "^SCPBR",    /* 电话本读取命令^SCPBR */
    "^SCPBW",    /* 电话本写命令^SCPBW */
    "^ECCLIST",    /* 紧急号码主动上报与查询 ^ECCLIST */
    "^SYSINFO",    /* 系统的信息查询命令^SYSINFO */
    "^SYSCFG",    /* 系统配置参考设置命令^SYSCFG */
    "^SYSCONFIG",    /* 系统配置参考设置命令^SYSCONFIG */
    "^FREQLOCK",    /* 锁频命令^FREQLOCK */
    "^HSDPA",    /* HSDPA Enable Command ^HSDPA */
    "^HSUPA",    /* HSUPA Enable Command ^HSUPA */
    "^HSPA",    /* WCDMA RRC版本命令^HSPA */
    "^PNN",    /* PNN读取命令^PNN */
    "^OPL",    /* OPL读取命令^OPL */
    "^CSNR",    /* RSCP和ECIO查询^CSNR */
    "^USSDMODE",    /* 切换USSD命令 ^USSDMODE */
    "^SYSINFOEX",    /* 扩展系统信息查询命令^SYSINFOEX */
    "^SYSCFGEX",    /* 扩展系统配置参考设置命令^SYSCFGEX */
    "^LTERSRP",    /* RSRP和RSRQ查询上报命令^LTERSRP */
    "^LTESCINFO",    /* 服务小区信息查询命令^LTESCINFO */
    "^LTECAT",    /* LTE服务等级查询命令^LTECAT */
    "^HWNATQRY",    /* 网络模式查询^HWNATQRY */
    "^HWNAT",    /* 网络模式变化指示^HWNAT */
    "^WAKEUPCFG",    /* 远程唤醒配置AT命令^WAKEUPCFG */
    "^LOCCHD",    /* Indicate Location Area Change ^LOCCHD */
    "^EONS",    /* 运营商网络名字查询命令：^EONS */
    "^HCSQ",    /* 信号查询上报命令 ^HCSQ */
    "^HFREQINFO",    /* 服务小区频点查询上报命令^HFREQINFO */
    "^CNWI",    /* 查询当前注册CDMA网络标识 ^CNWI */
    "^SLEEPCFG",    /* 睡眠参数设置命令：^SLEEPCFG */
    "^PORTSEL",    /* 主动事件上报口设置命令^PORTSEL */
    "^U2DIAG",    /* U盘与Diag端口切换控制命令^U1DIAG */
    "^BTSET",    /* 设置语音输出端口命令^BTSET */
    "^GETPORTMODE",    /* 网关获取数据卡类型及端口顺序的命令^GETPORTMODE */
    "^SETPORT",    /* 端口形态查询命令^SETPORT */
    "^STSF",    /* STK设置工具 ^STSF */
    "^STGR",    /* STK给予响应^STGR */
    "^STGI",    /* 获取信息命令数据^STGI */
    "^CUSATM",    /* 查询菜单命令 ^CUSATM */
    "^DHCP",    /* DHCP/IP查询指令 ^DHCP */
    "^NDISDUP",    /* NDIS拨号命令^NDISDUP */
    "^AUTHDATA",    /* 设置用户名密码^AUTHDATA（借用$QCPDPP） */
    "^CRPN",    /* 运营商查询命令^CRPN */
    "^ICCID",    /* ICCID查询命令^ICCID */
    "^NDISCONN",    /* NDIS拨号命令^NDISCONN */
    "^NDISADD",    /* NDIS地址设置命令^NDISADD */
    "^NDISSTATQRY",    /* 连接状态查询命令 ^NDISSTATQRY */
    "^IPV6CAP",    /* IPv6能力查询 ^IPV5CAP */
    "^DHCPV6",    /* DHCPV6信息查询 ^DHCPV5 */
    "^DNSP",    /* Set the primary DNS server address ^DNSP */
    "^DNSS",    /* Set the secondary DNS server address ^DNSS */
    "^APRAINFO",    /* 查询IPv6 Router参数 ^APRAINFO */
    "^ROAMPDPFB",    /* 设置/查询国际漫游拨号回退方案 ^ROAMPDPFB */
    "^IPINIT",    /* TCP/UDP连接初始化命令 ^IPINIT */
    "^IPOPEN",    /* 建立TCP/UDP链接命令 ^IPOPEN */
    "^IPLISTEN",    /* 服务器侦听命令 ^IPLISTEN */
    "^IPSEND",    /* TCP/UDP数据发送命令 ^IPSEND */
    "^IPDATA",    /* TCP/UDP数据到达指示 ^IPDATA */
    "^IPCLOSE",    /* 关闭TCP/UDP链接命令 ^IPCLOSE */
    "^IPENTRANS",    /* 进入透明传输模式命令 ^IPENTRANS */
    "^IPSENDEX",    /* TCP/UDP数据发送扩展命令 ^IPSENDEX */
    "^IPFLOWQ",    /* TCP/UDP链接数据包统计数据查询/清除命令^IPFLOWQ */
    "^IPCFL",    /* TCP/UDP静态参数配置命令 ^IPCFL */
    "^DVCFG",    /* ^DVCFG–Command for Setting Priority of Voice Call and Data Service */
    "^WPDOM",    /* AT^WPDOM–Set Operation Mode */
    "^WPDST",    /* AT^WPDST–Set Session Type */
    "^WPDFR",    /* AT^WPDFR–Set Positioning Frequency */
    "^WPQOS",    /* AT^WPQOS–Set QoS */
    "^WPDGL",    /* AT^WPDGL–Set GPS Session Lock */
    "^WPURL",    /* AT^WPURL–Set AGPS Server Address and Port on the 2GPP Network */
    "^WPDIM",    /* AT^WPDIM–Delete Auxiliary Data */
    "^WPDGP",    /* AT^WPDGP–Start Positioning Session */
    "^WPEND",    /* AT^WPEND–Terminate Positioning Process */
    "^WNICT",    /* AT^WNICT–Set NI Response */
    "^WPCAP",    /* AT^WPCAP–Disable/Enable GNSS System */
    "^AGNSSCFG",    /* AT^AGNSSCFG–Set an AGNSS System's Capabilities */
    "^WPTLS",    /* AT^WPTLS–Set TLS Certificate */
    "^WPPORT",    /* AT^WPPORT-Set the NMEA Sentence and GPS URC Output Port */
    "^WPINFO",    /* AT^WPINFO–Get GNSS Engine Status */
    "^NISMSFWD",    /* AT^NISMSFWD–Control the Report of SUPL NI Short Messages */
    "^INJTIME",    /* AT^INJTIME–Inject UTC/GPS Reference Time */
    "^INJPOS",    /* AT^INJPOS–Inject Reference Location Information */
    "^XTRATIME",    /* AT^XTRATIME–Inject XTRA Time */
    "^XTRADATA",    /* AT^XTRADATA–Inject Auxiliary XTRA Data */
    "^XTRASTA",    /* AT^XTRASTA–Query XTRA Data Status */
    "^XTRALOCK",    /* AT^XTRALOCK–Enable/Disable the XTRA Feature */
    "^ECLSTART",    /* eCall会话发起命令 ^ECLSTART */
    "^ECLSTOP",    /* eCall会话断开命令 ^ECLSTOP */
    "^ECLCFG",    /* eCall功能参数配置 ^ECLCFG */
    "^ECLMSD",    /* MSD数据透传设置命令 ^ECLMSD */
    "^AMRCFG",    /* 配置AMR编解码支持方式命令^AMRCFG */
    "+XTSM",    /* Set thermal sensor with the threshold +XTSM */
    "+XTS",    /* The URC denotes the threshold is reached +XTS */
    "+XTAMR",    /* Query the current temperature of a thermal sensor +XTAMR */
    "+XADPCLKFREQINFO",    /* Adaptive Clock Frequency Info +XADPCLKFREQINFO */
    "^AUTHVERIFY",    /* 鉴权 AT^AUTHVERIFY */
    "^DATAMODE",    /* 切换到数据模式命令^ DATAMODE */
    "^HVSST",    /* vSIM卡状态命令^HVSST */
    "^HVSDH",    /* vSIM卡DH密钥协商命令^HVSDH */
    "^HVSCONT",    /* vSIM卡数据查询命令^HVSCONT */
    "^HVPDH",    /* vSIM卡客户端DH公私钥命令^HVPDH */
    "^PLMN",    /* 驻留网络PLMN指示^PLMN */
    "^FOTAMODE",    /* 操作模式设置命令 ^FOTAMODE */
    "^FOTACFG",    /* FOTA连接参数设置 ^FOTACFG */
    "^FOTADET",    /* 手动版本检测命令 ^FOTADET */
    "^FOTADL",    /* 手动版本下载命令 ^FOTADL */
    "^FWUP",    /* 版本升级控制命令，此命令用于启动FOTA升级 ^FWUP */
    "^FOTASTATE",    /* FOTA状态上报 ^FOTASTATE */
    "^FOTADLQ",    /* 升级文件下载状态查询 ^FOTADLQ */
    "^FOTAP",    /* FOTA升级策略 */
    "^OMASERVER",    /* AT^OMASERVER-配置OMA服务器参数 */
    "^OMACONCFG",    /* AT^OMACONCFG-配置OMA拨号参数 */
    "^OMAMODE",    /* AT^OMAMODE-OMA DM操作模式设置 */
    "^OMASESSCFG",    /* AT^OMASESSCFG-OMA DM会话使能控制 */
    "^OMACTRL",    /* AT^OMACTRL-发起/关闭OMA DM会话 */
    "^OMAANSR",    /* AT^OMAANSR–OMA会话用户应答 */
    "^OMADLQ",    /* AT^OMADLQ 版本下载的时候进度查询 */
    "^LTEPROFILE",    /* LTE定制注册APN设置和查询^LTEPROFILE */
    "^TXPOWER",    /* 获取txpower参数命令 */
    "^LTEMCS",    /* 获取MCS(调制阶数)参数命令 */
    "^LTETDDSUBFRAME",    /* 获取tdd的上下行子帧配比和特殊子帧格式 */
    "+CSIM",    /* SIM卡文件读写命令AT+CSIM */
    "+CIREG",    /* 获取IMS的注册状态AT+CIREG */
    "^SPWORD",    /*  */
    "^DLOADVER",    /* 查询下载协议版本命令^DLOADVER */
    "^DLOADINFO",    /* 获取单板版信息命令^DLOADINFO */
    "^NVBACKUP",    /* NV备份命令^NVBACKUP */
    "^NVRESTORE",    /* NV恢复命令^NVRESTORE */
    "^AUTHORITYVER",    /* 查询鉴权方式^AUTHORITYVER */
    "^AUTHORITYID",    /* 查询鉴权标识 ^AUTHORITYID */
    "^GODLOAD",    /* 切换到下载模式命令^GODLOAD */
    "^RESET",    /* 单板重启命令^ RESET */
    "^NVRSTSTTS",    /* NV 自动恢复状态查询命令^ NVRSTSTTS */
    "^TEMPINFO",    /* 获取电池、SIM卡等温度信息 */
    "^LTEANTINFO",    /* 查询LTE各BAND天线状态 */
    "^NWSCAN",    /* 网络扫频命令^NWSCAN */
    "^ACCOUNT",    /* VDF付费方式设置查询 */
    "^AT2OM",    /* 从AT切换到OM端口 */
    "^CQST",    /* 设置是否快速开机模式 */
    "^FTYRESET",    /* 设置或查询恢复出厂级别 */
    "^NVRDWI2G",    /* 从NV中读取2.4G天线的校准参数 */
    "^NVRDWI5G",    /* 从NV中读取5G天线的校准参数 */
    "^NVWRWI2G",    /* 写入2.4G天线的校准参数到NV */
    "^NVWRWI5G",    /* 写入5G天线的校准参数到NV */
    "^SELFTEST",    /* slic芯片自检 */
    "^SETHWLOCK",    /* 设置simlock或datalock */
    "^TESTHWLOCK",    /* 检测simlock或datalock */
    "^TLEDSWITCH",    /* 用于led通路切换 */
    "^WIPAVARS2G",    /* 读取2.4G校准参数 */
    "^WIPAVARS5G",    /* 读取5G校准参数 */
    "+CPAS",    /* 查询MT当前状态 */
    "+CMOD",    /* 设置呼叫模式 */
    "+COLP",    /* 主动上报连接号码 */
    "+CCUG",    /* 闭合用户群 */
    "+CUUS1",    /* 用户信令服务1 */
    "+CTFR",    /* 呼叫偏转 */
    "+CEREG",    /* EPS域注册状态 */
    "+CGTFT",    /* 设置TFT */
    "+CGCMOD",    /* 修改PDP */
    "+CGANS",    /* 应答PDP */
    "+CGAUTO",    /* 设置PDP激活自动应答 */
    "+CGCONTRDP",    /* 读取EPS缺省承载参数 */
    "+CGSCONTRDP",    /* 读取EPS专用承载参数 */
    "+CGTFTRDP",    /* 读取EPSTFT参数 */
    "+CGEQOS",    /* 定义EPSQoS参数 */
    "+CGEQOSRDP",    /* 读取EPSQoS参数 */
    "+COPN",    /* 读取运营商名称 */
    "+CIREP",    /* 查询SRVCC状态： */
    "+CEVDP",    /* 设置语音呼叫优选模式： */
    "+CEUS",    /* 设置语音中心数据中心： */
    "^CPNN",    /* 读取当前注册网络对应的PNN： */
    "^OPWORD",    /* 获取设置DIAG 口和SHELL 口权限 */
    "^CPWORD",    /* 释放设置权限 */
    "^DISLOG",    /* 打开/关闭DIAG 口 */
    "^SHELL",    /* 配置/关闭/打开SHELL 口 */
    "^CELLROAM",    /* cellroam特性 */
    "+CSQLVL",    /* 查询信号强度 */
    "^CELLINFO",    /* 查询WCDMA主小区邻区测量信息 */
    "^CGCATT",    /* CS、PS域附着 */
    "^CERSSI",    /* 指示RSSI 变化 */
    "^CECELLID",    /* 查询4G 下小区ID 参数 */
    "^CFPLMN",    /* 禁止PLMN列表操作 */
    "^FDAC",    /*  */
    "^FPA",    /* 设置发射机PA的等级 */
    "^MDATE",    /* 设置生产日期 */
    "^TSCREEN",    /*  */
    "^TBATVOLT",    /*  */
    "^HUK",    /*  */
    "^FACAUTHPUBKEY",    /*  */
    "^IDENTIFYSTART",    /*  */
    "^IDENTIFYEND",    /*  */
    "^SIMLOCKDATAWRITE",    /*  */
    "^PHONESIMLOCKINFO",    /*  */
    "^SIMLOCKDATAREAD",    /*  */
    "^PHONEPHYNUM",    /*  */
    "^PORTATTRIBSET",    /*  */
    "^FPLLSTATUS",    /*  */
    "^MEID",    /*  */
    "^BANDSW",    /*  */
    "^FCHANS",    /*  */
    "^FSEGMENT",    /*  */
    "^FPOWS",    /*  */
    "^FPAS",    /*  */
    "^FLNAS",    /*  */
    "^FTXWAVE",    /*  */
    "^FSTART",    /*  */
    "^FRSSIS",    /*  */
    "^LWCLASH",    /* 查询LTE网络频率信息：AT */
    "^LCACELL",    /* LTE 小区CA状态查询 */
    "^WICAL",    /*  */
    "^WICALDATA",    /*  */
    "^WICALTEMP",    /*  */
    "^WICALFREQ",    /*  */
    "^WICALPOW",    /*  */
    "^NAVTYPE",    /*  */
    "^NAVENABLE",    /*  */
    "^GPIOLOOP",    /* M2M GPIO环回测试 */
    "+CMUX",    /* M2M高速串口测试 */
    "^BIPCMD",    /* bip功能测试需要 */
    "^HFEATURESTAT",    /* v722主线 ，E5生效 */
    "^MODEMLOOP",    /* 语音换回 */
    "^DCONNSTAT",    /* 查询当前拨号状态 */
    "^RRCSTAT",    /* 查询rrc状态 */
    "^LCELLINFO",    /* 查询4G 下小区ID 参数 */
    "^SETKEY",    /* secboot特性，设置单板模式 */
    "^GETKEYINFO",    /* 获取相关密码 */
    "^NFCCFG",    /* NFC设置和查询命令 */
};


const VOS_CHAR * MbbAtCmdAllowGetTable(VOS_VOID)
{
    return gszAllowCmdTable;
}


VOS_UINT32 MbbAtCmdAllowGetTableCount(VOS_VOID)
{
    return (sizeof(gszAllowCmdTable) / sizeof(gszAllowCmdTable[0]));
}


/******************************************************************************/
/***************************** MBB AT 二级白名单列表结束 **********************/
/******************************************************************************/


