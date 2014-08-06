/*! \file main.c
 */

/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*!
 * \defgroup main Marionette main
 *
 * @{
 */

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "mshell.h"
#include "board.h"

#include "fetch.h"
#include "fetch_adc.h"
#include "util_general.h"
#include "util_version.h"
#include "usbcfg.h"

#include "main.h"

/*! Virtual serial port over USB.*/
SerialUSBDriver SDU1;

extern const struct led        LED1 ;
extern const struct led        LED2 ;
extern const struct led        LED3 ;
extern const struct led        LED4 ;

extern       struct led_config led_cfg ;

/*! status
 */
Util_status      M_Status = { .status=GEN_OK };

#define SHELL_WA_SIZE   THD_WA_SIZE(8192)

/*! \brief Show memory usage
 */
static void cmd_mem(BaseSequentialStream * chp, int argc, char * argv[])
{
	size_t n, size;
	(void)argv;
	if (argc > 0)
	{
		chprintf(chp, "Usage: mem\r\n");
		return;
	}
	n = chHeapStatus(NULL, &size);
	chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
	chprintf(chp, "heap fragments   : %u\r\n", n);
	chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

/*! \brief Show running threads
 */
static void cmd_threads(BaseSequentialStream * chp, int argc, char * argv[])
{
	(void)chp;
	static const char * states[] = {THD_STATE_NAMES};
	Thread * tp;

	(void)argv;
	if (argc > 0)
	{
		chprintf(chp, "Usage: threads\r\n");
		return;
	}
	chprintf(chp, "addr\t\tstack\t\tprio\trefs\tstate\t\ttime\tname\r\n");
	tp = chRegFirstThread();
	do
	{
		chprintf(chp, "%.8lx\t%.8lx\t%4lu\t%4lu\t%9s\t%lu\t%s\r\n",
		         (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
		         (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
		         states[tp->p_state], (uint32_t)tp->p_time, tp->p_name);
		tp = chRegNextThread(tp);
	}
	while (tp != NULL);
}

/*! \brief MShell commands described in main
 * \sa MSHELL
 */
static const MShellCommand commands[] =
{
	{"mem", cmd_mem},
	{"threads", cmd_threads},
	{NULL, NULL}
};

/*! \brief MShell configuration
 * \sa MSHELL
 */
static const MShellConfig shell_cfg1 =
{
	(BaseSequentialStream *) & SDU1,
	commands
};

/*! \brief main application loop
 */
static void main_app(void)
{
	Thread             *            mshelltp = NULL;
	static          VERSIONData     version_data;

	mshellInit();

	util_hwversion(&version_data);
	usb_set_serial_strings(version_data.hardware.id_high, version_data.hardware.id_center,
	                       version_data.hardware.id_low);
	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);

	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(1000);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);

	fetch_adc_init((BaseSequentialStream *) &SDU1);

	//hbStart(&led_cfg);

	while (TRUE)
	{
		if (!mshelltp)
		{
			if (SDU1.config->usbp->state == USB_ACTIVE)
			{
				mshelltp = mshellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
			}
		}
		else
		{
			if (chThdTerminated(mshelltp))
			{
				chThdRelease(mshelltp);
				mshelltp = NULL;
			}
		}
		chThdSleepMilliseconds(500);
	}
}

int main(void)
{
	halInit();
	chSysInit();

	main_app();

	return(0);
}

//! @}

