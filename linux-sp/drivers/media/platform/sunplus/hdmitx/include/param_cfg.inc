#ifndef __HDMITX_PARAM_CFG_H__
#define __HDMITX_PARAM_CFG_H__

/*----------------------------------------------------------------------------*
 *					INCLUDE DECLARATIONS
 *---------------------------------------------------------------------------*/
#define CLKREF_27M

/*----------------------------------------------------------------------------*
 *					MACRO DECLARATIONS
 *---------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *					DATA TYPES
 *---------------------------------------------------------------------------*/
enum pixel_clk_mode {
	PIXEL_CLK_MODE_27_MHZ = 0,
	PIXEL_CLK_MODE_74P25_MHZ,
	PIXEL_CLK_MODE_148P5_MHZ,
	PIXEL_CLK_MODE_297_MHZ,
	PIXEL_CLK_MODE_23P75_MHZ,
	PIXEL_CLK_MODE_38P25_MHZ,
	PIXEL_CLK_MODE_63P5_MHZ,
	PIXEL_CLK_MODE_109_MHZ,
	PIXEL_CLK_MODE_19P75_MHZ,
	PIXEL_CLK_MODE_30P75_MHZ,
	PIXEL_CLK_MODE_52_MHZ,
	PIXEL_CLK_MODE_88P5_MHZ,
	PIXEL_CLK_MODE_MAX
};

enum color_space {
	COLOR_SPACE_LIMITED_RGB = 0,
	COLOR_SPACE_LIMITED_YUV,
	COLOR_SPACE_FULL_RGB,
	COLOR_SPACE_FULL_YUV,
	COLOR_SPACE_MAX
};

	#ifdef CONFIG_SOC_SP7021
struct pll_tv_param {
	unsigned char bypass;
	unsigned char r;
	unsigned char m;
	unsigned char n;
};
	#elif defined(CONFIG_SOC_I143)
struct pll_tv_param {
	unsigned short bypass;
	unsigned short pstdiv;
	unsigned short m_sdm;
	unsigned short ph_step_sel;
	unsigned short divr;
	unsigned short divn;
	unsigned short icp;
	unsigned short divm;
	unsigned short ph_sel;
	unsigned short k_sdm;
};
	#endif

struct csc_param {
	unsigned short on_off;
	unsigned short manual;
	unsigned short ycc_range; // 0:limit, 1:full
	unsigned short rgb_range; // 0:limit, 1:full
	unsigned short conversion_type; // 0:r2y, 1:y2r
	unsigned short colorimetry; // 0:601, 1:709
};

struct phy_param {
	#ifdef CONFIG_SOC_SP7021
	unsigned char aclk_mode; // G5.4[8:7]
	unsigned char is_data_double; // G5.4[6]
	unsigned char kv_mode; // G5.4[5:4]
	unsigned char term_mode; // G5.4[2:0]
	#elif defined(CONFIG_SOC_I143)
	unsigned char aclk_mode; // G5.1[8:7]
	unsigned char kv_mode; // G5.1[5:4]
	unsigned char term_mode; // G5.1[2:0]
	#endif
	unsigned char ectr_mode; // G387.26[15:12]
	unsigned char is_emp; // G387.26[11]
	unsigned char is_clk_detector; // G387.26[10]
	unsigned char fckdv_mode; // G387.26[9:8]
	unsigned char ckinv_mode; // G387.26[7:4]
	unsigned char is_from_odd; // G387.26[1]
	unsigned char is_clk_inv; // G387.26[0]
	unsigned char icp_mod_mode; // G387.27[15:12]
	#ifdef CONFIG_SOC_I143
	unsigned char pd_emp_mode; // G387.27[11]
	#endif
	unsigned char pd_d_mode; // G387.27[10:8]
	unsigned char cpst_mode; // G387.27[7:4]
	unsigned char icp_mode; // G387.27[1]
	unsigned char bgr_mode; // G387.28[15:13]
	unsigned char sw_ctrl; // G387.28[11:8]
	#ifdef CONFIG_SOC_I143
	unsigned char kv_sel; // G387.28[7:6]
	#endif
	unsigned char dsel_mode; // G387.28[5:0]
	unsigned char irt_mode; // G387.29[15:10]
	unsigned char dcnst_mode; // G387.29[9:0]
	unsigned char rv_model; // G387.30[1]
};

