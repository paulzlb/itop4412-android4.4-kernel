/* linux/arch/arm/mach-exynos/cpufreq-4x12.c
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS4X12 - CPU frequency scaling support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/cpufreq.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-pmu.h>
#include <mach/cpufreq.h>
#include <mach/asv.h>

#include <plat/clock.h>
#include <plat/cpu.h>

#define CPUFREQ_LEVEL_END	(L13 + 1)

#undef PRINT_DIV_VAL

#define ENABLE_CLKOUT

static int max_support_idx;
static int min_support_idx = (CPUFREQ_LEVEL_END - 1);
static struct clk *cpu_clk;
static struct clk *moutcore;
static struct clk *mout_mpll;
static struct clk *mout_apll;

struct cpufreq_clkdiv {
	unsigned int	index;
	unsigned int	clkdiv;
	unsigned int	clkdiv1;
};

static unsigned int exynos4x12_volt_table[CPUFREQ_LEVEL_END];

static struct cpufreq_frequency_table exynos4x12_freq_table[] = {
	{L0, 1500*1000},
	{L1, 1400*1000},
	{L2, 1300*1000},
	{L3, 1200*1000},
	{L4, 1100*1000},
	{L5, 1000*1000},
	{L6, 900*1000},
	{L7, 800*1000},
	{L8, 700*1000},
	{L9, 600*1000},
	{L10, 500*1000},
	{L11, 400*1000},
	{L12, 300*1000},
	{L13, 200*1000},
	{0, CPUFREQ_TABLE_END},
};

static struct cpufreq_clkdiv exynos4x12_clkdiv_table[CPUFREQ_LEVEL_END];

static unsigned int clkdiv_cpu0_4212[CPUFREQ_LEVEL_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL, DIVCORE2 }
	 */
	/* ARM L0: 1500Mhz */
	{ 0, 3, 7, 0, 6, 1, 2, 0 },

	/* ARM L1: 1400Mhz */
	{ 0, 3, 7, 0, 6, 1, 2, 0 },

	/* ARM L2: 1300Mhz */
	{ 0, 3, 7, 0, 6, 1, 2, 0 },

	/* ARM L3: 1200Mhz */
	{ 0, 3, 7, 0, 5, 1, 2, 0 },

	/* ARM L4: 1100MHz */
	{ 0, 3, 6, 0, 4, 1, 2, 0 },

	/* ARM L5: 1000MHz */
	{ 0, 2, 5, 0, 4, 1, 1, 0 },

	/* ARM L6: 900MHz */
	{ 0, 2, 5, 0, 3, 1, 1, 0 },

	/* ARM L7: 800MHz */
	{ 0, 2, 5, 0, 3, 1, 1, 0 },

	/* ARM L8: 700MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L9: 600MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L10: 500MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L11: 400MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L12: 300MHz */
	{ 0, 2, 4, 0, 2, 1, 1, 0 },

	/* ARM L13: 200MHz */
	{ 0, 1, 3, 0, 1, 1, 1, 0 },
};

static unsigned int clkdiv_cpu0_4412[CPUFREQ_LEVEL_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL, DIVCORE2 }
	 */
	/* ARM L0: 1500Mhz */
	{ 0, 3, 7, 0, 6, 1, 2, 0 },

	/* ARM L1: 1400Mhz */
	{ 0, 3, 7, 0, 6, 1, 2, 0 },

	/* ARM L2: 1300Mhz */
	{ 0, 3, 7, 0, 5, 1, 2, 0 },

	/* ARM L3: 1200Mhz */
	{ 0, 3, 7, 0, 5, 1, 2, 0 },

	/* ARM L4: 1100MHz */
	{ 0, 3, 6, 0, 4, 1, 2, 0 },

	/* ARM L5: 1000MHz */
	{ 0, 2, 5, 0, 4, 1, 1, 0 },

	/* ARM L6: 900MHz */
	{ 0, 2, 5, 0, 3, 1, 1, 0 },

	/* ARM L7: 800MHz */
	{ 0, 2, 5, 0, 3, 1, 1, 0 },

	/* ARM L8: 700MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L9: 600MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L10: 500MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L11: 400MHz */
	{ 0, 2, 4, 0, 3, 1, 1, 0 },

	/* ARM L12: 300MHz */
	{ 0, 2, 4, 0, 2, 1, 1, 0 },

	/* ARM L13: 200MHz */
	{ 0, 1, 3, 0, 1, 1, 1, 0 },
};

