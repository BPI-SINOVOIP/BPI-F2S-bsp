// SPDX-License-Identifier: GPL-2.0
/*
 * Sunplus q628 clock driver
 *
 * Copyright (C) 2020 Sunplus Inc.
 * Author: qinjian <qinjian@sunmedia.com.cn>
 */

#include <asm/arch/clk-sunplus.h>
#include <dt-bindings/clock/sp-q628.h>

#define CLK_PRINT(name, rate)	printf("%-16.16s:%11lu Hz\n", name, rate)

#define MASK_SET(shift, width, value) \
({ \
	u32 m = ((1 << (width)) - 1) << (shift); \
	(m << 16) | (((value) << (shift)) & m); \
})
#define MASK_GET(shift, width, value)	(((value) >> (shift)) & ((1 << (width)) - 1))

#define REG(g, i)	((void *)(0x9c000000 + ((g) * 32 + (i)) * 4))

#define PLLA_CTL	REG(4, 7)
#define PLLE_CTL	REG(4, 12)
#define PLLF_CTL	REG(4, 13)
#define PLLTV_CTL	REG(4, 14)
#define PLLTVA_CTL	REG(4, 15)
#define PLLSYS_CTL	REG(4, 26)

/* speical div_width values for PLLTV/PLLA */
#define DIV_TV	33
#define DIV_A	34

#define _M		1000000UL
#define F_27M	(27 * _M)

/* PLLTV parameters */
enum {
	SEL_FRA,
	SDM_MOD,
	PH_SEL,
	NFRA,
	DIVR,
	DIVN,
	DIVM,
	P_MAX
};

struct sp_pll {
	char	*name;
	int		parent;		/* parent clk->id, -1 for extclk */
	void	*reg;
	int		pd_bit;		/* power down bit idx */
	int		bp_bit;		/* bypass bit idx */
	ulong	brate;		/* base rate, FIXME: replace brate with muldiv */
	int		div_shift;
	int		div_width;
	u32		p[P_MAX];	/* for hold PLLTV/PLLA parameters */
};

#define _PLL(p0, p1, p2, p3, p4, p5, p6, p7) \
	{ \
		.name		= p0, \
		.parent		= p1, \
		.reg		= p2, \
		.pd_bit		= p3, \
		.bp_bit		= p4, \
		.brate		= p5, \
		.div_shift	= p6, \
		.div_width	= p7, \
	}

static struct sp_pll plls[] = {
	_PLL("plla",		EXTCLK,	PLLA_CTL,	11, 12, F_27M, 0, DIV_A),

	_PLL("plle",		EXTCLK,	PLLE_CTL,	6, 2, 50000000, 0, 0),
	_PLL("plle_2p5",	PLL_E,	PLLE_CTL,	13, -1, 2500000, 0, 0),
	_PLL("plle_25",		PLL_E,	PLLE_CTL,	12, -1, 25000000, 0, 0),
	_PLL("plle_112p5",	PLL_E,	PLLE_CTL,	11, -1, 112500000, 0, 0),

	_PLL("pllf",		EXTCLK,	PLLF_CTL,	0, 10, 13500000, 1, 4),

	_PLL("plltv",		EXTCLK,	PLLTV_CTL,	0, 15, F_27M, 0, DIV_TV),
	_PLL("plltv_a",		PLL_TV,	PLLTVA_CTL,	-1, -1, 0, 5, 1),

	_PLL("pllsys",		EXTCLK,	PLLSYS_CTL,	10, 9, 13500000, 0, 4),
};

