#include "adxl345.h"

#define LOG_MODULE_NAME adxl345
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

int adxl345_init(const struct device *dev_i2c)
{
    int ret;
    
    uint8_t config[2] = {POWER_CTL,0x08};
    ret = i2c_write(dev_i2c, config, sizeof(config), ADXL345_ADDR);
    if(ret != 0){
        LOG_INF("Failed to write to I2C device address %x at Reg. %x \n", ADXL345_ADDR,config[0]);
    }

    config[0] = DATA_FORMAT;
    config[1] = 0x00;
    ret = i2c_write(dev_i2c, config, sizeof(config), ADXL345_ADDR);
    if(ret != 0){
        LOG_INF("Failed to write to I2C device address %x at Reg. %x \n", ADXL345_ADDR,config[0]);
    }

    return ret;
}

int readXYZ(const struct device *dev_i2c, int16_t *x, int16_t *y, int16_t *z)
{
    int ret;
    
    uint8_t acc_reading[6]= {0};
    uint8_t sensor_regs[6] ={DATAX0,DATAX1,DATAY0,DATAY1,DATAZ0,DATAZ1};
    ret = i2c_write_read(dev_i2c,ADXL345_ADDR,&sensor_regs[0],1,&acc_reading[0],1);
    if(ret != 0){
        LOG_INF("Failed to write/read I2C device address %x at Reg. %x \n", ADXL345_ADDR,sensor_regs[0]);
    }
    ret = i2c_write_read(dev_i2c,ADXL345_ADDR,&sensor_regs[1],1,&acc_reading[1],1);
    if(ret != 0){
        LOG_INF("Failed to write/read I2C device address %x at Reg. %x \n", ADXL345_ADDR,sensor_regs[1]);
    }
    ret = i2c_write_read(dev_i2c,ADXL345_ADDR,&sensor_regs[2],1,&acc_reading[2],1);
    if(ret != 0){
        LOG_INF("Failed to write/read I2C device address %x at Reg. %x \n", ADXL345_ADDR,sensor_regs[2]);
    }
    ret = i2c_write_read(dev_i2c,ADXL345_ADDR,&sensor_regs[3],1,&acc_reading[3],1);
    if(ret != 0){
        LOG_INF("Failed to write/read I2C device address %x at Reg. %x \n", ADXL345_ADDR,sensor_regs[3]);
    }
    ret = i2c_write_read(dev_i2c,ADXL345_ADDR,&sensor_regs[4],1,&acc_reading[4],1);
    if(ret != 0){
        LOG_INF("Failed to write/read I2C device address %x at Reg. %x \n", ADXL345_ADDR,sensor_regs[4]);
    }
    ret = i2c_write_read(dev_i2c,ADXL345_ADDR,&sensor_regs[5],1,&acc_reading[5],1);
    if(ret != 0){
        LOG_INF("Failed to write/read I2C device address %x at Reg. %x \n", ADXL345_ADDR,sensor_regs[5]);
    }

    /* STEP 10 - Convert the two bytes to a 12-bits */
    *x = ((int16_t)acc_reading[1] << 8) + acc_reading[0];
    *y = ((int16_t)acc_reading[3] << 8) + acc_reading[2];
    *z = ((int16_t)acc_reading[5] << 8) + acc_reading[4];

    return ret;
}
