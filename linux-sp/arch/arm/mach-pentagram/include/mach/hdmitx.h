#ifndef __DRV_HDMITX_H__
#define __DRV_HDMITX_H__

/*----------------------------------------------------------------------------*
 *					INCLUDE DECLARATIONS
 *---------------------------------------------------------------------------*/
#include <linux/ioctl.h>

/*----------------------------------------------------------------------------*
 *					MACRO DECLARATIONS
 *---------------------------------------------------------------------------*/
/* hdmitx mode change by a module parameter */
// #define MODE_CHANGE

/* DDC parameters */
#define DDC_FIFO_CAPACITY		 16
#define EDID_CAPACITY			 256
#define DDC_SLV_DEVICE_ADDR              0xA0
#define EDID_TIMEOUT                     15000

/*----------------------------------------------------------------------------*
 *					DATA TYPES
 *---------------------------------------------------------------------------*/
/**
 * \brief hdmitx mode enumeration
 */
enum hdmitx_mode {
	HDMITX_MODE_DVI = 0,
	HDMITX_MODE_HDMI,
	HDMITX_MODE_MAX
};

/**
 * \brief timing enumeration
 */
enum hdmitx_timing {
	HDMITX_TIMING_480P = 0,
	HDMITX_TIMING_576P,
	HDMITX_TIMING_720P60,
	HDMITX_TIMING_1080P60,
	HDMITX_TIMING_MAX
};

/**
 * \brief color depth enumeration
 */
enum hdmitx_color_depth {
	HDMITX_COLOR_DEPTH_24BITS = 0,
	HDMITX_COLOR_DEPTH_30BITS,
	HDMITX_COLOR_DEPTH_36BITS,
	HDMITX_COLOR_DEPTH_48BITS,
	HDMITX_COLOR_DEPTH_MAX
};

/**
 * \brief color space conversion enumeration
 */
enum hdmitx_color_space_conversion
{
	HDMITX_COLOR_SPACE_CONV_LIMITED_RGB_TO_LIMITED_RGB = 0,
	HDMITX_COLOR_SPACE_CONV_LIMITED_RGB_TO_LIMITED_YUV444,
	HDMITX_COLOR_SPACE_CONV_LIMITED_RGB_TO_LIMITED_YUV422,
	HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_RGB,
	HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_FULL_RGB,
	HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_YUV444,
	HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_YUV422,
	HDMITX_COLOR_SPACE_CONV_LIMITED_YUV422_TO_LIMITED_RGB,
	HDMITX_COLOR_SPACE_CONV_LIMITED_YUV422_TO_FULL_RGB,
	HDMITX_COLOR_SPACE_CONV_LIMITED_YUV422_TO_LIMITED_YUV444,
	HDMITX_COLOR_SPACE_CONV_LIMITED_YUV422_TO_LIMITED_YUV422,
	HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_FULL_RGB,
	HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_LIMITED_YUV444,
	HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_LIMITED_YUV422,
	HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_FULL_YUV444,
	HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_FULL_YUV422,
	HDMITX_COLOR_SPACE_CONV_FULL_YUV444_TO_LIMITED_RGB,
	HDMITX_COLOR_SPACE_CONV_FULL_YUV444_TO_FULL_RGB,
	HDMITX_COLOR_SPACE_CONV_FULL_YUV444_TO_FULL_YUV444,
	HDMITX_COLOR_SPACE_CONV_FULL_YUV444_TO_FULL_YUV422,
	HDMITX_COLOR_SPACE_CONV_FULL_YUV422_TO_LIMITED_RGB,
	HDMITX_COLOR_SPACE_CONV_FULL_YUV422_TO_FULL_RGB,
	HDMITX_COLOR_SPACE_CONV_FULL_YUV422_TO_FULL_YUV444,
	HDMITX_COLOR_SPACE_CONV_FULL_YUV422_TO_FULL_YUV422,
	HDMITX_COLOR_SPACE_CONV_MAX
};

/**
 * \brief audio channel enumeration
 */
enum hdmitx_audio_channel {
	HDMITX_AUDIO_CHL_I2S = 0,
	HDMITX_AUDIO_CHL_SPDIF,
	HDMITX_AUDIO_CHL_MAX
};

/**
 * \brief audio type enumeration
 */
enum hdmitx_audio_type {
	HDMITX_AUDIO_TYPE_LPCM = 0,
	HDMITX_AUDIO_TYPE_MAX
};

/**
 * \brief audio sample size enumeration
 */
enum hdmitx_audio_sample_size {
	HDMITX_AUDIO_SAMPLE_SIZE_16BITS = 0,
	HDMITX_AUDIO_SAMPLE_SIZE_24BITS,
	HDMITX_AUDIO_SAMPLE_SIZE_MAX
};

