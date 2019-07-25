#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <mach/gpio_drv.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

//#define	PINCTRL_DEBUG

#define MAX_PINS_ONE_IP			128

#ifdef PINCTRL_DEBUG
void pinmux_reg_dump(void)
{
	u32 pinmux_value[120];
	printf("pinmux_reg_dump \n");
	pinmux_value[0] =  GPIO_PIN_MUX_GET(PMX_L2SW_CLK_OUT);
	pinmux_value[1] =  GPIO_PIN_MUX_GET(PMX_L2SW_MAC_SMI_MDC);
	pinmux_value[2] = GPIO_PIN_MUX_GET(PMX_L2SW_LED_FLASH0);
	pinmux_value[3] = GPIO_PIN_MUX_GET(PMX_L2SW_LED_FLASH1);
	pinmux_value[4] = GPIO_PIN_MUX_GET(PMX_L2SW_LED_ON0);
	pinmux_value[5] = GPIO_PIN_MUX_GET(PMX_L2SW_LED_ON1);
	pinmux_value[6] =  GPIO_PIN_MUX_GET(PMX_L2SW_MAC_SMI_MDIO);
	pinmux_value[7] =  GPIO_PIN_MUX_GET(PMX_L2SW_P0_MAC_RMII_TXEN);
	pinmux_value[8] =  GPIO_PIN_MUX_GET(PMX_L2SW_P0_MAC_RMII_TXD0);
	pinmux_value[9] =  GPIO_PIN_MUX_GET(PMX_L2SW_P0_MAC_RMII_TXD1);
	pinmux_value[10] = GPIO_PIN_MUX_GET(PMX_L2SW_P0_MAC_RMII_CRSDV);
	pinmux_value[11] = GPIO_PIN_MUX_GET(PMX_L2SW_P0_MAC_RMII_RXD0);
	pinmux_value[12] = GPIO_PIN_MUX_GET(PMX_L2SW_P0_MAC_RMII_RXD1);
	pinmux_value[13] = GPIO_PIN_MUX_GET(PMX_L2SW_P0_MAC_RMII_RXER);
	pinmux_value[14] = GPIO_PIN_MUX_GET(PMX_L2SW_P1_MAC_RMII_TXEN);
	pinmux_value[15] = GPIO_PIN_MUX_GET(PMX_L2SW_P1_MAC_RMII_TXD0);
	pinmux_value[16] = GPIO_PIN_MUX_GET(PMX_L2SW_P1_MAC_RMII_TXD1);
	pinmux_value[17] = GPIO_PIN_MUX_GET(PMX_L2SW_P1_MAC_RMII_CRSDV);
	pinmux_value[18] = GPIO_PIN_MUX_GET(PMX_L2SW_P1_MAC_RMII_RXD0);
	pinmux_value[19] = GPIO_PIN_MUX_GET(PMX_L2SW_P1_MAC_RMII_RXD1);
	pinmux_value[20] = GPIO_PIN_MUX_GET(PMX_L2SW_P1_MAC_RMII_RXER);
	pinmux_value[21] = GPIO_PIN_MUX_GET(PMX_DAISY_MODE);
	pinmux_value[22] = GPIO_PIN_MUX_GET(PMX_SDIO_CLK);
	pinmux_value[23] = GPIO_PIN_MUX_GET(PMX_SDIO_CMD); 
	pinmux_value[24] = GPIO_PIN_MUX_GET(PMX_SDIO_D0); 
	pinmux_value[25] = GPIO_PIN_MUX_GET(PMX_SDIO_D1);  
	pinmux_value[26] = GPIO_PIN_MUX_GET(PMX_SDIO_D2);  
	pinmux_value[27] = GPIO_PIN_MUX_GET(PMX_SDIO_D3);  
	pinmux_value[28] = GPIO_PIN_MUX_GET(PMX_PWM0);  
	pinmux_value[29] = GPIO_PIN_MUX_GET(PMX_PWM1);     
	pinmux_value[30] = GPIO_PIN_MUX_GET(PMX_PWM2);     
	pinmux_value[31] = GPIO_PIN_MUX_GET(PMX_PWM3);     
	pinmux_value[32] = GPIO_PIN_MUX_GET(PMX_PWM4);     
	pinmux_value[33] = GPIO_PIN_MUX_GET(PMX_PWM5);     
	pinmux_value[34] = GPIO_PIN_MUX_GET(PMX_PWM6);     
	pinmux_value[35] = GPIO_PIN_MUX_GET(PMX_PWM7);     
	pinmux_value[36] = GPIO_PIN_MUX_GET(PMX_ICM0_D);     
	pinmux_value[37] = GPIO_PIN_MUX_GET(PMX_ICM1_D);   
	pinmux_value[38] = GPIO_PIN_MUX_GET(PMX_ICM2_D);   
	pinmux_value[39] = GPIO_PIN_MUX_GET(PMX_ICM3_D);   
	pinmux_value[40] = GPIO_PIN_MUX_GET(PMX_ICM0_CLK);   
	pinmux_value[41] = GPIO_PIN_MUX_GET(PMX_ICM1_CLK); 
	pinmux_value[42] = GPIO_PIN_MUX_GET(PMX_ICM2_CLK); 
	pinmux_value[43] = GPIO_PIN_MUX_GET(PMX_ICM3_CLK); 
	pinmux_value[44] = GPIO_PIN_MUX_GET(PMX_SPIM0_INT); 
	pinmux_value[45] = GPIO_PIN_MUX_GET(PMX_SPIM0_CLK);
	pinmux_value[46] = GPIO_PIN_MUX_GET(PMX_SPIM0_CEN);
	pinmux_value[47] = GPIO_PIN_MUX_GET(PMX_SPIM0_DO);
	pinmux_value[48] = GPIO_PIN_MUX_GET(PMX_SPIM0_DI); 
	pinmux_value[49] = GPIO_PIN_MUX_GET(PMX_SPIM1_INT); 
	pinmux_value[50] = GPIO_PIN_MUX_GET(PMX_SPIM1_CLK);
	pinmux_value[51] = GPIO_PIN_MUX_GET(PMX_SPIM1_CEN);
	pinmux_value[52] = GPIO_PIN_MUX_GET(PMX_SPIM1_DO);
	pinmux_value[53] = GPIO_PIN_MUX_GET(PMX_SPIM1_DI); 
	pinmux_value[54] = GPIO_PIN_MUX_GET(PMX_SPIM2_INT); 
	pinmux_value[55] = GPIO_PIN_MUX_GET(PMX_SPIM2_CLK);
	pinmux_value[56] = GPIO_PIN_MUX_GET(PMX_SPIM2_CEN);
	pinmux_value[57] = GPIO_PIN_MUX_GET(PMX_SPIM2_DO);
	pinmux_value[58] = GPIO_PIN_MUX_GET(PMX_SPIM2_DI); 
	pinmux_value[59] = GPIO_PIN_MUX_GET(PMX_SPIM3_INT); 
	pinmux_value[60] = GPIO_PIN_MUX_GET(PMX_SPIM3_CLK);
	pinmux_value[61] = GPIO_PIN_MUX_GET(PMX_SPIM3_CEN);
	pinmux_value[62] = GPIO_PIN_MUX_GET(PMX_SPIM3_DO);
	pinmux_value[63] = GPIO_PIN_MUX_GET(PMX_SPIM3_DI); 
	pinmux_value[64] = GPIO_PIN_MUX_GET(PMX_SPI0S_INT); 
	pinmux_value[65] = GPIO_PIN_MUX_GET(PMX_SPI0S_CLK);
	pinmux_value[66] = GPIO_PIN_MUX_GET(PMX_SPI0S_EN);
	pinmux_value[67] = GPIO_PIN_MUX_GET(PMX_SPI0S_DO); 
	pinmux_value[68] = GPIO_PIN_MUX_GET(PMX_SPI0S_DI); 
	pinmux_value[69] = GPIO_PIN_MUX_GET(PMX_SPI1S_INT); 
	pinmux_value[70] = GPIO_PIN_MUX_GET(PMX_SPI1S_CLK);
	pinmux_value[71] = GPIO_PIN_MUX_GET(PMX_SPI1S_EN);
	pinmux_value[72] = GPIO_PIN_MUX_GET(PMX_SPI1S_DO); 
	pinmux_value[73] = GPIO_PIN_MUX_GET(PMX_SPI1S_DI); 
	pinmux_value[74] = GPIO_PIN_MUX_GET(PMX_SPI2S_INT); 
	pinmux_value[75] = GPIO_PIN_MUX_GET(PMX_SPI2S_CLK);
	pinmux_value[76] = GPIO_PIN_MUX_GET(PMX_SPI2S_EN);
	pinmux_value[77] = GPIO_PIN_MUX_GET(PMX_SPI2S_DO); 
	pinmux_value[78] = GPIO_PIN_MUX_GET(PMX_SPI2S_DI); 	
	pinmux_value[79] = GPIO_PIN_MUX_GET(PMX_SPI3S_INT); 
	pinmux_value[80] = GPIO_PIN_MUX_GET(PMX_SPI3S_CLK);
	pinmux_value[81] = GPIO_PIN_MUX_GET(PMX_SPI3S_EN);
	pinmux_value[82] = GPIO_PIN_MUX_GET(PMX_SPI3S_DO); 
	pinmux_value[83] = GPIO_PIN_MUX_GET(PMX_SPI3S_DI); 
	pinmux_value[84] = GPIO_PIN_MUX_GET(PMX_I2CM0_CK); 
	pinmux_value[85] = GPIO_PIN_MUX_GET(PMX_I2CM0_DAT); 
	pinmux_value[86] = GPIO_PIN_MUX_GET(PMX_I2CM1_CK);	
	pinmux_value[87] = GPIO_PIN_MUX_GET(PMX_I2CM1_DAT); 
	pinmux_value[88] = GPIO_PIN_MUX_GET(PMX_I2CM2_CK);		
	pinmux_value[89] = GPIO_PIN_MUX_GET(PMX_I2CM2_D); 
	pinmux_value[90] = GPIO_PIN_MUX_GET(PMX_I2CM3_CK);  		
	pinmux_value[91] = GPIO_PIN_MUX_GET(PMX_I2CM3_D); 
	pinmux_value[92] = GPIO_PIN_MUX_GET(PMX_UA1_TX);  		
	pinmux_value[93] = GPIO_PIN_MUX_GET(PMX_UA1_RX);   
	pinmux_value[94] = GPIO_PIN_MUX_GET(PMX_UA1_CTS);   	
	pinmux_value[95] = GPIO_PIN_MUX_GET(PMX_UA1_RTS);  
	pinmux_value[96] = GPIO_PIN_MUX_GET(PMX_UA2_TX);  	
	pinmux_value[97] = GPIO_PIN_MUX_GET(PMX_UA2_RX);   
	pinmux_value[98] = GPIO_PIN_MUX_GET(PMX_UA2_CTS);   	
	pinmux_value[99] = GPIO_PIN_MUX_GET(PMX_UA2_RTS);  
	pinmux_value[100] =GPIO_PIN_MUX_GET(PMX_UA3_TX);  	
	pinmux_value[101] =GPIO_PIN_MUX_GET(PMX_UA3_RX);   
	pinmux_value[102] =GPIO_PIN_MUX_GET(PMX_UA3_CTS);   	
	pinmux_value[103] =GPIO_PIN_MUX_GET(PMX_UA3_RTS);  
	pinmux_value[104] =GPIO_PIN_MUX_GET(PMX_UA4_TX);  	
	pinmux_value[105] =GPIO_PIN_MUX_GET(PMX_UA4_RX);   
	pinmux_value[106] =GPIO_PIN_MUX_GET(PMX_UA4_CTS);   	
	pinmux_value[107] =GPIO_PIN_MUX_GET(PMX_UA4_RTS);  
	pinmux_value[108] =GPIO_PIN_MUX_GET(PMX_TIMER0_INT); 		
	pinmux_value[109] =GPIO_PIN_MUX_GET(PMX_TIMER1_INT);
	pinmux_value[110] =GPIO_PIN_MUX_GET(PMX_TIMER2_INT);
	pinmux_value[111] =GPIO_PIN_MUX_GET(PMX_TIMER3_INT);
	pinmux_value[112] =GPIO_PIN_MUX_GET(PMX_GPIO_INT0);
	pinmux_value[113] =GPIO_PIN_MUX_GET(PMX_GPIO_INT1);	
	pinmux_value[114] =GPIO_PIN_MUX_GET(PMX_GPIO_INT2);	
	pinmux_value[115] =GPIO_PIN_MUX_GET(PMX_GPIO_INT3);	
	pinmux_value[116] =GPIO_PIN_MUX_GET(PMX_GPIO_INT4);	
	pinmux_value[117] =GPIO_PIN_MUX_GET(PMX_GPIO_INT5);	
	pinmux_value[118] =GPIO_PIN_MUX_GET(PMX_GPIO_INT6);	
	pinmux_value[119] =GPIO_PIN_MUX_GET(PMX_GPIO_INT7);

	printf("l2sw = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[0], pinmux_value[1], pinmux_value[2], pinmux_value[3], pinmux_value[4],
		pinmux_value[5], pinmux_value[6], pinmux_value[7], pinmux_value[8], pinmux_value[9]);
	printf("l2sw = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[10], pinmux_value[11], pinmux_value[12], pinmux_value[13], pinmux_value[14],
		pinmux_value[15], pinmux_value[16], pinmux_value[17], pinmux_value[18], pinmux_value[19]);
	printf("l2sw = 0x%02x 0x%02x ---- ---- ---- ---- ---- ---- ---- ---- \n",
		pinmux_value[20], pinmux_value[21]);
	printf("sdio = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- ---- ---- \n",
		pinmux_value[22], pinmux_value[23], pinmux_value[24], pinmux_value[25], pinmux_value[26],
		pinmux_value[27]);
	printf("pwm  = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- \n",
		pinmux_value[28], pinmux_value[29], pinmux_value[30], pinmux_value[31], pinmux_value[32],
		pinmux_value[33], pinmux_value[34], pinmux_value[35]);
	printf("icm  = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- \n",
		pinmux_value[36], pinmux_value[37], pinmux_value[38], pinmux_value[39], pinmux_value[40],
		pinmux_value[41], pinmux_value[42], pinmux_value[43]);
	printf("spim = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[44], pinmux_value[45], pinmux_value[46], pinmux_value[47], pinmux_value[48],
		pinmux_value[49], pinmux_value[50], pinmux_value[51], pinmux_value[52], pinmux_value[53]);
	printf("spim = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[54], pinmux_value[55], pinmux_value[56], pinmux_value[57], pinmux_value[58],
		pinmux_value[59], pinmux_value[60], pinmux_value[61], pinmux_value[62], pinmux_value[63]);
	printf("spis = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[64], pinmux_value[65], pinmux_value[66], pinmux_value[67], pinmux_value[68],
		pinmux_value[69], pinmux_value[70], pinmux_value[71], pinmux_value[72], pinmux_value[73]);
	printf("spis = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[74], pinmux_value[75], pinmux_value[76], pinmux_value[77], pinmux_value[78],
		pinmux_value[79], pinmux_value[80], pinmux_value[81], pinmux_value[82], pinmux_value[83]);
	printf("i2c  = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- \n",
		pinmux_value[84], pinmux_value[85], pinmux_value[86], pinmux_value[87], pinmux_value[88],
		pinmux_value[89], pinmux_value[90], pinmux_value[91]);
	printf("uart = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[92], pinmux_value[93], pinmux_value[94], pinmux_value[95], pinmux_value[96],
		pinmux_value[97], pinmux_value[98], pinmux_value[99], pinmux_value[100], pinmux_value[101]);
	printf("uart = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- ---- ---- \n",
		pinmux_value[102], pinmux_value[103], pinmux_value[104], pinmux_value[105], pinmux_value[106], 
		pinmux_value[107]);
	printf("tim  = 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- ---- ---- ---- ---- \n",
		pinmux_value[108], pinmux_value[109], pinmux_value[110], pinmux_value[111]);
	printf("int  = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- \n",
		pinmux_value[112], pinmux_value[113], pinmux_value[114], pinmux_value[115], pinmux_value[116],
		pinmux_value[117], pinmux_value[118], pinmux_value[119]);
		
}

