#ifndef __ADS1220_H_
#define __ADS1220_H_

#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * REGISTER ADDRESSES
 * ======================================================================== */
 
#define BASE_ADDRESS 0X00

#define CFGR0_OFFSET 0X00
#define CFGR1_OFFSET 0X01
#define CFGR2_OFFSET 0X02
#define CFGR3_OFFSET 0X03

#define CFGR0_ADDRESS (BASE_ADDRESS + CFGR0_OFFSET)
#define CFGR1_ADDRESS (BASE_ADDRESS + CFGR1_OFFSET)
#define CFGR2_ADDRESS (BASE_ADDRESS + CFGR2_OFFSET)
#define CFGR3_ADDRESS (BASE_ADDRESS + CFGR3_OFFSET)

/* ========================================================================
 * COMMAND DEFINITIONS
 * ======================================================================== */

#define ADS1220_CMD_RESET       0x06    // Reset device to default
#define ADS1220_CMD_START       0x08    // Start or restart conversions
#define ADS1220_CMD_POWERDOWN   0x02    // Enter power-down mode
#define ADS1220_CMD_RDATA       0x10    // Read data by command
#define ADS1220_CMD_RREG        0x20    // Read register   | Read nn registers starting at address rr  -  0010 rrnn
#define ADS1220_CMD_WREG        0x40    // write register  | Write nn registers starting at address rr -  0100 rrnn

/* ========================================================================
 * CONFIGURATION CONSTANTS 
 * ======================================================================== */

/* ----- MUX input selection ----- */
#define ADS1220_MUX_AIN0_AIN1   0x00
#define ADS1220_MUX_AIN0_AIN2   0x01
#define ADS1220_MUX_AIN0_AIN3   0x02
#define ADS1220_MUX_AIN1_AIN2   0x03
#define ADS1220_MUX_AIN1_AIN3   0x04
#define ADS1220_MUX_AIN2_AIN3   0x05
#define ADS1220_MUX_AIN1_AIN0   0x06
#define ADS1220_MUX_AIN3_AIN2   0x07
#define ADS1220_MUX_AIN0_AVSS   0x08
#define ADS1220_MUX_AIN1_AVSS   0x09
#define ADS1220_MUX_AIN2_AVSS   0x0A
#define ADS1220_MUX_AIN3_AVSS   0x0B
#define ADS1220_MUX_MONITOR_REF 0x0C
#define ADS1220_MUX_MONITOR_AVDD 0x0D
#define ADS1220_MUX_SHORTED     0x0E
#define ADS1220_MUX_RESERVED    0x0F

/* ----- Gain settings ----- */
#define ADS1220_GAIN_1          0x00
#define ADS1220_GAIN_2          0x01
#define ADS1220_GAIN_4          0x02
#define ADS1220_GAIN_8          0x03
#define ADS1220_GAIN_16         0x04
#define ADS1220_GAIN_32         0x05
#define ADS1220_GAIN_64         0x06
#define ADS1220_GAIN_128        0x07

/* ----- Operating mode ----- */
#define ADS1220_MODE_NORMAL     0x00
#define ADS1220_MODE_DUTYCYCLE  0x01
#define ADS1220_MODE_TURBO      0x02
#define ADS1220_MODE_RESERVED   0x03

/* ----- Data rates for NORMAL mode ----- */
#define ADS1220_DR_NORMAL_20    0x00
#define ADS1220_DR_NORMAL_45    0x01
#define ADS1220_DR_NORMAL_90    0x02
#define ADS1220_DR_NORMAL_175   0x03
#define ADS1220_DR_NORMAL_330   0x04
#define ADS1220_DR_NORMAL_600   0x05
#define ADS1220_DR_NORMAL_1000  0x06
#define ADS1220_DR_NORMAL_RES   0x07

/* ----- Data rates for DUTY-CYCLE mode ----- */
#define ADS1220_DR_DUTY_5       0x00
#define ADS1220_DR_DUTY_11_25   0x01
#define ADS1220_DR_DUTY_22_5    0x02
#define ADS1220_DR_DUTY_44      0x03
#define ADS1220_DR_DUTY_82_5    0x04
#define ADS1220_DR_DUTY_150     0x05
#define ADS1220_DR_DUTY_250     0x06
#define ADS1220_DR_DUTY_RES     0x07

/* ----- Data rates for TURBO mode ----- */
#define ADS1220_DR_TURBO_40     0x00
#define ADS1220_DR_TURBO_90     0x01
#define ADS1220_DR_TURBO_180    0x02
#define ADS1220_DR_TURBO_350    0x03
#define ADS1220_DR_TURBO_660    0x04
#define ADS1220_DR_TURBO_1200   0x05
#define ADS1220_DR_TURBO_2000   0x06
#define ADS1220_DR_TURBO_RES    0x07

