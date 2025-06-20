#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "as7341.h"

// Function to get the I2C device
const struct device *get_i2c_device(void)
{
    const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

    if (!device_is_ready(i2c_dev)) {
        printk("I2C device is not ready\n");
        return NULL;
    }

    return i2c_dev;
}

// Initialize the module
int DEV_ModuleInit(void)
{
    const struct device *i2c_dev = get_i2c_device();
    if (!i2c_dev) {
        printk("Failed to get I2C device\n");
        return -1;
    }

    printk("Module initialization complete\n");
    return 0; // Return success
}


//AS7341_Init
int AS7341_Init(const struct device *i2c_dev, eMode_t mode)
{
    
    AS7341_Enable(i2c_dev, true);
    uint8_t measureMode = mode;
    printk("AS7341 initialized successfully with mode %d \n", mode);
    return 0;
}


void AS7341_Enable(const struct device *i2c_dev, bool flag)
{
    
    uint8_t data=0, data1=0;
    //i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    if (!device_is_ready(i2c_dev)) {
        printk("I2C device not ready \n");
        return;
    }

    // Read the AS7341_ENABLE register
    if(i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_ENABLE, &data)<0)
    {
        printk("Failed to read ENABLE register \n");
        return;
    }

    // Set or clear bit 0 to enable or disable the AS7341
    if (flag==true) {
        data |= (1 << 0);
    } else {
        data &= ~(1<<0);
    }

    // Write the updated value back to the AS7341_ENABLE register
    if(i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_ENABLE, data)<0){
        printk("Failed to write ENABLE register\n");
        return;
    }

    k_msleep(500);
    if(i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_ENABLE, &data1)<0){
        printk("failed to verify ENABLE register\n");
        return;
    }
    


    if (data1 == data) 
    {
        printk("Initialization is complete!\n");
        return;
    } 
    else 
    {   
        printk("Initialization failed! Check I2C connections.\n");
        return;
        
    }

    

    //Write 0x30 to register 0x00 as per the original logic
    if(i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, 0x00, 0x30)<0){
        printk("failed to write to register\n");
        return;
    }
    printk("as7341 enable successfully\n");
    
}

//  query the i2c device for its response
int DEV_I2C_Query(const struct device *i2c_dev)
{
    uint8_t dummy = 0;
    int ret = i2c_write(i2c_dev, &dummy, 0, AS7341_ADDRESS);

    if (ret == 0) {
        printk("I2C device at address 0x%02X is responding \n", AS7341_ADDRESS);
        return 0; // No error
    } else {
        printk("I2C device at address 0x%02X not responding (error: %d)\n", AS7341_ADDRESS, ret);
        return -1; // Error occurred
    }
}



// AS7341_SetBank
void AS7341_SetBank(const struct device *i2c_dev, uint8_t addr)
{
    uint8_t data = 0;

    i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_CFG_0, &data);

    if (addr == 1) {
        data |= (1 << 4);  // Set bit 4 to select bank 1
    } else {
        data &= ~(1 << 4); // Clear bit 4 to select bank 0
    }

    i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_CFG_0, data);
}
//AS7341_EnableLED
void AS7341_EnableLED(const struct device *i2c_dev, bool flag)
{
    uint8_t data = 0;
    uint8_t data1 = 0;

    AS7341_SetBank(i2c_dev, 1);  // Switch to register bank 1

    i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_CONFIG, &data);
    i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_LED, &data1);

    if (flag) {
        data |= 0x08;  // Set bit to enable LED
    } else {
        data &= 0xF7;  // Clear bit to disable LED
        data1 &= 0x7F; // Clear bit 7 in LED register
        i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_LED, data1);
    }

    i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_CONFIG, data);

    AS7341_SetBank(i2c_dev, 0);  // Switch back to default bank
}

//AS7341_ATIME_config
void AS7341_ATIME_config(const struct device *i2c_dev, uint8_t value)
{
    // Write the value to the ATIME register
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_ATIME, value) < 0) {
        printk("Failed to write ATIME register \n");
    } else {
        printk("ATIME configured with value: 0x%02X \n", value);
    }
}

