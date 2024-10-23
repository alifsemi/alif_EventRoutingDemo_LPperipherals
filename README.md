# Event Routing Demo: LP Peripherals Only
Alif Ensemble processors are equipped with high-performance and high-efficiency
regions which can be clock gated or powered off. Industry leading battery life
can be achieved by employing power saving features such as dynamically switching
between high or low frequencies, clock gating as necessary, powering off unused
regions, and sleeping for extended periods.

In this Event Routing Demo, we demonstrate how to continuously capture sensor
data from an external SPI ADC at low power. We maximize CPU sleep time by using
the DMA peripheral and hardware event routing to drive SPI xfers without CPU
interaction. We also demonstrate a power saving feature in which PD-6 SYST is
powered off. All peripherals used in this Event Routing Demo reside in power
domains PD-3 and lower. This allows for power domains PD-4 and higher to be off.
As a result, the ***RTSS-HE can be viewed as a self-contained MCU within an MCU.***

Many of the Ensemble processors' shared resources and system interconnect reside
in a power domain called PD-6 SYST. The default device configuration leaves SYST
in the always-on state. This allows applications to configure and utilize shared
resources such as SRAM0/1 and system peripherals without special considerations.
As long as PD-6 SYST remains powered on then the contents of SRAM0/1 will be
retained and the configurations on system peripherals will not be lost.
Applications which are using the system peripherals will want to use SYST in the
always-on state.

Since this demo does not require the retention of SRAM0/1 memory and does not
require retention of system peripherals (because it is using RTSS-HE resources
like TCM, DMA2, and LP- peripherals) then PD-6 SYST can be configured to use the
dynamic off state. Any transactions which cross between subsystems will cause
PD-6 SYST to power on and service that transaction, but it will power off as
soon as the transaction is completed. Significant savings in dynamic power and
leakage power can be achieved by using SYST in the dynamic-off state.

***Note: This demo is supported on Gen 2 Ensemble devices only. The demo may be run
on the RTSS-HE only.***

## Project Setup
### Hardware Setup
1. Alif Ensemble DevKit (Gen 2)
   - attach a jumper wire connected between P15_0 and P15_1
   - attach a jumper wire connected between P9_2 and P15_2
   - attach a jumper wire connected between P11_4 and P11_5
2. Logic Analyzer to monitor the below MCU signals
   - P15_0  LPTIMER0    (loopback to P15_1 LPGPIO)
   - P9_2   LPUART_TX   (loopback to P15_2 LPGPIO)
   - P11_4  LPSPI_DATA  (loopback to P11_5)
   - P11_6  LPSPI_SCLK
   - P11_7  LPSPI_SELECT
   - P0_3   SYST_ACLK clock debug (optional)
   - P1_3   RTSS_HE_CLK clock debug (optional)
3. Power Analyzer connected to DevKit J5 (for current measurement on MCU_3V3)
4. SEGGER JLink Debug Probe (optional)

### Software Setup
The following software must be installed on your system (tested with Ubuntu 22.04 LTS)
1. Check the [Getting Started with VSCode CMSIS pack project](https://github.com/alifsemi/alif_vscode-template/blob/main/doc/getting_started.md) to set all the needed tools and how build the project. Especially for flashing and debugging this must be done https://github.com/alifsemi/alif_vscode-template/blob/main/doc/getting_started.md#visual-studio-code-settings
2. Modify the Alif Ensemble CMSIS DFP v1.3.0 with the files provided in this project
Extract the archive in the patch directory and overwrite the files at (in Linux) {user home}/.cache/arm/packs/AlifSemiconductor/Ensemble/1.3.0/

After setting up the development environment
1. Clone this repository and run 'git submodule update --init' in the folder
2. Open the folder as a project in VSCode.
3. Press F1 -> "Run Tasks" -> "First time pack installation" if not done already.
4. [Building](https://github.com/alifsemi/alif_vscode-template/blob/main/doc/getting_started.md#building-the-project-with-cmsis-solution)
5. Press F1 -> "Run Tasks" -> "Install Debug Stubs with Security Toolkit"
6. Press F1 -> "Run Tasks" -> "Program with Security Toolkit"
7. Press 5 to start debugging, **OR** *skip to the next step*

Note: To clean the project, press F1 -> "Run Tasks" -> "Clean all (including YML build files)".
Changes done to RTE_Config.h compared to original file in 1.3.0:
   * RTE_LPSPI_DMA_ENABLE set to 1
   * RTE_LPSPI_SELECT_DMA0 set to 0
   * RTE_LPUART_DMA_ENABLE set to 1
   * RTE_LPUART_SELECT_DMA0 set to 0
In [RTE_Config.h](app/RTE/Device/AE722F80F55D5LS_M55_HE/RTE_Device.h), if the DMA_ENABLE is set to 0 or if the SELECT_DMA0 is set to 1 then the demo will not work properly.

**Troubleshooting**
* [ERROR] Target did not respond
   1. Place the MCU in hard maintenance mode before programming
   2. In a terminal, navigate to the app-release-exec directory
   3. Run ./maintenance and then enter the below options
      * option 1 - Device Control
      * option 1 - Hard maintenance mode
   4. Press the RESET button on the Ensemble DevKit
   5. Hit enter until you exit the maintenance tool to release the serial port
   6. Re-try the "Program with Security Toolkit" step
* SPI transactions only occur once, and are not continuous
   1. DMA might not be enabled for this project
   2. Use git to restore the file at RTE/Device/AExxx/RTE_Device.h
