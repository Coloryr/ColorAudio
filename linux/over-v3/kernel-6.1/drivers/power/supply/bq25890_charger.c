// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * TI BQ25890 charger driver
 *
 * Copyright (C) 2015 Intel Corporation
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/power/bq25890_charger.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/types.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/usb/phy.h>

#include <linux/acpi.h>
#include <linux/of.h>

#define BQ25890_MANUFACTURER		"Texas Instruments"
#define BQ25890_IRQ_PIN			"bq25890_irq"

#define BQ25890_ID			3
#define BQ25895_ID			7
#define BQ25896_ID			0
#define SY6970_ID			1

#define PUMP_EXPRESS_START_DELAY	(5 * HZ)
#define PUMP_EXPRESS_MAX_TRIES		6
#define PUMP_EXPRESS_VBUS_MARGIN_uV	1000000

enum bq25890_chip_version {
	BQ25890,
	BQ25892,
	BQ25895,
	BQ25896,
	SY6970,
};

static const char *const bq25890_chip_name[] = {
	"BQ25890",
	"BQ25892",
	"BQ25895",
	"BQ25896",
	"SY6970",
};

enum bq25890_fields {
	F_EN_HIZ, F_EN_ILIM, F_IINLIM,				     /* Reg00 */
	F_BHOT, F_BCOLD, F_VINDPM_OFS,				     /* Reg01 */
	F_CONV_START, F_CONV_RATE, F_BOOSTF, F_ICO_EN,
	F_HVDCP_EN, F_MAXC_EN, F_FORCE_DPM, F_AUTO_DPDM_EN,	     /* Reg02 */
	F_BAT_LOAD_EN, F_WD_RST, F_OTG_CFG, F_CHG_CFG, F_SYSVMIN,
	F_MIN_VBAT_SEL,						     /* Reg03 */
	F_PUMPX_EN, F_ICHG,					     /* Reg04 */
	F_IPRECHG, F_ITERM,					     /* Reg05 */
	F_VREG, F_BATLOWV, F_VRECHG,				     /* Reg06 */
	F_TERM_EN, F_STAT_DIS, F_WD, F_TMR_EN, F_CHG_TMR,
	F_JEITA_ISET,						     /* Reg07 */
	F_BATCMP, F_VCLAMP, F_TREG,				     /* Reg08 */
	F_FORCE_ICO, F_TMR2X_EN, F_BATFET_DIS, F_JEITA_VSET,
	F_BATFET_DLY, F_BATFET_RST_EN, F_PUMPX_UP, F_PUMPX_DN,	     /* Reg09 */
	F_BOOSTV, F_PFM_OTG_DIS, F_BOOSTI,			     /* Reg0A */
	F_VBUS_STAT, F_CHG_STAT, F_PG_STAT, F_SDP_STAT, F_0B_RSVD,
	F_VSYS_STAT,						     /* Reg0B */
	F_WD_FAULT, F_BOOST_FAULT, F_CHG_FAULT, F_BAT_FAULT,
	F_NTC_FAULT,						     /* Reg0C */
	F_FORCE_VINDPM, F_VINDPM,				     /* Reg0D */
	F_THERM_STAT, F_BATV,					     /* Reg0E */
	F_SYSV,							     /* Reg0F */
	F_TSPCT,						     /* Reg10 */
	F_VBUS_GD, F_VBUSV,					     /* Reg11 */
	F_ICHGR,						     /* Reg12 */
	F_VDPM_STAT, F_IDPM_STAT, F_IDPM_LIM,			     /* Reg13 */
	F_REG_RST, F_ICO_OPTIMIZED, F_PN, F_TS_PROFILE, F_DEV_REV,   /* Reg14 */

	F_MAX_FIELDS
};

/* initial field values, converted to register values */
struct bq25890_init_data {
	u8 ichg;	/* charge current		*/
	u8 vreg;	/* regulation voltage		*/
	u8 iterm;	/* termination current		*/
	u8 iprechg;	/* precharge current		*/
	u8 sysvmin;	/* minimum system voltage limit */
	u8 boostv;	/* boost regulation voltage	*/
	u8 boosti;	/* boost current limit		*/
	u8 boostf;	/* boost frequency		*/
	u8 ilim_en;	/* enable ILIM pin		*/
	u8 treg;	/* thermal regulation threshold */
	u8 rbatcomp;	/* IBAT sense resistor value    */
	u8 vclamp;	/* IBAT compensation voltage limit */
};

struct bq25890_state {
	u8 online;
	u8 chrg_status;
	u8 chrg_fault;
	u8 vsys_status;
	u8 boost_fault;
	u8 bat_fault;
	u8 ntc_fault;
};

struct bq25890_device {
	struct i2c_client *client;
	struct device *dev;
	struct power_supply *charger;

	struct usb_phy *usb_phy;
	struct notifier_block usb_nb;
	struct work_struct usb_work;
	struct delayed_work pump_express_work;
	unsigned long usb_event;

	struct regmap *rmap;
	struct regmap_field *rmap_fields[F_MAX_FIELDS];

	bool skip_reset;
	bool read_back_init_data;
	u32 pump_express_vbus_max;
	enum bq25890_chip_version chip_version;
	struct bq25890_init_data init_data;
	struct bq25890_state state;

	struct workqueue_struct	*charger_wq;
	struct delayed_work pd_work;
	struct notifier_block nb;
	struct device_node *notify_node;
	int pd_vol;
	int pd_cur;

	struct mutex lock; /* protect state data */
};

static const struct regmap_range bq25890_readonly_reg_ranges[] = {
	regmap_reg_range(0x0b, 0x0c),
	regmap_reg_range(0x0e, 0x13),
};

static const struct regmap_access_table bq25890_writeable_regs = {
	.no_ranges = bq25890_readonly_reg_ranges,
	.n_no_ranges = ARRAY_SIZE(bq25890_readonly_reg_ranges),
};

static const struct regmap_range bq25890_volatile_reg_ranges[] = {
	regmap_reg_range(0x00, 0x00),
	regmap_reg_range(0x02, 0x02),
	regmap_reg_range(0x09, 0x09),
	regmap_reg_range(0x0b, 0x14),
};

static const struct regmap_access_table bq25890_volatile_regs = {
	.yes_ranges = bq25890_volatile_reg_ranges,
	.n_yes_ranges = ARRAY_SIZE(bq25890_volatile_reg_ranges),
};

static const struct regmap_config bq25890_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = 0x14,
	.cache_type = REGCACHE_RBTREE,

	.wr_table = &bq25890_writeable_regs,
	.volatile_table = &bq25890_volatile_regs,
};

