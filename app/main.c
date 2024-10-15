#include <RTE_Components.h>
#include CMSIS_device_header
#include <Driver_GPIO.h>
#include <dma_mapping.h>
//#include <pinconf.h>
#include <pm.h>

#include <se_services_port.h>
#define SERVICES_check_response {if ((ret != 0) || (service_response != 0)) while(1) __WFI();}

#define DISABLE_SEMIHOST
#ifdef DISABLE_SEMIHOST
#ifndef RTE_Compiler_IO_STDOUT_User
#define printf(...) (0)
#endif
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6100100)
__ASM(".global __use_no_semihosting");
void _sys_exit(int ret) { while (1); }
void _ttywrch(int ch) { return; }
#elif defined ( __GNUC__ )
#define TRAP_RET_ZERO  {__BKPT(0); return 0;}
int _close(int val) TRAP_RET_ZERO
int _lseek(int val0, int   val1, int val2) TRAP_RET_ZERO
int _read (int val0, char* val1, int val2) TRAP_RET_ZERO
int _write(int val0, char* val1, int val2) TRAP_RET_ZERO
#endif
#endif

void main (void)
{
    int32_t ret;
    uint32_t service_response;

    /* Initialize the SE services */
    se_services_port_init();

    /* Apply clock and power configuration */
    run_profile_t runp = {0};

    ret = SERVICES_get_run_cfg(se_services_s_handle, &runp, &service_response);
    SERVICES_check_response;

    /* PD6 SYST domain is NOT requested in runp.power_domains, allowing it to dynamically turn on/off
     *  SYST will turn on for transactions crossing the main bus and then turn off
     *  0x0200_0000 to 0x0827_FFFF SRAM0/1 will lose their contents when PD6 is off
     *  0x4800_0000 to 0x49FF_FFFF peripherals will lose their configuration and state
     *  0x8000_0000 to 0x8057_FFFF MRAM can be on or off depending on runp.memory_blocks
     * Note: you must keep MRAM ON anytime an M55 application is XIP from MRAM */
    runp.power_domains  = PD_SSE700_AON_MASK;   /* PD6 SYSTOP domain is NOT requested */
    //runp.power_domains |= PD_DBSS_MASK;    /* enable this line during debug */
    runp.dcdc_voltage   = 750;
    runp.dcdc_mode      = DCDC_MODE_PFM_FORCED;
    runp.aon_clk_src    = CLK_SRC_LFXO;
    runp.run_clk_src    = CLK_SRC_HFXO;
    runp.cpu_clk_freq   = CLOCK_FREQUENCY_38_4_XO_MHZ;
    runp.scaled_clk_freq = SCALED_FREQ_RC_ACTIVE_9_6_MHZ;
    runp.memory_blocks  = MRAM_MASK;
    runp.vdd_ioflex_3V3 = IOFLEX_LEVEL_1V8;

    ret = SERVICES_set_run_cfg(se_services_s_handle, &runp, &service_response);
    SERVICES_check_response;

    /* Alif Ensemble Development Kit typically uses 38.4MHz HFXO */
    set_HFXO_CLK( 38400000);        // top level HFXO clock
    set_HFRC_CLK( 38400000);        // top level HFRC clock

    uint32_t current_clk = get_HFXO_CLK();
    set_HFOSC_CLK(current_clk);     // HFOSC clock, used by some peripherals, is either HFRC/2 or HFXO depending on periph_xtal_sel[4]
    set_SOC_REFCLK(current_clk);    // SYST_REFCLK, when not using PLL, is either HFRC or HFXO depending on sys_xtal_sel[0]
    set_AXI_CLOCK(current_clk);     // SYST_ACLK is either REFCLK or SYSPLL (only SYSPLL can be divided by 1-32)
    set_AHB_CLOCK(current_clk>>1);  // SYST_HCLK is ACLK div by 1, 2, or 4
    set_APB_CLOCK(current_clk>>2);  // SYST_PCLK is ACLK div by 1, 2, or 4

    set_RTSS_HE_CLK(current_clk);   // RTSS_HE_CLK is HFRC, HFRC/2, HFXOx2, or HFXO
    set_RTSS_HP_CLK(current_clk);   // RTSS_HP_CLK is HFRC, HFRC/2, HFXOx2, or HFXO

    /* Alif Ensemble Development Kit typically uses 38.4MHz HFXO */
    //CLKCTL_PER_MST->CAMERA_PIXCLK_CTRL = 100U << 16 | 1;    // output SYST_ACLK/100 on CAM_XVCLK pin
	//pinconf_set(PORT_0, PIN_3, PINMUX_ALTERNATE_FUNCTION_6, 0);                 // P0_3: CAM_XVCLK  (mux mode 6)
    //pinconf_set(PORT_10, PIN_3, PINMUX_ALTERNATE_FUNCTION_7, 0);                // P10_3:CAM_XVCLK  (mux mode 7)
#if defined (M55_HE)
    //M55HE_CFG->HE_CAMERA_PIXCLK = 100U << 16 | 1;           // output RTSS_HE_CLK/100 on LPCAM_XVCLK pin
    //pinconf_set(PORT_0, PIN_3, PINMUX_ALTERNATE_FUNCTION_5, 0);                 // P0_3: LPCAM_XVCLK(mux mode 5)
    //pinconf_set(PORT_1, PIN_3, PINMUX_ALTERNATE_FUNCTION_5, 0);                 // P1_3: LPCAM_XVCLK(mux mode 5)
#endif

    /* this will allocate DMA2 Channel 0 for LPUART_TX */
    extern void LPUART_config();
    LPUART_config();

    /* this will allocate DMA2 Channel 1 for LPSPI_RX and DMA2 Channel 2 for LPSPI_TX */
    extern void LPSPI_config();
    LPSPI_config();

    /* LPTIMER will start the event chain. For the demo to work:
     *  P15_0 must be tied to P15_1
     *  P9_2 must be tied to P15_2
     * 
     * LPTIMER0 P15_0   ->  P15_1   LPGPIO1 -> LPUART TX event trigger
     * LPUART_TX P9_2   ->  P15_2   LPGPIO2 -> LPSPI TX event trigger
     * LPUART TX will output eight zero characters, each with a stop bit.
     * Each stop bit rising edge will cause LPSPI to transmit the next
     * item in the lpspi_tx_buffer.
     */
    extern void LPTIMER_config();
    LPTIMER_config();

    //ret = SERVICES_power_se_sleep_req(se_services_s_handle, 0, &service_response);
    //SERVICES_check_response;

    while (1) pm_core_enter_normal_sleep();
    //while (1) pm_core_enter_deep_sleep();
}