/* FIXME: parent clk incorrect cause clk_get_rate got error value */
u32 gates[] = {
	SYSTEM,
	RTC,
	IOCTL,
	IOP,
	OTPRX,
	NOC,
	BR,
	RBUS_L00,
	SPIFL,
	SDCTRL0,
	PERI0 | P_EXTCLK,
	A926,
	UMCTL2,
	PERI1 | P_EXTCLK,

	DDR_PHY0,
	ACHIP,
	STC0,
	STC_AV0,
	STC_AV1,
	STC_AV2,
	UA0 | P_EXTCLK,
	UA1 | P_EXTCLK,
	UA2 | P_EXTCLK,
	UA3 | P_EXTCLK,
	UA4 | P_EXTCLK,
	HWUA | P_EXTCLK,
	DDC0,
	UADMA | P_EXTCLK,

	CBDMA0,
	CBDMA1,
	SPI_COMBO_0,
	SPI_COMBO_1,
	SPI_COMBO_2,
	SPI_COMBO_3,
	AUD,
	USBC0,
	USBC1,
	UPHY0,
	UPHY1,

	I2CM0,
	I2CM1,
	I2CM2,
	I2CM3,
	PMC,
	CARD_CTL0,
	CARD_CTL1,

	CARD_CTL4,
	BCH,
	DDFCH,
	CSIIW0,
	CSIIW1,
	MIPICSI0,
	MIPICSI1,

	HDMI_TX,
	VPOST,

	TGEN,
	DMIX,
	TCON,
	INTERRUPT,

	RGST,
	GPIO,
	RBUS_TOP,

	MAILBOX,
	SPIND,
	I2C2CBUS,
	SEC,
	GPOST0,
	DVE,

	OSD0,
	DISP_PWM,
	UADBG,
	DUMMY_MASTER,
	FIO_CTL,
	FPGA,
	L2SW,
	ICM,
	AXI_GLOBAL,
};

extern ulong extclk_rate;
extern struct udevice *clkc_dev;

/************************************************* PLL_TV *************************************************/

//#define PLLTV_STEP_DIR (?) /* Unit: HZ */

/* TODO: set proper FVCO range */
#define FVCO_MIN	(100 * _M)
#define FVCO_MAX	(200 * _M)

#define F_MIN		(FVCO_MIN / 8)
#define F_MAX		(FVCO_MAX)

static ulong plltv_integer_div(struct sp_pll *pll, ulong freq)
{
	/* valid m values: 27M must be divisible by m, 0 means end */
	static const u32 m_table[] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 25, 27, 30, 32, 0};
	u32 m, n, r;
#ifdef PLLTV_STEP_DIR
	u32 step = (PLLTV_STEP_DIR > 0) ? PLLTV_STEP_DIR : -PLLTV_STEP_DIR;
	int calc_times = 1000000 / step;
#endif
	ulong fvco, nf;

	TRACE;

	/* check freq */
	if (freq < F_MIN) {
		pr_warn("[%s:%d] freq:%lu < F_MIN:%lu, round up\n", __FUNCTION__, __LINE__, freq, F_MIN);
		freq = F_MIN;
	} else if (freq > F_MAX) {
		pr_warn("[%s:%d] freq:%lu > F_MAX:%lu, round down\n", __FUNCTION__, __LINE__, freq, F_MAX);
		freq = F_MAX;
	}

#ifdef PLLTV_STEP_DIR
	if ((freq % step) != 0)
		freq += step - (freq % step) + ((PLLTV_STEP_DIR > 0) ? 0 : PLLTV_STEP_DIR);
#endif

#ifdef PLLTV_STEP_DIR
CALC:
	if (!calc_times) {
		pr_err("[%s:%d] freq:%lu out of recalc times\n", __FUNCTION__, __LINE__, freq);
		return -ETIMEOUT;
	}
#endif

	/* DIVR 0~3 */
	for (r = 0; r <= 3; r++) {
		fvco = freq << r;
		if (fvco <= FVCO_MAX)
			break;
	}

	/* DIVM */
	for (m = 0; m_table[m]; m++) {
		nf = fvco * m_table[m];
		n = nf / F_27M;
		if ((n * F_27M) == nf) {
			break;
		}
	}
	m = m_table[m];

	if (!m) {
#ifdef PLLTV_STEP_DIR
		freq += PLLTV_STEP_DIR;
		calc_times--;
		goto CALC;
#else
		pr_err("[%s:%d] freq:%lu not found a valid setting\n", __FUNCTION__, __LINE__, freq);
		return -EINVAL;
#endif
	}

	/* save parameters */
	pll->p[SEL_FRA] = 0;
	pll->p[DIVR]    = r;
	pll->p[DIVN]    = n;
	pll->p[DIVM]    = m;

	pr_info("[%s:%d]   M:%u N:%u R:%u   CKREF:%lu  FVCO:%lu  FCKOUT:%lu\n",
		__FUNCTION__, __LINE__, m, n, r, fvco /m, fvco, freq);

	return freq;
}

