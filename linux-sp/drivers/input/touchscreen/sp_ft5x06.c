#include <linux/module.h>
#include <linux/ratelimit.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/gpio/consumer.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/of_device.h>

struct sp_ft5x06_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct touchscreen_properties prop;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *irq_gpio;
	int screen_max_x;
	int screen_max_y;
	int max_support_points;
	
};

#define SP_TS_REG_WORK_MODE_SET			0x00
#define SP_TS_REG_PERIOD_SET			0x88
#define SP_TS_REG_VALID_THRESHOLD_SET	0x80
#define SP_TS_REG_INT_MODE_SET			0xA4


#define SP_TS_REG_POS_START_GET			0x03
#define SP_TS_REG_POS_END_GET			0x1E
#define SP_TS_REG_VERSION_GET			0xA1  
#define SP_TS_REG_TOUCH_NUMBER_GET		0x02

#define TS_TOUCH_POINT_MAX				5
#define TS_MAX_WIDTH					800
#define TS_MAX_HEIGHT					480

#define TOUCH_EVENT_DOWN				0x00
#define TOUCH_EVENT_UP					0x01
#define TOUCH_EVENT_ON					0x02
#define TOUCH_EVENT_RESERVED			0x03



static int sp_ft5x06_ts_readwrite(struct i2c_client *client,
				   u16 wr_len, u8 *wr_buf,
				   u16 rd_len, u8 *rd_buf)
{
	struct i2c_msg wrmsg[2];
	int i = 0;
	int ret;
	if (wr_len) {
		wrmsg[i].addr  = client->addr;
		wrmsg[i].flags = 0;
		wrmsg[i].len = wr_len;
		wrmsg[i].buf = wr_buf;
		i++;
	}
	if (rd_len) {
		wrmsg[i].addr  = client->addr;
		wrmsg[i].flags = I2C_M_RD;
		wrmsg[i].len = rd_len;
		wrmsg[i].buf = rd_buf;
		i++;
	}

	ret = i2c_transfer(client->adapter, wrmsg, i);
	if (ret < 0){
		return ret;
	}
	if (ret != i){
		return -EIO;
	}
	return 0;
}
static int sp_ft5x06_write_reg(struct i2c_client *client,u8 addr, u8 value)
{      
	u8 wrbuf[2];      
	int ret = -1;        
	wrbuf[0] = addr;      
	wrbuf[1] = value;   
	ret = sp_ft5x06_ts_readwrite(client,2,wrbuf,0,NULL);
	if (ret < 0) {
		dev_err(&client->dev, "[sp ft5x06] write reg(0x%x) failed! ret: %d", wrbuf[0], ret);
		return ret;
	}
	return 0;  
}    
static int sp_ft5x06_read_reg(struct i2c_client *client,u8 addr, u8 *pdata,int len)
{      
	int ret;      
	u8 wrbuf[1];
	
	wrbuf[0]=addr;
	ret = sp_ft5x06_ts_readwrite(client,1,wrbuf,len,pdata);
	if (ret < 0) {
		dev_err(&client->dev, "[sp ft5x06] read reg(0x%x) failed! ret: %d", addr, ret);
		return ret;
	}
	return 0;  
}


static int sp_ft5x06_i2c_test(struct i2c_client *client)
{
	char rdbuf[2];
	int ret=0;

	ret = sp_ft5x06_read_reg(client,SP_TS_REG_VERSION_GET,rdbuf,2);
	if(ret){
		return ret;
	}
	dev_info(&client->dev," ft5x06 read ts verison 0x%x \n",(rdbuf[0]<<8|rdbuf[1]));

	return 0;
}

