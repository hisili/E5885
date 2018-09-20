/* This file should be auto-generated by tool, please don't manully modify it.*/
/*由于硬件暂未提供xinwangda_3000mA电池数据，该文件暂拷贝自feimaotui_3000的数据*/
#include "../hisi_battery_data.h"

static struct single_row_lut xinwangda_3000_fcc_temp = {
    .x        = {-20, -10, 0, 25, 40, 60},
    .y        = {2973, 3035, 3054, 3094, 3074, 3056},
    .cols    = 6
};

static struct single_row_lut xinwangda_3000_fcc_sf = {
    .x        = {0, 100, 200, 300, 400, 500},
    .y        = {100, 96, 94, 92, 90, 88},
    .cols    = 6
};

static struct sf_lut xinwangda_3000_pc_sf = {
    .rows        = 1,
    .cols        = 1,
    .row_entries        = {0},
    .percent    = {100},
    .sf            = {
                {100}
    }
};

static struct sf_lut xinwangda_3000_rbatt_sf = {
        .rows           = 28,
        .cols           = 6,
        /* row_entries are temperature */
        .row_entries            = {-20, -10, 0, 25, 40, 60},
        .percent        = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1},
        .sf                     = {
                    {1309,   675,     362 ,     100,    81 ,   73},
                    {1256,   678,     356 ,     101,    81 ,   73},
                    {1224,   681,     351 ,     103,    81 ,   74},
                    {1185,   670,     349 ,     106,    83 ,   74},
                    {1148,   659,     353 ,     110,    85 ,   75},
                    {1121,   648,     354 ,     116,    87 ,   77},
                    {1116,   630,     344 ,     123,    92 ,   79},
                    {1109,   627,     334 ,     125,    96 ,   81},
                    {1116,   626,     326 ,     113,    91 ,   81},
                    {1142,   637,     330 ,     103,    82 ,   75},
                    {1179,   661,     336 ,     101,    81 ,   74},
                    {1218,   695,     348 ,     103,    82 ,   74},
                    {1260,   730,     370 ,     106,    84 ,   75},
                    {1294,   785,     398 ,     110,    87 ,   77},
                    {1361,   846,     430 ,     109,    86 ,   77},
                    {1439,   925,     466 ,     108,    82 ,   75},
                    {1544,   1027,    513 ,     109,    82 ,   74},
                    {1693,   1172,    583 ,     112,    83 ,   74},
                    {1940,   1381,    603 ,     110,    85 ,   76},
                    {2225,   1512,    625 ,     111,    85 ,   76},
                    {3045,   1645,    677 ,     112,    85 ,   76},
                    {3298,   1825,    727 ,     114,    85 ,   77},
                    {3596,   2077,    833 ,     114,    85 ,   77},
                    {3949,   2331,    927 ,     116,    85 ,   77},
                    {4390,   2760,    1078,     120,    87 ,   77},
                    {4938,   3379,    1267,     128,    89 ,   78},
                    {5599,   4226,    1632,     141,    94 ,   82},
                    {6588,   5810,    2509,     161,    103,   87}
                    }
};

static struct pc_temp_ocv_lut xinwangda_3000_pc_temp_ocv = {
    .rows        = 29,
    .cols        = 6,
    .temp        = {-20, -10, 0, 25, 40, 60},
    .percent    = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    .ocv        = {
                    {4326,  4323,  4322,  4317,  4309,  4302},
                    {4207,  4223,  4234,  4247,  4241,  4236},
                    {4135,  4158,  4173,  4190,  4185,  4180},
                    {4073,  4100,  4117,  4136,  4132,  4127},
                    {4013,  4045,  4066,  4086,  4082,  4078},
                    {3961,  3990,  4016,  4038,  4035,  4032},
                    {3917,  3940,  3967,  3994,  3992,  3989},
                    {3876,  3898,  3922,  3950,  3950,  3949},
                    {3841,  3860,  3881,  3905,  3907,  3908},
                    {3816,  3829,  3847,  3867,  3868,  3867},
                    {3791,  3805,  3819,  3838,  3839,  3838},
                    {3773,  3787,  3797,  3815,  3817,  3816},
                    {3751,  3769,  3781,  3797,  3799,  3797},
                    {3731,  3755,  3768,  3781,  3784,  3781},
                    {3712,  3739,  3755,  3765,  3765,  3760},
                    {3692,  3721,  3739,  3746,  3741,  3730},
                    {3674,  3698,  3717,  3723,  3719,  3708},
                    {3655,  3667,  3682,  3694,  3692,  3683},
                    {3629,  3628,  3640,  3670,  3669,  3659},
                    {3623,  3621,  3633,  3666,  3666,  3656},
                    {3615,  3611,  3626,  3660,  3661,  3651},
                    {3605,  3600,  3615,  3650,  3653,  3644},
                    {3591,  3588,  3603,  3629,  3634,  3626},
                    {3575,  3566,  3580,  3595,  3601,  3594},
                    {3554,  3536,  3547,  3546,  3555,  3550},
                    {3530,  3490,  3494,  3482,  3495,  3494},
                    {3498,  3427,  3420,  3407,  3418,  3422},
                    {3457,  3343,  3323,  3317,  3321,  3323},
                    {3200,  3200,  3200,  3200,  3200,  3200}
            }
};

static struct hisi_smartstar_coul_battery_data xinwangda_3000_battery_data = {
        .id_voltage_min = 840,                 /*2.5 * 0.4738 = 1.1845    0.5238(-5%/+10%)*/
        .id_voltage_max = 1030,                /*2.5 * 0.6238 = 1.5595*/
        .fcc            = 3094,
        .fcc_temp_lut        = &xinwangda_3000_fcc_temp,
        .fcc_sf_lut        = &xinwangda_3000_fcc_sf,
        .pc_temp_ocv_lut    = &xinwangda_3000_pc_temp_ocv,
        .pc_sf_lut        = &xinwangda_3000_pc_sf,
        .rbatt_sf_lut        = &xinwangda_3000_rbatt_sf,
        .default_rbatt_mohm        = 149 ,
        .max_currentmA = 2000,
        .max_voltagemV = 4320,
        .max_cin_currentmA = 2048,
};

