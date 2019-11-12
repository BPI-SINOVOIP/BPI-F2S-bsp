/*
 * sunplus digital (IEC0/IEC1) control
 *
 * Author:	 <@sunplus.com>
 *
*/

#include "spdif.h"
#include "spsoc_util.h"
#include "spsoc_pcm.h"

void F_InitIEC( UINT32 user_disc_fmt, UINT32 fs, UINT16 CodecCapability)
{
	F_ConfigSpdif( user_disc_fmt );

	F_ConfigHdmi( user_disc_fmt, CodecCapability);

	F_UpdateCGMS();
}

/**
* @brief   set frequency mask for IEC0 pcm mode
* @param user_disc_fmt [in] DISC_FORMAT_DVD, DISC_FORMAT_CD, DISC_FORMAT_DTSCD
*/
void F_ConfigSpdif( UINT32 user_disc_fmt )
{
	unsigned int Disc_Format;
	unsigned int Spidf_cfg;
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;//(volatile RegisterFile_Audio *)REG(60,0);


	if( aud_param.spdif_mode == SPDIF_MODE_NONE )
	{
		Spidf_cfg = 0x1;   // GROUND
		regs0->iec_cfg = Spidf_cfg;
		return;
	}

	//  Disc format == DTSCD
	if( user_disc_fmt  == DISC_FORMAT_DTSCD)
	{
		regs0->iec0_valid_out = 0;	//validy bit = 0 (PCM)
		Disc_Format =	( (CATEGORY_CD << 8) | (SampleRate_44K << 28) );
		regs0->iec0_par0_out = Disc_Format;

		if( aud_param.spdif_mode == SPDIF_MODE_PCM)
		{
			Spidf_cfg = regs0->iec_cfg & ~(0xf);
			regs0->iec_cfg = Spidf_cfg | 0x2;
		}
		else if( (aud_param.spdif_mode == SPDIF_MODE_RAW) | (aud_param.spdif_mode == SPDIF_MODE_REENC) )
		{
			Spidf_cfg = regs0->iec_cfg & ~(0xf);
			regs0->iec_cfg = Spidf_cfg | 0x4;
		}
	}
	else
	{
		/* setting Disc format:  default DVD */
		Disc_Format = ( (CATEGORY_DVD << 8) | (SampleRate_48K << 28) );

		if( user_disc_fmt == DISC_FORMAT_CD )
			Disc_Format = ( (CATEGORY_CD << 8)  | (SampleRate_44K << 28) );

		regs0->iec0_par0_out = Disc_Format;


		if( aud_param.spdif_mode == SPDIF_MODE_PCM)
		{
			regs0->iec0_valid_out = 0;									//validy bit = 0 (PCM)
			regs0->iec0_par0_out = regs0->iec0_par0_out & ~(0x40);	    //set bit1 = 0 (PCM)

			Spidf_cfg = regs0->iec_cfg & ~(0xf);
			regs0->iec_cfg = Spidf_cfg | 0x2;
		}
		else if( (aud_param.spdif_mode == SPDIF_MODE_RAW) | (aud_param.spdif_mode == SPDIF_MODE_REENC) )
		{
			regs0->iec0_valid_out = 1;									//validy bit = RAW
			regs0->iec0_par0_out = regs0->iec0_par0_out | 0x40;		//set bit1 = 1 (RAW)
			Spidf_cfg = regs0->iec_cfg & ~(0xf);
			regs0->iec_cfg = Spidf_cfg | 0x4;
		}
	}

	return;
}