/* paramters for PLLTV fractional divider */
/* FIXME: better parameter naming */
static const u32 pt[][5] = {
	/* conventional fractional */
	{
		1,			// factor
		5,			// 5 * p0 (nint)
		1,			// 1 * p0
		F_27M,		// F_27M / p0
		1,			// p0 / p2
	},
	/* phase rotation */
	{
		10,			// factor
		54,			// 5.4 * p0 (nint)
		2,			// 0.2 * p0
		F_27M / 10,	// F_27M / p0
		5,			// p0 / p2
	},
};
static const u32 mods[] = { 91, 55 }; /* SDM_MOD mod values */

static ulong plltv_fractional_div(struct sp_pll *pll, ulong freq)
{
	u32 m, r;
	u32 nint, nfra;
	u32 diff_min_quotient = 210000000, diff_min_remainder = 0;
	u32 diff_min_sign = 0;
	ulong fvco, nf, f, fout = 0;
	int sdm, ph;

	TRACE;
#if 1
	/* check freq */
	if (freq < F_MIN) {
		pr_warn("[%s:%d] freq:%lu < F_MIN:%lu, round up\n", __FUNCTION__, __LINE__, freq, F_MIN);
		freq = F_MIN;
	} else if (freq > F_MAX) {
		pr_warn("[%s:%d] freq:%lu > F_MAX:%lu, round down\n", __FUNCTION__, __LINE__, freq, F_MAX);
		freq = F_MAX;
	}
#endif

	/* DIVR 0~3 */
	for (r = 0; r <= 3; r++) {
		fvco = freq << r;
		if (fvco <= FVCO_MAX)
			break;
	}
	pr_info("freq:%lu fvco:%lu R:%u\n", freq, fvco, r);
	f = F_27M >> r;

	/* PH_SEL 1/0 */
	for (ph = 1; ph >= 0; ph--) {
		const u32 *pp = pt[ph];
#if 0
		/* Q628: for nint == p1, saving time */
		u32 ms = (F_27M * pp[1] / pp[0]) / fvco;
		if (ms > 32)
			ms &= ~1;
		else if (!ms)
			ms++;
#else
		u32 ms = 1;
#endif

		/* SDM_MOD 0/1 */
		for (sdm = 0; sdm <= 1; sdm++) {
			u32 mod = mods[sdm];

			/* DIVM 1~32 */
			for (m = ms; m <= 32; m++) {
				u32 diff_freq;
				u32 diff_freq_quotient = 0, diff_freq_remainder = 0;
				u32 diff_freq_sign = 0; /* 0:Positive number, 1:Negative number */

				nf = fvco * m;
				nint = nf / pp[3];

				if (nint < pp[1])
					continue;
				if (nint > pp[1])
					break;

				nfra = (((nf % pp[3]) * mod * pp[4]) + (F_27M / 2)) / F_27M;
				if (nfra)
					diff_freq = (f * (nint + pp[2]) / pp[0]) - (f * (mod - nfra) / mod / pp[4]);
				else
					diff_freq = (f * (nint) / pp[0]);

				diff_freq_quotient  = diff_freq / m;
				diff_freq_remainder = ((diff_freq % m) * 1000) / m;

				pr_info("m = %d N.f = %2d.%03d%03d, nfra = %d/%d  fout = %u\n",
					m, nint, (nfra * 1000) / mod, (((nfra * 1000) % mod) * 1000) / mod, nfra, mod, diff_freq_quotient);

				if (freq > diff_freq_quotient) {
					diff_freq_quotient  = freq - diff_freq_quotient - 1;
					diff_freq_remainder = 1000 - diff_freq_remainder;
					diff_freq_sign = 1;
				} else {
					diff_freq_quotient = diff_freq_quotient - freq;
					diff_freq_sign = 0;
				}

				if ((diff_min_quotient > diff_freq_quotient) ||
					((diff_min_quotient == diff_freq_quotient) && (diff_min_remainder > diff_freq_remainder))) {

					/* found a closer freq, save parameters */
					TRACE;
					pll->p[SEL_FRA] = 1;
					pll->p[SDM_MOD] = sdm;
					pll->p[PH_SEL]  = ph;
					pll->p[NFRA]    = nfra;
					pll->p[DIVR]    = r;
					pll->p[DIVM]    = m;

					fout = diff_freq / m;
					diff_min_quotient = diff_freq_quotient;
					diff_min_remainder = diff_freq_remainder;
					diff_min_sign = diff_freq_sign;
				}
			}
		}
	}

	if (!fout) {
		pr_err("[%s:%d] freq:%lu not found a valid setting\n", __FUNCTION__, __LINE__, freq);
		return -EINVAL;
	}

	//pr_info("MOD:%u PH_SEL:%u NFRA:%u M:%u R:%u\n", mods[pll->p[SDM_MOD]], pll->p[PH_SEL], pll->p[NFRA], pll->p[DIVM], pll->p[DIVR]);

	pr_info("[%s:%d] real out:%lu/%lu Hz(%u, %u, sign %u)\n",
		__FUNCTION__, __LINE__, fout, freq, diff_min_quotient, diff_min_remainder, diff_min_sign);

	return fout;
}