/* ----- Voltage reference selection ----- */
#define ADS1220_VREF_INTERNAL   0x00
#define ADS1220_VREF_EXTERNAL0  0x01
#define ADS1220_VREF_EXTERNAL1  0x02
#define ADS1220_VREF_AVDD       0x03

/* ----- FIR filter configuration  ----- */
#define ADS1220_FILTER_NONE     0x00
#define ADS1220_FILTER_50_60    0x01
#define ADS1220_FILTER_50HZ     0x02
#define ADS1220_FILTER_60HZ     0x03

/* ----- IDAC current settings ----- */
#define ADS1220_IDAC_OFF        0x00
#define ADS1220_IDAC_10UA       0x01
#define ADS1220_IDAC_50UA       0x02
#define ADS1220_IDAC_100UA      0x03
#define ADS1220_IDAC_250UA      0x04
#define ADS1220_IDAC_500UA      0x05
#define ADS1220_IDAC_1000UA     0x06
#define ADS1220_IDAC_1500UA     0x07

/* ----- IDAC routing channels ----- */
#define ADS1220_IDAC_DISABLED   0x00
#define ADS1220_IDAC_AIN0       0x01
#define ADS1220_IDAC_AIN1       0x02
#define ADS1220_IDAC_AIN2       0x03
#define ADS1220_IDAC_AIN3       0x04
#define ADS1220_IDAC_REFP0      0x05
#define ADS1220_IDAC_REFN0      0x06
#define ADS1220_IDAC_RESERVED   0x07

/* ========================================================================
 * STATUS CODES
 * ======================================================================== */

/**
 * @brief       ADS1220 Status Codes
 * @param  ADS1220_Status_t: Return type for all library functions.
 * @param  ADS1220_OK: Operation completed successfully.
 * @param  ADS1220_ERROR: General error (SPI communication, etc.).
 * @param  ADS1220_TIMEOUT: Operation timed out (DRDY never went low).
 * @param  ADS1220_INVALID_PARAM: Invalid parameter (NULL pointer, bad register).
 * @param  ADS1220_NOT_READY: Device not ready (no conversion data available).
 * @param  ADS1220_CONFIG_ERROR: Configuration settings invalid or conflicting.
 */
typedef enum {
    ADS1220_OK            = 0,
    ADS1220_ERROR         = 1,
    ADS1220_TIMEOUT       = 2,
    ADS1220_INVALID_PARAM = 3,
    ADS1220_NOT_READY     = 4,
    ADS1220_CONFIG_ERROR  = 5
} ADS1220_Status_t;

/* ========================================================================
 * CONFIGURATION REGISTERS 
 * ======================================================================== */
/**
 * @brief       Configuration Register 0
 * @param  raw: Raw 8 bit data that contain configuration fileds.
 * @param  bit: Structure that define the bit fileld of the raw.
 */
typedef union {
    uint8_t raw;
    struct {
        /**
        * @brief  
                Input multiplexer configuration
                These bits configure the input multiplexer.
                For settings where AINN = AVSS, the PGA must be disabled (PGA_BYPASS = 1)
                and only gains 1, 2, and 4 can be used.
        * @param  
                0000 : AINP = AIN0, AINN = AIN1 (default)
                0001 : AINP = AIN0, AINN = AIN2
                0010 : AINP = AIN0, AINN = AIN3
                0011 : AINP = AIN1, AINN = AIN2
                0100 : AINP = AIN1, AINN = AIN3
                0101 : AINP = AIN2, AINN = AIN3
                0110 : AINP = AIN1, AINN = AIN0
                0111 : AINP = AIN3, AINN = AIN2
                1000 : AINP = AIN0, AINN = AVSS
                1001 : AINP = AIN1, AINN = AVSS
                1010 : AINP = AIN2, AINN = AVSS
                1011 : AINP = AIN3, AINN = AVSS
                1100 : (V(REFPx) – V(REFNx)) / 4 monitor (PGA bypassed)
                1101 : (AVDD – AVSS) / 4 monitor (PGA bypassed)
                1110 : AINP and AINN shorted to (AVDD + AVSS) / 2
                1111 : Reserved
        */
        uint8_t mux       : 4;
        /**
        * @brief 
                Gain configuration
                These bits configure the device gain.
                Gains 1, 2, and 4 can be used without the PGA. In this case, gain is obtained by
                a switched-capacitor structure.
        * @param
                000 : Gain = 1 (default)
                001 : Gain = 2
                010 : Gain = 4
                011 : Gain = 8
                100 : Gain = 16
                101 : Gain = 32
                110 : Gain = 64
                111 : Gain = 128
        */
        uint8_t gain      : 3;
        /**
        * @brief 
                Disables and bypasses the internal low-noise PGA
                Disabling the PGA reduces overall power consumption and allows the commonmode voltage range (VCM) to span from AVSS – 0.1 V to AVDD + 0.1 V.
                The PGA can only be disabled for gains 1, 2, and 4.
                The PGA is always enabled for gain settings 8 to 128, regardless of the
                PGA_BYPASS setting.
        * @param 
                0 : PGA enabled (default)
                1 : PGA disabled and bypassed
        */
        uint8_t pga_bypass: 1;
    } bit;
} cfg0_t;

