#ifndef PHC_UTILS_H
#define PHC_UTILS_H

#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>

#include <ptp_clock.h>


#include "data_structs.h"

#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif


int get_sock_ts_info(char* name, struct sock_ts_info* sti);

clockid_t clock_open(char *device, int *phc_index);

#endif
