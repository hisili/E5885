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
 
 
#ifndef ATWLANAPI_H
#define ATWLANAPI_H

#ifdef __cplusplus
    #if __cplusplus
    extern "C" {
    #endif
#endif

#ifndef _UINT32_DEFINED
typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
#define _UINT32_DEFINED
#endif

#ifndef _UINT16_DEFINED
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
#define _UINT16_DEFINED
#endif

#ifndef _UINT8_DEFINED
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */
#define _UINT8_DEFINED
#endif

#ifndef _INT32_DEFINED
typedef  signed long int    int32;       /* Signed 32 bit value */
#define _INT32_DEFINED
#endif

#ifndef _INT16_DEFINED
typedef  signed short       int16;       /* Signed 16 bit value */
#define _INT16_DEFINED
#endif

#ifndef _INT8_DEFINED
typedef  char        int8;        /* Signed 8  bit value */
#define _INT8_DEFINED
#endif

/* 函数返回值列表 */
typedef enum
{
    AT_RETURN_FAILURE   =  -1,  /*配置失败*/
    AT_RETURN_SUCCESS   =   0   /*成功*/
}WLAN_AT_RETURN_TYPE;

/* 特性开关的枚举 */
typedef enum
{
    AT_FEATURE_DISABLE = 0,
    AT_FEATURE_ENABLE  = 1
}WLAN_AT_FEATURE_TYPE;

/* Buffer缓存结构 */
#define WLAN_AT_BUFFER_SIZE 512
typedef struct _WLAN_AT_BUFFER
{
    int32 reserve;                /* 保留字段 */
    int8 buf[WLAN_AT_BUFFER_SIZE];/* Buffer缓存，填充字符串带结束标记'\0' */
}WLAN_AT_BUFFER_STRU;

/*组成员信息*/
typedef struct _GROUPMEMBER_STRU
{
    int32 group;                             /*group值表示分组的index值,取值范围 0~255*/
    uint8 content[WLAN_AT_BUFFER_SIZE * 2];  /*对应分组成员的内容*/
}WLAN_AT_GROUPMEMBER_STRU;

/*===========================================================================
 (1)^WIENABLE 设置WiFi模块使能
===========================================================================*/
typedef enum
{
    AT_WIENABLE_OFF  = 0,   /*关闭 */
    AT_WIENABLE_ON   = 1,   /*打开正常模式 信令模式*/
    AT_WIENABLE_TEST = 2    /*打开测试模式（在WT和MT测试时使用该模式进行测试）非信令-系统加载时的默认配置*/
}WLAN_AT_WIENABLE_TYPE;
int32 WlanATSetWifiEnable(WLAN_AT_WIENABLE_TYPE onoff);
WLAN_AT_WIENABLE_TYPE WlanATGetWifiEnable(void);

/*===========================================================================
 (2)^WIMODE 设置WiFi模式参数 目前均为单模式测试
===========================================================================*/
typedef enum
{
    AT_WIMODE_CW      = 0,  /*CW模式      */
    AT_WIMODE_80211a  = 1,  /*802.11a制式 */
    AT_WIMODE_80211b  = 2,  /*802.11b制式 */
    AT_WIMODE_80211g  = 3,  /*802.11g制式 */
    AT_WIMODE_80211n  = 4,  /*802.11n制式 */
    AT_WIMODE_80211ac = 5,  /*802.11ac制式*/
    AT_WIMODE_MAX
}WLAN_AT_WIMODE_TYPE;
int32 WlanATSetWifiMode(WLAN_AT_WIMODE_TYPE mode);
/* 当前模式，以字符串形式返回eg: 2 */
int32 WlanATGetWifiMode(WLAN_AT_BUFFER_STRU *strBuf);
/* 支持的所有模式，以字符串形式返回eg: 2,3,4 */
int32 WlanATGetWifiModeSupport(WLAN_AT_BUFFER_STRU *strBuf);

/*===========================================================================
 (3)^WIBAND 设置、获取WiFi带宽参数
===========================================================================*/
typedef enum
{
    AT_WIBAND_20M   = 0,  /*带宽 20MHz*/
    AT_WIBAND_40M   = 1,  /*带宽 40MHz*/
    AT_WIBAND_80M   = 2,  /*带宽 80MHz*/
    AT_WIBAND_160M  = 3,  /*带宽160MHz*/
}WLAN_AT_WIBAND_TYPE;

int32 WlanATSetWifiBand(WLAN_AT_WIBAND_TYPE width);
/* 当前带宽，以字符串形式返回eg: 0 */
int32 WlanATGetWifiBand(WLAN_AT_BUFFER_STRU *strBuf);
/* 支持带宽，以字符串形式返回eg: 0,1 */
int32 WlanATGetWifiBandSupport(WLAN_AT_BUFFER_STRU *strBuf);

/*===========================================================================
 (4)^WIFREQ 设置WiFi频点
===========================================================================*/
typedef enum
{
    AT_WIFREQ_24G   = 0,  /*频率 2.4G*/
    AT_WIFREQ_50G   = 1,  /*频率 5G*/
}WLAN_AT_WIFREQ_TYPE;
typedef struct _WIFREQ_STRU
{
    uint16 value;   /*WiFi频点，单位为MHz，取值范围为 0～65535*/
    int16 offset;  /*设置频偏信息，单位为KHz，取值范围为-32768～32767，默认传0*/
}WLAN_AT_WIFREQ_STRU;
int32 WlanATSetWifiFreq(WLAN_AT_WIFREQ_STRU *pFreq);
int32 WlanATGetWifiFreq(WLAN_AT_WIFREQ_STRU *pFreq);

/*===========================================================================
 (5)^WIDATARATE 设置和查询当前WiFi速率集速率
  WiFi速率，单位为0.01Mb/s，取值范围为0～65535
===========================================================================*/
int32 WlanATSetWifiDataRate(uint32 rate);
uint32 WlanATGetWifiDataRate(void);

/*===========================================================================
 (6)^WIPOW 来设置WiFi发射功率
   WiFi发射功率，单位为0.01dBm，取值范围为 -32768～32767
===========================================================================*/
int32 WlanATSetWifiPOW(int32 power_dBm_percent);
int32 WlanATGetWifiPOW(void);

/*===========================================================================
 (7)^WITX 设置WiFi发射机状态
===========================================================================*/
int32 WlanATSetWifiTX(WLAN_AT_FEATURE_TYPE onoff);
WLAN_AT_FEATURE_TYPE WlanATGetWifiTX(void);

/*===========================================================================
 (8)^WIRX 设置WiFi接收机开关
===========================================================================*/
#define MAC_ADDRESS_LEN          20       /* MAC地址对应长度 */
typedef struct _WIRX_STRU
{
    WLAN_AT_FEATURE_TYPE onoff;     /*0-关闭 1-打开*/
    uint8 src_mac[MAC_ADDRESS_LEN]; /*可选参数，接收数据时，需要过滤的源的MAC地址，MAC地址需要带冒号*/
    uint8 dst_mac[MAC_ADDRESS_LEN]; /*可选参数，接收数据时，需要过滤的目的MAC地址，MAC地址需要带冒号*/
}WLAN_AT_WIRX_STRU;
int32 WlanATSetWifiRX(WLAN_AT_WIRX_STRU *params);
int32 WlanATGetWifiRX(WLAN_AT_WIRX_STRU *params);

/*===========================================================================
 (9)^WIRPCKG 查询WiFi接收机误包码，上报接收到的包的数量
===========================================================================*/
typedef struct _WIRPCKG_STRU
{
    uint16 good_result; /*单板接收到的好包数，取值范围为0~65535*/
    uint16 bad_result;  /*单板接收到的坏包数，取值范围为0~65535*/
}WLAN_AT_WIRPCKG_STRU;
int32 WlanATSetWifiRPCKG(int32 flag);
int32 WlanATGetWifiRPCKG(WLAN_AT_WIRPCKG_STRU *params);

/*===========================================================================
 (10)^WIINFO 查询WiFi的相关信息
===========================================================================*/
#define MAX_PWR_SIZE                  (8)
#define MAX_CHANNEL24G_SIZE           (32)                    /* 数组存储24个信道和之间逗号的ASCII码值 */
#define MAX_CHANNEL5G_SIZE            (96)
#define SIZE_OF_INFOGROUP(group) (sizeof(group) / sizeof(WLAN_AT_WIINFO_MEMBER_STRU))

#ifndef WLAN_TRACE_INFO
    #define WLAN_TRACE_INFO(fmt, ...)     printk("AT <INFO> [%s:%d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef WLAN_TRACE_ERROR 
    #define WLAN_TRACE_ERROR(fmt, ...)    printk("AT <ERRO> [%s:%d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

typedef struct _WLAN_AT_WIINFO_MEMBER
{
    const char *name;
    const char *value;
}WLAN_AT_WIINFO_MEMBER_STRU;
typedef struct _WLAN_AT_WIINFO_GROUP
{
    WLAN_AT_WIINFO_MEMBER_STRU *member;
    uint32 size;
}WLAN_AT_WIINFO_GROUP_STRU;
typedef enum
{
    AT_WIINFO_CHANNEL  = 0,    /*支持的信道号*/
    AT_WIINFO_POWER    = 1,    /*支持的目标功率*/
    AT_WIINFO_FREQ     = 2,    /*支持的频段*/
}WLAN_AT_WIINFO_TYPE_ENUM;
typedef struct _WIINFO_STRU
{
    WLAN_AT_WIINFO_TYPE_ENUM type; /*查询信息类型，取值范围为 0～255*/
    WLAN_AT_GROUPMEMBER_STRU member;/*其中内容包含多时，以字符串结束标记\0间隔，读取到空字符串结束*/
}WLAN_AT_WIINFO_STRU;

int32 WlanATGetWifiInfo(WLAN_AT_WIINFO_STRU *params);

/*===========================================================================
 (11)^WIPLATFORM 查询WiFi方案平台供应商信息
===========================================================================*/
typedef enum
{
    AT_WIPLATFORM_BROADCOM  = 0,    /* 博通芯片 */
    AT_WIPLATFORM_ATHEROS   = 1,    /* atheros芯片 */
    AT_WIPLATFORM_QUALCOMM  = 2,    /* 高通芯片 */
    AT_WIPLATFORM_TI        = 3,    /* TI芯片 */
    AT_WIPLATFORM_REALTEK   = 4,    /* realtek芯片 */
    AT_WIPLATFORM_HISI      = 5,    /* 海思芯片 */
}WLAN_AT_WIPLATFORM_TYPE;
WLAN_AT_WIPLATFORM_TYPE WlanATGetWifiPlatform(void);

/*===========================================================================
 (12)^TSELRF 查询设置单板的WiFi射频通路
===========================================================================*/
int32 WlanATGetTSELRF(void); /* 获取天线 */
int32 WlanATSetTSELRF(uint32 group);/*设置天线，非多通路传0 */
/* 支持的天线索引序列，以字符串形式返回eg: 0,1,2,3 */
int32 WlanATGetTSELRFSupport(WLAN_AT_BUFFER_STRU *strBuf);

/*===========================================================================
 (13)^WiPARANGE设置、读取WiFi PA的增益情况
===========================================================================*/
typedef enum
{
    AT_WiPARANGE_LOW    = 'l',  /*低增益模式，相当于NO PA*/
    AT_WiPARANGE_HIGH   = 'h',  /*高增益模式，相当于部分平台上的PA模式*/
    AT_WiPARANGE_BUTT   = 0xff /*最大值*/
}WLAN_AT_WiPARANGE_TYPE;
int32 WlanATSetWifiParange(WLAN_AT_WiPARANGE_TYPE pa_type);
WLAN_AT_WiPARANGE_TYPE WlanATGetWifiParange(void);

/* 支持的pa模式序列，以字符串形式返回eg: l,h */
int32 WlanATGetWifiParangeSupport(WLAN_AT_BUFFER_STRU *strBuf);

/*===========================================================================
 (14)^WICALTEMP设置、读取WiFi的温度补偿值
===========================================================================*/
typedef struct _AT_WICALTEMP_STRU
{
    int32 index;    /* 索引值，取值范围为 0～255 */
    int32 value;    /* 温度校准值，取值 -32768～32767 */
}WLAN_AT_WICALTEMP_STRU;
int32 WlanATGetWifiCalTemp(WLAN_AT_WICALTEMP_STRU *params);
int32 WlanATSetWifiCalTemp(WLAN_AT_WICALTEMP_STRU *params);

/*===========================================================================
 (15)^WICALDATA设置、读取指定类型的WiFi补偿数据
===========================================================================*/
typedef enum
{
    AT_WICALDATA_REL_CAL_POW   = 0,  /* 相对功率校准值（单位：dB） */
    AT_WICALDATA_ABS_CAL_POW   = 1,  /* 绝对功率校准值（单位：dBm）*/
    AT_WICALDATA_REL_IDX_POW   = 2,  /* 相对功率数字值（如Index值）*/
    AT_WICALDATA_ABS_IDX_POW   = 3,  /* 绝对功率数字值（如Index值）*/
    AT_WICALDATA_ABS_CAL_FREQ  = 4,  /* 频率补偿模拟值（单位：KHz）*/
    AT_WICALDATA_ABS_IDX_FREQ  = 5,  /* 频率补偿数字值（如Index值）*/
    AT_WICALDATA_ABS_CAL_TEMP  = 6,  /* 温度校准模拟值（单位：0.1摄氏度）*/
    AT_WICALDATA_ABS_IDX_TEMP  = 7,  /* 温度校准数字值（如Index值）*/
}WLAN_AT_WICALDATA_TYPE;
typedef struct _AT_WICALDATA_STRU
{
    WLAN_AT_WICALDATA_TYPE type;     /* 校准类型 */
    uint32 group;                    /* 天线索引 */
    WLAN_AT_WIMODE_TYPE mode;        /* 接入模式 */
    WLAN_AT_WIFREQ_TYPE band;        /* 频段信息 */
    WLAN_AT_WIBAND_TYPE bandwidth;  /* 带宽信息 */
    uint32 freq;                     /* 频率信息 */
    char  data[128];                /* 信息对应携带数据，数据最大长度128字节 */
}WLAN_AT_WICALDATA_STRU;
int32 WlanATSetWifiCalData(WLAN_AT_WICALDATA_STRU *params);
int32 WlanATGetWifiCalData(WLAN_AT_WICALDATA_STRU *params);

/*===========================================================================
 (16)^WICAL设置、读取校准的启动状态，是否支持补偿
===========================================================================*/
int32 WlanATSetWifiCal(WLAN_AT_FEATURE_TYPE onoff);
WLAN_AT_FEATURE_TYPE WlanATGetWifiCal(void);
WLAN_AT_FEATURE_TYPE WlanATGetWifiCalSupport(void);

/*===========================================================================
 (17)^WICALFREQ 设置、查询频率补偿值
===========================================================================*/
typedef enum
{
    AT_WICALFREQ_INDEX   = 0,  /* 频率对应索引 */
    AT_WICALFREQ_FREQ    = 1,  /* 具体频率值 */
}WLAN_AT_WICALFREQ_TYPE;
typedef struct _AT_WICALFREQ_STRU
{
    WLAN_AT_WICALFREQ_TYPE type;     /* 频率值类型 */
    int32 value;                     /* 对应值，-32768~32767 */
}WLAN_AT_WICALFREQ_STRU;
int32 WlanATSetWifiCalFreq(WLAN_AT_WICALFREQ_STRU *params);
int32 WlanATGetWifiCalFreq(WLAN_AT_WICALFREQ_STRU *params);

/*===========================================================================
 (18)^WICALPOW 设置、查询功率补偿值
===========================================================================*/
typedef enum
{
    AT_WICALPOW_INDEX   = 0,  /* 功率对应索引 */
    AT_WICALPOW_POWER   = 1,  /* 具体功率值 */
}WLAN_AT_WICALPOW_TYPE;
typedef struct _AT_WICALPOW_STRU
{
    WLAN_AT_WICALPOW_TYPE type;     /* 频率值类型 */
    int32 value;                     /* 对应值，-32768~32767 */
}WLAN_AT_WICALPOW_STRU;
int32 WlanATSetWifiCalPOW(WLAN_AT_WICALPOW_STRU *params);
int32 WlanATGetWifiCalPOW(WLAN_AT_WICALPOW_STRU *params);

extern int wlan_run_shell(const char *pshell);
extern void wifi_power_off_4356(void);

/*===========================================================================
 (19)^WIPAVARS2G 设置、查询功率补偿值
===========================================================================*/
#define     WIFI_2G_RF_GROUPS_PARA_NUMBER    (3)     /* 每组数据有三个参数 */
#define     WIFI_2G_RF_GROUP_PARA_LEN        (8)     /* 定义每个参数的缓冲区大小为八个字节 */

typedef struct _AT_PAVARS2G_STRU
{    
    int32 ANT_Index;      /*代表ANT0,1,2...*/
    char  data[WIFI_2G_RF_GROUPS_PARA_NUMBER][WIFI_2G_RF_GROUP_PARA_LEN];    /*16进制射频参数*/
}WLAN_AT_PAVARS2G_STRU;

int32 WlanATGetWifi2GPavars(WLAN_AT_BUFFER_STRU *params);
int32 WlanATSetWifi2GPavars(WLAN_AT_PAVARS2G_STRU *params);
/*===========================================================================
 (20)^WIPAVARS5G 设置、查询功率补偿值
===========================================================================*/
#define     WIFI_5G_RF_GROUPS_PARA_NUMBER    (12)    /* 每组数据有十二个参数 */
#define     WIFI_5G_RF_GROUP_PARA_LEN        (8)     /* 定义每个参数的缓冲区大小为八个字节 */

typedef struct _AT_PAVARS5G_STRU
{    
    int32 ANT_Index;      /*代表ANT0,1,2...*/
    char  data[WIFI_5G_RF_GROUPS_PARA_NUMBER][WIFI_5G_RF_GROUP_PARA_LEN];    /*16进制射频参数*/
}WLAN_AT_PAVARS5G_STRU;

int32 WlanATGetWifi5GPavars(WLAN_AT_BUFFER_STRU *params);
int32 WlanATSetWifi5GPavars(WLAN_AT_PAVARS5G_STRU *params);
/*===========================================================================
 (21)^WIFIDEBUG 读写WIFI寄存器的相关信息
===========================================================================*/
typedef enum
{
    AT_WIDEBUG_REG_READ         = 1,  /* 单个或者连续寄存器读操作 */
    AT_WIDEBUG_REG_WRITE        = 2,  /* 单个寄存器写操作 */
    AT_WIDEBUG_REG_FILE         = 3,  /* 驱动寄存器文件写操作 */
    AT_WIDEBUG_TEM_COM_SWTICH   = 4,  /* 温补开关 */
    AT_WIDEBUG_CALI_INTO_READ   = 5,  /* 打印上电校准和动态校准时的相关参数 */
}WLAN_AT_DEBUG_TYPE;

/*****************************************************************************
 函数名称  : int32 AT_SetWifiDebug(WLAN_AT_DEBUG_TYPE debug_type, uint32 value1, uint32 *value2)
 功能描述  :^WIFIDEBUG 读写WIFI寄存器的相关信息
 输入参数  :
    <debug_type> 查询类型1~5，可根据需求扩展功能
        1 单个或者连续寄存器读操作
        2 单个寄存器写操作
        3 读取所有寄存器值并写入文件(写入文件为/online/hal_all_reg_data*.txt)
        4 温补开关
        5 打印上电校准和动态校准时的相关参数 
    <value1> 输入配置，当type=1和2表示寄存器地址，type=3为空，其余为自定义配置；
    <value2> 输入/输出配置，当type=1和2表示寄存器值，type=3为空，其余为自定义配置；
 输出参数  : NA
 返 回 值  : WLAN_AT_RETURN_TYPE
 其他说明  : NA
*****************************************************************************/
int32 WlanATSetWifiDebug(WLAN_AT_DEBUG_TYPE debug_type, uint32 vaule1, uint32 *valule2);

#define HAL_DEVICE_CALI_2G_PATH_NUM    (2)          /* 2G支持2种saw */
#define WITP_CHANNEL_SUPPORT_NUMS      (2)          /* 芯片支持最大2根天线 */
#define EQUIPMENT_TEST_5G_SUB_BAND_NUM (4)          /* 装备测试5G 4个不同band */
typedef struct
{
    /* RC/R/C */
    uint8    uc_rc_code_40M ;
    uint8    uc_rc_code_20M ;
    uint8    uc_r_code      ;
    uint8    uc_c_code      ;

    /* 2G */
    /* RXDC */
    uint16   us_ana_rxdc[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint16   us_dig_rxdc_i[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint16   us_dig_rxdc_q[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];

    /* TXPOWER */
    uint8    uc_csw_band1[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_csw_band2[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_csw_band3[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_csw_band4[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_csw_band5[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];

    uint8    uc_upc_band1_init[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_upc_band2_init[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_upc_band3_init[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_upc_band4_init[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_upc_band5_init[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];

    uint8    uc_upc_band1[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_upc_band2[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_upc_band3[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_upc_band4[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    uint8    uc_upc_band5[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];


    /* TXDC */
    int8     c_txdc_i1[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_i2[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_q1[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_q2[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];

    /* TXIQ */
    int8     c_txiq_p[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txiq_e[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];

    /*******************RXIQ*****************************/
    int8     c_rxiq_p[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_rxiq_e[HAL_DEVICE_CALI_2G_PATH_NUM][WITP_CHANNEL_SUPPORT_NUMS];


    /* 5G */
    /* RXDC */
    uint16   us_ana_rxdc_40M[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    uint16   us_dig_rxdc_40M_i[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    uint16   us_dig_rxdc_40M_q[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];

    uint16   us_ana_rxdc_20M[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    uint16   us_dig_rxdc_20M_i[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    uint16   us_dig_rxdc_20M_q[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];

    /* TXPOWER */
    uint8    uc_csw_5g[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    uint8    uc_upc_5g_init[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    uint8    uc_upc_5g[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];

    /* TXDC */
    int8     c_txdc_40M_i1[WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_40M_i2[WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_40M_q1[WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_40M_q2[WITP_CHANNEL_SUPPORT_NUMS];

    int8     c_txdc_20M_i1[WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_20M_i2[WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_20M_q1[WITP_CHANNEL_SUPPORT_NUMS];
    int8     c_txdc_20M_q2[WITP_CHANNEL_SUPPORT_NUMS];

    /* TXIQ */
    int8     c_txiq_40M_p[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    int8     c_txiq_40M_e[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];

    int8     c_txiq_20M_p[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    int8     c_txiq_20M_e[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];


    /* RXIQ */
    int8     c_rxiq_p_5g[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];
    int8     c_rxiq_e_5g[WITP_CHANNEL_SUPPORT_NUMS][EQUIPMENT_TEST_5G_SUB_BAND_NUM];

}HI1151_EQUIP_CALI_INFO_STRU;

//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
    #if __cplusplus
    }
    #endif
#endif    
#endif //end of #ifndef ATWLANAPI_H


