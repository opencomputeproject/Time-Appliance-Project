#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "portaudio.h"
#include <string.h>
#include <sys/timerfd.h>


// WAVE file header format
struct HEADER {
	unsigned char riff[4];						// RIFF string
	unsigned int overall_size	;				// overall size of file in bytes
	unsigned char wave[4];						// WAVE string
	unsigned char fmt_chunk_marker[4];			// fmt string with trailing null char
	unsigned int length_of_fmt;					// length of the format data
	unsigned int format_type;					// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	unsigned int channels;						// no.of channels
	unsigned int sample_rate;					// sampling rate (blocks per second)
	unsigned int byterate;						// SampleRate * NumChannels * BitsPerSample/8
	unsigned int block_align;					// NumChannels * BitsPerSample/8
	unsigned int bits_per_sample;				// bits per sample, 8- 8bits, 16- 16 bits etc
	unsigned char data_chunk_header [4];		// DATA string or FLLR string
	unsigned int data_size;						// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};


#define CHUNK_SIZE 1024
#define FRAMES_PER_BUFFER (CHUNK_SIZE / (header.channels * (header.bits_per_sample/8)))
#define NUM_FRAMES(x) (x / size_of_each_sample)


/**
 * Convert seconds into hh:mm:ss format
 * Params:
 *	seconds - seconds value
 * Returns: hms - formatted string
 **/
 char* seconds_to_time(float raw_seconds) {
	char *hms;
	int hours, hours_residue, minutes, seconds, milliseconds;
	hms = (char*) malloc(100);

	sprintf(hms, "%f", raw_seconds);

	hours = (int) raw_seconds/3600;
	hours_residue = (int) raw_seconds % 3600;
	minutes = hours_residue/60;
	seconds = hours_residue % 60;
	milliseconds = 0;

	// get the decimal part of raw_seconds to get milliseconds
	char *pos;
	pos = strchr(hms, '.');
	int ipos = (int) (pos - hms);
	char decimalpart[15];
	memset(decimalpart, ' ', sizeof(decimalpart));
	strncpy(decimalpart, &hms[ipos+1], 3);
	milliseconds = atoi(decimalpart);	


	sprintf(hms, "%d:%d:%d.%d", hours, minutes, seconds, milliseconds);
	return hms;
}



// http://truelogic.org/wordpress/2015/09/04/parsing-a-wav-file-in-c/
int main(int argc, char ** argv) {
	int err = 0;
	PaStream *stream;
	char filename[1024];
	char device[16];
	FILE * ptr;
	int read = 0;
	unsigned long nano_offset = 0;
	struct HEADER header;
	char c;
	unsigned char buffer4[4];
	unsigned char buffer2[2];
	unsigned char soundbuf[CHUNK_SIZE*16];

	printf("Start!\n");


	if (argc < 5) {
		printf("Not enough arguments!\n");
		printf("%s -d <devicenum> -f <file> [-o <nanosecond offset>]\n", argv[1]);
		printf("Use python3.9 test_audio_devices.py to find devicenum\n");
		printf("By default will start audio on the next multiple of 30 seconds\n");
		printf("BE SURE TO ADD snd_timer.timer_tstamp_monotonic=0 TO GRUB CMDLINE\n");
		return 0;
	}
	while ((c = getopt (argc, argv, "d:f:o:")) != -1) {
		switch (c)
		{
		case 'd':
			strcpy(device,optarg);
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'o':
			sscanf(optarg, "%lu", &nano_offset);
			printf("Adding start offset %lu\n", nano_offset);
			break;
		default:
			break;
		}
	}
	


	ptr = fopen(filename, "rb");
	if (ptr == NULL) {
		printf("Error opening file %s\n", filename);
		return 0;
	}
	// read header
	read = fread(header.riff, sizeof(header.riff), 1, ptr);
	printf("(1-4): %s \n", header.riff);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	// convert little endian to big endian 4 byte int
	 header.overall_size  = buffer4[0] | 
		(buffer4[1]<<8) | 
		(buffer4[2]<<16) | 
		(buffer4[3]<<24);

	 printf("(5-8) Overall size: bytes:%u, Kb:%u \n", header.overall_size, header.overall_size/1024);


	 read = fread(header.wave, sizeof(header.wave), 1, ptr);
	 printf("(9-12) Wave marker: %s\n", header.wave);

	 read = fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, ptr);
	 printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

