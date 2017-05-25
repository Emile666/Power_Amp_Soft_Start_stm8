/*==================================================================
  File Name    : main.h
  Author       : Emile
  ------------------------------------------------------------------
  Purpose : main header file for this project.
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
#ifndef _SPOPA_MAIN_H
#define _SPOPA_MAIN_H

#include <iostm8s103.h>
#include "stdint.h"

// Hardware defines for register definitions
// These value were defined in IAR, but not in Cosmic STM8
#define TIM2_SR1_UIF      (0x01)
#define CLK_ICKCR_HSIEN   (0x01)
#define CLK_ICKCR_HSIRDY  (0x02)
#define ADC_CR1_SPSEL_MSK (0x70)
#define CLK_SWCR_SWBSY    (0x01)
#define CLK_SWCR_SWEN     (0x02)
#define TIM2_IER_UIE      (0x01)
#define TIM2_CR1_CEN      (0x01)

// System Defines
#define RLY_N   (0x02) /* PA1 */
#define RLY_L_R (0x04) /* PA2 */
#define RLY_L   (0x08) /* PA3 */

#define PWR_SW  (0x08) /* PC3 */
#define PWR_LED (0x10) /* PC4 */

#define PA_NC   ~(RLY_N | RLY_L_R | RLY_L)
#define PB_NC   (0x30) /* PB4, PB5 */
#define PC_NC   (0xE0) /* PC7, PC6, PC5 */
#define PD_NC   (0x7C) /* PD6..PD2. Note: PD1 = SWIM! */

// Defines for State Transition Diagram (STD)
#define STD_PWR_OFF  (0)
#define STD_N_ON     (1)
#define STD_L_R_ON   (2)
#define STD_PWR_ON   (3)
#define STD_PWR_OFF1 (4)
#define STD_PWR_OFF2 (5)

#define TMR_L_R     (10) /* 10 = 1000 msec. */

// Function Prototypes
void    initialise_system_clock(void);
void    setup_timer2(void);
void    setup_gpio_ports(void);
uint8_t read_power_switch(void);
void    tim_task(void);

#endif