static unsigned int clkdiv_cpu1_4212[CPUFREQ_LEVEL_END][2] = {
	/* Clock divider value for following
	 * { DIVCOPY, DIVHPM }
	 */
	/* ARM L0: 1500MHz */
	{ 6, 0 },

	/* ARM L1: 1400MHz */
	{ 6, 0 },

	/* ARM L2: 1300MHz */
	{ 5, 0 },

	/* ARM L3: 1200MHz */
	{ 5, 0 },

	/* ARM L4: 1100MHz */
	{ 4, 0 },

	/* ARM L5: 1000MHz */
	{ 4, 0 },

	/* ARM L6: 900MHz */
	{ 3, 0 },

	/* ARM L7: 800MHz */
	{ 3, 0 },

	/* ARM L8: 700MHz */
	{ 3, 0 },

	/* ARM L9: 600MHz */
	{ 3, 0 },

	/* ARM L10: 500MHz */
	{ 3, 0 },

	/* ARM L11: 400MHz */
	{ 3, 0 },

	/* ARM L12: 300MHz */
	{ 3, 0 },

	/* ARM L13: 200MHz */
	{ 3, 0 },
};

static unsigned int clkdiv_cpu1_4412[CPUFREQ_LEVEL_END][3] = {
	/* Clock divider value for following
	 * { DIVCOPY, DIVHPM, DIVCORES }
	 */
	/* ARM L0: 1500MHz */
	{ 6, 0, 7 },

	/* ARM L1: 1400MHz */
	{ 6, 0, 6 },

	/* ARM L2: 1300MHz */
	{ 5, 0, 6 },

	/* ARM L3: 1200MHz */
	{ 5, 0, 5 },

	/* ARM L4: 1100MHz */
	{ 4, 0, 5 },

	/* ARM L5: 1000MHz */
	{ 4, 0, 4 },

	/* ARM L6: 900MHz */
	{ 3, 0, 4 },

	/* ARM L7: 800MHz */
	{ 3, 0, 3 },

	/* ARM L8: 700MHz */
	{ 3, 0, 3 },

	/* ARM L9: 600MHz */
	{ 3, 0, 2 },

	/* ARM L10: 500MHz */
	{ 3, 0, 2 },

	/* ARM L11: 400MHz */
	{ 3, 0, 1 },

	/* ARM L12: 300MHz */
	{ 3, 0, 1 },

	/* ARM L13: 200MHz */
	{ 3, 0, 0 },
};

static unsigned int exynos4x12_apll_pms_table[CPUFREQ_LEVEL_END] = {
	/* APLL FOUT L0: 1500MHz */
	((250<<16)|(4<<8)|(0x0)),

	/* APLL FOUT L1: 1400MHz */
	((175<<16)|(3<<8)|(0x0)),

	/* APLL FOUT L2: 1300MHz */
	((325<<16)|(6<<8)|(0x0)),

	/* APLL FOUT L3: 1200MHz */
	((200<<16)|(4<<8)|(0x0)),

	/* APLL FOUT L4: 1100MHz */
	((275<<16)|(6<<8)|(0x0)),

	/* APLL FOUT L5: 1000MHz */
	((125<<16)|(3<<8)|(0x0)),

	/* APLL FOUT L6: 900MHz */
	((150<<16)|(4<<8)|(0x0)),

	/* APLL FOUT L7: 800MHz */
	((100<<16)|(3<<8)|(0x0)),

	/* APLL FOUT L8: 700MHz */
	((175<<16)|(3<<8)|(0x1)),

	/* APLL FOUT L9: 600MHz */
	((200<<16)|(4<<8)|(0x1)),

	/* APLL FOUT L10: 500MHz */
	((125<<16)|(3<<8)|(0x1)),

	/* APLL FOUT L11 400MHz */
	((100<<16)|(3<<8)|(0x1)),

	/* APLL FOUT L12: 300MHz */
	((200<<16)|(4<<8)|(0x2)),

	/* APLL FOUT L13: 200MHz */
	((100<<16)|(3<<8)|(0x2)),

};

/*
 * ASV group voltage table
 */

#define NO_ABB_LIMIT	L8

static const unsigned int asv_voltage_4212[CPUFREQ_LEVEL_END] = {
	1350000, 1300000, 1300000, 1225000, 1150000, 1100000, 1050000,
	1012500,  987500,  975000,  962500,  950000,  925000,  900000
};

static const unsigned int asv_voltage_s[CPUFREQ_LEVEL_END] = {
	1500000, 1375000, 1350000, 1250000, 1200000, 1175000, 1100000,
	1050000, 1025000, 1000000, 1000000, 1000000, 950000, 950000
};