static const struct reg_field bq25890_reg_fields[] = {
	/* REG00 */
	[F_EN_HIZ]		= REG_FIELD(0x00, 7, 7),
	[F_EN_ILIM]		= REG_FIELD(0x00, 6, 6),
	[F_IINLIM]		= REG_FIELD(0x00, 0, 5),
	/* REG01 */
	[F_BHOT]		= REG_FIELD(0x01, 6, 7),
	[F_BCOLD]		= REG_FIELD(0x01, 5, 5),
	[F_VINDPM_OFS]		= REG_FIELD(0x01, 0, 4),
	/* REG02 */
	[F_CONV_START]		= REG_FIELD(0x02, 7, 7),
	[F_CONV_RATE]		= REG_FIELD(0x02, 6, 6),
	[F_BOOSTF]		= REG_FIELD(0x02, 5, 5),
	[F_ICO_EN]		= REG_FIELD(0x02, 4, 4),
	[F_HVDCP_EN]		= REG_FIELD(0x02, 3, 3),  // reserved on BQ25896
	[F_MAXC_EN]		= REG_FIELD(0x02, 2, 2),  // reserved on BQ25896
	[F_FORCE_DPM]		= REG_FIELD(0x02, 1, 1),
	[F_AUTO_DPDM_EN]	= REG_FIELD(0x02, 0, 0),
	/* REG03 */
	[F_BAT_LOAD_EN]		= REG_FIELD(0x03, 7, 7),
	[F_WD_RST]		= REG_FIELD(0x03, 6, 6),
	[F_OTG_CFG]		= REG_FIELD(0x03, 5, 5),
	[F_CHG_CFG]		= REG_FIELD(0x03, 4, 4),
	[F_SYSVMIN]		= REG_FIELD(0x03, 1, 3),
	[F_MIN_VBAT_SEL]	= REG_FIELD(0x03, 0, 0), // BQ25896 only
	/* REG04 */
	[F_PUMPX_EN]		= REG_FIELD(0x04, 7, 7),
	[F_ICHG]		= REG_FIELD(0x04, 0, 6),
	/* REG05 */
	[F_IPRECHG]		= REG_FIELD(0x05, 4, 7),
	[F_ITERM]		= REG_FIELD(0x05, 0, 3),
	/* REG06 */
	[F_VREG]		= REG_FIELD(0x06, 2, 7),
	[F_BATLOWV]		= REG_FIELD(0x06, 1, 1),
	[F_VRECHG]		= REG_FIELD(0x06, 0, 0),
	/* REG07 */
	[F_TERM_EN]		= REG_FIELD(0x07, 7, 7),
	[F_STAT_DIS]		= REG_FIELD(0x07, 6, 6),
	[F_WD]			= REG_FIELD(0x07, 4, 5),
	[F_TMR_EN]		= REG_FIELD(0x07, 3, 3),
	[F_CHG_TMR]		= REG_FIELD(0x07, 1, 2),
	[F_JEITA_ISET]		= REG_FIELD(0x07, 0, 0), // reserved on BQ25895
	/* REG08 */
	[F_BATCMP]		= REG_FIELD(0x08, 5, 7),
	[F_VCLAMP]		= REG_FIELD(0x08, 2, 4),
	[F_TREG]		= REG_FIELD(0x08, 0, 1),
	/* REG09 */
	[F_FORCE_ICO]		= REG_FIELD(0x09, 7, 7),
	[F_TMR2X_EN]		= REG_FIELD(0x09, 6, 6),
	[F_BATFET_DIS]		= REG_FIELD(0x09, 5, 5),
	[F_JEITA_VSET]		= REG_FIELD(0x09, 4, 4), // reserved on BQ25895
	[F_BATFET_DLY]		= REG_FIELD(0x09, 3, 3),
	[F_BATFET_RST_EN]	= REG_FIELD(0x09, 2, 2),
	[F_PUMPX_UP]		= REG_FIELD(0x09, 1, 1),
	[F_PUMPX_DN]		= REG_FIELD(0x09, 0, 0),
	/* REG0A */
	[F_BOOSTV]		= REG_FIELD(0x0A, 4, 7),
	[F_BOOSTI]		= REG_FIELD(0x0A, 0, 2), // reserved on BQ25895
	[F_PFM_OTG_DIS]		= REG_FIELD(0x0A, 3, 3), // BQ25896 only
	/* REG0B */
	[F_VBUS_STAT]		= REG_FIELD(0x0B, 5, 7),
	[F_CHG_STAT]		= REG_FIELD(0x0B, 3, 4),
	[F_PG_STAT]		= REG_FIELD(0x0B, 2, 2),
	[F_SDP_STAT]		= REG_FIELD(0x0B, 1, 1), // reserved on BQ25896
	[F_VSYS_STAT]		= REG_FIELD(0x0B, 0, 0),
	/* REG0C */
	[F_WD_FAULT]		= REG_FIELD(0x0C, 7, 7),
	[F_BOOST_FAULT]		= REG_FIELD(0x0C, 6, 6),
	[F_CHG_FAULT]		= REG_FIELD(0x0C, 4, 5),
	[F_BAT_FAULT]		= REG_FIELD(0x0C, 3, 3),
	[F_NTC_FAULT]		= REG_FIELD(0x0C, 0, 2),
	/* REG0D */
	[F_FORCE_VINDPM]	= REG_FIELD(0x0D, 7, 7),
	[F_VINDPM]		= REG_FIELD(0x0D, 0, 6),
	/* REG0E */
	[F_THERM_STAT]		= REG_FIELD(0x0E, 7, 7),
	[F_BATV]		= REG_FIELD(0x0E, 0, 6),
	/* REG0F */
	[F_SYSV]		= REG_FIELD(0x0F, 0, 6),
	/* REG10 */
	[F_TSPCT]		= REG_FIELD(0x10, 0, 6),
	/* REG11 */
	[F_VBUS_GD]		= REG_FIELD(0x11, 7, 7),
	[F_VBUSV]		= REG_FIELD(0x11, 0, 6),
	/* REG12 */
	[F_ICHGR]		= REG_FIELD(0x12, 0, 6),
	/* REG13 */
	[F_VDPM_STAT]		= REG_FIELD(0x13, 7, 7),
	[F_IDPM_STAT]		= REG_FIELD(0x13, 6, 6),
	[F_IDPM_LIM]		= REG_FIELD(0x13, 0, 5),
	/* REG14 */
	[F_REG_RST]		= REG_FIELD(0x14, 7, 7),
	[F_ICO_OPTIMIZED]	= REG_FIELD(0x14, 6, 6),
	[F_PN]			= REG_FIELD(0x14, 3, 5),
	[F_TS_PROFILE]		= REG_FIELD(0x14, 2, 2),
	[F_DEV_REV]		= REG_FIELD(0x14, 0, 1)
};

/*
 * Most of the val -> idx conversions can be computed, given the minimum,
 * maximum and the step between values. For the rest of conversions, we use
 * lookup tables.
 */
enum bq25890_table_ids {
	/* range tables */
	TBL_ICHG,
	TBL_ITERM,
	TBL_IINLIM,
	TBL_VREG,
	TBL_BOOSTV,
	TBL_SYSVMIN,
	TBL_VBUSV,
	TBL_VBATCOMP,
	TBL_RBATCOMP,
	TBL_VINDPM,

