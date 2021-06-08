/*
Control.cpp - Implementation of the functions in Control.h

Date: 20.5.2021
Created by: Abdullah Arda Aþcý & Uður Yýlmaz
*/
#include "pch.h"
#include "Control.h"

#include <iostream>
#include <chrono>
#include <math.h>
#include "lime\LimeSuite.h"
//#include "gnuPlotPipe.h" //Uncomment for GNUPlot
#include <fstream>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

using namespace std;
// Constant values
const float_type LARMOR_FREQ = 63.795e6; // Center frequency of the 1.5 MR scanner in UMRAm
const float_type EXTREF_FREQ = 10e6; // External square wave freq
// Default values
const float_type DEF_SR = 0.5e6; // Sampling Rate 
const float_type DEF_OFFSET = 20e3; // Offset from the center frequency to avoid low freq artifacts caused by limeSDR
const int DEF_CH = 0; // Rx channel
const float_type DEF_NG = 1; // Normalized gain
const float_type DEF_LPFBW = 1.41e6; // Rx Low Pass Filter BandWidth
const double DEF_RX_LENGTH = 2; // receive length is seconds
//const long 

// LimeSDR device object
lms_device_t* device = NULL;
lms_stream_t* streamId = new lms_stream_t; //stream structure
// parameters
float_type sampling_rate = DEF_SR;
float_type freq_offset = DEF_OFFSET;
int rx_channel = DEF_CH;
float_type norm_gain = DEF_NG;
float_type lpf_bw = DEF_LPFBW;
double rx_length = DEF_RX_LENGTH;
long long sampleCnt = sampling_rate * rx_length;
int16_t* buffer = new int16_t[sampleCnt * long(2)];

/**
* Find the connected LimeSDR board and pass the parameters on the API to the card
*/
int ignite_lime()
{
	double frequency = LARMOR_FREQ + freq_offset;
	int n;
	lms_info_str_t list[8];
	if ((n = LMS_GetDeviceList(list)) < 0)
		return error();
	if (n < 1)
		return -1;
	if (LMS_Open(&device, list[0], NULL))
		return error();

	if (LMS_Init(device) != 0)
		return error();
	if (LMS_EnableChannel(device, LMS_CH_RX, rx_channel, true) != 0)
		return error();
	if (LMS_SetSampleRate(device, sampling_rate, 0) != 0)
		return error();
	if (LMS_SetClockFreq(device, LMS_CLOCK_EXTREF, EXTREF_FREQ) != 0)
		return error();
	if (LMS_SetLOFrequency(device, LMS_CH_RX, rx_channel, frequency) != 0)
		return error();
	if (LMS_SetAntenna(device, LMS_CH_RX, rx_channel, LMS_PATH_LNAL) != 0)
		return error();
	if (LMS_SetNormalizedGain(device, LMS_CH_RX, rx_channel, norm_gain) != 0)
		return error();
	if (LMS_SetLPFBW(device, LMS_CH_RX, rx_channel, lpf_bw))
		return error();

	streamId->channel = rx_channel;
	streamId->fifoSize = 2048 * 4096;
	streamId->throughputVsLatency = 0;
	streamId->isTx = false;
	streamId->dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers

	if (LMS_SetupStream(device, streamId) != 0)
		error();

	LMS_Calibrate(device, LMS_CH_RX, rx_channel, sampling_rate, 0);
	LMS_StartStream(streamId);
	LMS_StopStream(streamId);
	return 1;
}

/**
* Receive sampleCnt amount of samples
*/
int rec(int16_t** sample_ptr)
{
	if (buffer == nullptr)
		buffer = new int16_t[sampleCnt * 2];
	LMS_StartStream(streamId);
	int samplesRead = LMS_RecvStream(streamId, buffer, sampleCnt, nullptr, 1000);
	LMS_StopStream(streamId);
	LMS_DestroyStream(device, streamId);
	if (LMS_EnableChannel(device, LMS_CH_RX, rx_channel, false) != 0)
		return error();
	*sample_ptr = buffer;
	return samplesRead;
}

/**
* Set the receive length with an integer
*/
int set_rx_length_i(int new_rec_length)
{
	if (buffer != nullptr)
		delete[] buffer;
	rx_length = new_rec_length;
	sampleCnt = sampling_rate * int(rx_length);
	buffer = new int16_t[sampleCnt * 2L];
	return 1;
}

/**
* Set the receive length with a double
*/
int set_rx_length_d(double new_rec_length)
{
	if (buffer != nullptr)
		delete[] buffer;
	rx_length = new_rec_length;
	sampleCnt = sampling_rate * rx_length;
	buffer = new int16_t[sampleCnt * 2L];
	return 1;
}

//	BASIC SETTER FUNCTIONS
int set_sr(double new_sr)
{
	sampling_rate = new_sr;
	return 1;
}

int set_ch(int new_ch)
{
	rx_channel = new_ch;
	return 1;
}

int set_offset(double new_offset)
{
	freq_offset = new_offset;
	return 1;
}

int set_ng(double new_ng)
{
	norm_gain = new_ng;
	return 1;
}

int set_lpfbw(double new_bw)
{
	lpf_bw = new_bw;
	return 1;
}

/**
* Call before rec() and ignite_lime() to set the parameters in the API
*/
int set_params(const double rec_length, const double sample_rate, const double freq_offset, const int channel, const double norm_gain, const double lpfbw)
{
	if (set_rx_length_d(rec_length) != 1)
		return -1;
	if (set_sr(sample_rate) != 1)
		return -1;
	if (set_offset(freq_offset) != 1)
		return -1;
	if (set_ch(channel) != 1)
		return -1;
	if (set_ng(norm_gain) != 1)
		return -1;
	if (set_lpfbw(lpfbw) != 1)
		return -1;
	return 1;
}

void sleep(const long millis)
{
#ifdef _WIN32
	Sleep(millis);
#else
	usleep(millis * 1000);
#endif
}

int error()
{
	if (device != nullptr)
		LMS_Close(device);
	return -1;
}


