#pragma once
#ifdef LIMECONTROLDLL_EXPORTS
#define RECEIVE_API __declspec(dllexport)
#else
#define RECEIVE_API __declspec(dllimport)
#endif
#include "lime/LimeSuite.h"
extern "C" 
{
	RECEIVE_API int ignite_lime();
	RECEIVE_API int rec(int16_t** sample_ptr);
	RECEIVE_API int set_rx_length_d(double new_rec_length);
	RECEIVE_API int set_rx_length_i(int new_rec_length);
	RECEIVE_API int set_sr(double new_sr);
	RECEIVE_API int set_ch(int new_ch);
	RECEIVE_API int set_offset(double new_offset);
	RECEIVE_API int set_ng(double new_ng);
	RECEIVE_API int set_lpfbw(double new_bw);
	RECEIVE_API int set_params(const double rec_length, 
		const double sample_rate, const double freq_offset,
		const int channel, const double norm_gain, const double lpfbw);
}

void sleep(const long millis);
int error();

