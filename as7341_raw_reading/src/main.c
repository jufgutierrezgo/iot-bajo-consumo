#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "as7341.h"



// Main function to read channel data
int main(void)
{   
    if (DEV_ModuleInit() < 0) {
        printk("Module initialization failed\n");
        return;
    }
   
    const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    if (!device_is_ready(i2c_dev)) {
        printk("I2C device is not ready\n");
        return;
    }
    printk("I2C device is ready\n");

    eMode_t mode = eSpm;
    if (AS7341_Init(i2c_dev, mode) != 0) {
        printk("AS7341 initialization failed");
        return; 
    }

    
    // set parameter 

    AS7341_ATIME_config(i2c_dev,100);
    AS7341_ASTEP_config(i2c_dev,999);
    AS7341_AGAIN_config(i2c_dev, 6);
    AS7341_EnableLED(i2c_dev,true);
    
    AS7341_ControlLed(i2c_dev, false, 10);  
    k_msleep(100);

        // Start measurement
    sModeOneData_t data;
    sModeTwoData_t data2;


    
    while (1) {


        printk("Starting spectral measurements\n");

        AS7341_startMeasure(i2c_dev, eF1F4ClearNIR, eSpm);
        data = AS7341_ReadSpectralDataOne(i2c_dev);
                
        
        AS7341_startMeasure(i2c_dev, eF5F8ClearNIR, eSpm);
        data2=AS7341_ReadSpectralDataTwo(i2c_dev);
        
        printk("Channel 1:\t%d\n", data.channel1);
        printk("Channel 2:\t%d\n", data.channel2);
        printk("Channel 3:\t%d\n", data.channel3);
        printk("Channel 4:\t%d\n", data.channel4);
        printk("Channel 5:\t%d\n", data2.channel5);
        printk("Channel 6:\t%d\n", data2.channel6);
        printk("Channel 7:\t%d\n", data2.channel7);
        printk("Channel 8:\t%d\n", data2.channel8);
        printk("CLEAR: %d\n", data2.CLEAR);
        printk("NIR: %d\n", data2.NIR);

        printk("---------------------------------\n");

        k_msleep(3000); // Wait before next iteration
    }

    return 0; 
}