/**
* @brief   set frequency mask for IEC0 pcm mode
* @param user_disc_fmt [in] DISC_FORMAT_DVD, DISC_FORMAT_CD, DISC_FORMAT_DTSCD
* @param CodecCapability [in] CodecSupportHBR, CodecNotSupportHBR
*/
void F_ConfigHdmi(UINT32 user_disc_fmt , UINT16 CodecCapability)
{
	unsigned int Disc_Format;
	unsigned int Spidf_cfg;
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;//(volatile RegisterFile_Audio *)REG(60,0);

	if( aud_param.hdmi_mode== HDMI_MODE_NONE)
	{
		Spidf_cfg = 0x80; 		//MUTE
		regs0->iec_cfg = Spidf_cfg;
		return;
	}


	/*  Disc format == DTSCD */
	if( user_disc_fmt == DISC_FORMAT_DTSCD)
	{
		/* validy bit = 0 (PCM) */
		regs0->iec1_valid_out = 0;
		Disc_Format =	( (CATEGORY_CD << 8) | (SampleRate_44K << 28) );
		regs0->iec1_par0_out = Disc_Format;

		if( aud_param.hdmi_mode == HDMI_MODE_PCM)
		{
			F_DisHBRMode();

			Spidf_cfg = regs0->iec_cfg & ~(0xf0);
			regs0->iec_cfg = Spidf_cfg | 0x20;
		}
		else if( (aud_param.hdmi_mode == HDMI_MODE_RAW) | (aud_param.hdmi_mode == HDMI_MODE_REENC) )
		{
			F_DisHBRMode();

			Spidf_cfg = regs0->iec_cfg & ~(0xf0);
			regs0->iec_cfg = Spidf_cfg | 0x40;
		}
	}
	else
	{
		/* setting Disc format:  default DVD */
		Disc_Format = ( (CATEGORY_DVD << 8) | (SampleRate_48K << 28) );

		if( user_disc_fmt == DISC_FORMAT_CD )
			Disc_Format = ( (CATEGORY_CD << 8)  | (SampleRate_44K << 28) );

		regs0->iec1_par0_out = Disc_Format;


		if(aud_param.hdmi_mode == HDMI_MODE_PCM)
		{
			/* validy bit = 0 (PCM) */
			regs0->iec1_valid_out =  0;
			regs0->iec1_par0_out = regs0->iec1_par0_out & ~(0x40) ;

			F_DisHBRMode();

			Spidf_cfg = regs0->iec_cfg & ~(0xf0);
			regs0->iec_cfg = Spidf_cfg | 0x20;
		}
		else if( (aud_param.hdmi_mode == HDMI_MODE_RAW) || (aud_param.hdmi_mode == HDMI_MODE_REENC) )
		{
			/* validy bit = RAW */
			regs0->iec1_valid_out = 1;
			regs0->iec1_par0_out = regs0->iec1_par0_out | 0x40;

			F_DisHBRMode();

			Spidf_cfg = regs0->iec_cfg & ~(0xf0);
			regs0->iec_cfg = Spidf_cfg | 0x40;
		}

		else if( aud_param.hdmi_mode == HDMI_MODE_RAWHD)
		{
			/* need to check Codec Capability */
			if( CodecCapability )
				F_EnaHBRMode();
			else
				F_DisHBRMode();

			/* validy bit = RAW */
			regs0->iec1_valid_out = 1;
			regs0->iec1_par0_out = regs0->iec1_par0_out | 0x40;

			Spidf_cfg = regs0->iec_cfg & ~(0xf0);
			regs0->iec_cfg = Spidf_cfg | 0x40;
		}
	}
	return;
}

void F_DisHBRMode(void)
{
   	unsigned int HBR_MODE;
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;//(volatile RegisterFile_Audio *)REG(60,0);

   	HBR_MODE = regs0->aud_misc_ctrl;
   	HBR_MODE &= 0xfffff9ff; 		// disable Bit9,Bit10
   	regs0->aud_misc_ctrl = HBR_MODE;

	return;
}

void F_EnaHBRMode(void)
{
	/**
	 * PCM set 192k, data source set to IEC1 FIFO
	 * bit 9: HBR mode
	 * bit10: HDMI data source from IEC1 or PCM
	 */
	unsigned int HBR_MODE;
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;//(volatile RegisterFile_Audio *)REG(60,0);


	HBR_MODE = regs0->aud_misc_ctrl;
	HBR_MODE &= 0xfffffbff; 		// enable Bit9, disable Bit10
	regs0->aud_misc_ctrl = HBR_MODE|(0x1<<9);

	return;
}

void F_UpdateCGMS( void )
{
	UINT32 AUD_IEC0_OUT_PAR0 = 0;
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;//(volatile RegisterFile_Audio *)REG(60,0);

	AUD_IEC0_OUT_PAR0 = regs0->iec0_par0_out;
	AUD_IEC0_OUT_PAR0 |= (0x1<< 5);

	/*  cheak if bit[1] == 1, if yes reset bit[5] */
	if( aud_param.CGMS_mode & 0x2)
		AUD_IEC0_OUT_PAR0 &= ~(0x1 << 5);

	AUD_IEC0_OUT_PAR0 &= ~(0x1 << 8);

	/* check if bit[0] == 1, if yes reset bit[8] */
	if( (aud_param.CGMS_mode & 0x1) )
		AUD_IEC0_OUT_PAR0 |= (0x1 << 8);

	regs0->iec0_par0_out = AUD_IEC0_OUT_PAR0;

	return;
}

