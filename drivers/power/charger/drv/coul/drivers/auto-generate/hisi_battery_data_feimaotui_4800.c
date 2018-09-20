/* This file should be auto-generated by tool, please don't modify it manually.*/
#include "../hisi_battery_data.h"

static struct single_row_lut feimaotui_4800_fcc_temp = {
    .x = { -20,  -10,    0,   25,   40,   60},
    .y = {4588, 4641, 4665, 4773, 4828, 4724},
    .cols    = 6
};

static struct single_row_lut feimaotui_4800_fcc_sf = {
    .x        = {0, 100, 200, 300, 400, 500},
    .y        = {100, 96, 94, 92, 90, 88},
    .cols    = 6
};

static struct sf_lut feimaotui_4800_pc_sf = {
    .rows        = 1,
    .cols        = 1,
    .row_entries        = {0},
    .percent    = {100},
    .sf            = {
                {100}
    }
};

static struct sf_lut feimaotui_4800_rbatt_sf = {
        .rows           = 28,
        .cols           = 6,
        /* row_entries are temperature */
        .row_entries            = {-20, -10, 0, 25, 40, 60},
        .percent        = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1},
        .sf                     = {
                    {1174,    740,     361,     100,     76,     66},
                    {1027,    688,     359,     103,     77,     66},
                    {961,     670,     370,     107,     77,     67},
                    {893,     630,     367,     110,     79,     68},
                    {817,     589,     378,     114,     81,     69},
                    {782,     548,     347,     119,     82,     69},
                    {769,     526,     330,     126,     85,     71},
                    {760,     510,     311,     133,     93,     76},
                    {769,     505,     300,     107,     89,     69},
                    {799,     510,     297,     98,      75,     66},
                    {841,     537,     297,     98,      75,     67},
                    {888,     577,     311,     100,     77,     69},
                    {936,     621,     343,     102,     79,     71},
                    {986,     673,     394,     103,     79,     68},
                    {1046,    716,     443,     104,     75,     67},
                    {1117,    758,     487,     108,     74,     66},
                    {1201,    819,     536,     114,     77,     66},
                    {1318,    913,     614,     126,     83,     69},
                    {1441,    1034,    733,     175,     120,    88},
                    {1458,    1060,    760,     201,     130,    96},
                    {1474,    1092,    839,     218,     140,   110},
                    {1491,    1567,    980,     230,     151,   122},
                    {2830,    1701,    1040,    249,     163,   133},
                    {3179,    1782,    1084,    279,     182,   151},
                    {3401,    1811,    1179,    333,     214,   168},
                    {3610,    1944,    1283,    404,     248,   191},
                    {3901,    2115,    1442,    490,     294,   222},
                    {4226,    2388,    1674,    595,     363,   265}
                    }
};

static struct pc_temp_ocv_lut feimaotui_4800_pc_temp_ocv = {
    .rows        = 29,
    .cols        = 6,
    .temp        = {-20, -10, 0, 25, 40, 60},
    .percent    = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    .ocv        = {
                    {4336,    4333,    4328,    4324,    4331,    4311},
                    {4209,    4225,    4235,    4251,    4263,    4247},
                    {4122,    4147,    4162,    4189,    4202,    4187},
                    {4054,    4082,    4099,    4133,    4145,    4133},
                    {3977,    4019,    4053,    4085,    4096,    4089},
                    {3923,    3958,    3993,    4045,    4055,    4046},
                    {3882,    3912,    3946,    4000,    4010,    4002},
                    {3845,    3872,    3901,    3956,    3966,    3960},
                    {3821,    3839,    3862,    3898,    3913,    3902},
                    {3803,    3814,    3831,    3860,    3868,    3866},
                    {3787,    3799,    3805,    3832,    3840,    3838},
                    {3770,    3784,    3788,    3809,    3815,    3814},
                    {3751,    3768,    3776,    3789,    3794,    3794},
                    {3731,    3751,    3763,    3772,    3774,    3768},
                    {3710,    3728,    3744,    3756,    3750,    3740},
                    {3688,    3700,    3714,    3732,    3723,    3713},
                    {3657,    3674,    3681,    3692,    3691,    3672},
                    {3615,    3645,    3655,    3669,    3668,    3662},
                    {3549,    3587,    3609,    3630,    3630,    3614},
                    {3529,    3571,    3598,    3608,    3608,    3594},
                    {3510,    3556,    3580,    3583,    3582,    3567},
                    {3491,    3537,    3548,    3554,    3552,    3537},
                    {3468,    3505,    3513,    3519,    3517,    3504},
                    {3433,    3465,    3471,    3479,    3478,    3470},
                    {3396,    3415,    3425,    3437,    3439,    3434},
                    {3353,    3369,    3374,    3393,    3399,    3394},
                    {3310,    3318,    3322,    3346,    3352,    3349},
                    {3259,    3263,    3266,    3286,    3290,    3288},
                    {3200,    3200,    3200,    3200,    3200,    3200}
            }
};

static struct hisi_smartstar_coul_battery_data feimaotui_4800_battery_data = {
        .id_voltage_min = 220, 
        .id_voltage_max = 420,
        .fcc            = 4773,
        .fcc_temp_lut        = &feimaotui_4800_fcc_temp,
        .fcc_sf_lut        = &feimaotui_4800_fcc_sf,
        .pc_temp_ocv_lut    = &feimaotui_4800_pc_temp_ocv,
        .pc_sf_lut        = &feimaotui_4800_pc_sf,
        .rbatt_sf_lut        = &feimaotui_4800_rbatt_sf,
        .default_rbatt_mohm        = 126,
        .max_currentmA = 2000,
        .max_voltagemV = 4350,
        .max_cin_currentmA = 2048,
};
