/*==================================================================
  File Name    : main.c
  Author       : Emile
  ------------------------------------------------------------------
  Purpose : This files contains the main() function and all the 
            hardware related functions for the STM8S103F3P6 uC.
            The purpose of this project is to have a soft power-on
            for a power-amp. There are 3 relays, one for the Neutral
            line, one for the Live-wire with power resistors, and
            one for the Live-wire without power-resistors.
  ------------------------------------------------------------------
  Soft Power-On for Power-Amps (SPOPA) is free software: you can 
  redistribute it and/or modify it under the terms of the GNU General 
  Public License as published by the Free Software Foundation, either 
  version 3 of the License, or (at your option) any later version.
 
  SPOPA is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with SPOPA.  If not, see <http://www.gnu.org/licenses/>.
  ------------------------------------------------------------------
  $Log: $
  ==================================================================
*/ 
#include "main.h"
#include "scheduler.h"
#include "delay.h"

// Global variables
uint8_t pwr_sw;            // actual value of power-switch
uint8_t old_pwr_sw;        // previous value of pwr_sw
uint8_t std = STD_PWR_OFF; // STD state number
uint8_t tmr = 0;           // Timer for power-up through resistors
uint8_t led_on = 0;        // Status for power-led during blinking

// External variables, defined in other files
extern uint32_t t2_millis;        // needed for delay_msec()

/*-----------------------------------------------------------------------------
  Purpose  : This is the interrupt routine for the Timer 2 Overflow handler.
             It runs at 1 kHz and drives the scheduler and the multiplexer.
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
//#pragma vector = TIM2_OVR_UIF_vector
@interrupt void TIM2_UPD_OVF_IRQHandler(void)
{
    t2_millis++;      // update millisecond counter
	scheduler_isr();  // Run scheduler interrupt function
    TIM2_SR1 &= ~TIM2_SR1_UIF; // Reset interrupt (UIF bit) so it will not fire again straight away.
} // TIM2_UPD_OVF_IRQHandler()

/*-----------------------------------------------------------------------------
  Purpose  : This routine initialises the system clock to run at 16 MHz.
             It uses the internal HSI oscillator.
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
void initialise_system_clock(void)
{
    CLK_ICKCR  = 0;                //  Reset the Internal Clock Register.
    CLK_ICKCR |= CLK_ICKCR_HSIEN;  //  Enable the HSI.
    while ((CLK_ICKCR & CLK_ICKCR_HSIRDY) == 0); //  Wait for the HSI to be ready for use.
    CLK_CKDIVR     = 0;            //  Ensure the clocks are running at full speed.
 
    // The datasheet lists that the max. ADC clock is equal to 6 MHz (4 MHz when on 3.3V).
    // Because fMASTER is now at 16 MHz, we need to set the ADC-prescaler to 4.
    ADC_CR1     &= ~ADC_CR1_SPSEL_MSK;
    ADC_CR1     |= 0x20;          //  Set prescaler (SPSEL bits) to 4, fADC = 4 MHz
    CLK_SWIMCCR  = 0x00;          //  Set SWIM to run at clock / 2.
    CLK_SWR      = 0xE1;          //  Use HSI as the clock source.
    CLK_SWCR     = 0x00;          //  Reset the clock switch control register.
    CLK_SWCR    |= CLK_SWCR_SWEN; //  Enable switching.
    while ((CLK_SWCR & CLK_SWCR_SWBSY) != 0);  //  Pause while the clock switch is busy.
} // initialise_system_clock()

/*-----------------------------------------------------------------------------
  Purpose  : This routine initialises Timer 2 to generate a 1 kHz interrupt.
             16 MHz / (  16 *  1000) = 1000 Hz (1000 = 0x03E8)
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
void setup_timer2(void)
{
    TIM2_PSCR = 0x04;         //  Prescaler = 16
    TIM2_ARRH = 0x03;         //  High byte of 1000
    TIM2_ARRL = 0xE8;         //  Low  byte of 1000
    TIM2_IER  = TIM2_IER_UIE; //  Enable the update interrupts, disable all others
    TIM2_CR1  = TIM2_CR1_CEN; //  Finally enable the timer
} // setup_timer2()

/*-----------------------------------------------------------------------------
  Purpose  : This routine initialises all the GPIO pins of the STM8 uC.
             See stc1000p.h for a detailed description of all pin-functions.
             AN2854 section 6.1 states that: "All unused port pins should be 
             configured as output low level. Do not leave any unused I/O pin 
             configured as a floating input which could lead to useless high 
             consumption".
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
void setup_gpio_ports(void)
{
	PA_DDR     |=  (RLY_N | RLY_L_R | RLY_L); // Set as output
	PA_DDR     |=  PA_NC;                     // Set unused ports as output
	PA_CR1     |=  PA_NC;                     // Set to Push-Pull
    PA_ODR     &= ~PA_NC;                     // Set to low-level
    PA_CR1     |=  (RLY_N | RLY_L_R | RLY_L); // Set to Push-Pull
    PA_ODR     &= ~(RLY_N | RLY_L_R | RLY_L); // Disable PORTA outputs
		
    PB_DDR     |=  PB_NC;   // Set unused ports as output
    PB_CR1     |=  PB_NC;   // Set to Push-Pull
    PB_ODR     &= ~PB_NC;   // Set to low-level
		
    PC_DDR     |=  PWR_LED | RLY_LS; // Set as outputs
	PC_CR1     |=  PWR_LED | RLY_LS; // Set to Push-Pull
    PC_ODR     &= ~PWR_LED;          // Disable PWR_LED and speaker relay
    PC_ODR     |=  RLY_LS;           // Disable Speaker relay (active-low)
    PC_DDR     &= ~PWR_SW;           // set as Input
	PC_CR1     |=  PWR_SW;           // Enable pull-up
		
    PD_DDR     |=  PD_NC;   // Set unused ports as output
    PD_CR1     |=  PD_NC;   // Set to Push-Pull
    PD_ODR     &= ~PD_NC;   // Set to low-level
} // setup_output_ports()

/*-----------------------------------------------------------------------------
  Purpose  : This routine reads the values of the Power Switch button and 
             returns the result. Routine should be called every 100 msec.
             The result is returned in the global variable pwr_sw.
  Variables: -
  Returns  : 1: button transition from L -> H ; 0: no transition
  ---------------------------------------------------------------------------*/
