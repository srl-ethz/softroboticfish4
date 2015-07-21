#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <complex.h>

#include "rtl-sdr.h"
#include "convenience/convenience.h"
#include "demod.h"

#define DEFAULT_SAMPLE_RATE		256000
#define DEFAULT_SAMPLE_LENGTH		1024*40

#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)

static int do_exit = 0;
static uint32_t bytes_to_read = 0;
static rtlsdr_dev_t *dev = NULL;

void usage(void)
{
	fprintf(stderr,
		"rtl_sdr, an I/Q recorder for RTL2832 based DVB-T receivers\n\n"
		"Usage:\t -f carrier frequency_to_tune_to [Hz]\n"
		"\t[-b bandwidth (default: 1000)]\n"
		"\t[-s samples per symbol (default: 4)]\n"
		"\t[-d device_index (default: 0)]\n"
		"\t[-c direct sampling channel (default: 0=none; i=1,q=2)]\n"
		"\t[-n number of samples to read (default: 0, infinite)]\n"
		"\t[-S force sync output (default: async)]\n"
		"\tfilename (a '-' dumps samples to stdout)\n\n");
	exit(1);
}

static void sighandler(int signum)
{
	fprintf(stderr, "Signal caught, exiting!\n");
	do_exit = 1;
	rtlsdr_cancel_async(dev);
}

static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
	if (ctx) {
		if (do_exit)
			return;

		if ((bytes_to_read > 0) && (bytes_to_read < len)) {
			len = bytes_to_read;
			do_exit = 1;
			rtlsdr_cancel_async(dev);
		}

                demod_callback(buf, len, ctx);

		if (fwrite(buf, 1, len, (FILE*)ctx) != len) {
			fprintf(stderr, "Short write, samples lost, exiting!\n");
			rtlsdr_cancel_async(dev);
		}

		if (bytes_to_read > 0)
			bytes_to_read -= len;
	}
}

int main(int argc, char **argv)
{
	struct sigaction sigact;
	char *filename = NULL;
	int n_read;
	int r, opt;
	int direct = 0;
	int gain = 0;
	int sps = 4;
	int bw = 1000;
	int sync_mode = 0;
	FILE *file;
	uint8_t *buffer;
	int dev_index = 0;
	int dev_given = 0;
	uint32_t carrier = 32000; //100000000;
	uint32_t samp_rate = DEFAULT_SAMPLE_RATE;
	uint32_t out_block_size = DEFAULT_SAMPLE_LENGTH*2;

	while ((opt = getopt(argc, argv, "d:s:b:f:c:n:S")) != -1) {
		switch (opt) {
		case 'd':
			dev_index = verbose_device_search(optarg);
			dev_given = 1;
			break;
		case 'f':
			carrier = (uint32_t)atofs(optarg);
			break;
                case 'c':
                        direct = atoi(optarg);
                        break;
                case 's':
                        sps = atoi(optarg);
                        break;
                case 'b':
                        bw = atoi(optarg);
                        break;
		case 'n':
			bytes_to_read = (uint32_t)atof(optarg) * 2;
			break;
		case 'S':
			sync_mode = 1;
			break;
		default:
			usage();
			break;
		}
	}

	if (argc <= optind) {
		usage();
	} else {
		filename = argv[optind];
	}

	if(out_block_size < MINIMAL_BUF_LENGTH ||
	   out_block_size > MAXIMAL_BUF_LENGTH ){
		fprintf(stderr,
			"Output block size wrong value, falling back to default\n");
		fprintf(stderr,
			"Minimal length: %u\n", MINIMAL_BUF_LENGTH);
		fprintf(stderr,
			"Maximal length: %u\n", MAXIMAL_BUF_LENGTH);
		out_block_size = DEFAULT_SAMPLE_LENGTH*2;
	}

	buffer = malloc(out_block_size * sizeof(uint8_t));

	if (!dev_given) {
		dev_index = verbose_device_search("0");
	}

	if (dev_index < 0) {
		exit(1);
	}

	r = rtlsdr_open(&dev, (uint32_t)dev_index);
	if (r < 0) {
		fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
		exit(1);
	}
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);

	/* Set direct sampling */
	verbose_direct_sampling(dev, 2);

	/* Set the sample rate */
	verbose_set_sample_rate(dev, samp_rate);

	/* Set the frequency */
	verbose_set_frequency(dev, 0);

	if (0 == gain) {
		 /* Enable automatic gain */
		verbose_auto_gain(dev);
	} else {
		/* Enable manual gain */
		gain = nearest_gain(dev, gain);
		verbose_gain_set(dev, gain);
	}

	if(strcmp(filename, "-") == 0) { /* Write samples to stdout */
		file = stdout;
	} else {
		file = fopen(filename, "wb");
		if (!file) {
			fprintf(stderr, "Failed to open %s\n", filename);
			goto out;
		}
	}

	/* Reset endpoint before we start reading from it (mandatory) */
	verbose_reset_buffer(dev);
        demod_init(samp_rate, carrier, bw, sps);

	if (sync_mode) {
		fprintf(stderr, "Reading samples in sync mode...\n");
		while (!do_exit) {
			r = rtlsdr_read_sync(dev, buffer, out_block_size, &n_read);
			if (r < 0) {
				fprintf(stderr, "WARNING: sync read failed.\n");
				break;
			}

			if ((bytes_to_read > 0) && (bytes_to_read < (uint32_t)n_read)) {
				n_read = bytes_to_read;
				do_exit = 1;
			}

			if (fwrite(buffer, 1, n_read, file) != (size_t)n_read) {
				fprintf(stderr, "Short write, samples lost, exiting!\n");
				break;
			}

			if ((uint32_t)n_read < out_block_size) {
				fprintf(stderr, "Short read, samples lost, exiting!\n");
				break;
			}

			if (bytes_to_read > 0)
				bytes_to_read -= n_read;
		}
	} else {
		fprintf(stderr, "Reading samples in async mode...\n");
		r = rtlsdr_read_async(dev, rtlsdr_callback, (void *)file,
				      0, out_block_size);
	}

	if (do_exit)
		fprintf(stderr, "\nUser cancel, exiting...\n");
	else
		fprintf(stderr, "\nLibrary error %d, exiting...\n", r);

	if (file != stdout)
		fclose(file);

        demod_close();
	rtlsdr_close(dev);
	free (buffer);
out:
	return r >= 0 ? r : -r;
}
