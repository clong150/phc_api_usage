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

#include "data_structs.h"

/*
 * PPS reference: http://doc.ntp.org/4.2.6/kernpps.html
 *
 * Running PPS using phc2sys - you need to specify both
 * the PTP clock and the PPS device - for example:
 *     ./phc2sys -d/dev/pps0 -s/dev/ptp0 -O100
 * Note further that the slave must be CLOCK_REALTIME
 * in this case
 *
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
  int rc = 0;
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


/*
 * We need to enable PPS on the PHC.
 *
 * phc  the PHC clock as exposed in /dev (e.g. /dev/pps2)
 *
 * */
int enable_pps(char* phc)
{
  int rc = 0;
  int fd;
  int enable = 1;

  fd = open(phc, O_RDWR);
  rc = ioctl(fd, PTP_ENABLE_PPS, enable);
  if(rc < 0){
    printf("failed to enable PPS output\n");
  }
  return rc;
}

void print_pps_ktime(struct pps_ktime* p_kt)
{
  printf("%"PRIu64".%"PRId32", flags %"PRIu32"\n",
         (unsigned long)p_kt->sec, p_kt->nsec, p_kt->flags);
}

int get_pps_params(char* pps_dev)
{
  int fd, rc = 0;
  struct pps_kparams pk;

  fd = open(pps_dev, O_RDONLY);
  rc = ioctl(fd, PPS_GETPARAMS, &pk);

  printf("API version: %d\n", pk.api_version);
  printf("mode: %d\n", pk.mode);
  print_pps_ktime(&pk.assert_off_tu);
  print_pps_ktime(&pk.clear_off_tu);
  return rc;
}

/*
 * Read the PPS
 * */
int read_pps(char* pps_dev)
{
  int clk_fd;
  int rc = 0;
  struct pps_fdata fdata;

  fdata.timeout.sec = 5;
  fdata.timeout.nsec = 0;
  fdata.timeout.flags = ~PPS_TIME_INVALID;

  clk_fd = open(pps_dev, O_RDWR);
  rc = ioctl(clk_fd, PPS_FETCH, &fdata);
  if(rc < 0){
    printf("failed to fetch PPS data\n");
    return rc;
  }

  printf("assert #: %u, seconds: %lld, nanos: %"PRIu32"\n",
         fdata.info.assert_sequence,
         fdata.info.assert_tu.sec,
         fdata.info.assert_tu.nsec);
  printf("clear #: %u, seconds: %lld, nanos: %"PRIu32"\n",
         fdata.info.clear_sequence,
         fdata.info.clear_tu.sec,
         fdata.info.clear_tu.nsec);
/*  printf("clear - assert: %d\n",
         fdata.info.clear_tu.sec - fdata.info.assert_tu.sec);*/
  return rc;
}

int main(int argv, char* argc[]){
  int rc = 0;
  struct app_data ad;
  rc = parse_opts(argv, argc, &ad);

  if( rc > 0 ){
    ad.phc_index = phc_from_if(ad.name);
    sprintf(ad.clock, "/dev/pps%d", ad.phc_index);
    printf("clock reference set to %s\n", ad.clock);

    rc = enable_pps(ad.clock);
    rc = get_pps_params(ad.clock);
    read_pps(ad.clock);
  } else {
    rc = phc_from_if("enp4s0f0");
    rc = phc_from_if("enp5s0f0");

    char* str1 = "/dev/pps0";
    //char* str2 = "/dev/ptp1";

    rc = enable_pps(str1);
    rc = get_pps_params(str1);
    read_pps(str1);
  }

  return rc;
}