uint8_t read_power_switch(void)
{
    old_pwr_sw = pwr_sw;
    pwr_sw     = (PC_IDR & PWR_SW);          // PC3
    pwr_sw     = (pwr_sw ^ PWR_SW) & PWR_SW; // Invert button (0 = pressed)
    if (pwr_sw && !old_pwr_sw)
         return 1;
    else return 0;
} // read_power_switch()

/*-----------------------------------------------------------------------------
  Purpose  : This function blinks the LED from the Power-switch. It is called
             every 100 msec.
  Variables: led_on (global)
  Returns  : -
  ---------------------------------------------------------------------------*/
void blink_led(void)
{
     if (led_on) 
     {  // blink LED (off)
        led_on  = 0;
        PC_ODR &= ~PWR_LED;
     }
     else
     {  // blink LED (on)
        led_on = 1;
        PC_ODR |= PWR_LED;
     } // else
} // blink_led()

/*-----------------------------------------------------------------------------
  Purpose  : This task is called every 100 msec. and contains the main 
             functionality for this program.
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
void tim_task(void)
{
    uint8_t k = read_power_switch();
    
    switch (std)
    {
        case STD_PWR_OFF: // Power-Off mode, all relays and LED are off
             PA_ODR &= ~(RLY_N | RLY_L_R | RLY_L); // all relays off
             PC_ODR &= ~PWR_LED;    // power-led off
             PC_ODR |=  RLY_LS;     // Disable Speaker relay (active-low)
             if (k) std = STD_N_ON; // power switch pressed
             break;
        case STD_N_ON: // Enable Relay for Neutral wire
             PA_ODR |= RLY_N;    // Relay N on
             PC_ODR |= PWR_LED;  // Set power-led on
             led_on  = 1;
             tmr     = 0;        // reset counter
             std     = STD_L_R_ON;
             break;
        case STD_L_R_ON: // Energize Main Power-Supply through resistors
             PA_ODR |= RLY_L_R;  // Relay L with resistors on
             blink_led();
             if (++tmr >= TMR_L_R)
             {
                tmr = 0; // reset timer 
                std = STD_SPK_ON;
             } // if
             break;
        case STD_SPK_ON: // Wait for Power-Amp DC voltage to settle
             PA_ODR |= RLY_L;    // Relay L on
             delay_msec(5);      // Short delay
             PA_ODR &= ~RLY_L_R; // disable Relay L with resistors
             blink_led();
             if (++tmr >= TMR_SPK_ON)
             {
                tmr = 0; // reset timer 
                std = STD_PWR_ON;
             } // if
             break;
        case STD_PWR_ON: // Normal Power-On state
             PC_ODR |=  PWR_LED; // Power-LED is ON
             PC_ODR &= ~RLY_LS;  // Enable relay for Speaker (active-low)
             if (k) std = STD_PWR_OFF1; // power-switch pressed
             break;
        case STD_PWR_OFF1: // Disable Speaker Relay (should be done first)
             PC_ODR &= ~PWR_LED; // LED off
             PC_ODR |=  RLY_LS;  // Disable Speaker relay (active-low)
             std = STD_PWR_OFF2;
             break;
        case STD_PWR_OFF2: // Disable live wire
             PC_ODR |=  PWR_LED; // LED on
             PA_ODR &= ~RLY_L;   // Relay L off
             std = STD_PWR_OFF3;
             break;
        case STD_PWR_OFF3: // Disable neutral wire
             PC_ODR &= ~PWR_LED; // LED off
             PA_ODR &= ~RLY_N;   // Relay N off
             tmr = 0;
             std = STD_PWR_OFF4;
             break;
        case STD_PWR_OFF4: // Some more fancy blinking
             blink_led();
             if (tmr++ >= TMR_OFF)
             {
                tmr = 0; // reset timer 
                std = STD_PWR_OFF;
             } // if
             break;
        default:
             std = STD_PWR_OFF;
             break;
    } // switch
} // tim_task()

/*-----------------------------------------------------------------------------
  Purpose  : This is the main entry-point for the entire program.
             It initialises everything, starts the scheduler and dispatches
             all tasks.
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
int main(void)
{
    disable_interrupts();
    initialise_system_clock(); // Set system-clock to 16 MHz
    setup_gpio_ports();        // Init. needed output-ports for LED and keys
    setup_timer2();            // Set Timer 2 to 1 kHz
    
    // Initialise all tasks for the scheduler
	scheduler_init();                // clear task_list struct
    add_task(tim_task ,"TIM",0,100); // every 100 msec.
    enable_interrupts();

    while (1)
    {   // background-processes
        dispatch_tasks();     // Run task-scheduler()
        wait_for_interrupt(); // do nothing
    } // while
} // main()
