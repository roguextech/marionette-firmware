/*! \file io_manage.c
  *
  * Keep an accounting of i/o pin configurations
  *
  * \sa fetch.c
  * @defgroup io_manage IO Management
  * @{
   */

/*!
 * <hr>
 *
 * Track current state of IO pins
 *
 * Marionette should not be able to set a pin for a function that isn't available.
 * For example: DAC is only available on 2 output pins.
 * <hr>
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "hal.h"
#include "chprintf.h"

#include "util_general.h"
#include "util_messages.h"

#include "io_manage.h"
#include "io_manage_defs.h"


/*! \brief return pointer to the io port allocation table
 */
static IO_table * io_manage_get_table(ioportid_t port, uint32_t pad)
{
	for(uint8_t i = 0; i < NELEMS(io_manage_tables); ++i)
	{
		if(io_manage_tables[i]->port == port)
		{
			return io_manage_tables[i];
		}
	}
	return NULL;
}

/*! \brief check availability of requested function against table
 */
static bool io_manage_fn_avail(ioportid_t port, uint32_t pad, IO_alloc request_alloc, IO_table * table)
{
	IO_alloc         curr_alloc;

	if(table != NULL)
	{
		curr_alloc = table->pin[pad].current_alloc;
		if(curr_alloc == request_alloc)
		{
			return true;
		}
		if((request_alloc & table->pin[pad].fn_available) != 0)
		{
			return true;
		};
	}
	return false;
}

/*! \brief Update the port allocation table
*/
bool io_manage_set_mode(ioportid_t port, uint32_t pad, iomode_t new_mode, IO_alloc request_alloc)
{
	IO_table    *    table = io_manage_get_table(port, pad);

	if(io_manage_fn_avail(port, pad, request_alloc, table))
	{
		table->pin[pad].current_mode  = new_mode;
		table->pin[pad].current_alloc = request_alloc;
		palSetPadMode(port, pad, new_mode);
		return true;
	}
	return false;
}

/*! \brief Reset port allocation table to defaults and reset pad modes to defaults.
 */
void io_manage_to_defaults(void)
{
	IO_table    *    table;
	for(uint8_t i = 0; i < NELEMS(io_manage_tables); ++i)
	{
		table = io_manage_tables[i];
		for(uint8_t j = 0; j < NELEMS(table->pin); ++j)
		{
			table->pin[j].current_mode    = table->pin[j].default_mode;
			table->pin[j].current_alloc   = table->pin[j].default_alloc;
			palSetPadMode(table->port, table->pin[j].pad, table->pin[j].current_mode);
		}
	}
}
//! @}