/*----------------------------------------------------------------------------*
 *					GLOBAL VARIABLES
 *---------------------------------------------------------------------------*/
	#ifdef CONFIG_SOC_SP7021
static struct pll_tv_param g_pll_tv_cfg[PIXEL_CLK_MODE_MAX] = {
	// PIXEL_CLK_MODE_27_MHZ
	{.bypass = ENABLE, .r = 0x0, .m = 0x0, .n = 0x0},

	// PIXEL_CLK_MODE_74P25_MHZ
	{.bypass = DISABLE, .r = 0x1, .m = 0x1, .n = 0xa},

	// PIXEL_CLK_MODE_148P5_MHZ
	{.bypass = DISABLE, .r = 0x0, .m = 0x1, .n = 0xa},

	// PIXEL_CLK_MODE_297_MHZ
	{.bypass = DISABLE, .r = 0x0, .m = 0x9, .n = 0x2a},

	// PIXEL_CLK_MODE_23P75_MHZ
	{.bypass = DISABLE, .r = 0x3, .m = 0x42, .n = 0xbc},

	// PIXEL_CLK_MODE_38P25_MHZ
	{.bypass = DISABLE, .r = 0x2, .m = 0x42, .n = 0x97},

	// PIXEL_CLK_MODE_63P5_MHZ
	{.bypass = DISABLE, .r = 0x1, .m = 0x42, .n = 0x7d},

	// PIXEL_CLK_MODE_109_MHZ
	{.bypass = DISABLE, .r = 0x1, .m = 0x42, .n = 0xd8},

	// PIXEL_CLK_MODE_19P75_MHZ
	{.bypass = DISABLE, .r = 0x3, .m = 0x42, .n = 0x9c},

	// PIXEL_CLK_MODE_30P75_MHZ
	{.bypass = DISABLE, .r = 0x2, .m = 0x42, .n = 0x79},

	// PIXEL_CLK_MODE_52_MHZ
	{.bypass = DISABLE, .r = 0x1, .m = 0x42, .n = 0x66},

	// PIXEL_CLK_MODE_88P5_MHZ
	{.bypass = DISABLE, .r = 0x1, .m = 0x42, .n = 0xaf},
};
	#elif defined(CONFIG_SOC_I143)
static struct pll_tv_param g_pll_tv_cfg[PIXEL_CLK_MODE_MAX] = {
		#ifdef CLKREF_27M
	// PIXEL_CLK_MODE_27_MHZ
	{
		.bypass		= ENABLE,
		.pstdiv 	= 0x0,
		.m_sdm 		= 0x0,
		.ph_step_sel 	= 0x0,
		.divr 		= 0x0,
		.divn 		= 0x0,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x0,
		.k_sdm 		= 0x0,
	},

