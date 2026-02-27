# MPU6500 IMU SPI DMA Driver on STM32 NUCLEO-F446RE

**Note:** This project is written in bare-metal STM32 register-level C.
It implements a fast SPI1 + DMA2 driver for the MPU6500 using the IMU Data Ready (INT) pin to trigger sampling.

## This project exists to implement and demonstrate:

- Bare-metal **SPI1** configuration (register-level)
- Full-duplex SPI burst reads using **DMA** (TX+RX)
- Data Ready interrupt → start DMA transfer (event-driven sampling)
- DMA Transfer Complete ISR → stop DMA, release CS, parse data
- IMU initialization
- IMU calibration (blocking read used during startup only)

---
This project reads the **MPU6500** IMU over **SPI** using **DMA** and produces:
- Accelerometer data (X/Y/Z) in **g**
- Temperature in **°C**
- Gyroscope data (X/Y/Z) in **deg/s**
---

## Overview

The MPU6500 provides a continuous block of sensor output registers starting at **0x3B**:

- ACCEL_XOUT_H .. ACCEL_ZOUT_L (6 bytes)
- TEMP_OUT_H .. TEMP_OUT_L (2 bytes)
- GYRO_XOUT_H .. GYRO_ZOUT_L (6 bytes)

In the project **14 bytes** are read in a single burst read starting at **0x3B**.

## Sampling Rate (1 kHz) and Filtering

The MPU6500 default internal rate can be higher (8 kHz depending on configuration).
This project configures the IMU so that the Data Ready interrupt occurs at ~1 kHz by configuring the internal digital low-pass filter via the CONFIG register.

## High-Level Flow

- MPU6500 INT pin asserts when new sensor data is ready (Data Ready interrupt).
- STM32 EXTI interrupt fires (PC13 in this project).
- In EXTI15_10_IRQHandler():
    - configure DMA NDTR + buffer addresses
    - start DMA streams (TX and RX)
- SPI clocks out:
    - the read register address (to configure slave internal pointer)
    - dummy bytes to clock in the remaining sensor bytes (TX)
- DMA RX Stream Transfer Complete interrupt fires.
- In DMA2_Stream2_IRQHandler():
    - clear DMA flags
    - disable DMA streams
    - bring CS high
    - parse raw data → apply scaling and bias → update MPU6500_Data_t

---

## Hardware Connections

| Signal | MPU6500 | STM32 NUCLEO-F446RE |
|-------:|---------|---------------------|
| VCC    | VCC     | 3.3V                |
| GND    | GND     | GND                 |
| SCLK   | SCL/SCLK| PA5 (SPI1_SCK)      |
| MOSI   | SDA/SDI | PA7 (SPI1_MOSI)     |
| MISO   | AD0/SDO | PA6 (SPI1_MISO)     |
| SS     | NCS     | PA9 (GPIO Output)   |
| INT    | INT     | PC13 (GPIO Input)   |

---

## File Structure

### `main.c`

- System clock configuration
- GPIO initialization
- SPI in DMA mode initialization
- MPU6500 initialization
- IMU calibration
- EXTI configuration for Data Ready
- Interrupts used:
    -EXTI15_10_IRQHandler() → Data Ready triggers a DMA read
    -DMA2_Stream2_IRQHandler() → RX DMA transfer complete

---

### `mpu6500.h / mpu6500.c (Core->Inc/Src)`
Initializes MPU6500 IMU sensor, reads it, and writes to it using **SPI** in **DMA** mode.

Functions:
- `mpu6500_init();`
  
  Initializes the sensor by waking it up and configuring the measurement range for gyroscope and accelerometer. Configures LP filter and 1kHz frequency and enables Data Ready Interrupt pin.
  
- `mpu6500_read(...);`

  Starts a full-duplex SPI DMA burst read (TX dummy bytes + RX buffer fill).

- `mpu6500_write(...);`

  Starts a full-duplex SPI DMA burst write (RX dummy bytes + TX buffer fill).

- `mpu6500_read_blocking() / mpu6500_write_blocking();`

   SPI blocking read and write, used only for IMU initialization and calibration. In real-time only DMA SPI is used for data collection.

- `mpu6500_calibrate_imu(...);`

  Reading gyroscope and acclerometer info for given number of samples and finds average. The sensor is still while this is in progress. This value is used in processing in mpu6500_process, bias is subtracted.

- `dma_callback(...);`

  Handles disabling SPI slave and DMA transfer on DMA completed interrupt. Calls processing function.


---

### `spi.h / spi.c (Core->Inc/Src)`
Configures **SPI** peripheral in **DMA** mode.

- `dma2_stream_2_3_init();`

  Initializing DMA2 Stream 2 Channel 3 for SPI_RX. Initializing DMA2 Stream 3 Channel 3 for SPI_TX.

- `dma2_enable()/dma2_disable;`

  Enable and disable DMA helper functions.

  - `set_dma_transfer_length()/set_dma_source;`
 
  Sets DMA transfer length and DMA memory source for both transmit and receive.

  - `spi_gpio_init();`
 
  Initializes GPIO pins for SPI protocol.

  - `spi1_config();`
 
  Initializes SPI1 peripheral.

  - `cs_enable()/cs_disable();`
 
  Pulling CS line low to select the slave, pulls high to disable the slave.

  - `dma2_clear_spi1_flags;`
 
  Clear all pending interrupt flags (TC, HT, TE, DME, FE) for DMA2 Stream2 and Stream3.

---

### `exti.h / exti.c (Core->Inc/Src)`

 - `pc13_exti_init;`
 
  Initializes PC13 as input. Enabling rising edge interrupt. Using it for Data Ready Interrupt.
 
---


## Reference Materials
All register settings, timer configurations, and GPIO modes in this code are implemented based on:

- **Reference Manual:** RM0390 Rev 7  
- **Datasheet:** DS10693 Rev 10  
- **User Manual:** UM1724 Rev 17
- **Cortex M4 Generic User Guide:** DUI 0553A

  
* **Sensor Documentation:** MPU-6500 Register Map and Descriptions Revision 2.1
---

## How to import the project

Download STM32CubeF4 MCU package from ST website.
https://www.st.com/en/embedded-software/stm32cubef4.html
Unpack and place the files inside STM Cube workspace.

In Cube click File>Import>General>Existing Projects into Workspace>Next>Browse
Now find the location where you saved the downloaded project. Select that folder.
In Projects field there will be a projected automatically located by Cube, if it is not checked, check it. Click Finish.

In Cube locate the project in workspace and right click on it>Properties.
On the left side expand C/C++ General and locate Paths and Symbols. In Includes there is a field Include directories. There are 2 directories included. Edit paths of both, connect the path to the downloaded package. Make sure to replicate included directories, just proper file location.
