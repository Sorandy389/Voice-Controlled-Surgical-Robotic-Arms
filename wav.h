#ifndef WAV_H
#define WAV_H

#include <windows.h> 
#include <mmsystem.h>
#include <iostream>
#include<fstream> 
#include<math.h>

#define WAVE_HEAD_LENGTH 44//wav头文件长度
#define m_samplefreq 8000
#define m_channels 1
#define m_channelbits 16

using namespace std;
//.wav文件的文件头结构 
int Create(char *input, int lenInBytes, char *file);

#endif