static irqreturn_t sp_ft5x06_isr(int irq, void *dev_id)
{
	
	u8 rdbuf[30+1];
	int error=0,type=0,touch_num;
	int x,y,id,i=0;
	struct sp_ft5x06_data *tsdata = (struct sp_ft5x06_data *)dev_id;
	struct device *dev = &tsdata->client->dev;
	
	memset(rdbuf, 0, sizeof(rdbuf));
	disable_irq_nosync(tsdata->client->irq);

	error = sp_ft5x06_read_reg(tsdata->client,SP_TS_REG_TOUCH_NUMBER_GET,rdbuf,31);
	if (error) {
		dev_err(dev, "get touch number, error: %d\n",error);
		goto out;
	}
	touch_num = rdbuf[0];
	for (i = 0; i < touch_num; i++) {
		u8 *buf = &rdbuf[1+i*6];//start from reg 0x03
		bool down;

		type = buf[0] >> 6;
		/* ignore Reserved events */
		if (type == TOUCH_EVENT_RESERVED)
			continue;

		x = (((buf[0]&0x0f) << 8) | buf[1]) & 0x0fff;
		y = (((buf[2]&0x0f) << 8) | buf[3]) & 0x0fff;
		id = (buf[2] >> 4) & 0x0f;
		down = (type != TOUCH_EVENT_UP);

		input_mt_slot(tsdata->input_dev, id);
		input_mt_report_slot_state(tsdata->input_dev, MT_TOOL_FINGER, down);
		if (!down)
			continue;
		touchscreen_report_pos(tsdata->input_dev, &tsdata->prop, x, y,true);
	}
	input_mt_sync_frame(tsdata->input_dev);
	input_sync(tsdata->input_dev);
	enable_irq(tsdata->client->irq);
out:
	return IRQ_HANDLED;
}

static int sp_ft5x06_config_init(struct i2c_client *client)
{
	int ret=0,reg;
	char rdbuf[2];
	ret = sp_ft5x06_write_reg(client,SP_TS_REG_WORK_MODE_SET,0x00);//set work mode
	if(ret){
		reg = SP_TS_REG_WORK_MODE_SET;
		goto  _ERROR;
	}
	ret = sp_ft5x06_write_reg(client,SP_TS_REG_INT_MODE_SET,0x01); //set interrupt mode  1:intr,0;polling
	if(ret){
		reg = SP_TS_REG_INT_MODE_SET;
		goto  _ERROR;
	}
	
	ret = sp_ft5x06_write_reg(client,SP_TS_REG_VALID_THRESHOLD_SET,70); 
	if(ret){
		reg = SP_TS_REG_VALID_THRESHOLD_SET;
		goto  _ERROR;
	}
	
	ret = sp_ft5x06_write_reg(client,SP_TS_REG_PERIOD_SET,14); 
	if(ret){
		reg = SP_TS_REG_PERIOD_SET;
		goto  _ERROR;
	}
	ret = sp_ft5x06_read_reg(client,SP_TS_REG_PERIOD_SET,rdbuf,1);
	dev_info(&client->dev," ft5x06 read 0x88 is 0x%x \n",(rdbuf[0]));
	ret = sp_ft5x06_read_reg(client,SP_TS_REG_VALID_THRESHOLD_SET,rdbuf,1);
	dev_info(&client->dev," ft5x06 read 0x80 is 0x%x \n",(rdbuf[0]));
	ret = sp_ft5x06_read_reg(client,SP_TS_REG_INT_MODE_SET,rdbuf,1);
	dev_info(&client->dev," ft5x06 read 0xA4 is 0x%x \n",(rdbuf[0]));
	return 0;
_ERROR:
	dev_err(&client->dev, "I2C write/red reg 0x%x error.\n",reg);
	return ret;

}

static int sp_ft5x06_input_config(struct sp_ft5x06_data *tsdata)
{
	struct device *dev = &tsdata->client->dev;
	struct input_dev *input;
	int error=0;

	
	tsdata->screen_max_x = TS_MAX_WIDTH;
	tsdata->screen_max_y = TS_MAX_HEIGHT;
	tsdata->max_support_points = TS_TOUCH_POINT_MAX;
	input = devm_input_allocate_device(dev);
	if (!input) {
		dev_err(dev, "failed to allocate input device.\n");
		return -ENOMEM;
	}
	input->name = "SP_FT5X06 TouchScreen";
	input->phys = "input/ts";
	input->id.bustype = BUS_I2C;
	input->dev.parent = &tsdata->client->dev;

	input_set_capability(input, EV_ABS, ABS_MT_POSITION_X);
	input_set_capability(input, EV_ABS, ABS_MT_POSITION_Y);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);

	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, TS_TOUCH_POINT_MAX, 0, 0);
	touchscreen_parse_properties(input, true, &tsdata->prop);

	error = input_mt_init_slots(input, tsdata->max_support_points,
				INPUT_MT_DIRECT|INPUT_MT_DROP_UNUSED);
	if (error) {
		dev_err(dev, "Unable to init MT slots.\n");
		return error;
	}
	error = input_register_device(input);
	if (error) {
		dev_err(dev,"Failed to register input device: %d", error);
		return error;
	}
	
	tsdata->input_dev = input;
	return 0;

}
 