void SetCGMS(UINT8 mode, UINT8 val)
{
	switch(mode)
	{
		case CGMS_Init:
			aud_param.CGMS_mode &= 0xfffffff0;
			break;

		case CGMS_unlimited_copy:
			aud_param.CGMS_mode |= 0x0;
			break;

		case CGMS_one_copy_allowed:
			aud_param.CGMS_mode |= 0x2;
			break;

		case CGMS_no_copy_allowed:
			aud_param.CGMS_mode |= 0x3;
			break;

		case bit_resolution:
			aud_param.CGMS_mode |= (val << 2);
			break;

		case category_code:
			aud_param.CGMS_mode |= (val << 3);
			break;
	}

	return;
}


/**
* @brief   update IEC0 channel status
* @param user_disc_fmt [in] e.g., DISC_FORMAT_DVD, DISC_FORMAT_CD, DISC_FORMAT_DTSCD
* @param fs_id [in] e.g., FS_48K
*/
void F_UpdateIEC0_ChannelStatus( UINT32 user_disc_fmt )
{
	unsigned int ChannelStaus_SampleRate;
	unsigned int Pre_ChannelStaus;
	unsigned int New_ChannelStaus;
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;//(volatile RegisterFile_Audio *)REG(60,0);

	F_ConfigSpdif(user_disc_fmt);
	F_UpdateCGMS();

	switch( aud_param.fsclkInfo.IEC0_clk )
	{
		case FS_22K:
			ChannelStaus_SampleRate = SampleRate_22K;
			break;

		case FS_24K:
			ChannelStaus_SampleRate = SampleRate_24K;
			break;

		case FS_32K:
			ChannelStaus_SampleRate = SampleRate_32K;
			break;

		case FS_11K:
		case FS_44K:
			ChannelStaus_SampleRate = SampleRate_44K;
			break;

		case FS_6K:
		case FS_12K:
		case FS_48K:
			ChannelStaus_SampleRate = SampleRate_48K;
			break;

		case FS_88K:
			ChannelStaus_SampleRate = SampleRate_88K;
			break;

		case FS_96K:
			ChannelStaus_SampleRate = SampleRate_96K;
			break;

		case FS_176K:
			ChannelStaus_SampleRate = SampleRate_176K;
			break;

		case FS_192K:
			ChannelStaus_SampleRate = SampleRate_192K;
			break;

		default:
			ChannelStaus_SampleRate = SampleRate_NONE;   //error
			break;
	}

	/* set Channel Staus for sample rate */
	ChannelStaus_SampleRate = ChannelStaus_SampleRate << 28 ;
	Pre_ChannelStaus =  regs0->iec0_par0_out;

	if(ChannelStaus_SampleRate != (Pre_ChannelStaus & 0xF0000000))
	{
		New_ChannelStaus = ChannelStaus_SampleRate | (Pre_ChannelStaus & ~(0xF0000000));
		regs0->iec0_par0_out = New_ChannelStaus;
	}

	return;
}