/**
 * @brief       Configuration Register 1
 * @param  raw: Raw 8 bit data that contain configuration fileds.
 * @param  bit: Structure that define the bit fileld of the raw.
 */
typedef union {
    uint8_t raw;
    struct {
        /**
        * @brief 
                Data rate
                These bits control the data rate setting depending on the selected operating
                mode. Table 18 lists the bit settings for normal, duty-cycle, and turbo mode.
        * @param 
                Refrenced to datasheet.
        */
        uint8_t dr        : 3;
        /**
        * @brief 
                Operating mode
                These bits control the operating mode the device operates in.
        * @param 
                00 : Normal mode (256-kHz modulator clock, default)
                01 : Duty-cycle mode (internal duty cycle of 1:4)
                10 : Turbo mode (512-kHz modulator clock)
                11 : Reserved
        */
        uint8_t mode      : 2;
        /**
        * @brief 
                Conversion mode
                This bit sets the conversion mode for the device.
        * @param 
                0 : Single-shot mode (default)
                1 : Continuous conversion mode
        */
        uint8_t cm        : 1;
        /**
        * @brief 
                Temperature sensor mode
                This bit enables the internal temperature sensor and puts the device in
                temperature sensor mode.
                The settings of configuration register 0 have no effect and the device uses the
                internal reference for measurement when temperature sensor mode is enabled.
        * @param 
                0 : Disables temperature sensor (default)
                1 : Enables temperature sensor
        */
        uint8_t ts        : 1;
        /**
        * @brief 
                Burn-out current sources (We will not use it in Thermocouple it is useful in RTDs)
                This bit controls the 10-µA, burn-out current sources.
                The burn-out current sources can be used to detect sensor faults such as wire
                breaks and shorted sensors
        * @param 
                0 : Current sources off (default)
                1 : Current sources on

        */
        uint8_t bcs       : 1;
    } bit;
} cfg1_t;

/**
DR Bit Settings
------------------------------------------------|
NORMAL MODE   |DUTY-CYCLE MODE   |TURBO MODE    |
--------------|------------------|--------------|
000 = 20 SPS  | 000 = 5 SPS      |000 = 40 SPS  |
--------------|------------------|--------------|
001 = 45 SPS  |001 = 11.25 SPS   |001 = 90 SPS  |
--------------|------------------|--------------|
010 = 90 SPS  |010 = 22.5 SPS    |010 = 180 SPS |
--------------|------------------|--------------|
011 = 175 SPS |011 = 44 SPS      |011 = 350 SPS |
--------------|------------------|--------------|
100 = 330 SPS |100 = 82.5 SPS    |100 = 660 SPS |
--------------|------------------|--------------|
101 = 600 SPS |101 = 150 SPS     |101 = 1200 SPS|
--------------|------------------|--------------|
110 = 1000 SPS|110 = 250 SPS     |110 = 2000 SPS|
--------------|------------------|--------------|
111 = Reserved|111 = Reserved    |111 = Reserved|
--------------|------------------|--------------|
** Data rates provided are calculated using the internal oscillator or an external 4.096-MHz clock. The data rates scale proportionally with
the external clock frequency when an external clock other than 4.096 MHz is used.
*/
/**


/**
 * @brief       Configuration Register 2
 * @param  raw: Raw 8 bit data that contain configuration fileds.
 * @param  bit: Structure that define the bit fileld of the raw.
 */
