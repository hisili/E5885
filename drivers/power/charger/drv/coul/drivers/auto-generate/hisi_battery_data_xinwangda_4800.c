/* This file should be auto-generated by tool, please don't manully modify it.*/
#include "../hisi_battery_data.h"

static struct single_row_lut xinwangda_4800_fcc_temp = {
    .x        = {-20, -10, 0, 25, 40, 60},
    .y        = {4472, 4485, 4380, 4696, 4660, 4601},
    .cols    = 6
};

static struct single_row_lut xinwangda_4800_fcc_sf = {
    .x        = {0, 100, 200, 300, 400, 500},
    .y        = {100, 96, 94, 92, 90, 88},
    .cols    = 6
};

static struct sf_lut xinwangda_4800_pc_sf = {
    .rows        = 1,
    .cols        = 1,
    .row_entries        = {0},
    .percent    = {100},
    .sf            = {
                {100}
    }
};

static struct sf_lut xinwangda_4800_rbatt_sf = {
        .rows           = 28,
        .cols           = 6,
        /* row_entries are temperature */
        .row_entries            = {-20, -10, 0, 25, 40, 60},
        .percent        = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1},
        .sf                     = {
                    {1114,    639,     338,    100,    67,     58},
                    {984,     576,     307,    100,    66,     57},
                    {983,     585,     328,    102,    68,     60},
                    {942,     561,     326,    104,    69,     61},
                    {908,     548,     323,    108,    71,     61},
                    {883,     526,     317,    113,    74,     63},
                    {874,     518,     308,    122,    78,     64},
                    {874,     515,     300,    131,    86,     69},
                    {883,     516,     294,    110,    83,     66},
                    {902,     524,     293,    100,    72,     62},
                    {925,     537,     295,    100,    70,     61},
                    {949,     560,     302,    103,    72,     62},
                    {980,     589,     320,    105,    75,     64},
                    {1024,    615,     344,    108,    74,     63},
                    {1082,    642,     368,    112,    72,     62},
                    {1159,    672,     385,    118,    72,     62},
                    {1251,    735,     392,    123,    74,     63},
                    {1360,    844,     423,    132,    80,     65},
                    {1531,    996,     507,    177,    104,    81},
                    {1896,    1044,    529,    196,    110,    88},
                    {2394,    1094,    554,    224,    122,    94},
                    {2891,    1143,    585,    242,    145,   106},
                    {3314,    1192,    629,    242,    154,   132},
                    {3356,    1242,    673,    249,    162,   146},
                    {3612,    1556,    718,    273,    177,   165},
                    {3965,    1839,    769,    317,    200,   185},
                    {4387,    2042,    899,    373,    231,   211},
                    {4956,    2359,    1126,   442,    272,   244}
                    }
};

static struct pc_temp_ocv_lut xinwangda_4800_pc_temp_ocv = {
    .rows        = 29,
    .cols        = 6,
    .temp        = {-20, -10, 0, 25, 40, 60},
    .percent    = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    .ocv        = {
                    {4333,    4332,    4324,    4325,    4318,    4308},
                    {4222,    4239,    4243,    4257,    4254,    4247},
                    {4140,    4171,    4181,    4199,    4197,    4191},
                    {4074,    4108,    4125,    4144,    4143,    4137},
                    {4009,    4053,    4072,    4092,    4091,    4085},
                    {3951,    3993,    4018,    4043,    4042,    4037},
                    {3902,    3943,    3965,    3996,    3997,    3993},
                    {3859,    3898,    3918,    3952,    3955,    3952},
                    {3823,    3858,    3876,    3897,    3904,    3901},
                    {3792,    3824,    3841,    3856,    3860,    3860},
                    {3763,    3797,    3811,    3827,    3830,    3830},
                    {3731,    3775,    3789,    3803,    3808,    3807},
                    {3700,    3755,    3774,    3785,    3789,    3788},
                    {3671,    3730,    3760,    3770,    3771,    3765},
                    {3646,    3698,    3742,    3757,    3753,    3741},
                    {3621,    3660,    3716,    3735,    3731,    3716},
                    {3592,    3622,    3675,    3696,    3694,    3686},
                    {3553,    3587,    3625,    3667,    3672,    3662},
                    {3501,    3535,    3562,    3603,    3617,    3609},
                    {3484,    3514,    3548,    3584,    3602,    3588},
                    {3466,    3492,    3528,    3558,    3582,    3567},
                    {3447,    3471,    3502,    3526,    3548,    3542},
                    {3429,    3450,    3464,    3489,    3511,    3504},
                    {3408,    3428,    3427,    3447,    3467,    3463},
                    {3377,    3397,    3390,    3401,    3419,    3418},
                    {3343,    3358,    3353,    3354,    3369,    3371},
                    {3301,    3311,    3308,    3308,    3320,    3323},
                    {3255,    3258,    3258,    3259,    3266,    3268},
                    {3200,    3200,    3200,    3200,    3200,    3200}
            }
};

static struct hisi_smartstar_coul_battery_data xinwangda_4800_battery_data = {
        .id_voltage_min = 1530,
        .id_voltage_max = 1730,
        .fcc            = 4696,
        .fcc_temp_lut      = &xinwangda_4800_fcc_temp,
        .fcc_sf_lut        = &xinwangda_4800_fcc_sf,
        .pc_temp_ocv_lut    = &xinwangda_4800_pc_temp_ocv,
        .pc_sf_lut        = &xinwangda_4800_pc_sf,
        .rbatt_sf_lut        = &xinwangda_4800_rbatt_sf,
        .default_rbatt_mohm        = 118,
        .max_currentmA = 2000,
        .max_voltagemV = 4320,
        .max_cin_currentmA = 2048,
};

