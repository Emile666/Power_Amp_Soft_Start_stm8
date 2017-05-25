/*==================================================================
  File Name    : delay.h
  Author       : Emile
  ------------------------------------------------------------------
  Purpose : This files contains several time-delay functions.
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
#ifndef _DELAY_H
#define _DELAY_H

#include "stdint.h"

#define enable_interrupts()  {_asm("rim\n");} /* enable interrupts */
#define disable_interrupts() {_asm("sim\n");} /* disable interrupts */
#define wait_for_interrupt() {_asm("wfi\n");} /* Wait For Interrupt */

uint32_t millis(void);
void     delay_msec(uint16_t ms);
void     delay_usec(uint16_t us);

#endif