typedef union {
    uint8_t raw;
    struct {
        /**
        * @brief 
                Voltage reference selection
                These bits select the voltage reference source that is used for the conversion.
        * @param 
                00 : Internal 2.048-V reference selected (default)
                01 : External reference selected using dedicated REFP0 and REFN0 inputs
                10 : External reference selected using AIN0/REFP1 and AIN3/REFN1 inputs
                11 : Analog supply (AVDD– AVSS) used as reference     
        */
        uint8_t vref        : 2;
        /**
        * @brief 
                FIR filter configuration
                These bits configure the filter coefficients for the internal FIR filter.
                Only use these bits together with the 20-SPS setting in normal mode and the 5
                SPS setting in duty-cycle mode. Set to 00 for all other data rates.
        * @param 
                00 : No 50-Hz or 60-Hz rejection (default)
                01 : Simultaneous 50-Hz and 60-Hz rejection
                10 : 50-Hz rejection only
                11 : 60-Hz rejection only
        */
        uint8_t fir         : 2;
        /**
        * @brief 
                Low-side power switch configuration
                This bit configures the behavior of the low-side switch connected between
                AIN3/REFN1 and AVSS.
        * @param 
                0 : Switch is always open (default)
                1 : Switch automatically closes when the START/SYNC command is sent and
                opens when the POWERDOWN command is issued    
        */

        uint8_t psw         : 1;
        /**
        * @brief 
                IDAC current setting
                These bits set the current for both IDAC1 and IDAC2 excitation current sources.
        * @param 
                000 : Off (default)
                001 : 10 µA
                010 : 50 µA
                011 : 100 µA
                100 : 250 µA
                101 : 500 µA
                110 : 1000 µA
                111 : 1500 µA
        */

        uint8_t idac        : 3;
    } bit;
} cfg2_t;

/**
 * @brief       Configuration Register 3
 * @param  raw: Raw 8 bit data that contain configuration fileds.
 * @param  bit: Structure that define the bit fileld of the raw.
 */
typedef union {
    uint8_t raw;
    struct {
        /**
        * @brief 
                IDAC1 routing configuration
                These bits select the channel where IDAC1 is routed to.
        * @param 
                000 : IDAC1 disabled (default)
                001 : IDAC1 connected to AIN0/REFP1
                010 : IDAC1 connected to AIN1
                011 : IDAC1 connected to AIN2
                100 : IDAC1 connected to AIN3/REFN1
                101 : IDAC1 connected to REFP0
                110 : IDAC1 connected to REFN0
                111 : Reserved     
        */
        uint8_t mux1         : 3;
        /**
        * @brief 
                IDAC2 routing configuration
                These bits select the channel where IDAC2 is routed to.
        * @param 
                000 : IDAC2 disabled (default)
                001 : IDAC2 connected to AIN0/REFP1
                010 : IDAC2 connected to AIN1
                011 : IDAC2 connected to AIN2
                100 : IDAC2 connected to AIN3/REFN1
                101 : IDAC2 connected to REFP0
                110 : IDAC2 connected to REFN0
                111 : Reserved   
        */        
        uint8_t mux2         : 3;
        /**
        * @brief 
                DRDY mode
                This bit controls the behavior of the DOUT/DRDY pin when new data are ready.
        * @param 
                0 : Only the dedicated DRDY pin is used to indicate when data are ready (default)
                1 : Data ready is indicated simultaneously on DOUT/DRDY and DRDY  
        */        
        uint8_t drdy         : 1;
        /**
        * @brief 
                Always write 0
        * @param 
                  
        */        
        uint8_t reserved     : 1;
    } bit;
} cfg3_t;

/* ========================================================================
 * CONFIGURATION STRUCT 
 * ======================================================================== */
 
 /**
 * @brief       Configuration Struct.
 * @param       mux: Input multiplexer selection.
 * @param       gain: PGA gain setting.
 * @param       pga_bypass: Bypass PGA. true = bypass (gains 1,2,4 only, lower power).
 * @param       mode: Operating mode.
 * @param       datarate: Data rate setting (match with mode).
 * @param       continuous: Conversion mode. true = continuous, false = single-shot.
 * @param       vref: Voltage reference source.
 * @param       filter: FIR filter configuration for 50/60 Hz rejection.
 * @param       idac_current: IDAC current value for both sources.
 * @param       idac1_channel: Routing channel for IDAC1.
 * @param       idac2_channel: Routing channel for IDAC2.
 * @param       low_side_switch: Low-side switch behavior. true = auto close on START.
 * @param       burnout_currents: Burn-out current sources. true = enable 10µA sources.
 * @param       use_drdy_pin: Data ready pin selection. true = dedicated DRDY pin.
 */
typedef struct {
    
    uint8_t  mux;                
    uint8_t  gain;              
    bool     pga_bypass;          
    uint8_t  mode;              
    uint8_t  datarate;          
    bool     continuous;            
    uint8_t  vref;              
    uint8_t  filter;              
    uint8_t  idac_current;      
    uint8_t  idac1_channel;     
    uint8_t  idac2_channel;   
    bool     low_side_switch;   
    bool     burnout_currents;  
    bool     use_drdy_pin;      
} ads1220_config_t;