/**
 * \brief audio channel enumeration
 */
enum hdmitx_audio_layout {
	HDMITX_AUDIO_LAYOUT_2CH = 0,
	HDMITX_AUDIO_LAYOUT_6CH,
	HDMITX_AUDIO_LAYOUT_8CH,
	HDMITX_AUDIO_LAYOUT_MAX
};

/**
 * \brief audio sample rate enumeration
 */
enum hdmitx_audio_sample_rate {
	HDMITX_AUDIO_SAMPLE_RATE_32000HZ = 0,
	HDMITX_AUDIO_SAMPLE_RATE_48000HZ,
	HDMITX_AUDIO_SAMPLE_RATE_44100HZ,
	HDMITX_AUDIO_SAMPLE_RATE_88200HZ,
	HDMITX_AUDIO_SAMPLE_RATE_96000HZ,
	HDMITX_AUDIO_SAMPLE_RATE_176400HZ,
	HDMITX_AUDIO_SAMPLE_RATE_192000HZ,
	HDMITX_AUDIO_SAMPLE_RATE_MAX,
};

/**
 * \type of data block for tag code (EDID) enumeration
 */
enum edid_tag_code_data_block_type {
	EDID_RESERVED0 = 0,
	EDID_AUDIO_DATA_BLOCK,
	EDID_VIDEO_DATA_BLOCK,
	EDID_VENDOR_SPECIFIC_DATA_BLOCK,
	EDID_SPEAKER_ALLOCATION_DATA_BLOCK,
	EDID_VESA_DISP_TRANS_CHAR_DATA_BLOCK,
	EDID_RESERVED1,
	EDID_USE_EXTENDED_TAG,
};

/**
 * \type of data block for ext tag code (EDID) enumeration
 */
enum edid_ext_tag_code_data_block_type {
	EDID_VIDEO_CAPABILITY_DATA_BLOCK = 0,
	EDID_VENDOR_SPECIFIC_VIDEO_DATA_BLOCK,
	EDID_VESA_DISP_DEV_DATA_BLOCK,
	EDID_VESA_VIDEO_TIMING_BLOCK_EXT,
	EDID_RESERVED_HDMI_VIDEO_DATA_BLOCK,
	EDID_COLORIMETRY_DATA_BLOCK,
	EDID_RESERVED_VIDEO_RELATED_BLOCK0,
	EDID_RESERVED_VIDEO_RELATED_BLOCK1,
	EDID_RESERVED_VIDEO_RELATED_BLOCK2,
	EDID_RESERVED_VIDEO_RELATED_BLOCK3,
	EDID_RESERVED_VIDEO_RELATED_BLOCK4,
	EDID_RESERVED_VIDEO_RELATED_BLOCK5,
	EDID_RESERVED_VIDEO_RELATED_BLOCK6,
	EDID_VIDEO_FMT_PRE_DATA_BLOCK,
	EDID_YUV420_VIDEO_DATA_BLOCK,
	EDID_YUV420_CAPABILITY_MAP_DATA_BLOCK,
	EDID_RESERVED_CEA_MISC_AUDIO_FIELD,
	EDID_VENDOR_SPECIFIC_AUDIO_DATA_BLOCK,
	EDID_RESERVED_HDMI_AUDIO_DATA_BLOCK,
	EDID_RESERVED_AUDIO_RELATED_BLOCK0,
	EDID_RESERVED_AUDIO_RELATED_BLOCK1,
	EDID_RESERVED_AUDIO_RELATED_BLOCK2,
	EDID_RESERVED_AUDIO_RELATED_BLOCK3,
	EDID_RESERVED_AUDIO_RELATED_BLOCK4,
	EDID_RESERVED_AUDIO_RELATED_BLOCK5,
	EDID_RESERVED_AUDIO_RELATED_BLOCK6,
	EDID_RESERVED_AUDIO_RELATED_BLOCK7,
	EDID_RESERVED_AUDIO_RELATED_BLOCK8,
	EDID_RESERVED_AUDIO_RELATED_BLOCK9,
	EDID_RESERVED_AUDIO_RELATED_BLOCK10,
	EDID_RESERVED_AUDIO_RELATED_BLOCK11,
	EDID_RESERVED_AUDIO_RELATED_BLOCK12,
	EDID_INFOFRAME_DATA_BLOCK,
};

/*----------------------------------------------------------------------------*
 *					MACRO DECLARATIONS
 *---------------------------------------------------------------------------*/