//AS7341_ASTEP_config
void AS7341_ASTEP_config(const struct device *i2c_dev, uint16_t value)
{
    uint8_t lowValue = value & 0x00FF;
    uint8_t highValue = (value >> 8) & 0x00FF;

    // Write the low byte
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_ASTEP_L, lowValue) < 0) {
        printk("Failed to write low byte of ASTEP register \n");
        return;
    }

    // Write the high byte
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_ASTEP_H, highValue) < 0) {
        printk("Failed to write high byte of ASTEP register \n");
        return;
    }

    printk("ASTEP configured with value: 0x%04X \n", value);
}
// AS7341_AGAIN_config
void AS7341_AGAIN_config(const struct device *i2c_dev, uint8_t value)
{
    // Clamp the gain value to the maximum allowable (10)
    if (value > 10) {
        value = 10;
    }

    // Write the value to the CFG_1 register
    int ret = i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_CFG_1, value);
    if (ret < 0) {
        printk("Failed to write gain value to CFG_1 register (error %d)\n", ret);
    } else {
        printk("Gain configured successfully with value: %d \n", value);
    }
}

// AS7341_Conrtolled
void AS7341_ControlLed(const struct device *i2c_dev, bool LED, uint8_t current)
{
    uint8_t data = 0;

    // Clamp the current intensity between 1 and 19
    if (current < 1) current = 1;
    if (current > 19) current = 19;
    current--;

    AS7341_SetBank(i2c_dev, 1);  // Switch to bank 1 for LED control

    if (LED) {
        data = 0x80 | current;  // Set bit 7 to enable LED with specified brightness
    } else {
        data = current;         // Set brightness only, with LED disabled
    }

    i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_LED, data);

    k_msleep(100);  // Delay for 100 ms to apply settings

    AS7341_SetBank(i2c_dev, 0);  // Switch back to default bank
    
}

// Function to configure SMUX for F1-F4, CLEAR, NIR
void F1F4_Clear_NIR(const struct device *i2c_dev)
{
    uint8_t register_values[][2] = {
        {0x00, 0x30}, {0x01, 0x01}, {0x02, 0x00}, {0x03, 0x00},
        {0x04, 0x00}, {0x05, 0x42}, {0x06, 0x00}, {0x07, 0x00},
        {0x08, 0x50}, {0x09, 0x00}, {0x0A, 0x00}, {0x0B, 0x00},
        {0x0C, 0x20}, {0x0D, 0x04}, {0x0E, 0x00}, {0x0F, 0x30},
        {0x10, 0x01}, {0x11, 0x50}, {0x12, 0x00}, {0x13, 0x06}
    };

    for (size_t i = 0; i < sizeof(register_values) / sizeof(register_values[0]); i++) {
        if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, register_values[i][0], register_values[i][1]) < 0) {
            printk("Failed to write to register 0x%02X \n", register_values[i][0]);
        }
    }
}
//Function to configure SMUX for F5-F8, CLEAR, NIR
void F5F8_Clear_NIR(const struct device *i2c_dev)
{
    // Define register-value pairs
    uint8_t register_values[][2] = {
        {0x00, 0x00}, {0x01, 0x00}, {0x02, 0x00}, {0x03, 0x40},
        {0x04, 0x02}, {0x05, 0x00}, {0x06, 0x10}, {0x07, 0x03},
        {0x08, 0x50}, {0x09, 0x10}, {0x0A, 0x03}, {0x0B, 0x00},
        {0x0C, 0x00}, {0x0D, 0x00}, {0x0E, 0x24}, {0x0F, 0x00},
        {0x10, 0x00}, {0x11, 0x50}, {0x12, 0x00}, {0x13, 0x06}
    };

    // Write each register-value pair
    for (size_t i = 0; i < sizeof(register_values) / sizeof(register_values[0]); i++) {
        if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, register_values[i][0], register_values[i][1]) < 0) {
            printk("Failed to write to register 0x%02X \n", register_values[i][0]);
        } else {
            printk("Register 0x%02X set to 0x%02X \n", register_values[i][0], register_values[i][1]);
        }
    }

    printk("F5F8_Clear_NIR configuration completed \n");
}