/* ASV table for 12.5mV step */
/* modify by cym 20151111 */
#if 0
static const unsigned int asv_voltage_step_12_5[CPUFREQ_LEVEL_END][12] = {
	/*   ASV0,    ASV1,    ASV2,    ASV3,	 ASV4,	  ASV5,	   ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11 */
	{	0,       0,	  0,	   0,	    0,	     0,	      0,       0,       0,       0,	  0,       0 },	/* L0 - Not used */
	{ 1300000, 1300000, 1300000, 1300000, 1300000, 1287500, 1275000, 1250000, 1250000, 1237500, 1225000, 1212500 },	/* L1 */
	{ 1300000, 1300000, 1300000, 1300000, 1275000, 1250000, 1237500, 1225000, 1225000, 1225000, 1212500, 1175000 },	/* L2 */
	{ 1225000, 1212500, 1225000, 1225000, 1200000, 1187500, 1175000, 1150000, 1150000, 1150000, 1137500, 1112500 },	/* L3 */
	{ 1175000, 1162500, 1150000, 1137500, 1150000, 1137500, 1125000, 1100000, 1100000, 1075000, 1075000, 1062500 },	/* L4 */
	{ 1125000, 1112500, 1100000, 1087500, 1100000, 1087500, 1075000, 1050000, 1037500, 1025000, 1025000, 1012500 },	/* L5 */
	{ 1075000, 1062500, 1050000, 1050000, 1050000, 1037500, 1025000, 1012500, 1000000,  987500,  987500,  975000 },	/* L6 */
	{ 1037500, 1025000, 1012500, 1012500, 1012500, 1000000,  987500,  975000,  975000,  975000,  975000,  962500 },	/* L7 */
	{ 1012500, 1000000,  987500,  987500,  987500,  987500,  975000,  975000,  962500,  962500,  962500,  950000 },	/* L8 */
	{ 1000000,  987500,  975000,  975000,  975000,  975000,  962500,  962500,  950000,  950000,  950000,  937500 },	/* L9 */
	{  987500,  975000,  962500,  950000,  962500,  950000,  950000,  950000,  925000,  925000,  925000,  912500 },	/* L10 */
	{  975000,  962500,  950000,  925000,  950000,  925000,  925000 + 25000,  925000,  900000,  900000,  900000,  887500 },	/* L11 */
	{  950000,  937500,  925000,  900000,  925000,  900000,  900000 + 25000,  900000,  900000,  887500,  875000,  862500 },	/* L12 */
	{  925000,  912500,  900000,  900000,  900000,  900000,  900000 + 25000,  900000,  887500,  875000,  875000,  862500 },	/* L13 */
};
#else
#if defined(CONFIG_CPU_TYPE_POP2G_ELITE) || defined(CONFIG_CPU_TYPE_POP2G_SUPPER)
static const unsigned int asv_voltage_step_12_5[CPUFREQ_LEVEL_END][13] = {
        /*   ASV0,    ASV1,    ASV2,    ASV3,    ASV4,    ASV5,    ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11, ASV12 */
        //cym20151116{1312500, 1312500, 1312500,     1312500, 1300000, 1287500, 1275000, 1262500, 1250000, 1237500, 1212500, 1200000, 1187500},/*L0*/
        {1275000, 1262500, 1262500,     1262500, 1250000, 1237500, 1225000, 1212500, 1200000, 1187500, 1162500, 1150000, 1137500},/*L1*/
        {1237500, 1225000, 1225000,     1225000, 1212500, 1200000, 1187500, 1175000, 1162500, 1150000, 1125000, 1112500, 1100000},/*L2*/
        {1187500, 1175000, 1175000,     1175000, 1162500, 1150000, 1137500, 1125000, 1112500, 1100000, 1075000, 1062500, 1050000},/*L3*/
        {1150000, 1137500, 1137500,     1137500, 1125000, 1112500, 1100000, 1087500, 1075000, 1062500, 1037500, 1025000, 1012500},/*L4*/
        {1112500, 1100000, 1100000,     1100000, 1087500, 1075000, 1062500, 1050000, 1037500, 1025000, 1000000,  987500,  975000},/*L5*/
        {1087500, 1075000, 1075000,     1075000, 1062500, 1050000, 1037500, 1025000, 1012500, 1000000,  975000,  962500,  950000},/*L6*/
        {1062500, 1050000, 1050000,     1050000, 1037500, 1025000, 1012500, 1000000,  987500,  975000,  950000,  937500,  925000},/*L7*/
        {1025000, 1012500, 1012500,     1012500, 1000000,  987500,  975000,  962500,  950000,  937500,  912500,  900000,  887500},/*L8*/
        {1000000,  987500,  987500,      987500,  975000,  962500,  950000,  937500,  925000,  912500,  887500,  887500,  887500},/*L9*/
        { 975000,  962500,  962500+12500,      962500,  950000,  937500,  925000,  912500,  900000,  887500,  875000,  875000,  875000},/*L10*/
        { 962500,  950000,  950000+12500,      950000,  937500,  925000,  912500,  900000,  887500,  887500,  875000,  875000,  875000},/*L11*/
        { 950000,  937500,  937500+12500,      937500,  925000,  912500,  900000,  887500,  887500,  887500,  875000,  875000,  875000},/*L12*/
        { 937500,  925000,  925000+12500,      925000,  912500,  900000,  887500,  887500,  887500,  887500,  875000,  875000,  875000},/*L13*/
        { 925000,  912500,  912500+12500,      912500,  900000,  887500,  887500,  887500,  887500,  887500,  875000,  875000,  875000},/*L14*/
};
#elif defined(CONFIG_CPU_TYPE_SCP_ELITE) || defined(CONFIG_CPU_TYPE_SCP_ELITE)
static const unsigned int asv_voltage_step_12_5[CPUFREQ_LEVEL_END][12] = {
        /*   ASV0,    ASV1,    ASV2,    ASV3,    ASV4,    ASV5,    ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11 */
        { 1375000, 1375000, 1375000, 1375000, 1375000, 1375000,  1375000, 1375000, 1375000, 1375000, 1375000, 1375000 }, /* L0 - Not used */
        { 1350000/*1300000*/, 1350000/*1300000*/, 1350000/*1300000*/, 1350000/*1300000*/, 1350000/*1300000*/, 1350000/*1287500*/, 1275000, 1250000, 1250000, 1237500, 1225000, 1212500 },       /* L1 */
        { 1300000, 1300000, 1300000, 1300000, 1300000/*1275000*/, 1300000/*1250000*/, 1237500, 1225000, 1225000, 1225000, 1212500, 1175000 },   /* L2 */
        { 1275000/*1225000*/, 1275000/*1300000*//*1212500*/, 1275000/*1225000*/, 1275000/*1225000*/, 1275000/*1200000*/, 1275000/*1187500*/, 1175000, 1150000, 1150000, 1150000, 1137500, 1112500 },    /* L3 */
        { 1212500/*1175000*/, 1212500/*1162500*/, 1212500/*1150000*/, 1212500/*1137500*/, 1212500/*1150000*/, 1212500/*1137500*/, 1125000, 1100000, 1100000, 1075000, 1075000, 1062500 },       /* L4 */
        { 1175000/*1125000*/, 1175000/*1112500*/, 1175000/*1100000*/, 1175000/*1087500*/, 1175000/*1100000*/, 1175000/*1087500*/, 1075000, 1050000, 1037500, 1025000, 1025000, 1012500 },       /* L5 */
        { 1162500/*1075000*/, 1162500/*1062500*/, 1162500/*1050000*/, 1162500/*1050000*/, 1162500/*1050000*/, 1162500/*1037500*/, 1025000, 1012500, 1000000,  987500,  987500,  975000 },       /* L6 */
        { 1162500/*1037500*/, 1162500/*1025000*/, 1162500/*1012500*/, 1162500/*1012500*/, 1162500/*1012500*/, 1162500/*1000000*/,  987500,  975000,  975000,  975000,  975000,  962500 },       /* L7 */
        { 1062500/*1012500*/, 1062500/*1000000*/,  1062500/*987500*/,  1062500/*987500*/,  1062500/*987500*/,  1062500/*987500*/,  975000,  975000,  962500,  962500,  962500,  950000 },       /* L8 */
        { 1037500/*1000000*/,  1037500/*987500*/,  1037500/*975000*/,  1037500/*975000*/,  1037500/*975000*/,  1037500/*975000*/,  962500,  962500,  950000,  950000,  950000,  937500 },       /* L9 */
        {  1025000/*987500*/,  1025000/*975000*/,  1025000/*962500*/,  1025000/*950000*/,  1025000/*962500*/,  1025000/*950000*/,  950000,  950000,  925000,  925000,  925000,  912500 },       /* L10 */
        {  1000000/*975000*/,  1000000/*962500*/,  1000000/*950000*/,  1000000/*925000*/,  1000000/*950000*/,  1000000/*925000*/,  925000 + 25000,  925000,  900000,  900000,  900000,  887500 },       /* L11 */
        {  987500/*950000*/,  975000/*937500*/,  975000/*925000*/,  987500/*900000*/,  987500/*925000*/,  987500/*900000*/,  900000 + 25000,  900000,  900000,  887500,  875000,  862500 },     /* L12 */
        {  975000/*925000*/,  975000/*912500*/,  975000/*900000*/,  975000/*900000*/,  975000/*900000*/,  975000/*900000*/,  900000 + 25000,  900000,  887500,  875000,  875000,  862500 },     /* L13 */
};
#else
static const unsigned int asv_voltage_step_12_5[CPUFREQ_LEVEL_END][12] = {
        /*   ASV0,    ASV1,    ASV2,    ASV3,    ASV4,    ASV5,    ASV6,    ASV7,    ASV8,    ASV9,   ASV10,   ASV11 */
        {       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0 }, /* L0 - Not used */
        { 1300000, 1300000, 1300000, 1300000, 1300000, 1287500, 1275000, 1250000, 1250000, 1237500, 1225000, 1212500 }, /* L1 */
        { 1300000, 1300000, 1300000, 1300000, 1275000, 1250000, 1237500, 1225000, 1225000, 1225000, 1212500, 1175000 }, /* L2 */
        { 1225000, 1212500, 1225000, 1225000, 1200000, 1187500, 1175000, 1150000, 1150000, 1150000, 1137500, 1112500 }, /* L3 */
        { 1175000, 1162500, 1150000, 1137500, 1150000, 1137500, 1125000, 1100000, 1100000, 1075000, 1075000, 1062500 }, /* L4 */
        { 1125000, 1112500, 1100000, 1087500, 1100000, 1087500, 1075000, 1050000, 1037500, 1025000, 1025000, 1012500 }, /* L5 */
        { 1075000, 1062500, 1050000, 1050000, 1050000, 1037500, 1025000, 1012500, 1000000,  987500,  987500,  975000 }, /* L6 */
        { 1037500, 1025000, 1012500, 1012500, 1012500, 1000000,  987500,  975000,  975000,  975000,  975000,  962500 }, /* L7 */
        { 1012500, 1000000,  987500,  987500,  987500,  987500,  975000,  975000,  962500,  962500,  962500,  950000 }, /* L8 */
        { 1000000,  987500,  975000,  975000,  975000,  975000,  962500,  962500,  950000,  950000,  950000,  937500 }, /* L9 */
        {  987500,  975000,  962500,  950000,  962500,  950000,  950000,  950000,  925000,  925000,  925000,  912500 }, /* L10 */
        {  975000,  962500,  950000,  925000,  950000,  925000,  925000 + 25000,  925000,  900000,  900000,  900000,  887500 }, /* L11 */
        {  950000,  937500,  925000,  900000,  925000,  900000,  900000 + 25000,  900000,  900000,  887500,  875000,  862500 }, /* L12 */
        {  925000,  912500,  900000,  900000,  900000,  900000,  900000 + 25000,  900000,  887500,  875000,  875000,  862500 }, /* L13 */
};
#endif
#endif
/* end modify */

