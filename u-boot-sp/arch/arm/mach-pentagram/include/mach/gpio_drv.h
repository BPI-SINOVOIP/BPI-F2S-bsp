#ifndef _SP_GPIO_DRV_H_
#define _SP_GPIO_DRV_H_

#include <common.h>
#include <dm.h>

typedef enum
{
	
	//G2
	PMX_L2SW_CLK_OUT    	= 0x02004000,			// (0-64, 0)
	PMX_L2SW_MAC_SMI_MDC       = 0x02004008,			// (0-64, 8)	
	PMX_L2SW_LED_FLASH0        = 0x02014000,			// (0-64, 0)	
	PMX_L2SW_LED_FLASH1        = 0x02014008,			// (0-64, 8)
	PMX_L2SW_LED_ON0           = 0x02024000,			// (0-64, 0)
	PMX_L2SW_LED_ON1           = 0x02024008,			// (0-64, 8)
	PMX_L2SW_MAC_SMI_MDIO      = 0x02034000,			// (0-64, 0)
	PMX_L2SW_P0_MAC_RMII_TXEN  = 0x02034008,			// (0-64, 8)
	PMX_L2SW_P0_MAC_RMII_TXD0  = 0x02044000,			// (0-64, 0)	
	PMX_L2SW_P0_MAC_RMII_TXD1  = 0x02044008,			// (0-64, 8)
	PMX_L2SW_P0_MAC_RMII_CRSDV = 0x02054000,			// (0-64, 0)	
	PMX_L2SW_P0_MAC_RMII_RXD0  = 0x02054008,			// (0-64, 8)
	PMX_L2SW_P0_MAC_RMII_RXD1  = 0x02064000,			// (0-64, 0)	
	PMX_L2SW_P0_MAC_RMII_RXER  = 0x02064008,			// (0-64, 8)
	PMX_L2SW_P1_MAC_RMII_TXEN  = 0x02074000,			// (0-64, 0)	
	PMX_L2SW_P1_MAC_RMII_TXD0  = 0x02074008,			// (0-64, 8)
	PMX_L2SW_P1_MAC_RMII_TXD1  = 0x02084000,			// (0-64, 0)	
	PMX_L2SW_P1_MAC_RMII_CRSDV = 0x02084008,			// (0-64, 8)
	PMX_L2SW_P1_MAC_RMII_RXD0  = 0x02094000,			// (0-64, 0)	
	PMX_L2SW_P1_MAC_RMII_RXD1  = 0x02094008,			// (0-64, 8)
	PMX_L2SW_P1_MAC_RMII_RXER  = 0x020a4000,			// (0-64, 0)
	PMX_DAISY_MODE = 0x020a4008,			// (0-64, 8)
	PMX_SDIO_CLK   = 0x020b4000,			// (0-64, 0)
	PMX_SDIO_CMD   = 0x020b4008,			// (0-64, 8)
	PMX_SDIO_D0    = 0x020c4000,			// (0-64, 0)
	PMX_SDIO_D1    = 0x020c4008,			// (0-64, 8)
	PMX_SDIO_D2    = 0x020d4000,			// (0-64, 0)
	PMX_SDIO_D3    = 0x020d4008,			// (0-64, 8)
	PMX_PWM0       = 0x020e4000,			// (0-64, 0)
	PMX_PWM1       = 0x020e4008,			// (0-64, 8)
	PMX_PWM2       = 0x020f4000,			// (0-64, 0)
	PMX_PWM3       = 0x020f4008,			// (0-64, 8)
	PMX_PWM4       = 0x02104000,			// (0-64, 0)
	PMX_PWM5       = 0x02104008,			// (0-64, 8)
	PMX_PWM6       = 0x02114000,			// (0-64, 0)
	PMX_PWM7       = 0x02114008,			// (0-64, 8)
	PMX_ICM0_D     = 0x02124000,			// (0-64, 0)
	PMX_ICM1_D     = 0x02124008,			// (0-64, 8)
	PMX_ICM2_D     = 0x02134000,			// (0-64, 0)
	PMX_ICM3_D     = 0x02134008,			// (0-64, 8)
	PMX_ICM0_CLK   = 0x02144000,			// (0-64, 0)
	PMX_ICM1_CLK   = 0x02144008,			// (0-64, 8)
	PMX_ICM2_CLK   = 0x02154000,			// (0-64, 0)
	PMX_ICM3_CLK   = 0x02154008,			// (0-64, 8)
	PMX_SPIM0_INT  = 0x02164000,			// (0-64, 0)
	PMX_SPIM0_CLK  = 0x02164008,			// (0-64, 8)
	PMX_SPIM0_CEN  = 0x02174000,			// (0-64, 0)
	PMX_SPIM0_DO   = 0x02174008,			// (0-64, 8)
	PMX_SPIM0_DI   = 0x02184000,			// (0-64, 0)
	PMX_SPIM1_INT  = 0x02184008,			// (0-64, 8)
	PMX_SPIM1_CLK  = 0x02194000,			// (0-64, 0)
	PMX_SPIM1_CEN  = 0x02194008,			// (0-64, 8)
	PMX_SPIM1_DO   = 0x021a4000,			// (0-64, 0)
	PMX_SPIM1_DI   = 0x021a4008,			// (0-64, 8)
	PMX_SPIM2_INT  = 0x021b4000,			// (0-64, 0)
	PMX_SPIM2_CLK  = 0x021b4008,			// (0-64, 8)
	PMX_SPIM2_CEN  = 0x021c4000,			// (0-64, 0)
	PMX_SPIM2_DO   = 0x021c4008,			// (0-64, 8)
	PMX_SPIM2_DI   = 0x021d4000,			// (0-64, 0)
	PMX_SPIM3_INT  = 0x021d4008,			// (0-64, 8)
	PMX_SPIM3_CLK  = 0x021e4000,			// (0-64, 0)
	PMX_SPIM3_CEN  = 0x021e4008,			// (0-64, 8)
	PMX_SPIM3_DO   = 0x021f4000,			// (0-64, 0)
	PMX_SPIM3_DI   = 0x021f4008,			// (0-64, 8)
	//G3
	PMX_SPI0S_INT   = 0x03004000,			// (0-64, 0)
	PMX_SPI0S_CLK   = 0x03004008,			// (0-64, 8)
	PMX_SPI0S_EN    = 0x03014000,			// (0-64, 0)
	PMX_SPI0S_DO    = 0x03014008,			// (0-64, 8)
	PMX_SPI0S_DI    = 0x03024000,			// (0-64, 0)
	PMX_SPI1S_INT   = 0x03024008,			// (0-64, 8)
	PMX_SPI1S_CLK   = 0x03034000,			// (0-64, 0)
	PMX_SPI1S_EN    = 0x03034008,			// (0-64, 8)
	PMX_SPI1S_DO    = 0x03044000,			// (0-64, 0)
	PMX_SPI1S_DI    = 0x03044008,			// (0-64, 8)
	PMX_SPI2S_INT   = 0x03054000,			// (0-64, 0)
	PMX_SPI2S_CLK   = 0x03054008,			// (0-64, 8)
	PMX_SPI2S_EN    = 0x03064000,			// (0-64, 0)
	PMX_SPI2S_DO    = 0x03064008,			// (0-64, 8)
	PMX_SPI2S_DI    = 0x03074000,			// (0-64, 0)	
	PMX_SPI3S_INT   = 0x03074008,			// (0-64, 8)
	PMX_SPI3S_CLK   = 0x03084000,			// (0-64, 0)
	PMX_SPI3S_EN    = 0x03084008,			// (0-64, 8)
	PMX_SPI3S_DO    = 0x03094000,			// (0-64, 0)
	PMX_SPI3S_DI    = 0x03094008,			// (0-64, 8)
	PMX_I2CM0_CK    = 0x030a4000,			// (0-64, 0)
	PMX_I2CM0_DAT   = 0x030a4008,			// (0-64, 8)
	PMX_I2CM1_CK    = 0x030b4000,			// (0-64, 0)	
	PMX_I2CM1_DAT   = 0x030b4008,			// (0-64, 8)
	PMX_I2CM2_CK    = 0x030c4000,			// (0-64, 0)		
	PMX_I2CM2_D     = 0x030c4008,			// (0-64, 8)
	PMX_I2CM3_CK    = 0x030d4000,			// (0-64, 0)		
	PMX_I2CM3_D     = 0x030d4008,			// (0-64, 8)
	PMX_UA1_TX      = 0x030e4000,			// (0-64, 0)		
	PMX_UA1_RX      = 0x030e4008,			// (0-64, 8)
	PMX_UA1_CTS     = 0x030f4000,			// (0-64, 0)	
	PMX_UA1_RTS     = 0x030f4008,			// (0-64, 8)
	PMX_UA2_TX      = 0x03104000,			// (0-64, 0)	
	PMX_UA2_RX      = 0x03104008,			// (0-64, 8)
	PMX_UA2_CTS     = 0x03114000,			// (0-64, 0)	
	PMX_UA2_RTS     = 0x03114008,			// (0-64, 8)
	PMX_UA3_TX      = 0x03124000,			// (0-64, 0)	
	PMX_UA3_RX      = 0x03124008,			// (0-64, 8)
	PMX_UA3_CTS     = 0x03134000,			// (0-64, 0)	
	PMX_UA3_RTS     = 0x03134008,			// (0-64, 8)
	PMX_UA4_TX      = 0x03144000,			// (0-64, 0)	
	PMX_UA4_RX      = 0x03144008,			// (0-64, 8)
	PMX_UA4_CTS     = 0x03154000,			// (0-64, 0)	
	PMX_UA4_RTS     = 0x03154008,			// (0-64, 8)
	PMX_TIMER0_INT  = 0x03164000,			// (0-64, 0)		
	PMX_TIMER1_INT  = 0x03164008,			// (0-64, 8)
	PMX_TIMER2_INT  = 0x03174000,			// (0-64, 0)	
	PMX_TIMER3_INT  = 0x03174008,			// (0-64, 8)
	PMX_GPIO_INT0   = 0x03184000,			// (0-64, 0)	
	PMX_GPIO_INT1   = 0x03184008,			// (0-64, 8)	
	PMX_GPIO_INT2   = 0x03194000,			// (0-64, 0)	
	PMX_GPIO_INT3   = 0x03194008,			// (0-64, 8)	
	PMX_GPIO_INT4   = 0x031a4000,			// (0-64, 0)	
	PMX_GPIO_INT5   = 0x031a4008,			// (0-64, 8)	
	PMX_GPIO_INT6   = 0x031b4000,			// (0-64, 0)	
	PMX_GPIO_INT7   = 0x031b4008,			// (0-64, 8)

}PMXSEL_ID;

