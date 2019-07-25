#ifndef	__AUDCLK_H
#define	__AUDCLK_H



#define	AUDCLK_XCK_ENABLE		  	0x4000
#define	AUDCLK_XCKPAD_ENABLE		  	0x2000

/********zjg************************/
#define AUDCLK_128XCK_01024			0x698f			// 147.456/144,	128*8k,	
#define	AUDCLK_128XCK_02048     		0x6987			// 147.456/72,	128*16k,

#define	AUDCLK_128XCK_DPLL_14112		0x701f			// 45.1584/32,	128*11.025k,
#define	AUDCLK_128XCK_DPLL_28224  		0x700f			// 45.1584/16,	128*22.5k,
#define	AUDCLK_128XCK_DPLL_05644		0x7007			// 45.1584/8,		128*44.1k,
#define	AUDCLK_128XCK_DPLL_11290		0x7003			// 45.1584/4,		128*88.2k,
#define	AUDCLK_128XCK_DPLL_22579		0x7001			// 45.1584/2,		128*176.4k,

#define AUDCLK_256XCK_02048 		  	0x6987			// 147.456/72,	256*8k,	
#define	AUDCLK_256XCK_04096			0x6983			// 147.456/36,	256*16k,

#define	AUDCLK_256XCK_DPLL_28224 		0x700f			// 45.1584/16,	256*11.025k,
#define	AUDCLK_256XCK_DPLL_05644  		0x7007			// 45.1584/8,		256*22.5k,
#define	AUDCLK_256XCK_DPLL_11290		0x7003			// 45.1584/4,		256*44.1k,
#define	AUDCLK_256XCK_DPLL_22579		0x7001			// 45.1584/2,		256*88.2k,
#define	AUDCLK_256XCK_DPLL_44158		0x7000			// 45.1584/1,		256*176.4k,










/***********************************/


#define	AUDCLK_128XCK_04096     		0x6983		// 147.456/36,	128*32k,
#define	AUDCLK_128XCK_05644	     		0x6887		// 135.4752/24,	128*44.1k,
#define	AUDCLK_128XCK_06144     		0x6887		// 147.456/24,	128*48k,
#define	AUDCLK_128XCK_08192     		0x6981		// 147.456/18,	128*64k,
#define	AUDCLK_128XCK_11290    			0x6883		// 135.4752/12,	128*88.2k,
#define	AUDCLK_128XCK_12288     		0x6883		// 147.456/12,	128*96k,
#define	AUDCLK_128XCK_16384     		0x6980		// 147.456/9,		128*128k,
#define	AUDCLK_128XCK_22579    			0x6881		// 135.4752/6,	128*176.4k,
#define	AUDCLK_128XCK_24576     		0x6881		// 147.456/6,		128*192k,

#define	AUDCLK_256XCK_08192			0x6981		// 147.456/18,	256*32k,
#define	AUDCLK_256XCK_11290			0x6883		// 135.4752/12,	256*44.1k,
#define	AUDCLK_256XCK_12288			0x6883		// 147.456/12,	256*48k,
#define	AUDCLK_256XCK_16384			0x6980		// 147.456/9,		256*64k,
#define	AUDCLK_256XCK_22579			0x6881		// 135.4752/6,	256*88.2k,
#define	AUDCLK_256XCK_24576			0x6881		// 147.456/6,		256*96k,
#define	AUDCLK_256XCK_32768			0x6800		// 147.456/?,		256*128k, ??
#define	AUDCLK_256XCK_45158			0x6880		// 135.4752/3,	256*176.4k,
#define	AUDCLK_256XCK_49152			0x6880		// 147.456/3,		256*192k,

#define	AUDCLK_384XCK_08467     	 	0x06800		// not support
#define	AUDCLK_384XCK_09216     		0x06800		// not support
#define	AUDCLK_384XCK_12288     		0x06800		// not support
#define	AUDCLK_384XCK_16934     		0x06800		// not support
#define	AUDCLK_384XCK_18432     		0x06800		// not support
#define	AUDCLK_384XCK_33869     		0x06800		// not support
#define	AUDCLK_384XCK_36864     		0x06800		// not support
#define	AUDCLK_384XCK_67736     		0x06800		// not support
#define	AUDCLK_384XCK_73728     		0x06800		// not support

/* clk source is from DPLL(1080MHz) for MS10 issue */
/* audio clk are 49.152 and 45.1584 respectively */
#define	AUDCLK_128XCK_04096_DPLL		0x5083		// 49/12,	128*32k,
#define	AUDCLK_128XCK_05644_DPLL		0x5007		// 45/8,		128*44.1k,
#define	AUDCLK_128XCK_06144_DPLL		0x5007		// 49/8,		128*48k,
#define	AUDCLK_128XCK_08192_DPLL		0x5081		// 49/6,		128*64k,
#define	AUDCLK_128XCK_11290_DPLL		0x5003		// 45/4,		128*88.2k,
#define	AUDCLK_128XCK_12288_DPLL		0x5003		// 49/4,		128*96k,
#define	AUDCLK_128XCK_16384_DPLL		0x5080		// 49/3,		128*128k,
#define	AUDCLK_128XCK_22579_DPLL		0x5001		// 45/2,		128*176.4k,
#define	AUDCLK_128XCK_24576_DPLL		0x5001		// 49/2,		128*192k,

