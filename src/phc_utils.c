#include "phc_utils.h"

clockid_t phc_open(char* phc)
{
  clockid_t clkid;
  struct ptp_clock_caps caps;
  int fd = open(phc, O_RDRW);
  int rc;
  /* fd to clock id */
  clkid = ( (~(clockid_t) (fd) << 3) | 3);
  /* get caps */
  rc = ioctl(fd, PTP_CLOCK_GETCAPS, &caps);
  return clkid;
}

int get_sock_ts_info(char* name, struct sock_ts_info* sti)
{
#ifdef ETHTOOL_GET_TS_INFO
  struct ethtool_ts_info eti;
  struct ifreq ifq;
  int rc;
  int fd;

  memset(&eti, '\0', sizeof(eti));
  memset(&ifq, '\0', sizeof(ifq));
  memset(sti, '\0', sizeof(struct sock_ts_info));

  eti.cmd = ETHTOOL_GET_TS_INFO;
  strncpy(ifq.ifr_name, name, IFNAMSIZ - 1);
  ifq.ifr_data = (char *)&eti;
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if( fd < 0 ){
    printf("%s: failed to create socket\n", __FUNCTION__);
    goto error;
  }
  rc = ioctl(fd, SIOCETHTOOL, &ifq);
  if( rc < 0 ){
    close(fd);
    printf("%s: ioctl failed to do SIOCETHTOOL\n", __FUNCTION__);
    goto error;
  }
  close(fd);

  /* update state information*/
  sti->valid = 1;
  sti->phc index = eti.phc_index;
  strncpy(sti->ifr_name, name, IFNAMSIZ - 1);
  sti->so_ts_modes = eti.so_timestamping;;

  return 0;
#endif
error:
  return -1;
}

/* this may not work. should open phc device (in /dev), then */
clockid_t clock_open(char *device, int *phc_index)
{
	struct sock_ts_info ts_info;
	char phc_device[16];
	int clkid;

	/* check if device is CLOCK_REALTIME */
	if (!strcasecmp(device, "CLOCK_REALTIME"))
		return CLOCK_REALTIME;

	/* check if device is valid phc device */
	clkid = phc_open(device);
	if (clkid != CLOCK_INVALID)
		return clkid;

	/* check if device is a valid ethernet device */
	if (get_sock_ts_info(device, &ts_info) || !ts_info.valid) {
		fprintf(stderr, "unknown clock %s: %m\n", device);
		return CLOCK_INVALID;
	}

	if (ts_info.phc_index < 0) {
		fprintf(stderr, "interface %s does not have a PHC\n", device);
		return CLOCK_INVALID;
	}

	sprintf(phc_device, "/dev/ptp%d", ts_info.phc_index);
	clkid = phc_open(phc_device);
	if (clkid == CLOCK_INVALID)
		fprintf(stderr, "cannot open %s: %m\n", device);
	*phc_index = ts_info.phc_index;
	return clkid;
}


