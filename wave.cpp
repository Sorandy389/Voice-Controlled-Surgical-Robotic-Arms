/*
#include "pch.h"
#include "MFCLibrary1.h"
#include <mmsystem.h>
#include <math.h>
#include <dsound.h>
#include "wave.h"
*/

// #include "pch.h"
//#include "MFCLibrary1.h"
// #include <mmsystem.h>
// #include <math.h>
// #include <mex.h>
#include "wave.h"


void wave_reset() {
    flag_word_start = false;
    flag_poss_start = false;
    flag_word_end = false;
    flag_user_stop = false;

    amp_high = 10.0;
    amp_low = 2.0;
    zcr_high = 10.0;
    zcr_low = 5.0;
    zcr_thresold = 0.02;

    min_word_length = 50;
    max_silence_time = 25;
    silence_time = 0;
    word_length = 0;

    buff_begin = 0;
    buff_end = 0;
    buff_ptr = 0;
}

void wave_filter(double* buff) {
    static double tmp1 = 0, tmp2 = 0;

    for (int i = 0; i < FRAME_LEN; i++) {
        tmp1 = *buff - tmp2 * 0.9375;
        tmp2 = *buff;
        *(buff + 1) = tmp1;
    }
}

void wave_energy(double* buff) {
    amp = 0.0;
    for (int i = 0; i < FRAME_LEN; i++)
        amp += fabs(*buff++);

    if (word_length > 0)
        amp_buf[word_length - 1] = amp;
}

void wave_zcr(double* buff) {
    zcr = 0;
    for (int i = 0; i < FRAME_LEN - 1; i++) {
        double tmp1 = *buff++;
        double tmp2 = *buff;

        if (tmp1 * tmp2<0 && fabs(tmp1 - tmp2)>zcr_thresold)
            zcr++;
    }

    if (word_length > 0)
        zcr_buf[word_length - 1] = zcr;
}

int wave_vad() {
    if (flag_word_start) {
        if (amp >= amp_low || zcr >= zcr_low)
            goto set_word_start;

        silence_time++;
        if (silence_time < max_silence_time)
            goto inc_sp_count;

        if (word_length < min_word_length)
            goto reset_vars;

        flag_word_start = false;
        flag_word_end = true;
        word_length = silence_time / 2;
        return 1;
    }
    else {
        if (amp >= amp_high || zcr >= zcr_high)
            goto set_word_start;

        if (amp >= amp_low || zcr >= zcr_low) {
            flag_poss_start = true;
            goto inc_sp_count;
        }
        else goto reset_vars;
    }

set_word_start:
    flag_word_start = true;
    flag_word_end = false;
    silence_time = 0;
inc_sp_count:
    word_length++;
    if (word_length >= FRAMES - 3) {
        word_length = FRAMES - 3;
        flag_word_start = false;
        flag_poss_start = false;
        flag_word_end = true;
        return 1;
    }
    else return 0;
reset_vars:
    flag_poss_start = false;
    flag_word_start = false;
    flag_word_end = false;
    silence_time = 0;
    word_length = 0;
    return 0;
}

void wave_append(int16* sample, int len) {
    int i, j;
    double buff[FRAME_LEN];

    if (flag_word_end) return;

    for (i = 0; i < len; i++) {
        if ((buff_end + 1) % MAX_SAMPLES == buff_ptr)
            break;

        voice_buff[buff_end + 1] = *sample++;
        buff_end %= MAX_SAMPLES;
    }

    for (;;) {
        for (i = 0, j = buff_ptr; i < FRAME_LEN; i++) {
            buff[i] = voice_buff[j++] / 32768.0;
            j %= MAX_SAMPLES;
            if (j == buff_end)
                return;
        }

        wave_zcr(buff);
        wave_filter(buff);
        wave_energy(buff);

        if (word_length == 0)
            buff_begin = buff_ptr;

        if (wave_vad())
            return;

        buff_ptr += FRAME_SHIFT;
        buff_ptr %= MAX_SAMPLES;
    }
    return;
}

// void CALLBACK waveInProc(
//     HWAVEIN hwi,
//     UINT uMsg,
//     DWORD lpsample,
//     DWORD lpwvhdr,
//     DWORD reserved) {

