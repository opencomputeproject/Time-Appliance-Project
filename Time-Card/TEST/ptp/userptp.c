#include <err.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/ioctl.h>
#include <linux/ptp_clock.h>

struct {
	const char *ptp_dev;
	int channel;
	bool show;
	bool read;
	bool set;
	bool enable;
	bool pps;
} opt = {
	.ptp_dev = "/proc/driver/ocp0/ptp",
};

int
showpin(int fd, int pin)
{
	struct ptp_pin_desc desc = {};
	int rc;

	rc = ioctl(fd, PTP_PIN_GETFUNC, &desc, sizeof(desc));
	if (rc)
		err(1, "ioctl(GETFUNC)");

	printf("pin %d\n", pin);
	printf("    name:     %s\n", desc.name);
	printf("    index:    %d\n", desc.index);
	printf("    func:     %d\n", desc.func);
	printf("    chan:     %d\n", desc.chan);

	return 0;
}

int
showcaps(int fd)
{
	struct ptp_clock_caps caps = {};
	int i, rc;

	rc = ioctl(fd, PTP_CLOCK_GETCAPS, &caps, sizeof(caps));
	if (rc)
		err(1, "ioctl(GETCAPS)");

	printf("max_adj:      %d\n", caps.max_adj);
	printf("n_alarm:      %d\n", caps.n_alarm);
	printf("n_ext_ts:     %d\n", caps.n_ext_ts);
	printf("n_per_out:    %d\n", caps.n_per_out);
	printf("pps:          %d\n", caps.pps);
	printf("n_pins:       %d\n", caps.n_pins);
//	printf("adjust_phase: %d\n", caps.adjust_phase);

	for (i = 0; i < caps.n_pins; i++)
		showpin(fd, i);

	return 0;
}

int
extts_control(int fd, int channel, bool on)
{
	struct ptp_extts_request req = {};
	int rc;

	req.index = channel;
	req.flags = on ? PTP_ENABLE_FEATURE : 0;

	rc = ioctl(fd, PTP_EXTTS_REQUEST, &req, sizeof(req));
	if (rc)
		err(1, "ioctl(EXTTS_REQUEST)");

	return 0;
}

int
pps_control(int fd, bool on)
{
	int rc;

	rc = ioctl(fd, PTP_ENABLE_PPS, &on, sizeof(on));
	if (rc)
		err(1, "ioctl(ENABLE_PPS)");

	return 0;
}

void
ptp_control(int fd)
{
	const char *state = opt.enable ? "ON" : "OFF";

	if (opt.pps) {
		printf("pps = %s\n", state);
		pps_control(fd, opt.enable);
	} else {
		printf("channel %d = %s\n", opt.channel, state);
		extts_control(fd, opt.channel, opt.enable);
	}
}

int
get_msg(int fd)
{
	struct ptp_extts_event evt[8];
	int n, count;

	printf("Reading from PTP device, ^C to abort...\n");
	for (;;) {
		n = read(fd, &evt, sizeof(evt));
		if (n == -1)
			err(1, "read");
		if (n == 0)
			break;
		count = n / sizeof(evt[0]);
		if (count * sizeof(evt[0]) != n)
			errx(1, "size mismatch: %ld vs %d\n", 
			     count * sizeof(evt[0]), n);
		for (n = 0; n < count; n++) {
			printf("idx: %d time:%lld.%u  data:%d\n",
			    evt[n].index,
			    evt[n].t.sec, evt[n].t.nsec,
			    evt[n].rsv[0]);
		}
	}

	return 0;
}

static void
usage(const char *prog)
{
	printf("Usage: %s [options] [<extts#|pps> <on|off>]\n", prog);
	printf("\t-h: help\n");
	printf("\t-d: pps_device\n");
	printf("\t-s: show ptp information\n");
	printf("\t-r: read ptp timestamps\n");
	printf("\textts# = external timestamp channel #, starting at 0\n");
	exit(1);
}

#define OPTSTR "d:hrs"

static void
parse_cmdline(int argc, char **argv)
{
	char *prog = basename(argv[0]);
        int c;

        while ((c = getopt(argc, argv, OPTSTR)) != -1) {
                switch (c) {
                case 'd':
                        opt.ptp_dev = optarg;
                        break;
                case 'r':
                        opt.read = true;
                        break;
                case 's':
                        opt.show = true;
                        break;
                case 'h':
		default:
			usage(prog);
		}
	}

	if (argc - optind >= 2) {
		opt.channel = atoi(argv[optind]);
		opt.pps = !strcmp(argv[optind], "pps");
		opt.enable = !strcmp(argv[optind + 1], "on");
		opt.set = true;
		opt.read = true;
	}
	if (opt.show || opt.read || opt.set)
		return;

	if (argc != optind)
		usage(prog);

	opt.show = true;
}

int
main(int argc, char **argv)
{
	int fd;

	parse_cmdline(argc, argv);

	printf("Using PTP device %s\n", opt.ptp_dev);
	fd = open(opt.ptp_dev, O_RDWR);
	if (fd == -1)
		err(1, "open");

	if (opt.show)
		showcaps(fd);

	if (opt.set)
		ptp_control(fd);

	if (opt.read)
		get_msg(fd);

	return 0;
}
