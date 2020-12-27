#ifndef __SENSOR_POWER_H
#define __SENSOR_POWER_H

#include "isp_api.h"

void powerSensorOn_RAM(struct mipi_isp_info *isp_info);
void powerSensorDown_RAM(struct mipi_isp_info *isp_info);

#endif /* __SENSOR_POWER_H */