static void set_clkdiv(unsigned int div_index)
{
	unsigned int tmp;
	unsigned int stat_cpu1;

	/* Change Divider - CPU0 */

	tmp = exynos4x12_clkdiv_table[div_index].clkdiv;

	__raw_writel(tmp, EXYNOS4_CLKDIV_CPU);

	do {
		tmp = __raw_readl(EXYNOS4_CLKDIV_STATCPU);
	} while (tmp & 0x11111111);

#ifdef PRINT_DIV_VAL
	tmp = __raw_readl(EXYNOS4_CLKDIV_CPU);
	pr_info("DIV_CPU0[0x%x]\n", tmp);

#endif

	/* Change Divider - CPU1 */
	tmp = exynos4x12_clkdiv_table[div_index].clkdiv1;

	__raw_writel(tmp, EXYNOS4_CLKDIV_CPU1);
	if (soc_is_exynos4212())
		stat_cpu1 = 0x11;
	else
		stat_cpu1 = 0x111;

	do {
		tmp = __raw_readl(EXYNOS4_CLKDIV_STATCPU1);
	} while (tmp & stat_cpu1);
#ifdef PRINT_DIV_VAL
	tmp = __raw_readl(EXYNOS4_CLKDIV_CPU1);
	pr_info("DIV_CPU1[0x%x]\n", tmp);
#endif
}