// Function to enable  spectral measurement
void AS7341_EnableSpectralMeasure(const struct device *i2c_dev, bool flag)
{
    uint8_t data;
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_ENABLE, &data) < 0) {
        printk("Failed to read ENABLE register \n");
        return;
    }

    if (flag) {
        data |= (1 << 1);
    } else {
        data &= ~(1 << 1);
    }

    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_ENABLE, data) < 0) {
        printk("Failed to write ENABLE register \n");
    }
}

//AS7341_EnableSMUX
void AS7341_EnableSMUX(const struct device *i2c_dev, bool flag)
{
    uint8_t data;

    // Read the current value of the ENABLE register
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_ENABLE, &data) < 0) {
        printk("Failed to read ENABLE register \n");
        return;
    }

    // Modify the SMUX bit (bit 4)
    if (flag) {
        data |= (1 << 4);  // Set bit 4 to enable SMUX
    } else {
        data &= ~(1 << 4); // Clear bit 4 to disable SMUX
    }

    // Write the modified value back to the ENABLE register
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_ENABLE, data) < 0) {
        printk("Failed to write ENABLE register \n");
        return;
    }

    printk("SMUX %s successfully \n", flag ? "enabled" : "disabled");
}

// AS7341_Config
void AS7341_Config(const struct device *i2c_dev, eMode_t mode)
{
    uint8_t data;

    // Set the bank to 1 to access the CONFIG register
    AS7341_SetBank(i2c_dev, 1);

    // Read the current value of the CONFIG register
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_CONFIG, &data) < 0) {
        printk("Failed to read CONFIG register \n");
        AS7341_SetBank(i2c_dev, 0); // Ensure we return to bank 0
        return;
    }

    // Update the CONFIG register based on the selected mode
    switch (mode) {
        case eSpm:
            data = (data & ~0x03) | eSpm;
            break;
        case eSyns:
            data = (data & ~0x03) | eSyns;
            break;
        case eSynd:
            data = (data & ~0x03) | eSynd;
            break;
        default:
            printk("Invalid mode selected \n");
            AS7341_SetBank(i2c_dev, 0); // Ensure we return to bank 0
            return;
    }

    // Write the updated value back to the CONFIG register
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_CONFIG, data) < 0) {
        printk("Failed to write CONFIG register \n");
        AS7341_SetBank(i2c_dev, 0); // Ensure we return to bank 0
        return;
    }

    // Return to bank 0
    AS7341_SetBank(i2c_dev, 0);

    printk("AS7341 configured successfully for mode %d \n", mode);
}

// AS7341_SetGpioMode(
void AS7341_SetGpioMode(const struct device *i2c_dev, uint8_t mode)
{
    uint8_t data;

    // Read the current value of the GPIO_2 register
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_GPIO_2, &data) < 0) {
        printk("Failed to read GPIO_2 register \n");
        return;
    }

    // Modify the GPIO mode based on the input mode
    if (mode == INPUT) {
        data |= (1 << 2); // Set bit 2 for input mode
    } else if (mode == OUTPUT) {
        data &= ~(1 << 2); // Clear bit 2 for output mode
    } else {
        printk("Invalid mode specified: %d \n", mode);
        return;
    }

    // Write the updated value back to the GPIO_2 register
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_GPIO_2, data) < 0) {
        printk("Failed to write GPIO_2 register \n");
        return;
    }

    printk("GPIO mode set to %s \n", (mode == INPUT) ? "INPUT" : "OUTPUT");
}

//AS7341_MeasureComplet

bool AS7341_MeasureComplete(const struct device *i2c_dev)
{
    uint8_t status;

    // Read the STATUS_2 register
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_STATUS_2, &status) < 0) {
        printk("Failed to read STATUS_2 register \n");
        return false; // Assume measurement is not complete on failure
    }

    // Check if bit 6 is set
    return (status & (1 << 6)) != 0;
}



