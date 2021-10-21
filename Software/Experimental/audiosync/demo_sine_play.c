
    /*
     * $Id$
     *
     * This program uses the PortAudio Portable Audio Library.
     * For more information see: http://www.portaudio.com/
     * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
     *
     * Permission is hereby granted, free of charge, to any person obtaining
     * a copy of this software and associated documentation files
     * (the "Software"), to deal in the Software without restriction,
     * including without limitation the rights to use, copy, modify, merge,
     * publish, distribute, sublicense, and/or sell copies of the Software,
     * and to permit persons to whom the Software is furnished to do so,
     * subject to the following conditions:
     *
     * The above copyright notice and this permission notice shall be
     * included in all copies or substantial portions of the Software.
     *
     * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
     * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
     * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
     * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
     * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
     * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
     * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
     */

    /*
     * The text above constitutes the entire PortAudio license; however, 
     * the PortAudio community also makes the following non-binding requests:
     *
     * Any person wishing to distribute modifications to the Software is
     * requested to send the modifications to the original developer so that
     * they can be incorporated into the canonical version. It is also 
     * requested that these non-binding requests be included along with the 
     * license above.
     */

#include <stdio.h>
#include <math.h>
#include "portaudio.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <string.h>

#define NUM_SECONDS         (1)
#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (128)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

// make TABLE_SIZE the same as SAMPLE_RATE , then left_inc is the frequency
#define TABLE_SIZE   (SAMPLE_RATE)
float sine[TABLE_SIZE];         /* sine wavetable */

#define CHORD_FREQUENCY_COUNT 5
struct chord {
    int frequency_count;
    int frequencies[CHORD_FREQUENCY_COUNT];
    int phases[2][CHORD_FREQUENCY_COUNT];
    float amplitudes[CHORD_FREQUENCY_COUNT];
};

struct chord C_chord = { 3, {262, 330, 392, 0, 0}, {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, {0.1, 0.1, 0.1, 0, 0} };
struct chord A_chord = { 1, {600, 233, 247, 262}, {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, {0.1, 0.1, 0.1, 0.1, 0} };
struct chord G_chord = { 1, {370, 392, 415, 440}, {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, {0.1, 0.1, 0.1, 0.1, 0} };

struct music {
    int chord_count;
    struct chord *chords[];
};

struct music demo = { 2, {&C_chord, &A_chord, &G_chord} };

void fill_chord(int channel, struct chord *chord, float buffer[FRAMES_PER_BUFFER][2])
{
    int j = 0;
    int k = 0;
    for (j = 0; j < FRAMES_PER_BUFFER; j++) {
        buffer[j][channel] = 0;
        for (k = 0; k < chord->frequency_count; k++) {
            buffer[j][channel] += chord->amplitudes[k] * sine[chord->phases[channel][k]];
            chord->phases[channel][k] += chord->frequencies[k];
            if (chord->phases[channel][k] >= TABLE_SIZE)
                chord->phases[channel][k] -= TABLE_SIZE;
        }

    }
}

int main(int argc, char ** argv)
{
    PaStreamParameters outputParameters;
    const PaDeviceInfo * paDev = NULL;
    PaStream *stream;
	int devnum = -1;
    PaError err;
	char device[16];
	char c;
    float buffer[FRAMES_PER_BUFFER][2]; /* stereo output buffer */

    int initial_left_phase = 0;
    int initial_right_phase = 0;
    int i;
    int bufferCount;
    int sync_time = 10;

	unsigned long nano_offset = 0;

	strcpy(device, "");
	while ((c = getopt (argc, argv, "s:l:r:d:o:")) != -1) {
		switch (c)
		{
		case 's':
			sscanf(optarg, "%d", &sync_time);
			printf("Starting at the nearest multiple of %d seconds\n", sync_time);
			break;
		case 'd':
			strcpy(device,optarg);
			break;
		case 'o':
			sscanf(optarg, "%lu", &nano_offset);
			printf("Adding start offset %lu\n", nano_offset);
			break;
		case 'l':
			sscanf(optarg, "%d", &initial_left_phase);
			printf("Adding initial left phase offset %d\n", initial_left_phase);
			break;
		case 'r':
			sscanf(optarg, "%d", &initial_right_phase);
			printf("Adding initial right phase offset %d\n", initial_right_phase);
			break;
		default:
			break;
		}
	}


	while ( initial_left_phase >= TABLE_SIZE ) {
		initial_left_phase -= TABLE_SIZE;
	}
	while ( initial_right_phase >= TABLE_SIZE ) {
		initial_right_phase -= TABLE_SIZE;
	}
	for ( i = 0; i < demo.chord_count; i++ ) {
		// add in initial phase offsets 
		for ( int f = 0; f < CHORD_FREQUENCY_COUNT; f++ ) {
			demo.chords[i]->phases[0][f] = initial_left_phase;
		       	demo.chords[i]->phases[1][f] = initial_right_phase;	
		}
	}


    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    /* initialise sinusoidal wavetable */
    for (i = 0; i < TABLE_SIZE; i++) {
        sine[i] = (float)sin(((double)i / (double)TABLE_SIZE) * M_PI * 2.);
    }

    err = Pa_Initialize();
    if (err != paNoError) {
        goto error;
    }


	if ( strlen(device) > 0 ) {
		if ( sscanf(device, "%d", &devnum) != 1 ) {
			printf("Failed to parse %s as a device number, using default\n", device);
		} else {
			paDev = Pa_GetDeviceInfo(devnum);
			if ( paDev != NULL ) {
				printf("For device %d, name %s, outputChannels %d lowlatency=%f\n", devnum, paDev->name, paDev->maxOutputChannels, 
						Pa_GetDeviceInfo(devnum)->defaultLowOutputLatency);
				outputParameters.device = devnum;
			} else {
				printf("Failed to find portaudio device %d\n", devnum);
			}
		}
	} 

    if (outputParameters.device == paNoDevice) {
	fprintf(stderr, "Error: No default output device.\n");
	goto error;
    }
    outputParameters.channelCount = 2;  /* stereo output */
    outputParameters.sampleFormat = paFloat32;  /* 32 bit floating point output */

    if ( devnum == -1 ) {
	    outputParameters.device = Pa_GetDefaultOutputDevice();
    }
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device)->defaultLowOutputLatency; 
    //outputParameters.suggestedLatency = 0.050;  // Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream, NULL,  /* no input */
                        &outputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff,   /* we won't output out of range samples so don't bother clipping them */
                        NULL,   /* no callback, use blocking API */
                        NULL);  /* no callback, so no callback userData */
    if (err != paNoError)
        goto error;