void gpio_reg_dump(void)
{
	int j;
	u32 gpio_value[100];
	printf("gpio_reg_dump \n");
	for (j = 0; j < 72; j++)
	{
		gpio_value[j] = GPIO_PARA_GET(j);
		if(j%8 ==7)
		{
			printf("GPIO_P%d 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",(j/8),
			gpio_value[j-7],gpio_value[j-6],gpio_value[j-5],gpio_value[j-4],
			gpio_value[j-3],gpio_value[j-2],gpio_value[j-1],gpio_value[j]);
		}
	}	
}
#endif

static int sunplus_pinctrl_config(int offset)
{
	u32 pin_mux[MAX_PINS_ONE_IP];
	u32 gpio_sp[MAX_PINS_ONE_IP];
	//u32 value;
	int len1,len2;
	/*
	 * check for "pinmux" property in each subnode 
	   of pin controller phandle "pinctrl-0"
	 * */
	fdt_for_each_subnode(offset, gd->fdt_blob, offset) {
		int i,j;
		
		len1 = fdtdec_get_int_array_count(gd->fdt_blob, offset,
						 "pinmux", pin_mux,
						 ARRAY_SIZE(pin_mux));
		#ifdef PINCTRL_DEBUG
		printf("%s: no of pinmux entries= %d\n", __func__, len1);
		#endif
		if ((len1 < 0) || (len1%2 != 0)) {
			#ifdef PINCTRL_DEBUG
			printf("%s: none pinmux setting in subnode= %d\n", __func__, len1);
			#endif
			//return -EINVAL;
		}
		else {
			for (i = 0; i < len1; i++) {
				if(i%2 ==1)
				{
					debug("pinmux = 0x%08x 0x%08x\n", *(pin_mux + i-1),*(pin_mux + i));
					GPIO_PIN_MUX_SEL(*(pin_mux + i-1),*(pin_mux + i));
					GPIO_F_SET(*(pin_mux + i),0);
					GPIO_M_SET(*(pin_mux + i),1);
					//value = GPIO_PIN_MUX_GET(*(pin_mux + i-1));
					//printf("pinmux get = 0x%02x \n", value);
				}
			}
		}
		#ifdef PINCTRL_DEBUG
		pinmux_reg_dump();
		#endif

		len2 = fdtdec_get_int_array_count(gd->fdt_blob, offset,
						 "sunplus,pins", gpio_sp,
						 ARRAY_SIZE(gpio_sp));
		#ifdef PINCTRL_DEBUG
		printf("%s: no of gpio entries= %d\n", __func__, len2);
		#endif
		if ((len2 < 0) || (len2%3 != 0)) {
			#ifdef PINCTRL_DEBUG
			printf("%s: none gpio setting in subnode %d\n", __func__, len2);
			#endif
			//return -EINVAL;
		}
		else {
			for (j = 0; j < len2; j++) {
				if(j%3 ==2)
				{
					debug("gpio_sp = 0x%08x 0x%08x 0x%08x \n", *(gpio_sp + j-2), *(gpio_sp + j-1), *(gpio_sp + j));
					if((*(gpio_sp + j-2)) < 72)
					{
						#ifdef PINCTRL_DEBUG
						if(GPIO_F_GET(*(gpio_sp + j-2)) == 1)
						{
							if(GPIO_M_GET(*(gpio_sp + j-2)) == 1)
								debug("Before Change : GPIO Mode \n");
							else
								debug("Before Change : IOP Mode \n");
						}
						else
						{
							debug("Before Change : Pinmux Mode \n");
						}
						#endif
						if((*(gpio_sp + j-1) == 0) || (*(gpio_sp + j-1) == 1)) //Output or //Open Drain
						{
							if(*(gpio_sp + j-1) == 1)
								GPIO_OD_SET(*(gpio_sp + j-2),1);
							else if(*(gpio_sp + j-1) == 0)
								GPIO_OD_SET(*(gpio_sp + j-2),0);
							else
								return -EINVAL;
	
							if(*(gpio_sp + j) == 1) //high
							{
								if(GPIO_O_INV_GET(*(gpio_sp + j-2)) == 0)
									GPIO_O_SET(*(gpio_sp + j-2),1);
								else if(GPIO_O_INV_GET(*(gpio_sp + j-2)) == 1)
									GPIO_O_SET(*(gpio_sp + j-2),0);
								else
									return -EINVAL;
								GPIO_E_SET(*(gpio_sp + j-2),1);
								debug("Set Pin %d to High \n", (*(gpio_sp + j-2)));
							}
							else if(*(gpio_sp + j) == 0) //low
							{
								if(GPIO_O_INV_GET(*(gpio_sp + j-2)) == 0)
									GPIO_O_SET(*(gpio_sp + j-2),0);
								else if(GPIO_O_INV_GET(*(gpio_sp + j-2)) == 1)
									GPIO_O_SET(*(gpio_sp + j-2),1);
								else
									return -EINVAL;
								GPIO_E_SET(*(gpio_sp + j-2),1);
								debug("Set Pin %d to Low \n", (*(gpio_sp + j-2)));
							}
							else
								return -EINVAL;
							GPIO_F_SET(*(gpio_sp + j-2),1);
							GPIO_M_SET(*(gpio_sp + j-2),1);
						}
						else if(*(gpio_sp + j-1) == 2) //Input
						{
							if(GPIO_O_INV_GET(*(gpio_sp + j-2)) == 0)
							{
								if(GPIO_I_GET(*(gpio_sp + j-2))==0)
									debug("Pin %d Current setting : Input Low \n", (*(gpio_sp + j-2)));
								else if(GPIO_I_GET(*(gpio_sp + j-2))==1)
									debug("Pin %d Current setting : Input High \n", (*(gpio_sp + j-2)));
								else
									return -EINVAL;
							}
							else if(GPIO_O_INV_GET(*(gpio_sp + j-2)) == 1)
							{
								if(GPIO_I_GET(*(gpio_sp + j-2))==0)
									debug("Pin %d Current setting : Input High (with Invert) \n", (*(gpio_sp + j-2)));
								else if(GPIO_I_GET(*(gpio_sp + j-2))==1)
									debug("Pin %d Current setting : Input Low (with Invert) \n", (*(gpio_sp + j-2)));
								else
									return -EINVAL;
							}
							else
								return -EINVAL;
							GPIO_F_SET(*(gpio_sp + j-2),1);
							GPIO_M_SET(*(gpio_sp + j-2),1);
						}
						else
							return -EINVAL;
	
						//value = GPIO_PARA_GET(*(gpio_sp + j-2));
						//printf("gpio value = 0x%08x \n", value);
						#ifdef PINCTRL_DEBUG
						if(GPIO_F_GET(*(gpio_sp + j-2)) == 1)
						{
							if(GPIO_M_GET(*(gpio_sp + j-2)) == 1)
								debug("After Change : GPIO Mode \n");
							else
								debug("After Change : IOP Mode \n");
						}
						else
						{
							debug("After Change : Pinmux Mode \n");
						}
						#endif
					}
					else
					{
						debug("Over setting range \n");
					}
				}
			}
		}
		#ifdef PINCTRL_DEBUG
		gpio_reg_dump();
		#endif
	}

	return 0;
}

static int sunplus_pinctrl_set_state(struct udevice *dev, struct udevice *config)
{
	return sunplus_pinctrl_config(dev_of_offset(config));
}

static struct pinctrl_ops sunplus_pinctrl_ops = {
	.set_state		= sunplus_pinctrl_set_state,
};

static const struct udevice_id sunplus_pinctrl_ids[] = {
 { .compatible = "sunplus,sppctl"},
 { /* zero */ }
};

U_BOOT_DRIVER(pinctrl_sunplus) = {
	.name		= "pinctrl_sunplus",
	.id			= UCLASS_PINCTRL,
	.of_match	= sunplus_pinctrl_ids,
	.ops		= &sunplus_pinctrl_ops,
	.bind		= dm_scan_fdt_dev,
};