	// PIXEL_CLK_MODE_74P25_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x1,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x5,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x5,
		.k_sdm 		= 0x0,
	},

	// PIXEL_CLK_MODE_148P5_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x0,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x5,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x5,
		.k_sdm 		= 0x0,
	},

	// PIXEL_CLK_MODE_297_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x0,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0xa,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0xa,
		.k_sdm 		= 0x0,
	},

	// PIXEL_CLK_MODE_23P75_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x3,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x7,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x0,
		.k_sdm 		= 0x2f6,
	},

	// PIXEL_CLK_MODE_38P25_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x2,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x1,
		.divn 		= 0x2,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x8,
		.k_sdm 		= 0x2aa,
	},

	// PIXEL_CLK_MODE_63P5_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x1,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x2,
		.divn 		= 0x1,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x5,
		.k_sdm 		= 0x056d,
	},

	// PIXEL_CLK_MODE_109_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x0,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x4,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x0,
		.k_sdm 		= 0x2f6,
	},

	// PIXEL_CLK_MODE_19P75_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x3,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x5,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x8,
		.k_sdm 		= 0x425,
	},

	// PIXEL_CLK_MODE_30P75_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x2,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x4,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x5,
		.k_sdm 		= 0x471,
	},

	// PIXEL_CLK_MODE_52_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x1,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x3,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x8,
		.k_sdm 		= 0x425,
	},

	// PIXEL_CLK_MODE_88P5_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x1,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x6,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x5,
		.k_sdm 		= 0x471,
	},
		#else
	// PIXEL_CLK_MODE_27_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x2,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x1,
		.divn 		= 0x2,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x1,
		.k_sdm 		= 0x4cc,
	},

	// PIXEL_CLK_MODE_74P25_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x1,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x6,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0xf,
		.k_sdm 		= 0x332,
	},

	// PIXEL_CLK_MODE_148P5_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x0,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x6,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0xf,
		.k_sdm 		= 0x332,
	},

	// PIXEL_CLK_MODE_297_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x0,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0xb,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x8,
		.k_sdm 		= 0x665,
	},

	// PIXEL_CLK_MODE_23P75_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x3,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x1,
		.divn 		= 0x3,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x8,
		.k_sdm 		= 0x0,
	},

	// PIXEL_CLK_MODE_38P25_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x2,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x6,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x1,
		.k_sdm 		= 0x199,
	},

	// PIXEL_CLK_MODE_63P5_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x1,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x5,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x5,
		.k_sdm 		= 0x0665,
	},

	// PIXEL_CLK_MODE_109_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x0,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x1,
		.divn 		= 0x2,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x1,
		.k_sdm 		= 0x665,
	},

	// PIXEL_CLK_MODE_19P75_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x3,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x1,
		.divn 		= 0x3,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x1,
		.k_sdm 		= 0x4cc,
	},

	// PIXEL_CLK_MODE_30P75_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x2,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x5,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0xf,
		.k_sdm 		= 0x199,
	},

	// PIXEL_CLK_MODE_52_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x1,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x4,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x1,
		.k_sdm 		= 0x4cc,
	},

	// PIXEL_CLK_MODE_88P5_MHZ
	{
		.bypass		= DISABLE,
		.pstdiv 	= 0x1,
		.m_sdm 		= 0x7ff,
		.ph_step_sel 	= 0x1,
		.divr 		= 0x0,
		.divn 		= 0x7,
		.icp 		= 0x0,
		.divm 		= 0x0,
		.ph_sel 	= 0x0,
		.k_sdm 		= 0x665,
	},
		#endif
};
	#endif
static struct csc_param g_csc_cfg[COLOR_SPACE_MAX][COLOR_SPACE_MAX] = {
	// on_off, manual, ycc_range, rgb_range, conversion_type, colorimetry
	//
	{
		// RGB limited to RGB limited
		{0, 0, 0, 0, 0, 0},

		// RGB limited to YC limited
		{1, 0, 0, 0, 0, 0},

		// RGB limited to RGB full
		{1, 1, 0, 0, 0, 0},

		// RGB limited to YC full
		{1, 0, 1, 0, 0, 0},
	},

	{
		// YC limited to RGB limited
		{1, 0, 0, 0, 1, 0},

		// YC limited to YC limited
		{0, 0, 0, 0, 0, 0},

		// YC limited to RGB full
		{1, 0, 0, 1, 1, 0},

		// YC limited to YC full
		{1, 1, 1, 0, 0, 0},
	},

	{
		// RGB full to RGB limited
		{1, 1, 0, 0, 0, 0},

		// RGB full to YC limited
		{1, 0, 0, 1, 0, 0},

		// RGB full to RGB full
		{0, 0, 0, 0, 0, 0},

		// RGB full to YC full
		{1, 0, 1, 1, 0, 0},
	},

	{
		// YC full to RGB limited
		{1, 0, 1, 0, 1, 0},

		// YC full to YC limited
		{1, 1, 0, 1, 0, 0},

		// YC full to RGB full
		{1, 0, 1, 1, 1, 0},

		// YC full to YC full
		{0, 0, 0, 0, 0, 0},
	}
};

