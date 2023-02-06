/*
#include "pch.h"
#include <mmsystem.h>
#include <math.h>
#include <dsound.h>
#include "MFCLibrary1.h"
*/
#ifndef WAVE_H
#define WAVE_H

#include<windows.h>
#include<ctime>
// #include<vector>
// #include<complex>
#include<cmath>
// #include<string>
#include<iostream>
// #include<stdlib.h>
// #include<malloc.h>
// #include<bitset>
using   namespace   std;

typedef int16_t int16;

#define FRAME_LEN   240
#define FRAME_SHIFT 80
#define FRAMES      300*10000
#define BUFFER_LEN  2400*10000
#define MAX_SAMPLES (FRAMES*FRAME_LEN)


// HWAVEIN hwi;
// WAVEHDR wh1, wh2;

// int16 frame_buff_a[BUFFER_LEN];
// int16 frame_buff_b[BUFFER_LEN];

static int buff_begin, buff_end, buff_ptr;

static bool flag_word_start;
static bool flag_word_end;
static bool flag_poss_start;
static bool flag_user_stop;

static int max_silence_time;
static int min_word_length;
static int silence_time;
static int word_length;

static double amp, amp_high, amp_low, amp_buf[FRAMES];
static double zcr, zcr_high, zcr_low, zcr_buf[FRAMES];
static double zcr_thresold;

void wave_reset();

// void wave_filter(double* buff);

// void wave_energy(double* buff);

// void wave_zcr(double* buff);

// int wave_vad();

// void wave_append(int16* sample, int len);

// void CALLBACK waveInProc(
//     HWAVEIN hwi,
//     UINT uMsg,
//     DWORD lpsample,
//     DWORD lpwvhdr,
//     DWORD reserved);

// void wave_stop();

// void wave_start();

// int wave_get_len();

// int wave_get_frames();

// void wave_get_data(double* buff);

// void wave_get_zcr(double* buff);

// void wave_get_energy(double* buff);

#endif