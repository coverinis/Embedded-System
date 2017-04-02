/*
 * FILE          : sc_hook.c
 * PROJECT       : PROG1360 - Calling Functions Assignment #2
 * PROGRAMMER    : Shawn Coverini
 * FIRST VERSION : 02/10/2016
 * DESCRIPTION   : Add function to Boards interface
 */

/*
 *	C to assembler menu hook
 *
 */

//Imports
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include "common.h"

/*
 * FUNCTION    : sc_led_demo
 * DESCRIPTION : Fetch user arguments and run sc_led_demo
 * PARAMETERS  : int count : number of loops
 *               int delay : time delay for lights
 * RETURNS     : void
 */
void sc_led_demo(int count, int delay);

/*
 * FUNCTION    : scA2
 * DESCRIPTION : Fetch user arguments and run sc_led_demo
 * PARAMETERS  : int action : take in command for help
 * RETURNS     : void
 */

void scA2(int action)
{
  //Added code
  uint32_t delay;
  uint32_t count;

  //Get first user argument
  int fetch_status;
  fetch_status = fetch_uint32_arg(&count);

  //check for the input
  if(fetch_status) {
    // Use a default count value
    count = 0x2;
  }

  //Get seconed user argument
  fetch_status = fetch_uint32_arg(&delay);

  //Check for arguments input
  if(fetch_status) {
    // Use a default delay value
    delay = 0xFFFFFF;
  }

  if(action==CMD_SHORT_HELP) return;
  if(action==CMD_LONG_HELP) {
    printf("LED Demo\n\n"
	   "Assignment 2 function\n"
	   );

    return;
  }
  printf("Count: %d Delay: %d\n",(int)count, (int)delay);
  sc_led_demo(count,delay);
}

ADD_CMD("scA2",scA2,"Assignment 2")
