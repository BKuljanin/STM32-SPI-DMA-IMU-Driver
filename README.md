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