/* ========================================================================
 * HARDWARE ABSTRACTION LAYER (HAL) CALLBACKS
 * ======================================================================== */

/**
 * @brief       HAL Callbacks Struct.
 * @param  ads1220_hal_t: Structure containing hardware abstraction function pointers.
 * @param  spi_transfer: SPI transfer function (full-duplex).
 * @param  cs_control: Chip select control. true = active (CS low), false = inactive.
 * @param  drdy_read: Read DRDY pin state. Returns 1 = high (not ready), 0 = low (ready).
 * @param  delay_us: Microsecond delay (blocking).
 * @param  delay_ms: Millisecond delay (blocking). Optional, can be NULL.
 */
typedef struct {
    void (*spi_transfer)(uint8_t *tx, uint8_t *rx, uint16_t len);
    void (*cs_control)(bool active);
    int  (*drdy_read)(void);
    void (*delay_us)(uint32_t us);
    void (*delay_ms)(uint32_t ms);
} ads1220_hal_t;

/* ========================================================================
 * GLOBAL HAL INSTANCE
 * ======================================================================== */

/**
 * @brief       Global HAL instance.
 * @param  g_ads1220_hal: Global hardware abstraction callbacks.
 * @note        User must initialize this before calling any library functions.
 */
extern ads1220_hal_t g_ads1220_hal;


/* ========================================================================
 * 9. FUNCTION PROTOTYPES 
 * ======================================================================== */

ADS1220_Status_t external_adc_init(void);


/**
 * @brief       Initialize ADS1220 with given configuration.
 * @param  cfg: Pointer to configuration (can be stack allocated).
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_init(const ads1220_config_t *cfg);

/**
 * @brief       Reset device to default settings.
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_reset(void);

/**
 * @brief       Apply current configuration to device (reconfigure without re-init).
 * @param  cfg: Pointer to new configuration.
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_reconfigure(const ads1220_config_t *cfg);


/**
 * @brief       Write to a configuration register.
 * @param  reg: Register address (0x00 to 0x03).
 * @param  value: 8-bit value to write.
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_write_reg(uint8_t reg, uint8_t value);

/**
 * @brief       Read a configuration register.
 * @param  reg: Register address (0x00 to 0x03).
 * @param  value: Pointer to store read value.
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_read_reg(uint8_t reg, uint8_t *value);

/**
 * @brief       Start a conversion (single-shot) or restart continuous conversions.
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_start_conversion(void);

/**
 * @brief       Enter power-down mode (lowest power consumption).
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_powerdown(void);

/**
 * @brief       Read raw 24-bit conversion result (two's complement).
 * @param  result: Pointer to store 24-bit signed result (sign-extended to 32-bit).
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_read_raw(int32_t *result);

/**
 * @brief       Read conversion result as voltage (Vref/gain based).
 * @param  voltage_volts: Pointer to store voltage in volts.
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_read_voltage(float *voltage_volts);

/**
 * @brief       Read internal temperature sensor.
 * @param  temp_celsius: Pointer to store temperature in degrees Celsius.
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_read_temperature(float *temp_celsius);

/**
 * @brief       Perform offset calibration (shorts inputs internally).
 * @param  num_samples: Number of samples to average (recommended: 8-16).
 * @retval ADS1220_OK on success, otherwise error code.
 */
ADS1220_Status_t ads1220_calibrate_offset(uint8_t num_samples);

/**
 * @brief       Apply offset correction to a raw value.
 * @param  raw_value: Raw ADC value to correct.
 * @retval Offset-corrected value (raw_value - stored_offset)
 */
int32_t ads1220_apply_offset_correction(int32_t raw_value);

/**
 * @brief       Convert raw ADC code to voltage.
 * @param  raw_code: 24-bit two's complement ADC code.
 * @param  gain: PGA gain setting. Use ADS1220_GAIN_* defines.
 * @param  vref: Reference voltage in volts.
 * @retval Voltage in volts.
 */
float ads1220_raw_to_voltage(int32_t raw_code, uint8_t gain, float vref);

/**
 * @brief       Convert raw temperature code to Celsius.
 * @param  raw_code: 24-bit temperature sensor reading.
 * @retval Temperature in degrees Celsius.
 * @note        Formula: temp = ((raw_code >> 10) & 0x3FFF) * 0.03125f
 */
float ads1220_raw_to_temperature(int32_t raw_code);

#endif