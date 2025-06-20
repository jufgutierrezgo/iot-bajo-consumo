#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zsl/zsl.h>
#include <zsl/matrices.h>
#include <zsl/colorimetry.h>

#include "as7341.h"
#include "cm.h"
#include "v_lambda.h"

#define COLOR_METRICS
#define SPECTRUM_ESTIMATION

#define CM_ROWS 401
#define CM_COLS 8

#define SPD_SIZE 401  // Wavelengths from 380 nm to 780 nm

#define STACK_SIZE 4096
#define PRIORITY 7

// Message queue buffer
#define QUEUE_SIZE 5  // Store up to 5 sensor readings
K_MSGQ_DEFINE(sensor_msgq, sizeof(int[8]), QUEUE_SIZE, 4);

// Semaphore to signal processing thread
K_SEM_DEFINE(sem_process, 0, 1);

// Thread stacks
K_THREAD_STACK_DEFINE(sensor_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(process_stack, STACK_SIZE);

// Thread control
struct k_thread sensor_thread;
struct k_thread process_thread;

float result_matrix[CM_ROWS] = {0};

// Declare the global XYZ struct
struct zsl_clr_xyz xyz; 
struct zsl_clr_xyy xyy;
struct zsl_clr_uv60 uv;
struct zsl_clr_cct cct;
enum zsl_clr_uv_cct_method method = ZSL_CLR_UV_CCT_METHOD_MCCAMY;

// define illuminance

typedef struct {
    float wavelength;
    float value;
} PhotopicData;

// Thread to read sensor data
void thread_read_sensor(void *p1, void *p2, void *p3) {
    if (DEV_ModuleInit() < 0) {
        printk("Module initialization failed\n");
        return;
    }

    const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    if (!device_is_ready(i2c_dev)) {
        printk("I2C device is not ready\n");
        return;
    }

    eMode_t mode = eSpm;
    if (AS7341_Init(i2c_dev, mode) != 0) {
        printk("AS7341 initialization failed\n");
        return;
    }

    AS7341_ATIME_config(i2c_dev, 100);
    AS7341_ASTEP_config(i2c_dev, 999);
    AS7341_AGAIN_config(i2c_dev, 6);
    AS7341_EnableLED(i2c_dev, true);
    k_msleep(100);

    sModeOneData_t data;
    sModeTwoData_t data2;

       

    while (1) {

        AS7341_startMeasure(i2c_dev, eF1F4ClearNIR, eSpm);
        data = AS7341_ReadSpectralDataOne(i2c_dev);
                
        
        AS7341_startMeasure(i2c_dev, eF5F8ClearNIR, eSpm);
        data2=AS7341_ReadSpectralDataTwo(i2c_dev);
        
        printk("[Thread Read] Reading sensor data...\n");

        int sensor_data[8];
        sensor_data[0] = data.channel1;
        sensor_data[1] = data.channel2;
        sensor_data[2] = data.channel3;
        sensor_data[3] = data.channel4;
        sensor_data[4] = data2.channel5;
        sensor_data[5] = data2.channel6;
        sensor_data[6] = data2.channel7;
        sensor_data[7] = data2.channel8;

        printk("[Thread Read] Storing data in queue...\n");

        // Send sensor data to message queue (blocking if full)
        k_msgq_put(&sensor_msgq, &sensor_data, K_FOREVER);

        // Print sensor vector for debugging
        for (int i = 0; i < 8; i++) {
            printk("sensor_data[%d] = %d\n", i, sensor_data[i]);
        }

        // Signal processing thread that data is available
        k_sem_give(&sem_process);

        k_msleep(2000);
    }
}

// Function to multiply matrices
void matrix_multiply(int *sensor_data) {
    printk("[Matrix Multiply] Starting multiplication...\n");

    for (int i = 0; i < CM_ROWS; i++) {
        float sum = 0.0f;
        for (int j = 0; j < CM_COLS; j++) {
            sum += cm_data[i][j] * sensor_data[j];
        }
        result_matrix[i] = sum;
    }

    printk("[Matrix Multiply] Multiplication complete.\n");

    // for (int i = 0; i < 401; i++) {
    //     printk("result_matrix[%d] = %f\n", i, result_matrix[i]);
    // }
}

//calculate illumiance
float calculate_illuminance(void) {
    float integral = 0.0;

    for (int i = 0; i < SPD_SIZE - 1; i++) {
        float s1 = result_matrix[i] * v_lambda[i];
        float s2 = result_matrix[i + 1] * v_lambda[i + 1];

    
        integral += (s1 + s2) * 0.5f * 1.0f;  // Δλ = 1 nm
    }

    float lux = 683* integral;
   


    return lux;
}

#ifdef COLOR_METRICS
void convert_XYZ(void) {
    
    printk("Starting SPD to XYZ Conversion...\n");
    
    // Allocate memory for SPD structure
    struct zsl_clr_spd *spd = malloc(sizeof(struct zsl_clr_spd) + SPD_SIZE * sizeof(spd->comps[0]));


    if (spd == NULL) {
        printk("Memory allocation failed for SPD components!\n");
        return;
    }

    spd->size = SPD_SIZE;

    for (int i = 0; i < SPD_SIZE; i++) {
        spd->comps[i].nm = 380 + i;
        spd->comps[i].value = result_matrix[i];
    }
    
    float lux = calculate_illuminance();
    printk("Calculated Illuminance: %.2f lux\n", lux);


    int result = zsl_clr_conv_spd_xyz(spd, ZSL_CLR_OBS_2_DEG, &xyz);
    

    if (result == 0) {

        
        printk("XYZ: X = %f, Y = %f, Z = %f\n", xyz.xyz_x, xyz.xyz_y, xyz.xyz_z);
        
        

        zsl_clr_conv_xyz_xyy(&xyz, &xyy); // convert xyz into xyY

        
        printk("xyY: x = %f, y = %f \n", xyy.xyy_x, xyy.xyy_y);
        
        
        // calculate cct
        float n = (xyy.xyy_x - 0.3320) / (0.1858 - xyy.xyy_y);
        //float cct_approx = 449.0 * pow(n, 3) + 3525.0 * pow(n, 2) + 6823.3 * n + 5520.33;
        //printk("CCT (McCamy Approximation): %f K\n", cct_approx);
        zsl_clr_conv_xyy_uv60(&xyy, &uv);
        
        printk(" u = %f, v = %f \n",uv.uv60_u,uv.uv60_v);
        
        zsl_clr_conv_uv60_cct(method, &uv, &cct);

        
        printk("CCT: %f K \n", cct.cct);
                
        
    
    } else {
        #ifndef low_power
        printk("XYZ to xyY conversion failed!\n");
        #endif

    }

    free(spd);
}
#endif

#ifdef SPECTRUM_ESTIMATION
// Thread to process sensor data
void thread_data_analyse(void *p1, void *p2, void *p3) {
    int sensor_data[8];

    while (1) {
        // Wait for new data
        k_sem_take(&sem_process, K_FOREVER);

        // Retrieve sensor data from queue
        if (k_msgq_get(&sensor_msgq, &sensor_data, K_NO_WAIT) == 0) {
            printk("[Thread Analyse] Processing sensor data...\n");

            matrix_multiply(sensor_data);

            #ifdef COLOR_METRICS
            convert_XYZ();
            #endif

            static int iteration = 0;
            iteration++;
            printk("Number of iterations = %d\n", iteration);

            k_msleep(1000);
        }
    }


}
#endif

// Main function
int main(void) {
    printk("Starting Zephyr application...\n");

    k_thread_create(&sensor_thread, sensor_stack, STACK_SIZE,
                    thread_read_sensor, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&process_thread, process_stack, STACK_SIZE,
                    thread_data_analyse, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    printk("All threads started successfully!\n");
    k_sleep(K_FOREVER);
}