/*about ioctrl*/
#define HDMITXIO_TYPE 'h'
#define HDMITXIO_SET_TIMING      _IOW(HDMITXIO_TYPE, 0x00, enum hdmitx_timing)
#define HDMITXIO_GET_TIMING      _IOR(HDMITXIO_TYPE, 0x01, enum hdmitx_timing)
#define HDMITXIO_SET_COLOR_DEPTH _IOW(HDMITXIO_TYPE, 0x02, enum hdmitx_color_depth)
#define HDMITXIO_GET_COLOR_DEPTH _IOR(HDMITXIO_TYPE, 0x03, enum hdmitx_color_depth)
#define HDMITXIO_GET_RX_READY    _IOR(HDMITXIO_TYPE, 0x04, unsigned char)
#define HDMITXIO_DISPLAY         _IOW(HDMITXIO_TYPE, 0x05, unsigned char)
#define HDMITXIO_PTG             _IOW(HDMITXIO_TYPE, 0x06, unsigned char)

/*about print msg*/
#define _HDMITX_ERR_MSG_
#define _HDMITX_WARNING_MSG_
#define _HDMITX_INFO_MSG_
// #define _HDMITX_DBG_MSG_

/*----------------------------------------------------------------------------*
 *					EXTERNAL DECLARATIONS
 *---------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *					FUNCTION DECLARATIONS
 *---------------------------------------------------------------------------*/
/******************************************************************************
 *
 * \fn      void hdmitx_set_timming(enum hdmitx_timing timing)
 *
 * \brief   Set video timing.
 *
 * \param [in]  timing, please refer to enum hdmitx_timing.
 * \param [out] none.
 *
 * \return none.
 *
 * \note none
 *
 *****************************************************************************/
void hdmitx_set_timming(enum hdmitx_timing timing);

/******************************************************************************
 *
 * \fn      void hdmitx_get_timming(enum hdmitx_timing *timing)
 *
 * \brief   Get video timing.
 *
 * \param [in]  none.
 * \param [out] *timing, please refer to enum hdmitx_timing.
 *
 * \return none.
 *
 * \note none
 *
 *****************************************************************************/
void hdmitx_get_timming(enum hdmitx_timing *timing);

/******************************************************************************
 *
 * \fn      void hdmitx_set_color_depth(enum hdmitx_color_depth color_depth)
 *
 * \brief   Set video color depth.
 *
 * \param [in]  color_depth, please refer to enum hdmitx_color_depth.
 * \param [out] none.
 *
 * \return none.
 *
 * \note none
 *
 *****************************************************************************/
void hdmitx_set_color_depth(enum hdmitx_color_depth color_depth);

/******************************************************************************
 *
 * \fn      void hdmitx_get_color_depth(enum hdmitx_color_depth *color_depth)
 *
 * \brief   Get video color depth.
 *
 * \param [in]  none.
 * \param [out] *color_depth, please refer to enum hdmitx_color_depth.
 *
 * \return none.
 *
 * \note none
 *
 *****************************************************************************/
void hdmitx_get_color_depth(enum hdmitx_color_depth *color_depth);

/******************************************************************************
 *
 * \fn      unsigned char hdmitx_get_rx_ready(void)
 *
 * \brief   Get ready status of hdmi receiver.
 *
 * \param [in]  none.
 * \param [out] noen.
 *
 * \return ready status, 0: not ready, 1: ready.
 *
 * \note none
 *
 *****************************************************************************/
unsigned char hdmitx_get_rx_ready(void);

/******************************************************************************
 *
 * \fn      int hdmitx_enable_display(void)
 *
 * \brief   Enable video and audio output of hdmitx.
 *
 * \param [in]  none.
 * \param [out] none.
 *
 * \return err, 0: success, not 0: failed.
 *
 * \note none
 *
 *****************************************************************************/
int hdmitx_enable_display(int);

/******************************************************************************
 *
 * \fn      void hdmitx_disable_display(void)
 *
 * \brief   Disable video and audio output of hdmitx.
 *
 * \param [in]  none.
 * \param [out] none.
 *
 * \return none.
 *
 * \note none
 *
 *****************************************************************************/
void hdmitx_disable_display(void);

/******************************************************************************
 *
 * \fn      void hdmitx_enable_pattern(void)
 *
 * \brief   Enable hdmitx video pattern generation.
 *
 * \param [in]  none.
 * \param [out] none.
 *
 * \return none.
 *
 * \note none
 *
 *****************************************************************************/
void hdmitx_enable_pattern(void);

/******************************************************************************
 *
 * \fn      void hdmitx_disable_pattern(void)
 *
 * \brief   Disable hdmitx video pattern generation.
 *
 * \param [in]  none.
 * \param [out] none.
 *
 * \return none.
 *
 * \note none
 *
 *****************************************************************************/
void hdmitx_disable_pattern(void);

#endif // __DRV_HDMITX_H__
