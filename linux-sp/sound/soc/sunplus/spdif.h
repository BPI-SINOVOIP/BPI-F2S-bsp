/*
 * spdif.h - spsoc digital interface(IEC0/1) 
 *
 *  Copyright (C) 2013 S+
 */

#ifndef _SPSOC_SPDIF_H
#define _SPSOC_SPDIF_H

#include "spsoc_util.h"


/* SPDIF_MODE  */
#define SPDIF_MODE_NONE		   	0 
#define SPDIF_MODE_PCM			1 
#define SPDIF_MODE_RAW			2 
#define SPDIF_MODE_REENC		3 
#define SPDIF_MODE_IECRX		4 
#define SPDIF_MODE_PCM_DECAY 		5 
#define SPDIF_MODE_MAX			5

/* HDMI_MODE */
#define HDMI_MODE_NONE		  	0
#define HDMI_MODE_PCM			1
#define HDMI_MODE_RAW			2
#define HDMI_MODE_REENC		  	3
#define HDMI_MODE_RAWHD		  	4
#define HDMI_MODE_PCM8CH		5
#define HDMI_MODE_PCM_DECAY		6
#define HDMI_MODE_MAX			6

/* DISC_FORMAT */
#define DISC_FORMAT_DVD	  	0 
#define DISC_FORMAT_CD		1
#define DISC_FORMAT_DTSCD	2

/* Codec Capability */
#define CodecNotSupportHBR	0
#define CodecSupportHBR		1

/* Channel status : Category code (bit8 - bit15) */
#define CATEGORY_CD	0x80
#define CATEGORY_DVD	0x98

/* Channel status : Sampling Rate (bit28 - bit31)  */   
#define SampleRate_44K	0x0
#define SampleRate_88K	0x1
#define SampleRate_22K	0x2
#define SampleRate_176K	0x3
#define SampleRate_48K	0x4
#define SampleRate_96K	0x5
#define SampleRate_24K	0x6
#define SampleRate_192K	0x7
#define SampleRate_32K	0xc
#define SampleRate_768K	0x9
#define SampleRate_NONE	0x8	//Not indicated

typedef enum _CGMSMode
{
	CGMS_Init		= 0x0,
	CGMS_unlimited_copy	= 0x1,
	CGMS_one_copy_allowed	= 0x2,
	CGMS_no_copy_allowed	= 0x3,
	bit_resolution		= 0x4,
	category_code		= 0x5,
}CGMSMode;

typedef enum _CCA_Type
{
	unlimited_copy		= 0x0,
	//no_use				  = 0x1,	
	one_copy_allowed	= 0x2,
	no_copy_allowed		= 0x3,	
}CCA_Type;

/***********************************************************************
 * 							Function Prototype
 ***********************************************************************/
void F_InitIEC( UINT32 user_disc_fmt, UINT32 fs, UINT16 CodecCapability);
void F_ConfigSpdif( UINT32 user_disc_fmt );
void F_ConfigHdmi(UINT32 user_disc_fmt , UINT16 CodecCapability);
void F_EnaHBRMode(void);
void F_DisHBRMode(void);
void F_UpdateIEC0_ChannelStatus( UINT32 user_disc_fmt );
void F_UpdateIEC1_ChannelStatus( UINT32 user_disc_fmt, UINT16 CodecCapability);
void F_UpdateCGMS( void );
void SetCGMS(UINT8 mode, UINT8 val);
void F_Set_Spdif_ChannelStatus(UINT8 CGMS_type , UINT8 bit_res, UINT8 disc_type);


#endif /* _SPSOC_SPDIF_H */