	/* lookup tables */
	TBL_TREG,
	TBL_BOOSTI,
	TBL_TSPCT,
};

/* Thermal Regulation Threshold lookup table, in degrees Celsius */
static const u32 bq25890_treg_tbl[] = { 60, 80, 100, 120 };

#define BQ25890_TREG_TBL_SIZE		ARRAY_SIZE(bq25890_treg_tbl)

/* Boost mode current limit lookup table, in uA */
static const u32 bq25890_boosti_tbl[] = {
	500000, 700000, 1100000, 1300000, 1600000, 1800000, 2100000, 2400000
};

#define BQ25890_BOOSTI_TBL_SIZE		ARRAY_SIZE(bq25890_boosti_tbl)

/* NTC 10K temperature lookup table in tenths of a degree */
static const u32 bq25890_tspct_tbl[] = {
	850, 840, 830, 820, 810, 800, 790, 780,
	770, 760, 750, 740, 730, 720, 710, 700,
	690, 685, 680, 675, 670, 660, 650, 645,
	640, 630, 620, 615, 610, 600, 590, 585,
	580, 570, 565, 560, 550, 540, 535, 530,
	520, 515, 510, 500, 495, 490, 480, 475,
	470, 460, 455, 450, 440, 435, 430, 425,
	420, 410, 405, 400, 390, 385, 380, 370,
	365, 360, 355, 350, 340, 335, 330, 320,
	310, 305, 300, 290, 285, 280, 275, 270,
	260, 250, 245, 240, 230, 225, 220, 210,
	205, 200, 190, 180, 175, 170, 160, 150,
	145, 140, 130, 120, 115, 110, 100, 90,
	80, 70, 60, 50, 40, 30, 20, 10,
	0, -10, -20, -30, -40, -60, -70, -80,
	-90, -10, -120, -140, -150, -170, -190, -210,
};

#define BQ25890_TSPCT_TBL_SIZE		ARRAY_SIZE(bq25890_tspct_tbl)

struct bq25890_range {
	u32 min;
	u32 max;
	u32 step;
};

struct bq25890_lookup {
	const u32 *tbl;
	u32 size;
};

static const union {
	struct bq25890_range  rt;
	struct bq25890_lookup lt;
} bq25890_tables[] = {
	/* range tables */
	/* TODO: BQ25896 has max ICHG 3008 mA */
	[TBL_ICHG] =	 { .rt = {0,        5056000, 64000} },	 /* uA */
	[TBL_ITERM] =	 { .rt = {64000,    1024000, 64000} },	 /* uA */
	[TBL_IINLIM] =   { .rt = {100000,   3250000, 50000} },	 /* uA */
	[TBL_VREG] =	 { .rt = {3840000,  4608000, 16000} },	 /* uV */
	[TBL_BOOSTV] =	 { .rt = {4550000,  5510000, 64000} },	 /* uV */
	[TBL_SYSVMIN] =  { .rt = {3000000,  3700000, 100000} },	 /* uV */
	[TBL_VBUSV] =	 { .rt = {2600000, 15300000, 100000} },	 /* uV */
	[TBL_VBATCOMP] = { .rt = {0,         224000, 32000} },	 /* uV */
	[TBL_RBATCOMP] = { .rt = {0,         140000, 20000} },	 /* uOhm */
	[TBL_VINDPM] =	 { .rt = {100000,   3100000, 100000} },	 /* uV */

	/* lookup tables */
	[TBL_TREG] =	{ .lt = {bq25890_treg_tbl, BQ25890_TREG_TBL_SIZE} },
	[TBL_BOOSTI] =	{ .lt = {bq25890_boosti_tbl, BQ25890_BOOSTI_TBL_SIZE} },
	[TBL_TSPCT] =	{ .lt = {bq25890_tspct_tbl, BQ25890_TSPCT_TBL_SIZE} }
};

static int bq25890_field_read(struct bq25890_device *bq,
			      enum bq25890_fields field_id)
{
	int ret;
	int val;

	ret = regmap_field_read(bq->rmap_fields[field_id], &val);
	if (ret < 0)
		return ret;

	return val;
}

static int bq25890_field_write(struct bq25890_device *bq,
			       enum bq25890_fields field_id, u8 val)
{
	return regmap_field_write(bq->rmap_fields[field_id], val);
}

static u8 bq25890_find_idx(u32 value, enum bq25890_table_ids id)
{
	u8 idx;

	if (id >= TBL_TREG) {
		const u32 *tbl = bq25890_tables[id].lt.tbl;
		u32 tbl_size = bq25890_tables[id].lt.size;

		for (idx = 1; idx < tbl_size && tbl[idx] <= value; idx++)
			;
	} else {
		const struct bq25890_range *rtbl = &bq25890_tables[id].rt;
		u8 rtbl_size;

		rtbl_size = (rtbl->max - rtbl->min) / rtbl->step + 1;

		for (idx = 1;
		     idx < rtbl_size && (idx * rtbl->step + rtbl->min <= value);
		     idx++)
			;
	}

	return idx - 1;
}

static u32 bq25890_find_val(u8 idx, enum bq25890_table_ids id)
{
	const struct bq25890_range *rtbl;

	/* lookup table? */
	if (id >= TBL_TREG)
		return bq25890_tables[id].lt.tbl[idx];

	/* range table */
	rtbl = &bq25890_tables[id].rt;

	return (rtbl->min + idx * rtbl->step);
}

enum bq25890_status {
	STATUS_NOT_CHARGING,
	STATUS_PRE_CHARGING,
	STATUS_FAST_CHARGING,
	STATUS_TERMINATION_DONE,
};

enum bq25890_chrg_fault {
	CHRG_FAULT_NORMAL,
	CHRG_FAULT_INPUT,
	CHRG_FAULT_THERMAL_SHUTDOWN,
	CHRG_FAULT_TIMER_EXPIRED,
};

enum bq25890_ntc_fault {
	NTC_FAULT_NORMAL = 0,
	NTC_FAULT_WARM = 2,
	NTC_FAULT_COOL = 3,
	NTC_FAULT_COLD = 5,
	NTC_FAULT_HOT = 6,
};

static bool bq25890_is_adc_property(enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_TEMP:
		return true;

	default:
		return false;
	}
}

static irqreturn_t __bq25890_handle_irq(struct bq25890_device *bq);

static int bq25890_get_vbus_voltage(struct bq25890_device *bq)
{
	int ret;

	ret = bq25890_field_read(bq, F_VBUSV);
	if (ret < 0)
		return ret;

	return bq25890_find_val(ret, TBL_VBUSV);
}

static int bq25890_power_supply_get_property(struct power_supply *psy,
					     enum power_supply_property psp,
					     union power_supply_propval *val)
{
	struct bq25890_device *bq = power_supply_get_drvdata(psy);
	struct bq25890_state state;
	bool do_adc_conv;
	int ret;