//AS7341_startMeasure
void AS7341_startMeasure(const struct device *i2c_dev, eChChoose_t mode, eMode_t measureMode)
{
    uint8_t data;

    // Read CFG_0 register
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_CFG_0, &data) < 0) {
        printk("Failed to read CFG_0 register \n");
        return;
    }

    // Clear bit 4 of CFG_0
    data &= ~(1 << 4);
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_CFG_0, data) < 0) {
        printk("Failed to write CFG_0 register \n");
        return;
    }

    // Disable spectral measurement
    AS7341_EnableSpectralMeasure(i2c_dev, false);

    // Configure SMUX command
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_SMUX_CMD, 0x10) < 0) {
        printk("Failed to configure SMUX command \n");
        return;
    }

    // Configure channels based on the selected mode
    if (mode == eF1F4ClearNIR) {
        F1F4_Clear_NIR(i2c_dev);
    } else if (mode == eF5F8ClearNIR) {
        F5F8_Clear_NIR(i2c_dev);
    }

    // Enable SMUX
    AS7341_EnableSMUX(i2c_dev, true);

    // Configure the measurement mode
    if (measureMode == eSyns) {
        AS7341_SetGpioMode(i2c_dev,OUTPUT);
        AS7341_Config(i2c_dev, eSyns);
    } else if (measureMode == eSpm) {
        AS7341_Config(i2c_dev, eSpm);
    }

    // Enable spectral measurement
    AS7341_EnableSpectralMeasure(i2c_dev, true);

    // Wait for measurement to complete if in SPM mode
    if (measureMode == eSpm) {
        while (!AS7341_MeasureComplete(i2c_dev)) {
            k_msleep(1); // Use Zephyr's delay API
        }
    }

    printk("Measurement started successfully in mode %d \n", measureMode);
}
// Function to read data from a specific channel
int AS7341_GetChannelData(const struct device *i2c_dev, uint8_t channel, uint16_t *data)
{
    uint8_t data_low, data_high;

    // Read the low byte of the channel data
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_CH0_DATA_L + channel * 2, &data_low) < 0) {
        printk("Failed to read low byte of channel %d \n", channel);
        return -1;
    }

    // Read the high byte of the channel data
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_CH0_DATA_H + channel * 2, &data_high) < 0) {
        printk("Failed to read high byte of channel %d \n", channel);
        return -1;
    }

    // Combine high and low bytes into a 16-bit value
    *data = (data_high << 8) | data_low;
    k_msleep(50); // Optional delay for stability
    return 0;
}


// Function to read spectral data using SMUX from low channel
sModeOneData_t AS7341_ReadSpectralDataOne(const struct device *i2c_dev)
{
    sModeOneData_t data = {0};

    AS7341_GetChannelData(i2c_dev, 0, &data.channel1);
    AS7341_GetChannelData(i2c_dev, 1, &data.channel2);
    AS7341_GetChannelData(i2c_dev, 2, &data.channel3);
    AS7341_GetChannelData(i2c_dev, 3, &data.channel4);
    AS7341_GetChannelData(i2c_dev, 4, &data.CLEAR);
    AS7341_GetChannelData(i2c_dev, 5, &data.NIR);

    return data;
}
// Function to read spectral data using SMUX from high channel


sModeTwoData_t AS7341_ReadSpectralDataTwo(const struct device *i2c_dev)
{
    sModeTwoData_t data2 = {0};

    AS7341_GetChannelData(i2c_dev, 0, &data2.channel5);
    AS7341_GetChannelData(i2c_dev, 1, &data2.channel6);
    AS7341_GetChannelData(i2c_dev, 2, &data2.channel7);
    AS7341_GetChannelData(i2c_dev, 3, &data2.channel8);
    AS7341_GetChannelData(i2c_dev, 4, &data2.CLEAR);
    AS7341_GetChannelData(i2c_dev, 5, &data2.NIR);

    return data2;
}
// read flicker data

