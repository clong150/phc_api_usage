#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <linux/ethtool.h>
#include <sys/timex.h>
#include <time.h>
//#include <net/if.h>
#include <fcntl.h>
#include <linux/sockios.h>
#include <inttypes.h>
#include <syscall.h>

#include "data_structs.h"

static int clock_adjtime(clockid_t id, struct timex *tx)
{
  return syscall(__NR_clock_adjtime, id, tx);
}

/*
 * PPS reference: http://doc.ntp.org/4.2.6/kernpps.html
 * */
/* tasks:
 *   find out how to determine PPS index from PHC
 *   read diff using pps
 * */
int sk_get_ts_info(char* device, struct sock_ts_info* sti)
{
#ifdef ETHTOOL_GET_TS_INFO
  int fd, rc = 0;
  struct ethtool_ts_info info;
  struct ifreq ifr;

  memset(&ifr, 0, sizeof(ifr));
  memset(&info, 0, sizeof(info));
  memset(sti, 0, sizeof(struct sock_ts_info));

  info.cmd = ETHTOOL_GET_TS_INFO;
  strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);
  ifr.ifr_data = (char *) &info;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if( fd < 0 ){
    perror("SOCK_DGRAM failed");
    return fd;
  }

  rc = ioctl(fd, SIOCETHTOOL, &ifr);
  if( rc < 0 ){
    printf("rc=%d, errno=%d\n",rc, errno);
    perror("SIOCETHTOOL failed");
    return rc;
  }

  sti->valid = 1;
  sti->phc_index = info.phc_index;
  sti->so_timestamping = info.so_timestamping;
  sti->tx_types = info.tx_types;
  sti->rx_filters = info.rx_filters;

  return rc;
#endif
  printf("ERROR: ETHTOOL_GET_TS_INFO not supported\n");
  memset(sti, 0, sizeof(struct sock_ts_info));
  return -1;
}


int phc_from_if(char* device)
{
  //int rc = 0;
  struct sock_ts_info ts_info;
  TRY( sk_get_ts_info(device, &ts_info) );

  if( ts_info.phc_index < 0 ){
    printf("interface %s does not have a PHC clock\n", device);
  } else {
    printf("interface %s has clock /dev/ptp%d\n",device, ts_info.phc_index);
  }

  return ts_info.phc_index;

}

clockid_t phc_open(char* phc)
{
  clockid_t clkid;
  struct ptp_clock_caps caps;
  int fd = open(phc, O_RDWR);
  if( fd < 0 ){
    perror("failed to open clock\n");
  }
  /* fd to clock id */
  clkid = FD_TO_CLOCKID(fd);
  /* get caps */
  int rc = ioctl(fd, PTP_CLOCK_GETCAPS, &caps);
  if( rc < 0 ){
    perror("PTP_CLOCK_GETCAPS");
  }
  printf("fd=%d, clkid=%d\n",fd, clkid);
  return clkid;
}


int parse_opts(int argc, char* argv[], struct app_data* ad){
  int rc = -1;
  char opt;
  while ((opt = getopt(argc, argv, "i:")) != -1) {
    switch(opt){
      case 'i':
        sprintf(ad->name, "%s", optarg);
        printf("Interface %s provided\n", ad->name);
	rc = 1;
        break;
    }
  }
  return rc;
}

int adjt(char* clkref)
{
  int rc = 0;
  clockid_t clkid;
  struct timex tx;

  memset(&tx, 0, sizeof(tx));
  tx.modes = ADJ_SETOFFSET | ADJ_NANO;
  tx.time.tv_sec = 10;
  tx.time.tv_usec = 1000;

  clkid = phc_open(clkref);
  /* finally, we can use the PHC API */
  rc = clock_adjtime(clkid, &tx);
  if( rc < 0 )
    printf("Failed to adjust time\n");
  return rc;
}

int gett(char* clkref)
{
  int rc = 0;
  clockid_t clkid;
  struct timespec ts;

  clkid = phc_open(clkref);
  /* finally, we can use the PHC API */
  clock_gettime(clkid, &ts);

  printf("time: %ld.%ld\n", ts.tv_sec, ts.tv_nsec);
  return rc;
}

void nudge(char* str1)
{
  gett(str1);
  adjt(str1);
  gett(str1);
}

int get_clk_diffs(){
  int rc = 0;
  return rc;
}

int main(int argv, char* argc[]){
  int rc = 0;
  struct app_data ad;

  rc = parse_opts(argv, argc, &ad);
  if( rc > 0 ) {
    ad.phc_index = phc_from_if(ad.name);
    sprintf(ad.clock, "/dev/ptp%d",ad.phc_index);
    printf("clock reference set to %s\n", ad.clock);
    nudge(ad.clock);
  } else {
    //phc_from_if("enp5s0f0");
    //phc_from_if("enp4s0f0");

    //char* str1 = "/dev/ptp0";
    //nudge(str1);
  }

  return rc;
}