	mutex_lock(&bq->lock);
	/* update state in case we lost an interrupt */
	__bq25890_handle_irq(bq);
	state = bq->state;
	do_adc_conv = !state.online && bq25890_is_adc_property(psp);
	if (do_adc_conv)
		bq25890_field_write(bq, F_CONV_START, 1);
	mutex_unlock(&bq->lock);

	if (do_adc_conv)
		regmap_field_read_poll_timeout(bq->rmap_fields[F_CONV_START],
			ret, !ret, 25000, 1000000);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (!state.online)
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (state.chrg_status == STATUS_NOT_CHARGING)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (state.chrg_status == STATUS_PRE_CHARGING ||
			 state.chrg_status == STATUS_FAST_CHARGING)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (state.chrg_status == STATUS_TERMINATION_DONE)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;

		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (!state.online || state.chrg_status == STATUS_NOT_CHARGING ||
		    state.chrg_status == STATUS_TERMINATION_DONE)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		else if (state.chrg_status == STATUS_PRE_CHARGING)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_STANDARD;
		else if (state.chrg_status == STATUS_FAST_CHARGING)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else /* unreachable */
			val->intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
		break;

	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = BQ25890_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = bq25890_chip_name[bq->chip_version];
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = state.online;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		if (!state.chrg_fault && !state.bat_fault && !state.boost_fault)
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		else if (state.bat_fault)
			val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		else if (state.chrg_fault == CHRG_FAULT_TIMER_EXPIRED)
			val->intval = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
		else if (state.chrg_fault == CHRG_FAULT_THERMAL_SHUTDOWN)
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		else
			val->intval = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->intval = bq25890_find_val(bq->init_data.ichg, TBL_ICHG);

		/* When temperature is too low, charge current is decreased */
		if (bq->state.ntc_fault == NTC_FAULT_COOL) {
			ret = bq25890_field_read(bq, F_JEITA_ISET);
			if (ret < 0)
				return ret;

			if (ret)
				val->intval /= 5;
			else
				val->intval /= 2;
		}
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		if (!state.online) {
			val->intval = 0;
			break;
		}

		ret = bq25890_field_read(bq, F_BATV); /* read measured value */
		if (ret < 0)
			return ret;

		/* converted_val = 2.304V + ADC_val * 20mV (table 10.3.15) */
		val->intval = 2304000 + ret * 20000;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		val->intval = bq25890_find_val(bq->init_data.vreg, TBL_VREG);
		break;

	case POWER_SUPPLY_PROP_PRECHARGE_CURRENT:
		val->intval = bq25890_find_val(bq->init_data.iprechg, TBL_ITERM);
		break;

	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		val->intval = bq25890_find_val(bq->init_data.iterm, TBL_ITERM);
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = bq25890_field_read(bq, F_IINLIM);
		if (ret < 0)
			return ret;

		val->intval = bq25890_find_val(ret, TBL_IINLIM);
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = bq25890_field_read(bq, F_SYSV); /* read measured value */
		if (ret < 0)
			return ret;

		/* converted_val = 2.304V + ADC_val * 20mV (table 10.3.15) */
		val->intval = 2304000 + ret * 20000;
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		ret = bq25890_field_read(bq, F_ICHGR); /* read measured value */
		if (ret < 0)
			return ret;

		/* converted_val = ADC_val * 50mA (table 10.3.19) */
		val->intval = ret * -50000;
		break;

	case POWER_SUPPLY_PROP_TEMP:
		ret = bq25890_field_read(bq, F_TSPCT);
		if (ret < 0)
			return ret;

		/* convert TS percentage into rough temperature */
		val->intval = bq25890_find_val(ret, TBL_TSPCT);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int bq25890_power_supply_set_property(struct power_supply *psy,
					     enum power_supply_property psp,
					     const union power_supply_propval *val)
{
	struct bq25890_device *bq = power_supply_get_drvdata(psy);
	u8 lval;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		lval = bq25890_find_idx(val->intval, TBL_IINLIM);
		return bq25890_field_write(bq, F_IINLIM, lval);
	default:
		return -EINVAL;
	}
}

static int bq25890_power_supply_property_is_writeable(struct power_supply *psy,
						      enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return true;
	default:
		return false;
	}
}

/* On the BQ25892 try to get charger-type info from our supplier */
static void bq25890_charger_external_power_changed(struct power_supply *psy)
{
	return;
	struct bq25890_device *bq = power_supply_get_drvdata(psy);
	union power_supply_propval val;
	int input_current_limit, ret;

	if (bq->chip_version != BQ25892)
		return;

	ret = power_supply_get_property_from_supplier(psy,
						      POWER_SUPPLY_PROP_USB_TYPE,
						      &val);
	if (ret)
		return;

	switch (val.intval) {
	case POWER_SUPPLY_USB_TYPE_DCP:
		input_current_limit = bq25890_find_idx(2000000, TBL_IINLIM);
		if (bq->pump_express_vbus_max) {
			queue_delayed_work(system_power_efficient_wq,
					   &bq->pump_express_work,
					   PUMP_EXPRESS_START_DELAY);
		}
		break;
	case POWER_SUPPLY_USB_TYPE_CDP:
	case POWER_SUPPLY_USB_TYPE_ACA:
		input_current_limit = bq25890_find_idx(1500000, TBL_IINLIM);
		break;
	case POWER_SUPPLY_USB_TYPE_SDP:
	default:
		input_current_limit = bq25890_find_idx(500000, TBL_IINLIM);
	}

	bq25890_field_write(bq, F_IINLIM, input_current_limit);
	power_supply_changed(psy);
}

static int bq25890_get_chip_state(struct bq25890_device *bq,
				  struct bq25890_state *state)
{
	int i, ret;

	struct {
		enum bq25890_fields id;
		u8 *data;
	} state_fields[] = {
		{F_CHG_STAT,	&state->chrg_status},
		{F_PG_STAT,	&state->online},
		{F_VSYS_STAT,	&state->vsys_status},
		{F_BOOST_FAULT, &state->boost_fault},
		{F_BAT_FAULT,	&state->bat_fault},
		{F_CHG_FAULT,	&state->chrg_fault},
		{F_NTC_FAULT,	&state->ntc_fault}
	};

	for (i = 0; i < ARRAY_SIZE(state_fields); i++) {
		ret = bq25890_field_read(bq, state_fields[i].id);
		if (ret < 0)
			return ret;

		*state_fields[i].data = ret;
	}

	dev_dbg(bq->dev, "S:CHG/PG/VSYS=%d/%d/%d, F:CHG/BOOST/BAT/NTC=%d/%d/%d/%d\n",
		state->chrg_status, state->online, state->vsys_status,
		state->chrg_fault, state->boost_fault, state->bat_fault,
		state->ntc_fault);

	return 0;
}