//     WAVEHDR* pWhdr;
//     int16* pBuff;

//     switch (uMsg) {
//     case WIM_OPEN: break;
//     case WIM_CLOSE: break;
//     case WIM_DATA:
//         pWhdr = (WAVEHDR*)lpwvhdr;
//         pBuff = (int16*)(pWhdr->lpData);
//         wave_append(pBuff, BUFFER_LEN);
//         if (flag_word_end | flag_user_stop) {
//             waveInStop(hwi);
//             waveInUnprepareHeader(hwi, &wh1, sizeof(WAVEHDR));
//             waveInUnprepareHeader(hwi, &wh2, sizeof(WAVEHDR));
//             waveInClose(hwi);
//         }
//         else
//             waveInAddBuffer(hwi, pWhdr, sizeof(WAVEHDR));
//         break;
//     }
// }

void wave_stop() {
    flag_user_stop = true;
}

// void wave_start() {
//     MMRESULT tmp;
//     WAVEFORMATEX wfx;

//     if (0 == waveInGetNumDevs()) {
//         mexWarnMsgTxt("No Audio");
//     }

//     wfx.wFormatTag = WAVE_FORMAT_PCM;
//     wfx.nChannels = 1;
//     wfx.nSamplesPerSec = 8000;
//     wfx.nAvgBytesPerSec = 16000;
//     wfx.nBlockAlign = 2;
//     wfx.wBitsPerSample = 16;
//     wfx.cbSize = 0;

//     tmp = waveInOpen(
//         NULL,
//         0,
//         &wfx,
//         NULL,
//         NULL,
//         WAVE_FORMAT_QUERY);

//     if (tmp != MMSYSERR_NOERROR) {
//         mexWarnMsgTxt("Input Port does not support PCM 8kHz 16bit");
//         return;
//     }

//     tmp = waveInOpen(&hwi, 0, &wfx, (DWORD)&waveInProc, NULL, CALLBACK_FUNCTION);
//     if (tmp != MMSYSERR_NOERROR) {
//         mexWarnMsgTxt("Input Port cannot open");
//         return;
//     }

//     wh1.lpData = (char*)&frame_buff_a;
//     wh1.dwBufferLength = BUFFER_LEN * 2;
//     wh1.dwBytesRecorded = NULL;
//     wh1.dwUser = NULL;
//     wh1.dwFlags = NULL;
//     wh1.dwLoops = NULL;
//     wh1.lpNext = NULL;
//     wh1.reserved = NULL;

//     wh2.lpData = (char*)&frame_buff_b;
//     wh2.dwBufferLength = BUFFER_LEN * 2;
//     wh2.dwBytesRecorded = NULL;
//     wh2.dwUser = NULL;
//     wh2.dwFlags = NULL;
//     wh2.dwLoops = NULL;
//     wh2.lpNext = NULL;
//     wh2.reserved = NULL;

//     waveInPrepareHeader(hwi, &wh1, sizeof(WAVEHDR));
//     waveInPrepareHeader(hwi, &wh2, sizeof(WAVEHDR));
//     waveInAddBuffer(hwi, &wh1, sizeof(WAVEHDR));
//     waveInAddBuffer(hwi, &wh2, sizeof(WAVEHDR));

//     wave_reset();

//     waveInStart(hwi);
// }

int wave_get_len() {
    if (flag_word_end) {
        return word_length * FRAME_SHIFT;
    }
    else return 0;
}

int wave_get_frames() {
    if (flag_word_end) {
        return word_length;
    }
    else return 0;
}

void wave_get_data(double* buff) {
    for (int i = 0; i < word_length * FRAME_SHIFT; i++) {
        int ptr = (i + buff_begin) % MAX_SAMPLES;
        *buff++ = voice_buff[ptr];
    }
}

void wave_get_zcr(double* buff) {
    for (int i = 0; i < word_length; i++)
        *buff++ = zcr_buf[i];
}

void wave_get_energy(double* buff) {
    for (int i = 0; i < word_length; i++)
        *buff++ = amp_buf[i];
}