	 read = fread(buffer4, sizeof(buffer4), 1, ptr);
	 printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	 // convert little endian to big endian 4 byte integer
	 header.length_of_fmt = buffer4[0] |
		(buffer4[1] << 8) |
		(buffer4[2] << 16) |
		(buffer4[3] << 24);
	printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);

	read = fread(buffer2, sizeof(buffer2), 1, ptr); printf("%u %u \n", buffer2[0], buffer2[1]);

	header.format_type = buffer2[0] | (buffer2[1] << 8);
	char format_name[10] = "";
	if (header.format_type == 1)
	strcpy(format_name,"PCM"); 
	else if (header.format_type == 6)
	strcpy(format_name, "A-law");
	else if (header.format_type == 7)
	strcpy(format_name, "Mu-law");

	printf("(21-22) Format type: %u %s \n", header.format_type, format_name);

	read = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.channels = buffer2[0] | (buffer2[1] << 8);
	printf("(23-24) Channels: %u \n", header.channels);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.sample_rate = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);

	printf("(25-28) Sample rate: %un", header.sample_rate);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.byterate  = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);
	printf("(29-32) Byte Rate: %u , Bit Rate:%u\n", header.byterate, header.byterate*8);

	read = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.block_align = buffer2[0] |
					(buffer2[1] << 8);
	printf("(33-34) Block Alignment: %u \n", header.block_align);

	read = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.bits_per_sample = buffer2[0] |
					(buffer2[1] << 8);
	printf("(35-36) Bits per sample: %u \n", header.bits_per_sample);

	read = fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, ptr);
	printf("(37-40) Data Marker: %s \n", header.data_chunk_header);

	read = fread(buffer4, sizeof(buffer4), 1, ptr);
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.data_size = buffer4[0] |
				(buffer4[1] << 8) |
				(buffer4[2] << 16) | 
				(buffer4[3] << 24 );
	printf("(41-44) Size of data chunk: %u \n", header.data_size);


	// calculate no.of samples
	long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
	printf("Number of samples:%lu \n", num_samples);

	long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
	printf("Size of each sample:%ld bytes\n", size_of_each_sample);

	// calculate duration of file
	float duration_in_seconds = (float) header.overall_size / header.byterate;
	printf("Approx.Duration in seconds=%f\n", duration_in_seconds);
	printf("Approx.Duration in h:m:s=%s\n", seconds_to_time(duration_in_seconds));



	err = Pa_Initialize();
	if( err != paNoError ) {
		printf("Portaudio initialize error %d\n", err);
		return 0;
	}



	/* Open an audio I/O stream. */
	err = Pa_OpenDefaultStream( &stream,
		0,          /* no input channels */
		header.channels,          /* stereo output */
		paInt16,  /* 16 bit integer output */
		header.sample_rate,    /* Sample rate */
		FRAMES_PER_BUFFER,        /* frames per buffer, i.e. the number
				   of sample frames that PortAudio will
				   request from the callback. Many apps
				   may want to use
				   paFramesPerBufferUnspecified, which
				   tells PortAudio to pick the best,
				   possibly changing, buffer size.*/
		0, /* this is your callback function */
		0 ); /*This is a pointer that will be passed to
				   your callback*/



	Pa_StartStream(stream);


	/* Use Linux kernel timerfd to synchronize start */
	int timerfd = 0;
	long seconds = 0;
	fd_set rfds;
	struct timespec now;
	struct itimerspec timerval;
	int first = 1;
	timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
	clock_gettime(CLOCK_REALTIME, &now);
	printf("Now seconds %lu\n", now.tv_sec);
	// set nanoseconds to zero
	now.tv_nsec = 0;
	// set seconds to next 30 seconds
	seconds = now.tv_sec % 30;
	now.tv_sec += (30 - seconds);
	printf("Waiting until %lu sec %lu nsec\n", now.tv_sec, nano_offset);

	timerval.it_value.tv_sec = now.tv_sec;
	timerval.it_value.tv_nsec = nano_offset; // usually zero, add offset here
	timerval.it_interval.tv_sec = 0;
	timerval.it_interval.tv_nsec = 0;

	timerfd_settime(timerfd, TFD_TIMER_ABSTIME,
		&timerval,
		NULL);
			
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	FD_SET(timerfd, &rfds);

	 // read each sample from data chunk if PCM
	 if (header.format_type == 1) { // PCM
		long i =0;

		// make sure that the bytes-per-sample is completely divisible by num.of channels
		long bytes_in_each_channel = (size_of_each_sample / header.channels);
		if ((bytes_in_each_channel  * header.channels) != size_of_each_sample) {
			printf("Error: %ld x %ud <> %ldn", bytes_in_each_channel, header.channels, size_of_each_sample);
		}
		for (i =1; i <= num_samples; i++) {
			read = fread(soundbuf, 1, size_of_each_sample*CHUNK_SIZE, ptr);
			//printf("Write chunk %d read %d samplesize %d\n", i, read, size_of_each_sample);
			if ( first == 1 ) {
				//printf("Waiting!\n");
				select(timerfd+1, &rfds, NULL, NULL, NULL); /* Last parameter = NULL --> wait forever */
			}
			if ( (read > 0 ) && ((read % size_of_each_sample)==0) ) {
				Pa_WriteStream(stream, soundbuf, NUM_FRAMES(read));
			}
			if ( first == 1 ) {
				close(timerfd);
				first = 0;
				//clock_gettime(CLOCK_REALTIME, &now);
				//printf("Done waiting at %d sec %d nsec!\n", now.tv_sec, now.tv_nsec);
			}
		} // 	for (i =1; i <= num_samples; i++) {
	 } //  if (header.format_type == 1) { 



	fclose(ptr);



	printf("Done!\n");
	return 0;
}