static void set_apll(unsigned int new_index,
			     unsigned int old_index)
{
	unsigned int tmp, pdiv;

	/* 1. MUX_CORE_SEL = MPLL,
	 * ARMCLK uses MPLL for lock time */
	if (clk_set_parent(moutcore, mout_mpll))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
				mout_mpll->name, moutcore->name);

	do {
		tmp = (__raw_readl(EXYNOS4_CLKMUX_STATCPU)
			>> EXYNOS4_CLKSRC_CPU_MUXCORE_SHIFT);
		tmp &= 0x7;
	} while (tmp != 0x2);

	/* 2. Set APLL Lock time */
	pdiv = ((exynos4x12_apll_pms_table[new_index] >> 8) & 0x3f);

	__raw_writel((pdiv * 250), EXYNOS4_APLL_LOCK);

	/* 3. Change PLL PMS values */
	tmp = __raw_readl(EXYNOS4_APLL_CON0);
	tmp &= ~((0x3ff << 16) | (0x3f << 8) | (0x7 << 0));
	tmp |= exynos4x12_apll_pms_table[new_index];
	__raw_writel(tmp, EXYNOS4_APLL_CON0);

	/* 4. wait_lock_time */
	do {
		tmp = __raw_readl(EXYNOS4_APLL_CON0);
	} while (!(tmp & (0x1 << EXYNOS4_APLLCON0_LOCKED_SHIFT)));

	/* 5. MUX_CORE_SEL = APLL */
	if (clk_set_parent(moutcore, mout_apll))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
				mout_apll->name, moutcore->name);

	do {
		tmp = __raw_readl(EXYNOS4_CLKMUX_STATCPU);
		tmp &= EXYNOS4_CLKMUX_STATCPU_MUXCORE_MASK;
	} while (tmp != (0x1 << EXYNOS4_CLKSRC_CPU_MUXCORE_SHIFT));

}

