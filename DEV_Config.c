/*****************************************************************************
* | File        :   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*                Used to shield the underlying layers of each master
*                and enhance portability
*----------------
* |	This version:   V1.0
* | Date        :   2018-10-15
* | Info        :   Basic version
*
******************************************************************************/
#include "DEV_Config.h"
#include "Debug.h"  //DEBUG()

#include <wiringPi.h>

/**
 * Module Initialize, use BCM2835 library.
 *
 * Example:
 * if(DEV_ModuleInit())
 *   exit(0);
 */
UBYTE DEV_ModuleInit(void)
{
    //1.wiringPiSetupGpio
    //if(wiringPiSetup() < 0)//use wiringpi Pin number table
    if(wiringPiSetupGpio() < 0) { //use BCM2835 Pin number table
        printf(" set wiringPi lib failed	!!! \r\n");
        return 1;
    } else {
        printf(" set wiringPi lib success  !!! \r\n");
    }
    
    //motor 1 
    pinMode(M1_ENABLE_PIN, OUTPUT);
    pinMode(M1_DIR_PIN, OUTPUT);
    pinMode(M1_STEP_PIN, OUTPUT);
    pinMode(M1_M0_PIN, OUTPUT);
    pinMode(M1_M1_PIN, OUTPUT);
    pinMode(M1_M2_PIN, OUTPUT);

    //motor 2 
    pinMode(M2_ENABLE_PIN, OUTPUT);
    pinMode(M2_DIR_PIN, OUTPUT);
    pinMode(M2_STEP_PIN, OUTPUT);
    pinMode(M2_M0_PIN, OUTPUT);
    pinMode(M2_M1_PIN, OUTPUT);
    pinMode(M2_M2_PIN, OUTPUT);

    return 0;
}

/**
 * Module Exit, close BCM2835 library.
 *
 * Example:
 * DEV_ModuleExit();
 */
void DEV_ModuleExit(void)
{
    // bcm2835_close();
}

/**
 * Millisecond delay.
 *
 * @param xms: time.
 *
 * Example:
 * DEV_Delay_ms(500);//delay 500ms
 */
void DEV_Delay_ms(uint32_t xms)
{
    delay(xms);
}

/**
 * Microsecond delay.
 *
 * @param xus: time.
 *
 * Example:
 * DEV_Delay_us(500);//delay 500us
 */
void DEV_Delay_us(uint32_t xus)
{
    delayMicroseconds(xus);
}