/**
* @brief   update IEC1 channel status
* @param user_disc_fmt [in] e.g., DISC_FORMAT_DVD, DISC_FORMAT_CD, DISC_FORMAT_DTSCD
* @param fs_id [in] e.g., FS_48K
* @param CodecCapability [in] e.g., CodecSupportHBR, CodecNotSupportHBR
*/
void F_UpdateIEC1_ChannelStatus( UINT32 user_disc_fmt, UINT16 CodecCapability)
{
	UINT32 ChannelStaus_HD_Setting = 0;
	UINT32 ChannelStaus_SampleRate;
	UINT32 Pre_ChannelStaus;
	UINT32 New_ChannelStaus;
	int fs_id = 0;
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;//(volatile RegisterFile_Audio *)REG(60,0);

	F_ConfigHdmi(user_disc_fmt, CodecCapability);

	if( ( aud_param.hdmi_mode == HDMI_MODE_RAWHD )  && ((CodecCapability &0xf0) == 0x20 ) )
		fs_id = aud_param.fsclkInfo.hdmi_i2s_clk;
	else
		fs_id = aud_param.fsclkInfo.hdmi_iec_clk;

	switch( fs_id )
	{
		case FS_22K:
			ChannelStaus_SampleRate = SampleRate_22K;
			ChannelStaus_HD_Setting = 0x30000000;	//176kHz
			break;

		case FS_24K:
			ChannelStaus_SampleRate = SampleRate_24K;
			ChannelStaus_HD_Setting =  0x70000000;	// 192kHz
			break;

		case FS_32K:
			ChannelStaus_SampleRate = SampleRate_32K;
			ChannelStaus_HD_Setting  = 0xD1000000;	// 128kHz
			break;

		case FS_11K:
		case FS_44K:
			ChannelStaus_SampleRate = SampleRate_44K;
			ChannelStaus_HD_Setting = 0x30000000;	//176kHz
			break;

		case FS_6K:
		case FS_12K:
		case FS_48K:
			ChannelStaus_SampleRate = SampleRate_48K;
			ChannelStaus_HD_Setting =  0x70000000;	// 192kHz
			break;

		case FS_88K:
			ChannelStaus_SampleRate = SampleRate_88K;
			ChannelStaus_HD_Setting = 0x30000000; 	//176kHz
			break;

		case FS_96K:
			ChannelStaus_SampleRate = SampleRate_96K;
			ChannelStaus_HD_Setting =  0x70000000;	// 192kHz
			break;

		case FS_176K:
			ChannelStaus_SampleRate = SampleRate_176K;
			ChannelStaus_HD_Setting = 0x30000000;	//176kHz
			break;

		case FS_192K:
			ChannelStaus_SampleRate = SampleRate_192K;
			ChannelStaus_HD_Setting =  0x70000000;	// 192kHz
			break;

		default:
			ChannelStaus_SampleRate = SampleRate_NONE;   //error
			break;
	}

	/* set Channel Staus for sample rate */
	ChannelStaus_SampleRate = ChannelStaus_SampleRate << 28 ;

	if( ( aud_param.hdmi_mode == HDMI_MODE_RAWHD )  && ((CodecCapability &0xf0) == 0x20 ) )
	{
		Pre_ChannelStaus =  regs0->iec1_par0_out;
		if(ChannelStaus_HD_Setting != (Pre_ChannelStaus & 0xF3000000)) // mask bit[31:28] & bit[25 :24]
		{
			New_ChannelStaus = ChannelStaus_HD_Setting | (Pre_ChannelStaus & ~(0xF3000000));
			regs0->iec1_par0_out = New_ChannelStaus;
		}
	}
	else
	{	// pcm
		Pre_ChannelStaus =  regs0->iec1_par0_out;
		if(ChannelStaus_SampleRate != (Pre_ChannelStaus & 0xF3000000)) //mask bit[31:28] & bit[25 :24]
		{
			New_ChannelStaus = ChannelStaus_SampleRate | (Pre_ChannelStaus & ~(0xF3000000));
			regs0->iec1_par0_out = New_ChannelStaus;
		}
	}

	return;
}

/**
* @brief
* @param CGMS_type [IN] IEC 61937 Copy Generation Management System(bit[1:0])
*			  | bit2 _ bit15 ____________________
*		0 0    |   1        x   unlimited copies allowed
*		0 1    |   -        -   not used
*		1 0    |   0        0   one copy allowed
*		1 1    |   0        1   no copy allowed
*@param bit_res [IN] bit[2]
*		0: SPDIF/PCM output 16-bit
*		1: SPDIF/PCM output 24-bit
*@param disc_type [IN] byte1 category code(bit[3])
*		0: set CD
*		1: set DVD
*/
void F_Set_Spdif_ChannelStatus(UINT8 CGMS_type , UINT8 bit_res, UINT8 disc_type)
{
	/* clear last four bits */
	SetCGMS(CGMS_Init,0);

	/* set CGMS type */
	switch(CGMS_type)
	{
		case unlimited_copy:
			SetCGMS(CGMS_unlimited_copy,0);
			//HDMI_IF_Set_AudioGMS(CGMS_UnlimitCopy);
			break;
		case one_copy_allowed:
			SetCGMS(CGMS_one_copy_allowed,0);
			//HDMI_IF_Set_AudioGMS(CGMS_OneCopyAllowed);
			break;
		case no_copy_allowed:
			SetCGMS(CGMS_no_copy_allowed,0);
			//HDMI_IF_Set_AudioGMS(CGMS_NoCopyAllowed);
			break;
		default:
			SetCGMS(CGMS_no_copy_allowed,0);
			//HDMI_IF_Set_AudioGMS(CGMS_NoCopyAllowed);
			break;
	}

	/* set bit resolution */
	SetCGMS(bit_resolution,bit_res);

	/* set category code */
	SetCGMS(category_code,disc_type);

	return;
}