bool exynos4x12_pms_change(unsigned int old_index, unsigned int new_index)
{
	unsigned int old_pm = (exynos4x12_apll_pms_table[old_index] >> 8);
	unsigned int new_pm = (exynos4x12_apll_pms_table[new_index] >> 8);

	return (old_pm == new_pm) ? 0 : 1;
}

static void exynos4x12_set_frequency(unsigned int old_index,
				  unsigned int new_index)
{
	unsigned int tmp;

	if (old_index > new_index) {
		if (!exynos4x12_pms_change(old_index, new_index)) {
			/* 1. Change the system clock divider values */
			set_clkdiv(new_index);
			/* 2. Change just s value in apll m,p,s value */
			tmp = __raw_readl(EXYNOS4_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (exynos4x12_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, EXYNOS4_APLL_CON0);

		} else {
			/* Clock Configuration Procedure */
			/* 1. Change the system clock divider values */
			set_clkdiv(new_index);
			/* 2. Change the apll m,p,s value */
			set_apll(new_index, old_index);
		}
	} else if (old_index < new_index) {
		if (!exynos4x12_pms_change(old_index, new_index)) {
			/* 1. Change just s value in apll m,p,s value */
			tmp = __raw_readl(EXYNOS4_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (exynos4x12_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, EXYNOS4_APLL_CON0);
			/* 2. Change the system clock divider values */
			set_clkdiv(new_index);
		} else {
			/* Clock Configuration Procedure */
			/* 1. Change the apll m,p,s value */
			set_apll(new_index, old_index);
			/* 2. Change the system clock divider values */
			set_clkdiv(new_index);
		}
	}

	/* ABB value is changed in below case */
	if (soc_is_exynos4412() && (exynos_result_of_asv > 3)) {
		if (new_index == L13)
			exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_100V);
		else
			exynos4x12_set_abb_member(ABB_ARM, ABB_MODE_130V);
	}
}

#if defined(CONFIG_CPU_TYPE_POP2G_ELITE) || defined(CONFIG_CPU_TYPE_POP2G_SUPPER)
#define CPU_MARGIN (12500)
#endif

static void __init set_volt_table(void)
{
	bool for_1500 = false, for_1200 = false, for_1400 = false;
	unsigned int i;

#ifdef CONFIG_EXYNOS4X12_1500MHZ_SUPPORT
	for_1500 = true;
	max_support_idx = L0;
#elif defined(CONFIG_EXYNOS4X12_1200MHZ_SUPPORT)
	for_1200 = true;
	max_support_idx = L3;
#elif defined(CONFIG_EXYNOS4X12_1400MHZ_SUPPORT)
	for_1400 = true;
	max_support_idx = L1;
#else
	max_support_idx = L5;
#endif
	/*
	 * Should be fixed !!!
	 */
#if 0
	if ((asv_group == 0) || !for_1400)
		exynos4212_freq_table[L0].frequency = CPUFREQ_ENTRY_INVALID;
#else
	if (!for_1500 && !for_1200 && !for_1400) {
		exynos4x12_freq_table[L0].frequency = CPUFREQ_ENTRY_INVALID;
		exynos4x12_freq_table[L1].frequency = CPUFREQ_ENTRY_INVALID;
		exynos4x12_freq_table[L2].frequency = CPUFREQ_ENTRY_INVALID;
		exynos4x12_freq_table[L3].frequency = CPUFREQ_ENTRY_INVALID;
		exynos4x12_freq_table[L4].frequency = CPUFREQ_ENTRY_INVALID;
	} else if (for_1200) {
		exynos4x12_freq_table[L0].frequency = CPUFREQ_ENTRY_INVALID;
		exynos4x12_freq_table[L1].frequency = CPUFREQ_ENTRY_INVALID;
		exynos4x12_freq_table[L2].frequency = CPUFREQ_ENTRY_INVALID;
	} else if (for_1400) {
		exynos4x12_freq_table[L0].frequency = CPUFREQ_ENTRY_INVALID;
	}

#endif

	pr_info("DVFS : VDD_ARM Voltage table set with %d Group\n", exynos_result_of_asv);

	if (soc_is_exynos4212()) {
		for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++)
			exynos4x12_volt_table[i] = asv_voltage_4212[i];

	} else if (soc_is_exynos4412()) {
		if (exynos_result_of_asv == 0xff) {
			for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++)
				exynos4x12_volt_table[i] = asv_voltage_s[i];
		} else {
			for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++)
			{
#if defined(CONFIG_CPU_TYPE_POP2G_ELITE) || defined(CONFIG_CPU_TYPE_POP2G_SUPPER)
				exynos4x12_volt_table[i] =
                                        asv_voltage_step_12_5[i][exynos_result_of_asv] + CPU_MARGIN;
#else
				exynos4x12_volt_table[i] =
					asv_voltage_step_12_5[i][exynos_result_of_asv];
#endif
				printk("exynos4x12_volt_table[%d]:%d\n", i, exynos4x12_volt_table[i]);
			}
		}
	} else {
		pr_err("%s: Can't find SoC type \n", __func__);
	}
}

