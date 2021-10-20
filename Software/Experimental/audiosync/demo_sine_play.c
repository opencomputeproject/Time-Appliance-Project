
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

#include <sys/timerfd.h>

#define NUM_SECONDS         (1)
#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (128)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

// make TABLE_SIZE the same as SAMPLE_RATE , then left_inc is the frequency
#define TABLE_SIZE   (SAMPLE_RATE)
float sine[TABLE_SIZE];	/* sine wavetable */

struct chord {
	int frequency_count;
	int frequencies[5];
	int phases[2][5];
	float amplitudes[5];
};

struct chord C_chord = { 3, {262,330,392,0,0}, {{0,0,0,0,0}, {0,0,0,0,0}}, {0.1, 0.1, 0.1,0,0} };
struct chord A_chord = { 1, {600,233,247,262}, {{0,0,0,0,0}, {0,0,0,0,0}}, {0.1, 0.1, 0.1,0.1,0} };
struct chord G_chord = { 1, {370,392, 415, 440}, {{0,0,0,0,0}, {0,0,0,0,0}}, {0.1, 0.1, 0.1, 0.1, 0} }; 

struct music { 
	int chord_count;
	struct chord * chords[];
};

struct music demo = {3, {&C_chord, &A_chord, &G_chord} }; 



void fill_chord(int channel, struct chord * chord, float buffer[FRAMES_PER_BUFFER][2] ) {
	int j = 0;
	int k = 0;
	for ( j = 0; j < FRAMES_PER_BUFFER; j++ ) {
		buffer[j][channel] = 0;
		for ( k = 0; k < chord->frequency_count; k++ ) {
			buffer[j][channel] += chord->amplitudes[k] * sine[ chord->phases[channel][k] ];
			chord->phases[channel][k] += chord->frequencies[k];
			if ( chord->phases[channel][k] >= TABLE_SIZE ) 
				chord->phases[channel][k] -= TABLE_SIZE;
		}

	}
}


int main (void);
int
main (void)
{
  PaStreamParameters outputParameters;
  PaStream *stream;
  PaError err;
  float buffer[FRAMES_PER_BUFFER][2];	/* stereo output buffer */
  int left_phase = 0;
  int right_phase = 0;



  // middle C , E, G
  int left_frequencies_count = 3;
  int left_frequencies[] = {262, 330, 392};
  float left_amplitudes[] = {0.1, 0.1, 0.1};
  int left_phases[] = {0,0,0};


  int right_frequencies_count = 3;
  int right_frequencies[] = {262, 330, 392};
  float right_amplitudes[] = {0.1, 0.1, 0.1};
  int right_phases[] = {0,0,0};


  int i, j, k;
  int bufferCount;

  printf ("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n",
	  SAMPLE_RATE, FRAMES_PER_BUFFER);

  /* initialise sinusoidal wavetable */
  for (i = 0; i < TABLE_SIZE; i++)
    {
      sine[i] = (float) sin (((double) i / (double) TABLE_SIZE) * M_PI * 2.);
    }


  err = Pa_Initialize ();
  if (err != paNoError)
    goto error;

  outputParameters.device = Pa_GetDefaultOutputDevice ();	/* default output device */
  if (outputParameters.device == paNoDevice)
    {
      fprintf (stderr, "Error: No default output device.\n");
      goto error;
    }
  outputParameters.channelCount = 2;	/* stereo output */
  outputParameters.sampleFormat = paFloat32;	/* 32 bit floating point output */
  outputParameters.suggestedLatency = 0.050;	// Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = NULL;

  err = Pa_OpenStream (&stream, NULL,	/* no input */
		       &outputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff,	/* we won't output out of range samples so don't bother clipping them */
		       NULL,	/* no callback, use blocking API */
		       NULL);	/* no callback, so no callback userData */
  if (err != paNoError)
    goto error;


  printf ("Play 3 times, higher each time.\n");




  err = Pa_StartStream (stream);
  float left_amplitude = 0.25;
  float right_amplitude = 0.25;
  while (1)
    {
	  if (err != paNoError)
	    goto error;

	  printf ("Play for %f seconds.\n", NUM_SECONDS);

	for ( int chord = 0; chord < demo.chord_count; chord++) {
		
		  bufferCount = ((NUM_SECONDS * SAMPLE_RATE) / FRAMES_PER_BUFFER);
		  //left_phase = 0;
		  //right_phase = 0;
		  for (i = 0; i < bufferCount; i++)
		    {
			    fill_chord(0, demo.chords[chord], buffer);
			    fill_chord(1, demo.chords[chord], buffer);
		      for (j = 0; j < FRAMES_PER_BUFFER; j++)
			{
				break;	
				buffer[j][0] = 0;
				buffer[j][1] = 0;
				/*
				buffer[j][0] = sine[ left_phase ];
				left_phase += left_frequencies[freq];
				if ( left_phase >= TABLE_SIZE )
					left_phase -= TABLE_SIZE;
				buffer[j][0] *= left_amplitude;
				*/
				for ( k =0; k < left_frequencies_count; k++ ) {
				  buffer[j][0] += left_amplitudes[k] * sine[ left_phases[k] ];	// left 
				  left_phases[k] += left_frequencies[k];
				  if (left_phases[k] >= TABLE_SIZE)
				    left_phases[k] -= TABLE_SIZE;
				}
				
				//printf("Write left buffer %d\n", j);
				/*
				buffer[j][1] = 0;
				buffer[j][1] = sine[ right_phase ];
				right_phase += right_frequencies[freq];
				if ( right_phase >= TABLE_SIZE )
					right_phase -= TABLE_SIZE;
				buffer[j][1] *= right_amplitude;
				*/
				for ( k =0; k < right_frequencies_count; k++ ) {
				  buffer[j][1] += right_amplitudes[k] * sine[ right_phases[k] ];	// right 
				  right_phases[k] += right_frequencies[k];
				  if (right_phases[k] >= TABLE_SIZE)
				    right_phases[k] -= TABLE_SIZE;
				}
				//printf("Write right buffer %d\n", j);
			}
		      err = Pa_WriteStream (stream, buffer, FRAMES_PER_BUFFER);
		      if (err != paNoError)
			goto error;
		    }

		  if (err != paNoError)
		    goto error;
	}

    }

  err = Pa_CloseStream (stream);
  if (err != paNoError)
    goto error;

  Pa_Terminate ();
  printf ("Test finished.\n");

  return err;

error:
  fprintf (stderr, "An error occured while using the portaudio stream\n");
  fprintf (stderr, "Error number: %d\n", err);
  fprintf (stderr, "Error message: %s\n", Pa_GetErrorText (err));
  // Print more information about the error.
  if (err == paUnanticipatedHostError)
    {
      const PaHostErrorInfo *hostErrorInfo = Pa_GetLastHostErrorInfo ();
      fprintf (stderr, "Host API error = #%ld, hostApiType = %d\n",
	       hostErrorInfo->errorCode, hostErrorInfo->hostApiType);
      fprintf (stderr, "Host API error = %s\n", hostErrorInfo->errorText);
    }
  Pa_Terminate ();
  return err;
}