static irqreturn_t __bq25890_handle_irq(struct bq25890_device *bq)
{
	struct bq25890_state new_state;
	int ret;

	ret = bq25890_get_chip_state(bq, &new_state);
	if (ret < 0)
		return IRQ_NONE;

	if (!memcmp(&bq->state, &new_state, sizeof(new_state)))
		return IRQ_NONE;

	if (!new_state.online && bq->state.online) {	    /* power removed */
		/* disable ADC */
		ret = bq25890_field_write(bq, F_CONV_RATE, 0);
		if (ret < 0)
			goto error;
	} else if (new_state.online && !bq->state.online) { /* power inserted */
		/* enable ADC, to have control of charge current/voltage */
		ret = bq25890_field_write(bq, F_CONV_RATE, 1);
		if (ret < 0)
			goto error;
	}

	bq->state = new_state;
	// power_supply_changed(bq->charger);

	return IRQ_HANDLED;
error:
	dev_err(bq->dev, "Error communicating with the chip: %pe\n",
		ERR_PTR(ret));
	return IRQ_HANDLED;
}

static irqreturn_t bq25890_irq_handler_thread(int irq, void *private)
{
	struct bq25890_device *bq = private;
	irqreturn_t ret;

	mutex_lock(&bq->lock);
	ret = __bq25890_handle_irq(bq);
	mutex_unlock(&bq->lock);

	return ret;
}

static int bq25890_chip_reset(struct bq25890_device *bq)
{
	int ret;
	int rst_check_counter = 10;

	ret = bq25890_field_write(bq, F_REG_RST, 1);
	if (ret < 0)
		return ret;

	do {
		ret = bq25890_field_read(bq, F_REG_RST);
		if (ret < 0)
			return ret;

		usleep_range(5, 10);
	} while (ret == 1 && --rst_check_counter);

	if (!rst_check_counter)
		return -ETIMEDOUT;

	return 0;
}