typedef struct PMXSEL_S {
	uint id;
	uint val;
}PMXSEL_T;

extern int gpio_debug_1(u32 bit);
extern int gpio_debug_0(u32 bit);
extern int gpio_input_invert_1(u32 bit);
extern int gpio_input_invert_0(u32 bit);
extern u32 gpio_input_invert_val_get(u32 bit);
extern int gpio_output_invert_1(u32 bit);
extern int gpio_output_invert_0(u32 bit);
extern u32 gpio_output_invert_val_get(u32 bit);
extern int gpio_open_drain_1(u32 bit);
extern int gpio_open_drain_0(u32 bit);
extern u32 gpio_open_drain_val_get(u32 bit);
extern int gpio_first_1(u32 bit);
extern int gpio_first_0(u32 bit);
extern u32 gpio_first_val_get(u32 bit);
extern int gpio_master_1(u32 bit);
extern int gpio_master_0(u32 bit);
extern u32 gpio_master_val_get(u32 bit);
extern int gpio_set_oe(u32 bit);
extern int gpio_clr_oe(u32 bit);
extern u32 gpio_oe_val_get(u32 bit);
extern int gpio_out_1(u32 bit);
extern int gpio_out_0(u32 bit);
extern u32 gpio_out_val_get(u32 bit);
extern u32 gpio_in_val(u32 bit);