uint8_t AS7341_ReadFlickerData(const struct device *i2c_dev)
{

	uint8_t flicker;
	uint8_t data=0;
	// Read CFG_0 register
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_CFG_0, &data) < 0) {
        printk("Failed to read CFG_0 register \n");
        return;
    }
	// Clear bit 4 of CFG_0
    data = data& (~(1 << 4));
    if (i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, AS7341_CFG_0, data) < 0) {
        printk("Failed to write CFG_0 register \n");
        return;
    }

	AS7341_EnableSpectralMeasure(i2c_dev,false);
    //write value 0x10 to 0xAF register
    if(i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, 0xAF, 0x10)<0){
        printk("failed to write to register \n");
        return;
    }
	FDConfig(i2c_dev);
	AS7341_EnableSMUX(i2c_dev,true);
	AS7341_EnableSpectralMeasure(i2c_dev,true);
	AS7341_EnableFlickerDetection(i2c_dev,true);
    if(i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, 0xDA, 0x10)<0){
        printk("failed to write to register \n");
        return;
    }
    uint8_t retry=100;
    if(retry==0){
        printk("data access error \n");
    }
    AS7341_EnableFlickerDetection(i2c_dev,true);
    k_msleep(600);
	if((flicker = i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_STATUS,&flicker))<0){
        printk("failed to read AS7341_STATUS register \n");
        return;
    }
    printk("Flicker : %d\n",flicker);
	
	AS7341_EnableFlickerDetection(i2c_dev,false);
	switch(flicker){
		case 37:
		  flicker = 100;
		  break;
		case 40:
		  flicker = 0;
		  break;
		case 42:
		  flicker = 120;
		  break;		  
		case 44:
		  flicker = 1;
		  break;		  
		case 45:
		  flicker = 2;
		  break;		  
		default:
		  flicker = 2;
	  }
	return flicker;
}


// enable flicker detection
void AS7341_EnableFlickerDetection(const struct device *i2c_dev, bool flag)
{
    uint8_t data = 0;

    // Read the AS7341_ENABLE register
    if (i2c_reg_read_byte(i2c_dev, AS7341_ADDRESS, AS7341_ENABLE, &data) < 0) {
        printk("Failed to read AS7341_ENABLE register \n");
        return; 
    }

    // Set or clear the flicker detection bit (bit 6)
    if (flag) {
        data |= (1 << 6); // Enable flicker detection
        data |= (1<<0);
    } else {
        data &= ~(1 << 6); // Disable flicker detection
        data &= ~(1 << 0);
    }

    // Write back the updated value to the AS7341_ENABLE register
    if (i2c_reg_write_byte(i2c_dev,AS7341_ADDRESS, AS7341_ENABLE, data) < 0) {
        printk("Failed to write AS7341_ENABLE register \n");
        return; // Exit if write operation fails
    }
    
    printk("Flicker detection %s successfully\n", flag ? "enabled" : "disabled");
}

//FDCongig

void FDConfig(const struct device *i2c_dev) 
{
    // Array of register-value pairs for configuration
    const uint8_t register_values[][2] = {
        {0x00, 0x00}, {0x01, 0x00}, {0x02, 0x00}, {0x03, 0x00},
        {0x04, 0x00}, {0x05, 0x00}, {0x06, 0x00}, {0x07, 0x00},
        {0x08, 0x00}, {0x09, 0x00}, {0x0A, 0x00}, {0x0B, 0x00},
        {0x0C, 0x00}, {0x0D, 0x00}, {0x0E, 0x00}, {0x0F, 0x00},
        {0x10, 0x00}, {0x11, 0x00}, {0x12, 0x00}, {0x13, 0x60}
    };

    // Loop through the register-value pairs
    for (size_t i = 0; i < sizeof(register_values) / sizeof(register_values[0]); i++) {
        int ret = i2c_reg_write_byte(i2c_dev, AS7341_ADDRESS, 
                                     register_values[i][0], 
                                     register_values[i][1]);
        if (ret < 0) {
            printk("Failed to write to register 0x%02X\n", register_values[i][0]);
        } else {
            printk("Register 0x%02X set to 0x%02X\n", register_values[i][0], register_values[i][1]);
        }
    }

    printk("FDConfig completed\n");
}