static int bq25890_rw_init_data(struct bq25890_device *bq)
{
	bool write = !bq->read_back_init_data;
	int ret;
	int i;

	const struct {
		enum bq25890_fields id;
		u8 *value;
	} init_data[] = {
		{F_ICHG,	 &bq->init_data.ichg},
		{F_VREG,	 &bq->init_data.vreg},
		{F_ITERM,	 &bq->init_data.iterm},
		{F_IPRECHG,	 &bq->init_data.iprechg},
		{F_SYSVMIN,	 &bq->init_data.sysvmin},
		{F_BOOSTV,	 &bq->init_data.boostv},
		{F_BOOSTI,	 &bq->init_data.boosti},
		{F_BOOSTF,	 &bq->init_data.boostf},
		{F_EN_ILIM,	 &bq->init_data.ilim_en},
		{F_TREG,	 &bq->init_data.treg},
		{F_BATCMP,	 &bq->init_data.rbatcomp},
		{F_VCLAMP,	 &bq->init_data.vclamp},
	};

	for (i = 0; i < ARRAY_SIZE(init_data); i++) {
		if (write) {
			ret = bq25890_field_write(bq, init_data[i].id,
						  *init_data[i].value);
		} else {
			ret = bq25890_field_read(bq, init_data[i].id);
			if (ret >= 0)
				*init_data[i].value = ret;
		}
		if (ret < 0) {
			dev_dbg(bq->dev, "Accessing init data failed %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int bq25890_hw_init(struct bq25890_device *bq)
{
	int ret;

	if (!bq->skip_reset) {
		ret = bq25890_chip_reset(bq);
		if (ret < 0) {
			dev_dbg(bq->dev, "Reset failed %d\n", ret);
			return ret;
		}
	} else {
		/*
		 * Ensure charging is enabled, on some boards where the fw
		 * takes care of initalizition F_CHG_CFG is set to 0 before
		 * handing control over to the OS.
		 */
		ret = bq25890_field_write(bq, F_CHG_CFG, 1);
		if (ret < 0) {
			dev_dbg(bq->dev, "Enabling charging failed %d\n", ret);
			return ret;
		}
	}

	/* disable watchdog */
	ret = bq25890_field_write(bq, F_WD, 0);
	if (ret < 0) {
		dev_dbg(bq->dev, "Disabling watchdog failed %d\n", ret);
		return ret;
	}

	/* initialize currents/voltages and other parameters */
	ret = bq25890_rw_init_data(bq);
	if (ret)
		return ret;

	ret = bq25890_get_chip_state(bq, &bq->state);
	if (ret < 0) {
		dev_dbg(bq->dev, "Get state failed %d\n", ret);
		return ret;
	}

	/* Configure ADC for continuous conversions when charging */
	ret = bq25890_field_write(bq, F_CONV_RATE, !!bq->state.online);
	if (ret < 0) {
		dev_dbg(bq->dev, "Config ADC failed %d\n", ret);
		return ret;
	}

	return 0;
}

static const enum power_supply_property bq25890_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_PRECHARGE_CURRENT,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_TEMP,
};

static char *bq25890_charger_supplied_to[] = {
	"main-battery",
};

static const struct power_supply_desc bq25890_power_supply_desc = {
	.name = "bq25890-charger",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = bq25890_power_supply_props,
	.num_properties = ARRAY_SIZE(bq25890_power_supply_props),
	.get_property = bq25890_power_supply_get_property,
	.set_property = bq25890_power_supply_set_property,
	.property_is_writeable = bq25890_power_supply_property_is_writeable,
	.external_power_changed	= bq25890_charger_external_power_changed,
};

static int bq25890_power_supply_init(struct bq25890_device *bq)
{
	struct power_supply_config psy_cfg = { .drv_data = bq, };

	psy_cfg.supplied_to = bq25890_charger_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(bq25890_charger_supplied_to);
	psy_cfg.of_node = bq->dev->of_node;

	bq->charger = devm_power_supply_register(bq->dev,
						 &bq25890_power_supply_desc,
						 &psy_cfg);

	return PTR_ERR_OR_ZERO(bq->charger);
}

static int bq25890_set_otg_cfg(struct bq25890_device *bq, u8 val)
{
	int ret;

	ret = bq25890_field_write(bq, F_OTG_CFG, val);
	if (ret < 0)
		dev_err(bq->dev, "Error switching to boost/charger mode: %d\n", ret);

	return ret;
}

static void bq25890_pump_express_work(struct work_struct *data)
{
	struct bq25890_device *bq =
		container_of(data, struct bq25890_device, pump_express_work.work);
	int voltage, i, ret;

	dev_dbg(bq->dev, "Start to request input voltage increasing\n");

	/* Enable current pulse voltage control protocol */
	ret = bq25890_field_write(bq, F_PUMPX_EN, 1);
	if (ret < 0)
		goto error_print;

	for (i = 0; i < PUMP_EXPRESS_MAX_TRIES; i++) {
		voltage = bq25890_get_vbus_voltage(bq);
		if (voltage < 0)
			goto error_print;
		dev_dbg(bq->dev, "input voltage = %d uV\n", voltage);

		if ((voltage + PUMP_EXPRESS_VBUS_MARGIN_uV) >
					bq->pump_express_vbus_max)
			break;

		ret = bq25890_field_write(bq, F_PUMPX_UP, 1);
		if (ret < 0)
			goto error_print;

		/* Note a single PUMPX up pulse-sequence takes 2.1s */
		ret = regmap_field_read_poll_timeout(bq->rmap_fields[F_PUMPX_UP],
						     ret, !ret, 100000, 3000000);
		if (ret < 0)
			goto error_print;

		/* Make sure ADC has sampled Vbus before checking again */
		msleep(1000);
	}

	bq25890_field_write(bq, F_PUMPX_EN, 0);

	dev_info(bq->dev, "Hi-voltage charging requested, input voltage is %d mV\n",
		 voltage);

	power_supply_changed(bq->charger);

	return;
error_print:
	bq25890_field_write(bq, F_PUMPX_EN, 0);
	dev_err(bq->dev, "Failed to request hi-voltage charging\n");
}

static void bq25890_usb_work(struct work_struct *data)
{
	int ret;
	struct bq25890_device *bq =
			container_of(data, struct bq25890_device, usb_work);

	switch (bq->usb_event) {
	case USB_EVENT_ID:
		/* Enable boost mode */
		bq25890_set_otg_cfg(bq, 1);
		break;

	case USB_EVENT_NONE:
		/* Disable boost mode */
		ret = bq25890_set_otg_cfg(bq, 0);
		if (ret == 0)
			power_supply_changed(bq->charger);
		break;
	}
}

static int bq25890_usb_notifier(struct notifier_block *nb, unsigned long val,
				void *priv)
{
	struct bq25890_device *bq =
			container_of(nb, struct bq25890_device, usb_nb);

	bq->usb_event = val;
	queue_work(system_power_efficient_wq, &bq->usb_work);

	return NOTIFY_OK;
}

#ifdef CONFIG_REGULATOR
static int bq25890_vbus_enable(struct regulator_dev *rdev)
{
	struct bq25890_device *bq = rdev_get_drvdata(rdev);

	return bq25890_set_otg_cfg(bq, 1);
}

static int bq25890_vbus_disable(struct regulator_dev *rdev)
{
	struct bq25890_device *bq = rdev_get_drvdata(rdev);

	return bq25890_set_otg_cfg(bq, 0);
}

static int bq25890_vbus_is_enabled(struct regulator_dev *rdev)
{
	struct bq25890_device *bq = rdev_get_drvdata(rdev);

	return bq25890_field_read(bq, F_OTG_CFG);
}

static const struct regulator_ops bq25890_vbus_ops = {
	.enable = bq25890_vbus_enable,
	.disable = bq25890_vbus_disable,
	.is_enabled = bq25890_vbus_is_enabled,
};

static const struct regulator_desc bq25890_vbus_desc = {
	.name = "usb_otg_vbus",
	.of_match = "usb-otg-vbus",
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
	.ops = &bq25890_vbus_ops,
	.fixed_uV = 5000000,
	.n_voltages = 1,
};

static int bq25890_register_regulator(struct bq25890_device *bq)
{
	struct bq25890_platform_data *pdata = dev_get_platdata(bq->dev);
	struct regulator_config cfg = {
		.dev = bq->dev,
		.driver_data = bq,
	};
	struct regulator_dev *reg;

	if (!IS_ERR_OR_NULL(bq->usb_phy))
		return 0;

	if (pdata)
		cfg.init_data = pdata->regulator_init_data;

	reg = devm_regulator_register(bq->dev, &bq25890_vbus_desc, &cfg);
	if (IS_ERR(reg)) {
		return dev_err_probe(bq->dev, PTR_ERR(reg),
				     "registering vbus regulator");
	}

	return 0;
}
#else
static inline int
bq25890_register_regulator(struct bq25890_device *bq)
{
	return 0;
}
#endif

static int bq25890_get_chip_version(struct bq25890_device *bq)
{
	int id, rev;

	id = bq25890_field_read(bq, F_PN);
	if (id < 0) {
		dev_err(bq->dev, "Cannot read chip ID: %d\n", id);
		return id;
	}

	rev = bq25890_field_read(bq, F_DEV_REV);
	if (rev < 0) {
		dev_err(bq->dev, "Cannot read chip revision: %d\n", rev);
		return rev;
	}

	switch (id) {
	case BQ25890_ID:
		bq->chip_version = BQ25890;
		break;

	/* BQ25892 and BQ25896 share same ID 0 */
	case BQ25896_ID:
		switch (rev) {
		case 2:
			bq->chip_version = BQ25896;
			break;
		case 1:
			bq->chip_version = BQ25892;
			break;
		default:
			dev_err(bq->dev,
				"Unknown device revision %d, assume BQ25892\n",
				rev);
			bq->chip_version = BQ25892;
		}
		break;

	case BQ25895_ID:
		bq->chip_version = BQ25895;
		break;

	case SY6970_ID:
		bq->chip_version = SY6970;
		break;

	default:
		dev_err(bq->dev, "Unknown chip ID %d\n", id);
		return -ENODEV;
	}

	return 0;
}

static int bq25890_irq_probe(struct bq25890_device *bq)
{
	struct gpio_desc *irq;

	irq = devm_gpiod_get(bq->dev, BQ25890_IRQ_PIN, GPIOD_IN);
	if (IS_ERR(irq))
		return dev_err_probe(bq->dev, PTR_ERR(irq),
				     "Could not probe irq pin.\n");

	return gpiod_to_irq(irq);
}

static int bq25890_fw_read_u32_props(struct bq25890_device *bq)
{
	int ret;
	u32 property;
	int i;
	struct bq25890_init_data *init = &bq->init_data;
	struct {
		char *name;
		bool optional;
		enum bq25890_table_ids tbl_id;
		u8 *conv_data; /* holds converted value from given property */
	} props[] = {
		/* required properties */
		{"ti,charge-current", false, TBL_ICHG, &init->ichg},
		{"ti,battery-regulation-voltage", false, TBL_VREG, &init->vreg},
		{"ti,termination-current", false, TBL_ITERM, &init->iterm},
		{"ti,precharge-current", false, TBL_ITERM, &init->iprechg},
		{"ti,minimum-sys-voltage", false, TBL_SYSVMIN, &init->sysvmin},
		{"ti,boost-voltage", false, TBL_BOOSTV, &init->boostv},
		{"ti,boost-max-current", false, TBL_BOOSTI, &init->boosti},

		/* optional properties */
		{"ti,thermal-regulation-threshold", true, TBL_TREG, &init->treg},
		{"ti,ibatcomp-micro-ohms", true, TBL_RBATCOMP, &init->rbatcomp},
		{"ti,ibatcomp-clamp-microvolt", true, TBL_VBATCOMP, &init->vclamp},
	};

	/* initialize data for optional properties */
	init->treg = 3; /* 120 degrees Celsius */
	init->rbatcomp = init->vclamp = 0; /* IBAT compensation disabled */

	for (i = 0; i < ARRAY_SIZE(props); i++) {
		ret = device_property_read_u32(bq->dev, props[i].name,
					       &property);
		if (ret < 0) {
			if (props[i].optional)
				continue;

			dev_err(bq->dev, "Unable to read property %d %s\n", ret,
				props[i].name);

			return ret;
		}

		*props[i].conv_data = bq25890_find_idx(property,
						       props[i].tbl_id);
	}

	return 0;
}

static int bq25890_fw_probe(struct bq25890_device *bq)
{
	int ret;
	struct bq25890_init_data *init = &bq->init_data;

	/* Optional, left at 0 if property is not present */
	device_property_read_u32(bq->dev, "linux,pump-express-vbus-max",
				 &bq->pump_express_vbus_max);

	bq->skip_reset = device_property_read_bool(bq->dev, "linux,skip-reset");
	bq->read_back_init_data = device_property_read_bool(bq->dev,
						"linux,read-back-settings");
	if (bq->read_back_init_data)
		return 0;

	ret = bq25890_fw_read_u32_props(bq);
	if (ret < 0)
		return ret;

	init->ilim_en = device_property_read_bool(bq->dev, "ti,use-ilim-pin");
	init->boostf = device_property_read_bool(bq->dev, "ti,boost-low-freq");
	bq->notify_node = of_parse_phandle(bq->dev->of_node,
					   "ti,usb-charger-detection", 0);

	return 0;
}

static void bq25890_set_pd_param(struct bq25890_device *bq, int vol, int cur)
{
	int vindpm, iilim, ichg, vol_limit;
	int i = 0;

	iilim = bq25890_find_idx(cur, TBL_IINLIM);
	ichg = bq25890_find_idx(cur, TBL_ICHG);

	vol_limit = vol;
	if (vol < 5000000)
		vol_limit = 5000000;
	vol_limit = vol_limit - 1280000 - 3200000;

	if (vol > 6000000)
		vol_limit /= 2;
	vindpm = bq25890_find_idx(vol_limit, TBL_VINDPM);

	while (!bq25890_field_read(bq, F_PG_STAT) && i < 5) {
		msleep(500);
		i++;
	}

	bq25890_field_write(bq, F_IINLIM, iilim);
	bq25890_field_write(bq, F_VINDPM_OFS, vindpm);
	bq25890_field_write(bq, F_ICHG, ichg);
	dev_info(bq->dev, "vol=%d cur=%d  INPUT_CURRENT:%x, INPUT_VOLTAGE:%x, CHARGE_CURRENT:%x\n",
		 vol, cur, iilim, vindpm, ichg);

	bq25890_get_chip_state(bq, &bq->state);
	power_supply_changed(bq->charger);
}

static int bq25890_pd_notifier_call(struct notifier_block *nb,
				    unsigned long val, void *v)
{
	struct bq25890_device *bq =
		container_of(nb, struct bq25890_device, nb);
	struct power_supply *psy = v;
	union power_supply_propval prop;
	int ret;

	if (val != PSY_EVENT_PROP_CHANGED)
		return NOTIFY_OK;

	/* Ignore event if it was not send by notify_node/notify_device */
	if (bq->notify_node) {
		if (!psy->dev.parent ||
		    psy->dev.parent->of_node != bq->notify_node)
			return NOTIFY_OK;
	}

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_ONLINE, &prop);
	if (ret != 0)
		return NOTIFY_OK;
	/* online=0: USB out */
	if (prop.intval == 0) {
		bq->pd_cur = 450000;
		bq->pd_vol = 5000000;
		queue_delayed_work(bq->charger_wq, &bq->pd_work,
				   msecs_to_jiffies(10));
		return NOTIFY_OK;
	}

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_CURRENT_NOW, &prop);
	if (ret != 0)
		return NOTIFY_OK;
	bq->pd_cur = prop.intval;
	if (bq->pd_cur > 0) {
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW,
						&prop);
		if (ret != 0)
			return NOTIFY_OK;
		bq->pd_vol = prop.intval;

		queue_delayed_work(bq->charger_wq, &bq->pd_work,
				   msecs_to_jiffies(100));
	}

	return NOTIFY_OK;
}