static struct phy_param g_phy_cfg[TIMING_MAX][COLOR_DEPTH_MAX] = {
	// TIMING_480P
	{
		// COLOR_DEPTH_24BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 0,
			.term_mode       = 0,
			.ectr_mode       = 5,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 0,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_30BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 0,
			.term_mode       = 0,
			.ectr_mode       = 5,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 0,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_36BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 0,
			.term_mode       = 0,
			.ectr_mode       = 5,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 0,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_48BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 0,
			.term_mode       = 0,
			.ectr_mode       = 5,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.kv_sel		 = 0,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},
	},

	// TIMING_576P
	{
		// COLOR_DEPTH_24BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 0,
			.term_mode       = 0,
			.ectr_mode       = 0,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
	#ifdef CONFIG_SOC_SP7021
			.rv_model        = 0,
	#elif defined(CONFIG_SOC_I143)
			.rv_model        = 1,
	#endif
		},

		// COLOR_DEPTH_30BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 0,
			.term_mode       = 0,
			.ectr_mode       = 0,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 0,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
	#ifdef CONFIG_SOC_SP7021
			.rv_model        = 0,
	#elif defined(CONFIG_SOC_I143)
			.rv_model        = 1,
	#endif
		},

		// COLOR_DEPTH_36BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 0,
			.term_mode       = 0,
			.ectr_mode       = 0,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 0,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
	#ifdef CONFIG_SOC_SP7021
			.rv_model        = 0,
	#elif defined(CONFIG_SOC_I143)
			.rv_model        = 1,
	#endif
		},

		// COLOR_DEPTH_48BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 0,
			.term_mode       = 0,
			.ectr_mode       = 0,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 0,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
	#ifdef CONFIG_SOC_SP7021
			.rv_model        = 0,
	#elif defined(CONFIG_SOC_I143)
			.rv_model        = 1,
	#endif
		},
	},

	// TIMING_720P60
	{
		// COLOR_DEPTH_24BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 1,
			.term_mode       = 0,
			.ectr_mode       = 5,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 5,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 1,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_30BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 1,
			.term_mode       = 0,
			.ectr_mode       = 5,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 5,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 5,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 1,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_36BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 1,
			.term_mode       = 1,
			.ectr_mode       = 7,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 5,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 6,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 1,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_48BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 1,
			.term_mode       = 1,
			.ectr_mode       = 7,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 5,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 6,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 3,
			.sw_ctrl         = 0xa,
			.kv_sel		 = 1,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},
	},

	// TIMING_1080P60
	{
		// COLOR_DEPTH_24BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 3,
			.term_mode       = 1,
			.ectr_mode       = 7,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0xf,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 6,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 7,
			.sw_ctrl         = 0,
			.kv_sel		 = 1,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_30BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 3,
			.term_mode       = 1,
			.ectr_mode       = 7,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0xf,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 1,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 6,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 7,
			.sw_ctrl         = 0,
			.kv_sel		 = 3,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_36BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 3,
	#ifdef CONFIG_SOC_SP7021
			.term_mode       = 2,
			.ectr_mode       = 7,
			.is_emp          = 0,
	#elif defined(CONFIG_SOC_I143)
			.term_mode       = 1,
			.ectr_mode       = 5,
			.is_emp          = 1,
	#endif
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0xf,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 0,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 7,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 7,
			.sw_ctrl         = 0,
			.kv_sel		 = 3,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},

		// COLOR_DEPTH_48BITS
		{
			.aclk_mode       = 0,
	#ifdef CONFIG_SOC_SP7021
			.is_data_double  = 0,
	#endif
			.kv_mode         = 3,
			.term_mode       = 2,
			.ectr_mode       = 7,
			.is_emp          = 0,
			.is_clk_detector = 1,
			.fckdv_mode      = 1,
			.ckinv_mode      = 0,
			.is_from_odd     = 0,
			.is_clk_inv      = 0,
			.icp_mod_mode    = 0xf,
	#ifdef CONFIG_SOC_I143
			.pd_emp_mode	 = 0,
	#endif
			.pd_d_mode       = 0,
			.cpst_mode       = 7,
			.icp_mode        = 1,
	#ifdef CONFIG_SOC_SP7021
			.bgr_mode        = 3,
			.sw_ctrl         = 7,
			.dsel_mode       = 0x15,
	#elif defined(CONFIG_SOC_I143)
			.bgr_mode        = 7,
			.sw_ctrl         = 0,
			.kv_sel		 = 3,
			.dsel_mode       = 0x15,
	#endif
			.irt_mode        = 0,
			.dcnst_mode      = 0x1f,
			.rv_model        = 1,
		},
	},
};

/*----------------------------------------------------------------------------*
 *					EXTERNAL DECLARATIONS
 *---------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *					FUNCTION DECLARATIONS
 *---------------------------------------------------------------------------*/


#endif // __HDMITX_PARAM_CFG_H__

