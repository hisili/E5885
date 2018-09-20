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
/* This file should be auto-generated by tool, please don't manully modify it.*/



#include "../hisi_battery_data.h"

static struct single_row_lut feimaotui_1500_fcc_temp = {
    .x        = {-20, -10, 0, 25, 40, 60},
    .y        = {1405, 1460, 1481, 1503, 1496, 1485},
    .cols    = 6
};

static struct single_row_lut feimaotui_1500_fcc_sf = {
    .x        = {0, 100, 200, 300, 400, 500},
    .y        = {100, 96, 94, 92, 90, 88},
    .cols    = 6
};

static struct sf_lut feimaotui_1500_pc_sf = {
    .rows        = 1,
    .cols        = 1,
    .row_entries        = {0},
    .percent    = {100},
    .sf            = {
                {100}
    }
};

static struct sf_lut feimaotui_1500_rbatt_sf = {
        .rows           = 28,
        .cols           = 6,
        /* row_entries are temperature */
        .row_entries            = {-20, -10, 0, 25, 40, 60},
        .percent        = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1},
        .sf                     = {
                    {1750,   698,     295 ,     100,    57 ,   47},
                    {1598,   692,     298 ,     102,    59 ,   48},
                    {1661,   720,     307 ,     104,    61 ,   49},
                    {1591,   718,     315 ,     109,    63 ,   50},
                    {1536,   709,     320 ,     114,    66 ,   52},
                    {1497,   693,     323 ,     121,    70 ,   54},
                    {1479,   690,     309 ,     125,    75 ,   57},
                    {1474,   687,     297 ,     122,    82 ,   61},
                    {1485,   691,     286 ,     103,    68 ,   55},
                    {1571,   701,     286 ,     96 ,    61 ,   51},
                    {1563,   722,     287 ,     95 ,    60 ,   49},
                    {1620,   765,     293 ,     97 ,    62 ,   50},
                    {1684,   818,     305 ,     100,    64 ,   52},
                    {1570,   876,     327 ,     102,    68 ,   54},
                    {1826,   949,     349 ,     101,    64 ,   50},
                    {1921,   1042,    377 ,     97 ,    62 ,   50},
                    {2046,   1163,    400 ,     98 ,    62 ,   51},
                    {2231,   1344,    445 ,     102,    62 ,   50},
                    {2533,   1693,    553 ,     112,    65 ,   52},
                    {2632,   1785,    572 ,     110,    65 ,   53},
                    {2796,   1876,    521 ,     109,    66 ,   53},
                    {2960,   1576,    533 ,     109,    66 ,   52},
                    {3124,   2241,    570 ,     111,    63 ,   51},
                    {3288,   2717,    682 ,     114,    64 ,   52},
                    {3670,   3395,    762 ,     117,    67 ,   54},
                    {4889,   4292,    939 ,     124,    70 ,   53},
                    {5819,   5651,    1370,     148,    75 ,   55},
                    {6740,   7628,    2481,     315,    123,   68}
                    }
};

static struct pc_temp_ocv_lut feimaotui_1500_pc_temp_ocv = {
    .rows        = 29,
    .cols        = 6,
    .temp        = {-20, -10, 0, 25, 40, 60},
    .percent    = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    .ocv        = {
                    {4292,  4281,  4284,  4277,  4273,  4265}, 
                    {4179,  4195,  4210,  4214,  4212,  4206}, 
                    {4108,  4134,  4153,  4159,  4159,  4154}, 
                    {4043,  4078,  4102,  4109,  4108,  4104}, 
                    {3985,  4025,  4053,  4061,  4061,  4058}, 
                    {3936,  3973,  4006,  4016,  4017,  4014}, 
                    {3895,  3929,  3959,  3973,  3977,  3975}, 
                    {3860,  3890,  3916,  3930,  3938,  3936}, 
                    {3831,  3856,  3878,  3888,  3894,  3894}, 
                    {3808,  3827,  3848,  3855,  3859,  3858}, 
                    {3788,  3803,  3821,  3830,  3833,  3832}, 
                    {3770,  3786,  3800,  3809,  3812,  3811}, 
                    {3752,  3772,  3784,  3792,  3796,  3794}, 
                    {3734,  3757,  3772,  3777,  3781,  3779}, 
                    {3716,  3742,  3760,  3762,  3762,  3753}, 
                    {3697,  3727,  3746,  3743,  3739,  3728}, 
                    {3680,  3706,  3724,  3723,  3720,  3708}, 
                    {3663,  3675,  3694,  3694,  3693,  3682}, 
                    {3642,  3639,  3655,  3663,  3668,  3659}, 
                    {3636,  3632,  3647,  3661,  3664,  3655}, 
                    {3629,  3625,  3642,  3655,  3660,  3650}, 
                    {3621,  3617,  3634,  3647,  3652,  3643}, 
                    {3614,  3606,  3622,  3630,  3636,  3626}, 
                    {3606,  3592,  3605,  3599,  3603,  3594}, 
                    {3598,  3574,  3572,  3554,  3560,  3552}, 
                    {3587,  3545,  3521,  3497,  3506,  3500}, 
                    {3574,  3502,  3455,  3423,  3436,  3432}, 
                    {3555,  3447,  3366,  3328,  3343,  3340}, 
                    {3400,  3368,  3200,  3200,  3200,  3200}
            }
};

static struct hisi_smartstar_coul_battery_data feimaotui_1500_battery_data = {
        .id_voltage_min = 1080,
        .id_voltage_max = 1300,
        .fcc            = 1503,
        .fcc_temp_lut        = &feimaotui_1500_fcc_temp,
        .fcc_sf_lut        = &feimaotui_1500_fcc_sf,
        .pc_temp_ocv_lut    = &feimaotui_1500_pc_temp_ocv,
        .pc_sf_lut        = &feimaotui_1500_pc_sf,
        .rbatt_sf_lut        = &feimaotui_1500_rbatt_sf,
        .default_rbatt_mohm        = 231,
        .max_currentmA = 1000,
        .max_voltagemV = 4300,
        .max_cin_currentmA = 1200,
};