/* BCK settings */
#define	AUDCLK_32BCK_384        		0x6083		// 384/32
#define	AUDCLK_48BCK_384        		0x6007		// 384/48
#define	AUDCLK_64BCK_384        		0x6081		// 384/64
#define	AUDCLK_32BCK_256        		0x6007		// 256/32
#define	AUDCLK_64BCK_256        		0x6003		// 256/64
#define	AUDCLK_64BCK_128        		0x6001		// 128/64

/* IEC BCK, always 128*fs */
#define	AUDCLK_128IEC_384	        	0x6080		// 384/128
#define	AUDCLK_128IEC_256	        	0x6001		// 256/128
#define	AUDCLK_128IEC_128	        	0x6000		// 128/128
#define	AUDCLK_64IEC_128	        	0x6001		// 128/64
#define	AUDCLK_32IEC_128	        	0x6003		// 128/32

/* ADC CLK */
#define	AUDCLK_64ADC_384	        	0x6081		// 384/64
#define	AUDCLK_64ADC_256	        	0x7003		// 256/64
#define	AUDCLK_128ADC_384	        	0x6080		// 384/128
#define	AUDCLK_128ADC_256	        	0x6001		// 256/128

/* BT clock */
/* source is frome PLLA(147.456/135.4752) */
#define	BTCLK_XCK_128K				0x69ff		// 9*128*128K
#define	BTCLK_XCK_256K				0x69bf		// 9*64*256K
#define	BTCLK_XCK_512K				0x699f		// 9*32*512K
#define	BTCLK_XCK_1024K				0x698f		// 9*16*1024K
#define	BTCLK_XCK_1536K				0x689f		// 3*32*1536K
#define	BTCLK_XCK_2048K				0x6987		// 9*8*2048K

/* BT BCK soure is from BT_XCK */
#define	BTCLK_BCK_Div1				0x6000
#define	BTCLK_BCK_Div2				0x6001

/*  XCK  source selection */
#define PLLA 1
#define DPLL 2

#define PLLA_FRE 147456000
#define DPLL_FRE 45158400




/*============================================================*/
// To ease the control of audio clocks,
// currently PCMBCK, IECBCK, ADCBCK all source from XCK.
// as a result, when changing sampling frequency, only XCK change is need.
//
// note: if there are needs for enable/disable individual clocks
// we must increase other interfaces
/*============================================================*/
static const unsigned int extdac_xck_256fs_table[] =
{
	AUDCLK_256XCK_08192, AUDCLK_256XCK_11290, AUDCLK_256XCK_12288, 0, 0,
	AUDCLK_256XCK_22579, AUDCLK_256XCK_24576, 0, 0,
	AUDCLK_256XCK_45158, AUDCLK_256XCK_49152, 0, 0
};

static const unsigned int extdac_xck_fs_table[] =
{
	AUDCLK_128XCK_04096, AUDCLK_128XCK_05644, AUDCLK_128XCK_06144, 0, AUDCLK_128XCK_08192,
	AUDCLK_128XCK_11290, AUDCLK_128XCK_12288, 0, AUDCLK_128XCK_16384,
	AUDCLK_128XCK_22579, AUDCLK_128XCK_24576, 0, 0
};

static const unsigned int intdac_xck_fs_table[] =
{
	AUDCLK_128XCK_04096, AUDCLK_128XCK_05644, AUDCLK_128XCK_06144, 0, AUDCLK_128XCK_08192,
	AUDCLK_128XCK_11290, AUDCLK_128XCK_12288, 0, AUDCLK_128XCK_16384,
	AUDCLK_128XCK_22579, AUDCLK_128XCK_24576, 0, 0
};

static const unsigned int extADC_xck_fs_table[] =
{
	AUDCLK_256XCK_08192, AUDCLK_256XCK_11290, AUDCLK_256XCK_12288, 0, 0,
	AUDCLK_256XCK_22579, AUDCLK_256XCK_24576, 0, 0,
	AUDCLK_256XCK_45158, AUDCLK_256XCK_49152, 0, 0
};

static const unsigned int IEC0_bclk_fs_table[] =
{
	AUDCLK_128XCK_04096, AUDCLK_128XCK_05644, AUDCLK_128XCK_06144, 0, AUDCLK_128XCK_08192,
	AUDCLK_128XCK_11290, AUDCLK_128XCK_12288, 0, AUDCLK_128XCK_16384,
	AUDCLK_128XCK_22579, AUDCLK_128XCK_24576, 0, 0
};

static const unsigned int IEC1_bclk_fs_table[] =
{
	AUDCLK_128XCK_04096, AUDCLK_128XCK_05644, AUDCLK_128XCK_06144, 0, AUDCLK_128XCK_08192,
	AUDCLK_128XCK_11290, AUDCLK_128XCK_12288, 0, AUDCLK_128XCK_16384,
	AUDCLK_128XCK_22579, AUDCLK_128XCK_24576, 0, 0
};

static const unsigned int HDMI_MCLK_fs_table[] =
{
	AUDCLK_256XCK_08192, AUDCLK_256XCK_11290, AUDCLK_256XCK_12288, 0, AUDCLK_256XCK_16384,
	AUDCLK_256XCK_22579, AUDCLK_256XCK_24576, 0, AUDCLK_256XCK_32768,
	AUDCLK_256XCK_45158, AUDCLK_256XCK_49152, 0, 0
};

#define	INT_DAC_BCK_SETTING	AUDCLK_64BCK_128 // for internal dac bck clock
#define	EXT_ADC_BCK_SETTING	AUDCLK_64BCK_256 // for external adc BCK setting
#define	HDMI_TX_BCK_SETTING	AUDCLK_64BCK_256

#endif/*__AUDCLK_H*/