int exynos4x12_cpufreq_init(struct exynos_dvfs_info *info)
{
	int i;
	unsigned int tmp;
	unsigned long rate;

	set_volt_table();
#if defined(CONFIG_CPU_TYPE_SCP_ELITE) || defined(CONFIG_CPU_TYPE_SCP_SUPPER)
	printk("%s:topeet scp\n", __FUNCTION__);
#elif defined(CONFIG_CPU_TYPE_POP2G_ELITE) || defined(CONFIG_CPU_TYPE_POP2G_SUPPER)
	printk("%s:topeet pop 2G\n", __FUNCTION__);
#else
	printk("%s:topeet pop 1G\n", __FUNCTION__);
#endif

	cpu_clk = clk_get(NULL, "armclk");
	if (IS_ERR(cpu_clk))
		return PTR_ERR(cpu_clk);

	moutcore = clk_get(NULL, "moutcore");
	if (IS_ERR(moutcore))
		goto err_moutcore;
	
	mout_mpll = clk_get(NULL, "mout_mpll");
	if (IS_ERR(mout_mpll))
		goto err_mout_mpll;
	
	rate = clk_get_rate(mout_mpll) / 1000;
	
	mout_apll = clk_get(NULL, "mout_apll");
	if (IS_ERR(mout_apll))
		goto err_mout_apll;

	for (i = L0; i <  CPUFREQ_LEVEL_END; i++) {

		exynos4x12_clkdiv_table[i].index = i;

		tmp = __raw_readl(EXYNOS4_CLKDIV_CPU);

		tmp &= ~(EXYNOS4_CLKDIV_CPU0_CORE_MASK |
			EXYNOS4_CLKDIV_CPU0_COREM0_MASK |
			EXYNOS4_CLKDIV_CPU0_COREM1_MASK |
			EXYNOS4_CLKDIV_CPU0_PERIPH_MASK |
			EXYNOS4_CLKDIV_CPU0_ATB_MASK |
			EXYNOS4_CLKDIV_CPU0_PCLKDBG_MASK |
			EXYNOS4_CLKDIV_CPU0_APLL_MASK);

		if (soc_is_exynos4212()) {
			tmp |= ((clkdiv_cpu0_4212[i][0] << EXYNOS4_CLKDIV_CPU0_CORE_SHIFT) |
				(clkdiv_cpu0_4212[i][1] << EXYNOS4_CLKDIV_CPU0_COREM0_SHIFT) |
				(clkdiv_cpu0_4212[i][2] << EXYNOS4_CLKDIV_CPU0_COREM1_SHIFT) |
				(clkdiv_cpu0_4212[i][3] << EXYNOS4_CLKDIV_CPU0_PERIPH_SHIFT) |
				(clkdiv_cpu0_4212[i][4] << EXYNOS4_CLKDIV_CPU0_ATB_SHIFT) |
				(clkdiv_cpu0_4212[i][5] << EXYNOS4_CLKDIV_CPU0_PCLKDBG_SHIFT) |
				(clkdiv_cpu0_4212[i][6] << EXYNOS4_CLKDIV_CPU0_APLL_SHIFT));
		} else {
			tmp &= ~EXYNOS4_CLKDIV_CPU0_CORE2_MASK;

			tmp |= ((clkdiv_cpu0_4412[i][0] << EXYNOS4_CLKDIV_CPU0_CORE_SHIFT) |
				(clkdiv_cpu0_4412[i][1] << EXYNOS4_CLKDIV_CPU0_COREM0_SHIFT) |
				(clkdiv_cpu0_4412[i][2] << EXYNOS4_CLKDIV_CPU0_COREM1_SHIFT) |
				(clkdiv_cpu0_4412[i][3] << EXYNOS4_CLKDIV_CPU0_PERIPH_SHIFT) |
				(clkdiv_cpu0_4412[i][4] << EXYNOS4_CLKDIV_CPU0_ATB_SHIFT) |
				(clkdiv_cpu0_4412[i][5] << EXYNOS4_CLKDIV_CPU0_PCLKDBG_SHIFT) |
				(clkdiv_cpu0_4412[i][6] << EXYNOS4_CLKDIV_CPU0_APLL_SHIFT) |
				(clkdiv_cpu0_4412[i][7] << EXYNOS4_CLKDIV_CPU0_CORE2_SHIFT));
		}

		exynos4x12_clkdiv_table[i].clkdiv = tmp;

		tmp = __raw_readl(EXYNOS4_CLKDIV_CPU1);

		if (soc_is_exynos4212()) {
			tmp &= ~(EXYNOS4_CLKDIV_CPU1_COPY_MASK |
				EXYNOS4_CLKDIV_CPU1_HPM_MASK);
			tmp |= ((clkdiv_cpu1_4212[i][0] << EXYNOS4_CLKDIV_CPU1_COPY_SHIFT) |
				(clkdiv_cpu1_4212[i][1] << EXYNOS4_CLKDIV_CPU1_HPM_SHIFT));
		} else {
			tmp &= ~(EXYNOS4_CLKDIV_CPU1_COPY_MASK |
				EXYNOS4_CLKDIV_CPU1_HPM_MASK |
				EXYNOS4_CLKDIV_CPU1_CORES_MASK);
			tmp |= ((clkdiv_cpu1_4412[i][0] << EXYNOS4_CLKDIV_CPU1_COPY_SHIFT) |
				(clkdiv_cpu1_4412[i][1] << EXYNOS4_CLKDIV_CPU1_HPM_SHIFT) |
				(clkdiv_cpu1_4412[i][2] << EXYNOS4_CLKDIV_CPU1_CORES_SHIFT));
		}
		exynos4x12_clkdiv_table[i].clkdiv1 = tmp;
	}

	info->mpll_freq_khz = rate;
	info->pm_lock_idx = max_support_idx;
	info->pll_safe_idx = L7;
	info->max_support_idx = max_support_idx;
	info->min_support_idx = min_support_idx;
	info->cpu_clk = cpu_clk;
	info->volt_table = exynos4x12_volt_table;
	info->freq_table = exynos4x12_freq_table;
	info->set_freq = exynos4x12_set_frequency;
	info->need_apll_change = exynos4x12_pms_change;
        //Robin Wang, clkgate for g2d pass..
	tmp = __raw_readl(EXYNOS4_CLKGATE_IP_DMC);	
	tmp |= (0x7<<23);	
	__raw_writel(tmp, EXYNOS4_CLKGATE_IP_DMC);

#ifdef ENABLE_CLKOUT
	tmp = __raw_readl(EXYNOS4_CLKOUT_CMU_CPU);
	tmp &= ~0xffff;
	tmp |= 0x1904;
	__raw_writel(tmp, EXYNOS4_CLKOUT_CMU_CPU);

	tmp = __raw_readl(S5P_PMU_DEBUG);
	tmp &= ~0xf00;
	tmp |= 0x400;
	__raw_writel(tmp, S5P_PMU_DEBUG);

#endif

	return 0;

err_mout_apll:
	if (!IS_ERR(mout_mpll))
		clk_put(mout_mpll);
err_mout_mpll:
	if (!IS_ERR(moutcore))
		clk_put(moutcore);
err_moutcore:
	if (!IS_ERR(cpu_clk))
		clk_put(cpu_clk);

	pr_debug("%s: failed initialization\n", __func__);
	return -EINVAL;
}
EXPORT_SYMBOL(exynos4x12_cpufreq_init);