static void bq25890_pd_evt_worker(struct work_struct *work)
{
	struct bq25890_device *bq = container_of(work,
						 struct bq25890_device,
						 pd_work.work);

	bq25890_set_pd_param(bq, bq->pd_vol, bq->pd_cur);
}

static int bq25890_register_pd_psy(struct bq25890_device *bq)
{
	struct power_supply *notify_psy = NULL;
	union power_supply_propval prop;
	int ret;

	if (!bq->notify_node)
		return -EINVAL;

	bq->charger_wq = alloc_ordered_workqueue("%s",
						 WQ_MEM_RECLAIM |
						 WQ_FREEZABLE,
						 "bq25890-charge-wq");
	INIT_DELAYED_WORK(&bq->pd_work,
			  bq25890_pd_evt_worker);

	bq->nb.notifier_call = bq25890_pd_notifier_call;
	ret = power_supply_reg_notifier(&bq->nb);
	if (ret) {
		dev_err(bq->dev, "failed to reg notifier: %d\n", ret);
		return ret;
	}

	bq25890_field_write(bq, F_AUTO_DPDM_EN, 0);
	if (bq->nb.notifier_call) {
		notify_psy = power_supply_get_by_phandle(bq->dev->of_node,
						"ti,usb-charger-detection");
		if (IS_ERR_OR_NULL(notify_psy)) {
			dev_info(bq->dev, "bq25700 notify_psy is error\n");
			notify_psy = NULL;
		}
	}

	if (notify_psy) {
		ret = power_supply_get_property(notify_psy,
						POWER_SUPPLY_PROP_CURRENT_MAX,
						&prop);
		if (ret != 0)
			return ret;
		bq->pd_cur = prop.intval;

		ret = power_supply_get_property(notify_psy,
						POWER_SUPPLY_PROP_VOLTAGE_MAX,
						&prop);
		if (ret != 0)
			return ret;
		bq->pd_vol = prop.intval;

		queue_delayed_work(bq->charger_wq, &bq->pd_work,
				   msecs_to_jiffies(10));
	}

	return 0;
}

static void bq25890_non_devm_cleanup(void *data)
{
	struct bq25890_device *bq = data;

	cancel_delayed_work_sync(&bq->pump_express_work);
}