static ulong plltv_div(struct sp_pll *pll, unsigned long freq)
{
	TRACE;
	if (freq % 100)
		return plltv_fractional_div(pll, freq);
	else
		return plltv_integer_div(pll, freq);
}

static void plltv_set_rate(struct sp_pll *pll)
{
	u32 reg;

	//pr_info("MOD:%u PH_SEL:%u NFRA:%u M:%u R:%u\n", mods[pll->p[SDM_MOD]], pll->p[PH_SEL], pll->p[NFRA], pll->p[DIVM], pll->p[DIVR]);
	reg  = MASK_SET(1, 1, pll->p[SEL_FRA]);
	reg |= MASK_SET(2, 1, pll->p[SDM_MOD]);
	reg |= MASK_SET(4, 1, pll->p[PH_SEL]);
	reg |= MASK_SET(6, 7, pll->p[NFRA]);
	writel(reg, pll->reg);

	reg  = MASK_SET(7, 2, pll->p[DIVR]);
	writel(reg, pll->reg + 4);

	reg  = MASK_SET(0, 8, pll->p[DIVN] - 1);
	reg |= MASK_SET(8, 7, pll->p[DIVM] - 1);
	writel(reg, pll->reg + 8);
}

/************************************************* PLL_A *************************************************/

/* from Q628_PLLs_REG_setting.xlsx */
struct {
	u32 rate;
	u32 regs[5];
} pa[] = {
	{
		.rate = 135475200,
		.regs = {
			0x4801,
			0x02df,
			0x248f,
			0x0211,
			0x33e9
		}
	},
	{
		.rate = 147456000,
		.regs = {
			0x4801,
			0x1adf,
			0x2490,
			0x0349,
			0x33e9
		}
	},
	{
		.rate = 196608000,
		.regs = {
			0x4801,
			0x42ef,
			0x2495,
			0x01c6,
			0x33e9
		}
	},
};

static void plla_set_rate(struct sp_pll *pll)
{
	const u32 *pp = pa[pll->p[0]].regs;
	int i;

	for (i = 0; i < ARRAY_SIZE(pa->regs); i++) {
		clk_writel(0xffff0000 | pp[i], pll->reg + (i * 4));
		pr_info("%04x\n", pp[i]);
	}
}