static int sp_ft5x06_probe(struct i2c_client *client,
					 const struct i2c_device_id *id)
{
	struct sp_ft5x06_data *tsdata;
	unsigned long irq_flags;
	int error;
	printk("[sp ft5x06]touch screen probe \n");
	dev_info(&client->dev,"I2C Address: 0x%02x  irq = %d  \n", client->addr,client->irq);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C check functionality failed.\n");
		return -ENXIO;
	}
	tsdata = devm_kzalloc(&client->dev, sizeof(*tsdata), GFP_KERNEL);
	if (!tsdata)
		return -ENOMEM;

	tsdata->client = client;

	tsdata->reset_gpio = devm_gpiod_get_optional(&client->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(tsdata->reset_gpio)) {
			error = PTR_ERR(tsdata->reset_gpio);
			dev_err(&client->dev,
				"Failed to request GPIO reset pin, error %d\n", error);
			return error;
		}
	if (tsdata->reset_gpio) {
		usleep_range(5000, 10000);
		gpiod_set_value_cansleep(tsdata->reset_gpio, 1);
		usleep_range(30000, 40000);
	}
	i2c_set_clientdata(client, tsdata);
	
	sp_ft5x06_i2c_test(client);
	
	sp_ft5x06_config_init(client);
	
	sp_ft5x06_input_config(tsdata);

	irq_flags = irq_get_trigger_type(client->irq);
	irq_flags |= IRQF_ONESHOT;
	error = devm_request_threaded_irq(&client->dev, client->irq,
					NULL, sp_ft5x06_isr, irq_flags,client->name, tsdata);
	if (error) {
		dev_err(&client->dev, "Unable to request touchscreen IRQ.\n");
		return error;
	}
	return 0;
}

static int sp_ft5x06_remove(struct i2c_client *client)
{
	struct sp_ft5x06_data *tsdata = i2c_get_clientdata(client);
	devm_free_irq(&tsdata->client->dev,tsdata->client->irq, tsdata);
	input_unregister_device(tsdata->input_dev);
	kfree(tsdata);
	return 0;
}

static int __maybe_unused sp_ft5x06_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	if (device_may_wakeup(dev))
		enable_irq_wake(client->irq);

	return 0;
}

static int __maybe_unused sp_ft5x06_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	if (device_may_wakeup(dev))
		disable_irq_wake(client->irq);

	return 0;
}

static SIMPLE_DEV_PM_OPS(sp_ft5x06_pm_ops,sp_ft5x06_suspend, sp_ft5x06_resume);

MODULE_DEVICE_TABLE(acpi, spft5x06_acpi_i2c_match);

#ifdef CONFIG_OF
static const struct of_device_id spft5x06_of_i2c_match[] = {
	{ .compatible = "sun1plus,sp-ft5x06", .data = NULL },
	{ },
};
MODULE_DEVICE_TABLE(of, spft5x06_of_i2c_match);
#else
#define spft5x06_of_i2c_match NULL
#endif

static const struct i2c_device_id spft5x06_i2c_id[] = {
	{"spft5x06", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, spft5x06_i2c_id);

static struct i2c_driver spft5x06_i2c_driver = {
	.driver = {
		.name	= "spft5x06",
		.of_match_table = of_match_ptr(spft5x06_of_i2c_match),
		.pm = &sp_ft5x06_pm_ops,
	},
		.probe	  = sp_ft5x06_probe,
		.remove   = sp_ft5x06_remove,

	.id_table	= spft5x06_i2c_id,
};

module_i2c_driver(spft5x06_i2c_driver);
MODULE_DESCRIPTION("SP FT5x06 I2C Touchscreen Driver");
MODULE_LICENSE("GPL");