extern u32 gpio_para_get(u32 bit);
extern int gpio_pin_mux_sel(PMXSEL_ID id, u32 sel);
extern u32 gpio_pin_mux_get(PMXSEL_ID id);

#define GPIO_DEBUG_SET(a,d) do { \
                            if(d) { \
                                gpio_debug_1(a); \
                            } else { \
                                gpio_debug_0(a); \
                            } \
                        } while(0)

#define GPIO_I_INV_SET(a,d) do { \
                            if(d) { \
                                gpio_input_invert_1(a); \
                            } else { \
                                gpio_input_invert_0(a); \
                            } \
                        } while(0)
#define GPIO_I_INV_GET(a)	gpio_input_invert_val_get(a)
                        
#define GPIO_O_INV_SET(a,d) do { \
                            if(d) { \
                                gpio_output_invert_1(a); \
                            } else { \
                                gpio_output_invert_0(a); \
                            } \
                        } while(0)
#define GPIO_O_INV_GET(a)	gpio_output_invert_val_get(a)						
                        
#define GPIO_OD_SET(a,d) do { \
                            if(d) { \
                                gpio_open_drain_1(a); \
                            } else { \
                                gpio_open_drain_0(a); \
                            } \
                        } while(0)
#define GPIO_OD_GET(a)	gpio_open_drain_val_get(a)						

#define GPIO_F_SET(a,d) do { \
                            if(d) { \
                                gpio_first_1(a); \
                            } else { \
                                gpio_first_0(a); \
                            } \
                        } while(0)
#define GPIO_F_GET(a)	gpio_first_val_get(a)

#define GPIO_M_SET(a,d) do { \
                            if(d) { \
                                gpio_master_1(a); \
                            } else { \
                                gpio_master_0(a); \
                            } \
                        } while(0)
#define GPIO_M_GET(a)	gpio_master_val_get(a)					

#define GPIO_E_SET(a,d) do { \
                            if(d) { \
                                gpio_set_oe(a); \
                            } else { \
                                gpio_clr_oe(a); \
                            } \
                        } while(0)
#define GPIO_E_GET(a)	gpio_oe_val_get(a)

#define GPIO_O_SET(a,d) do { \
                            if(d) { \
                                gpio_out_1(a); \
                            } else { \
                                gpio_out_0(a); \
                            } \
                        } while(0)
#define GPIO_O_GET(a)	gpio_out_val_get(a)

#define GPIO_I_GET(a)   gpio_in_val(a)

#define	GPIO_PARA_GET(a) gpio_para_get(a)

#define GPIO_PIN_MUX_SEL(a,d) gpio_pin_mux_sel(a,d)
#define GPIO_PIN_MUX_GET(a) gpio_pin_mux_get(a)

#endif /* _SP_GPIO_DRV_H_ */