static ulong plla_round_rate(struct sp_pll *pll, ulong rate)
{
	int i = ARRAY_SIZE(pa);

	while (--i) {
		if (rate >= pa[i].rate)
			break;
	}
	pll->p[0] = i;
	return pa[i].rate;
}

/************************************************* SP_PLL *************************************************/

static ulong sp_pll_calc_div(struct sp_pll *pll, ulong rate, ulong prate)
{
	u32 fbdiv;
	u32 max = 1 << pll->div_width;

	if (!pll->brate)
		fbdiv = DIV_ROUND_CLOSEST(prate, rate);
	else
		fbdiv = DIV_ROUND_CLOSEST(rate, pll->brate);
	if (fbdiv > max)
		fbdiv = max;
	return fbdiv;
}

static ulong sp_pll_round_rate(struct sp_pll *pll, ulong rate, ulong *prate)
{
	ulong ret;

	TRACE;
	//pr_info("round_rate: %lu %lu\n", rate, *prate);

	if (rate == *prate)
		ret = *prate; /* bypass */
	else if (pll->div_width == DIV_A) {
		ret = plla_round_rate(pll, rate);
	} else if (pll->div_width == DIV_TV) {
		ret = plltv_div(pll, rate);
		if (ret < 0)
			ret = *prate;
	} else {
		u32 fbdiv = sp_pll_calc_div(pll, rate, *prate);
		if (!pll->brate) 
			ret = *prate / fbdiv;
		else
			ret = pll->brate * fbdiv;
	}

	return ret;
}

static ulong sp_pll_recalc_rate(struct sp_pll *pll, ulong prate)
{
	u32 reg = clk_readl(pll->reg);
	ulong ret = 0;

	//TRACE;
	if ((pll->bp_bit >= 0) && (reg & BIT(pll->bp_bit)))
		ret = prate; /* bypass */
	else if (pll->div_width == DIV_A) {
		ret = pa[pll->p[0]].rate;
		//reg = clk_readl(pll->reg + 12); // G4.10 K_SDM_A
	} else if (pll->div_width == DIV_TV) {
		u32 m, r, reg2;

		//pr_info("!!!!!!! %p:%p %08x\n", clk, clk->reg, reg);
		r = MASK_GET(7, 2, clk_readl(pll->reg + 4));
		reg2 = clk_readl(pll->reg + 8);
		m = MASK_GET(8, 7, reg2) + 1;

		if (reg & BIT(1)) { /* SEL_FRA */
			/* fractional divider */
			u32 sdm  = MASK_GET(2, 1, reg);
			u32 ph   = MASK_GET(4, 1, reg);
			u32 nfra = MASK_GET(6, 7, reg);
			const u32 *pp = pt[ph];
			//pr_info("MOD:%u PH_SEL:%u NFRA:%u M:%u R:%u\n", mods[sdm], ph, nfra, m, r);
			ret = prate >> r;
			ret = (ret * (pp[1] + pp[2]) / pp[0]) - (ret * (mods[sdm] - nfra) / mods[sdm] / pp[4]);
			ret /= m;
		} else {
			/* integer divider */
			u32 n = MASK_GET(0, 8, reg2) + 1;
			ret = (prate / m * n) >> r;
		}
	} else {
		u32 fbdiv = MASK_GET(pll->div_shift, pll->div_width, reg) + 1;
		if (!pll->brate)
			ret = prate / fbdiv;
		else
			ret = pll->brate * fbdiv;
	}
	//pr_info("recalc_rate: %lu -> %lu\n", prate, ret);

	return ret;
}

