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

int set_rx_length_i(int new_rec_length)
{
	if (buffer != nullptr)
		delete[] buffer;
	rx_length = new_rec_length;
	sampleCnt = sampling_rate * int(rx_length);
	buffer = new int16_t[sampleCnt * 2L];
	return 1;
}

int set_rx_length_d(double new_rec_length)
{
	if (buffer != nullptr)
		delete[] buffer;
	rx_length = new_rec_length;
	sampleCnt = sampling_rate * rx_length;
	buffer = new int16_t[sampleCnt * 2L];
	return 1;
}

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

int receive_config(int16_t** rec_arr, const char *config_name)
{
	// TODO or not TODO
	return -1;
}

int receive(int16_t** rec_arr, const double rec_length, const double sample_rate, const double freq_offset)
{
	const int CHANNEL = 0;
	const int NORM_GAIN = 1;
	const double LPFBW = 1.41e6;
	return receive_main(rec_arr, rec_length, sample_rate, freq_offset, CHANNEL, NORM_GAIN, LPFBW);
}

int receive_main(int16_t** rec_arr, const double rec_length, const double sampling_rate, const double freq_offset, const int channel, const int norm_gain, const double lpfbw)
{
#ifdef _WIN32
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
	double frequency = 63.795e6 + freq_offset;//10e3;
	const double sample_rate = sampling_rate;// 0.5e6;
	const int rx_channel = channel;
	int n;
	lms_info_str_t list[8];
	if ((n = LMS_GetDeviceList(list)) < 0)
		error();
	if (n < 1)
		return -1;
	if (LMS_Open(&device, list[0], NULL))
		error();

	if (LMS_Init(device) != 0)
		error();
	if (LMS_EnableChannel(device, LMS_CH_RX, rx_channel, true) != 0)
		error();
	if (LMS_SetSampleRate(device, sample_rate, 0) != 0)
		error();
	if (LMS_SetClockFreq(device, LMS_CLOCK_EXTREF, 10e6) != 0)
		error();
	if (LMS_SetLOFrequency(device, LMS_CH_RX, rx_channel, frequency) != 0)
		error();
	if (LMS_SetAntenna(device, LMS_CH_RX, rx_channel, LMS_PATH_LNAL) != 0)
		error();
	if (LMS_SetNormalizedGain(device, LMS_CH_RX, rx_channel, norm_gain) != 0)
		error();
	if (LMS_SetLPFBW(device, LMS_CH_RX, rx_channel, lpfbw))
		error();

	uint8_t gpio_dir = 0xFF;
	uint8_t zeros = 0x00;
	lms_stream_t streamId; //stream structure
	streamId.channel = rx_channel; //channel number
	streamId.fifoSize = 2048 * 4096; //fifo size in samples
	streamId.throughputVsLatency = 0; //optimize for max throughput
	streamId.isTx = false; //RX channel
	streamId.dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers

	if (LMS_SetupStream(device, &streamId) != 0)
		error();

	const long sampleCnt = sample_rate * rec_length; //complex samples per buffer
	int16_t*  buffer = new int16_t[sampleCnt * 2.0]; //buffer to hold complex values (2*samples))

	LMS_Calibrate(device, LMS_CH_RX, rx_channel, sample_rate, 0);
	//Streaming
	LMS_StartStream(&streamId);
	LMS_StopStream(&streamId);
	uint8_t gpio_buffer = 0x00;

	//MAIN RECEIVE LOOP
	int samplesRead = 0;
	for (int i = 0; i < 1; i++)
	{
		LMS_StartStream(&streamId);
		samplesRead = LMS_RecvStream(&streamId, buffer, sampleCnt, nullptr, 1000);
		LMS_StopStream(&streamId);
	}
	LMS_StopStream(&streamId);
	LMS_DestroyStream(device, &streamId);
	if (LMS_EnableChannel(device, LMS_CH_RX, rx_channel, false) != 0)
		error();

	*rec_arr = buffer;
	return samplesRead;
}