    err = Pa_StartStream(stream);


	/* Use Linux kernel timerfd to synchronize start */
    	int first=1;
	int timerfd = 0;
	long seconds = 0;
	fd_set rfds;
	struct timespec now;
	struct itimerspec timerval;
	timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
	clock_gettime(CLOCK_REALTIME, &now);
	printf("Now seconds %lu\n", now.tv_sec);
	// set nanoseconds to zero
	now.tv_nsec = 0;
	// set seconds to next 10 seconds
	seconds = now.tv_sec % sync_time;
	now.tv_sec += (sync_time - seconds);
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






    while (1) {
        if (err != paNoError)
            goto error;

        for (int chord = 0; chord < demo.chord_count; chord++) {

            bufferCount = ((NUM_SECONDS * SAMPLE_RATE) / FRAMES_PER_BUFFER);
            //left_phase = 0;
            //right_phase = 0;
            for (i = 0; i < bufferCount; i++) {
                fill_chord(0, demo.chords[chord], buffer);
                fill_chord(1, demo.chords[chord], buffer);
		if ( first == 1 ) {
			select(timerfd+1, &rfds, NULL, NULL, NULL); /* Last parameter = NULL --> wait forever */
		}
                err = Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER);
		if ( first == 1 ) {
			close(timerfd);
			first = 0;
		}
                if (err != paNoError)
                    goto error;
            }

            if (err != paNoError)
                goto error;
        }

    }

    err = Pa_CloseStream(stream);
    if (err != paNoError)
        goto error;

    Pa_Terminate();
    printf("Test finished.\n");

    return err;

 error:
    fprintf(stderr, "An error occured while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    // Print more information about the error.
    if (err == paUnanticipatedHostError) {
        const PaHostErrorInfo *hostErrorInfo = Pa_GetLastHostErrorInfo();
        fprintf(stderr, "Host API error = #%ld, hostApiType = %d\n",
                hostErrorInfo->errorCode, hostErrorInfo->hostApiType);
        fprintf(stderr, "Host API error = %s\n", hostErrorInfo->errorText);
    }
    Pa_Terminate();
    return err;
}
