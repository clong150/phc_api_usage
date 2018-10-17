#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

#include <linux/pps.h>
#include <linux/ptp_clock.h>
#include "timepps.h"

#include <net/if.h>
#include <stdbool.h>

#ifndef ADJ_SETOFFSET
#define ADJ_SETOFFSET 0X0100
#endif

#ifndef NS_PER_SEC
#define NS_PER_SEC 1000000000LL
#endif

#define TRY(x)                                                  \
  do {                                                          \
    int __rc = (x);                                             \
    if( __rc < 0 ) {                                            \
      fprintf(stderr, "ERROR: TRY(%s) failed\n", #x);           \
      fprintf(stderr, "ERROR: at %s:%d\n", __FILE__, __LINE__); \
      fprintf(stderr, "ERROR: rc=%d errno=%d (%s)\n",           \
              __rc, errno, strerror(errno));                    \
      exit(1);                                                  \
    }                                                           \
  } while( 0 )


/* weird fd-to-clockid mapping (and it's reverse function). It just works */
#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)       ((~(clockid_t) (fd) << 3) | CLOCKFD)
#define CLOCKID_TO_FD(clk)      ((unsigned int) ~((clk) >> 3))

#define CLOCK_STR_SIZE 50

/**                                                                             
 * Contains timestamping information returned by the GET_TS_INFO ioctl.         
 * @valid:            set to non-zero when the info struct contains valid data. 
 * @phc_index:        index of the PHC device.                                  
 * @so_timestamping:  supported time stamping modes.                            
 * @tx_types:         driver level transmit options for the HWTSTAMP ioctl.     
 * @rx_filters:       driver level receive options for the HWTSTAMP ioctl.      
 */
struct sock_ts_info {
  bool valid;//is the data here valid?
  int phc_index;//PHC index
  char name[IFNAMSIZ];//interface name
  unsigned int so_ts_modes;//supported time stamp modes
  unsigned int so_timestamping;
  unsigned int tx_types;
  unsigned int rx_filters;
};

struct pps_data {
  pps_handle_t pps_handle;
};

struct app_data {
  char name[IFNAMSIZ];
  int phc_index;
  char clock[CLOCK_STR_SIZE];
  struct sock_ts_info s_ts_i;
};

#endif