static int bq25890_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct bq25890_device *bq;
	int ret;

	bq = devm_kzalloc(dev, sizeof(*bq), GFP_KERNEL);
	if (!bq)
		return -ENOMEM;

	bq->client = client;
	bq->dev = dev;

	mutex_init(&bq->lock);
	INIT_DELAYED_WORK(&bq->pump_express_work, bq25890_pump_express_work);

	bq->rmap = devm_regmap_init_i2c(client, &bq25890_regmap_config);
	if (IS_ERR(bq->rmap))
		return dev_err_probe(dev, PTR_ERR(bq->rmap),
				     "failed to allocate register map\n");

	ret = devm_regmap_field_bulk_alloc(dev, bq->rmap, bq->rmap_fields,
					   bq25890_reg_fields, F_MAX_FIELDS);
	if (ret)
		return ret;

	i2c_set_clientdata(client, bq);

	ret = bq25890_get_chip_version(bq);
	if (ret) {
		dev_err(dev, "Cannot read chip ID or unknown chip: %d\n", ret);
		return ret;
	}

	ret = bq25890_fw_probe(bq);
	if (ret < 0)
		return dev_err_probe(dev, ret, "reading device properties\n");

	ret = bq25890_hw_init(bq);
	if (ret < 0) {
		dev_err(dev, "Cannot initialize the chip: %d\n", ret);
		return ret;
	}

	if (client->irq <= 0)
		client->irq = bq25890_irq_probe(bq);

	if (client->irq < 0) {
		dev_err(dev, "No irq resource found.\n");
		return client->irq;
	}

	/* OTG reporting */
	bq->usb_phy = devm_usb_get_phy(dev, USB_PHY_TYPE_USB2);

	/*
	 * This must be before bq25890_power_supply_init(), so that it runs
	 * after devm unregisters the power_supply.
	 */
	ret = devm_add_action_or_reset(dev, bq25890_non_devm_cleanup, bq);
	if (ret)
		return ret;

	ret = bq25890_register_regulator(bq);
	if (ret)
		return ret;

	if (!IS_ERR_OR_NULL(bq->usb_phy)) {
		INIT_WORK(&bq->usb_work, bq25890_usb_work);
		bq->usb_nb.notifier_call = bq25890_usb_notifier;
		usb_register_notifier(bq->usb_phy, &bq->usb_nb);
	}

	ret = bq25890_power_supply_init(bq);
	if (ret < 0) {
		dev_err(dev, "Failed to register power supply\n");
		goto err_unregister_usb_notifier;
	}

	ret = devm_request_threaded_irq(dev, client->irq, NULL,
					bq25890_irq_handler_thread,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					BQ25890_IRQ_PIN, bq);
	if (ret)
		goto err_unregister_usb_notifier;

	bq25890_register_pd_psy(bq);

	bq25890_field_write(bq, F_AUTO_DPDM_EN, 0);
	bq25890_field_write(bq, F_EN_ILIM, 0);
	bq25890_field_write(bq, F_IINLIM, bq25890_find_idx(2000000, TBL_IINLIM));
	dev_info(bq->dev, "bq25700 set done\n");	

	return 0;

err_unregister_usb_notifier:
	if (!IS_ERR_OR_NULL(bq->usb_phy))
		usb_unregister_notifier(bq->usb_phy, &bq->usb_nb);

	return ret;
}

static void bq25890_remove(struct i2c_client *client)
{
	struct bq25890_device *bq = i2c_get_clientdata(client);

	if (!IS_ERR_OR_NULL(bq->usb_phy))
		usb_unregister_notifier(bq->usb_phy, &bq->usb_nb);

	if (!bq->skip_reset) {
		/* reset all registers to default values */
		bq25890_chip_reset(bq);
	}
}

static void bq25890_shutdown(struct i2c_client *client)
{
	struct bq25890_device *bq = i2c_get_clientdata(client);

	/*
	 * TODO this if + return should probably be removed, but that would
	 * introduce a function change for boards using the usb-phy framework.
	 * This needs to be tested on such a board before making this change.
	 */
	if (!IS_ERR_OR_NULL(bq->usb_phy))
		return;

	/*
	 * Turn off the 5v Boost regulator which outputs Vbus to the device's
	 * Micro-USB or Type-C USB port. Leaving this on drains power and
	 * this avoids the PMIC on some device-models seeing this as Vbus
	 * getting inserted after shutdown, causing the device to immediately
	 * power-up again.
	 */
	bq25890_set_otg_cfg(bq, 0);
}

#ifdef CONFIG_PM_SLEEP
static int bq25890_suspend(struct device *dev)
{
	struct bq25890_device *bq = dev_get_drvdata(dev);

	/*
	 * If charger is removed, while in suspend, make sure ADC is diabled
	 * since it consumes slightly more power.
	 */
	return bq25890_field_write(bq, F_CONV_RATE, 0);
}

static int bq25890_resume(struct device *dev)
{
	int ret;
	struct bq25890_device *bq = dev_get_drvdata(dev);

	mutex_lock(&bq->lock);

	ret = bq25890_get_chip_state(bq, &bq->state);
	if (ret < 0)
		goto unlock;

	/* Re-enable ADC only if charger is plugged in. */
	if (bq->state.online) {
		ret = bq25890_field_write(bq, F_CONV_RATE, 1);
		if (ret < 0)
			goto unlock;
	}

	/* signal userspace, maybe state changed while suspended */
	power_supply_changed(bq->charger);

unlock:
	mutex_unlock(&bq->lock);

	return ret;
}
#endif

static const struct dev_pm_ops bq25890_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(bq25890_suspend, bq25890_resume)
};

static const struct i2c_device_id bq25890_i2c_ids[] = {
	{ "bq25890", 0 },
	{ "bq25892", 0 },
	{ "bq25895", 0 },
	{ "bq25896", 0 },
	{ "sy6970", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, bq25890_i2c_ids);

static const struct of_device_id bq25890_of_match[] = {
	{ .compatible = "ti,bq25890", },
	{ .compatible = "ti,bq25892", },
	{ .compatible = "ti,bq25895", },
	{ .compatible = "ti,bq25896", },
	{ .compatible = "sy,sy6970", },
	{ },
};
MODULE_DEVICE_TABLE(of, bq25890_of_match);

#ifdef CONFIG_ACPI
static const struct acpi_device_id bq25890_acpi_match[] = {
	{"BQ258900", 0},
	{},
};
MODULE_DEVICE_TABLE(acpi, bq25890_acpi_match);
#endif

static struct i2c_driver bq25890_driver = {
	.driver = {
		.name = "bq25890-charger",
		.of_match_table = of_match_ptr(bq25890_of_match),
		.acpi_match_table = ACPI_PTR(bq25890_acpi_match),
		.pm = &bq25890_pm,
	},
	.probe_new = bq25890_probe,
	.remove = bq25890_remove,
	.shutdown = bq25890_shutdown,
	.id_table = bq25890_i2c_ids,
};
module_i2c_driver(bq25890_driver);

MODULE_AUTHOR("Laurentiu Palcu <laurentiu.palcu@intel.com>");
MODULE_DESCRIPTION("bq25890 charger driver");
MODULE_LICENSE("GPL");