static ulong _sp_pll_set_rate(struct sp_pll *pll, ulong rate, ulong prate)
{
	u32 reg = 0;

	//TRACE;
	pr_info("set_rate: %lu -> %lu\n", prate, rate);

	if (pll->bp_bit >= 0)
		reg = BIT(pll->bp_bit + 16); /* HIWORD_MASK */

	if (pll->bp_bit >= 0 && rate == prate)
		reg |= BIT(pll->bp_bit); /* bypass */
	else if (pll->div_width == DIV_A)
		plla_set_rate(pll);
	else if (pll->div_width == DIV_TV)
		plltv_set_rate(pll);
	else if (pll->div_width) {
		u32 fbdiv = sp_pll_calc_div(pll, rate, prate);
		reg |= MASK_SET(pll->div_shift, pll->div_width, fbdiv - 1);
	}

	clk_writel(reg, pll->reg);

	return 0;
}

static ulong sp_pll_get_prate(struct sp_pll *pll)
{
	ulong prate = extclk_rate;

	if (pll->parent != EXTCLK) {
		struct clk clk;
		sunplus_clk_get_by_index(pll->parent, &clk);
		prate = clk_get_rate(&clk);
		clk_free(&clk);
	}

	return prate;	
}

static u32 to_gate(int clk_id)
{
	int i, j = 0, k = ARRAY_SIZE(gates);
	u32 id;

	while (j <= k) {
		i = (j + k) >> 1;
		id = gates[i] & 0xff;
		//pr_debug(" >>> %d %d -> %d: %02x\n", j, k, i, id);
		if (id == clk_id) {
			return gates[i];
		}
		if (id < clk_id) {
			j = i + 1;
		} else {
			k = i - 1;
		}
	}

	return 0;
}

/************************************************** API **************************************************/

ulong sp_gate_get_rate(int id)
{
	u32 gate = to_gate(id);

	if (!gate)
		return -ENOENT;

	if (gate & P_EXTCLK)
		return extclk_rate;

	return sp_pll_get_rate(PLL_SYS);
}

ulong sp_pll_get_rate(int id)
{
	struct sp_pll *pll = &plls[id];
	ulong prate = sp_pll_get_prate(pll);

	return sp_pll_recalc_rate(pll, prate);
}

ulong sp_pll_set_rate(int id, ulong rate)
{
	struct sp_pll *pll = &plls[id];
	ulong prate = sp_pll_get_prate(pll);

	rate = sp_pll_round_rate(pll, rate, &prate);
	_sp_pll_set_rate(pll, rate, prate);
	return rate;
}

int sp_clk_is_enabled(struct clk *clk)
{
	u32 id = clk->id;

	if (id < PLL_MAX) {
		struct sp_pll *pll = &plls[id];
		if (pll->pd_bit < 0)
			pll = &plls[pll->parent];
		return clk_readl(pll->reg) & BIT(pll->pd_bit);
	} else {
		struct sunplus_clk *priv = dev_get_priv(clk->dev);
		return clk_readl(priv->base + (id >> 4) * 4) & BIT(id & 0x0f);
	}
}

void sp_clk_endisable(struct clk *clk, int enable)
{
	u32 id = clk->id;

	if (id < PLL_MAX) {
		struct sp_pll *pll = &plls[id];
		if (pll->pd_bit >= 0)
			clk_writel(BIT(pll->pd_bit + 16) | (enable << pll->pd_bit), pll->reg);
	} else {
		struct sunplus_clk *priv = dev_get_priv(clk->dev);
		u32 j = id & 0x0f;
		clk_writel((enable << j) | BIT(j + 16), priv->base + (id >> 4) * 4);
	}
}

void sp_clk_dump(struct clk *clk)
{
	int i = clk->id;
	ulong rate = sp_clk_is_enabled(clk) ? clk_get_rate(clk) : 0;

	if (clk->dev == clkc_dev) 
		if (i < PLL_MAX)
			CLK_PRINT(plls[i].name, rate);
		else {
			char s[10];
			sprintf(s, "clkc:0x%02x", i);
			CLK_PRINT(s, rate);
		}
	else
		CLK_PRINT(clk->dev->name, rate);
}

void sp_plls_dump(void)
{
	for (int i = 0; i < ARRAY_SIZE(plls); i++)
		CLK_PRINT(plls[i].name, sp_pll_get_rate(i));
}
