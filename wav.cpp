#include"wav.h"

typedef struct
{
    char chRIFF[4];
    DWORD dwRIFFLen;
    char chWAVE[4];
    char chFMT[4];
    DWORD dwFMTLen;
    PCMWAVEFORMAT pwf;
    char chDATA[4];
    DWORD dwDATALen;
    //UINT8* pBufer;
}WaveHeader;

int Create(char *input, int lenInBytes, char *file)//频率、音量、持续时间
{
    WaveHeader *pHeader = new WaveHeader;
    DWORD totalLen = lenInBytes + 44;//文件总长度=(采样率 * 通道数 * 比特数 / 8) * 持续时间(s)
    pHeader->chRIFF[0] = 'R';
    pHeader->chRIFF[1] = 'I';
    pHeader->chRIFF[2] = 'F';
    pHeader->chRIFF[3] = 'F';
    pHeader->dwRIFFLen = totalLen - 8;//文件的总长度-8bits

    pHeader->chWAVE[0] = 'W';
    pHeader->chWAVE[1] = 'A';
    pHeader->chWAVE[2] = 'V';
    pHeader->chWAVE[3] = 'E';

    pHeader->chFMT[0] = 'f';
    pHeader->chFMT[1] = 'm';
    pHeader->chFMT[2] = 't';
    pHeader->chFMT[3] = ' ';

    pHeader->dwFMTLen = 0x0010;//一般情况下Size为16，如果为18则最后多了2个字节的附加信息
    pHeader->pwf.wf.wFormatTag = 0x0001;//编码方式
    pHeader->pwf.wf.nChannels = m_channels; //1为单通道，2为双通道
    pHeader->pwf.wf.nSamplesPerSec = m_samplefreq;  //=44.1KHz
    pHeader->pwf.wf.nAvgBytesPerSec = m_samplefreq * m_channels * m_channelbits / 8;//每秒所需字节数
    pHeader->pwf.wf.nBlockAlign = m_channels * m_channelbits / 8;//一个采样的字节数
    pHeader->pwf.wBitsPerSample = m_channelbits;//16位，即设置PCM的方式为16位立体声(双通道)

    pHeader->chDATA[0] = 'd';
    pHeader->chDATA[1] = 'a';
    pHeader->chDATA[2] = 't';
    pHeader->chDATA[3] = 'a';
    pHeader->dwDATALen = totalLen - WAVE_HEAD_LENGTH;//数据的长度，=文件总长度-头长度(44bit)

    char *pWaveBuffer = new char[totalLen]; //音频数据
    memcpy(pWaveBuffer, pHeader, WAVE_HEAD_LENGTH);

    memcpy(pWaveBuffer+ WAVE_HEAD_LENGTH, input, lenInBytes);
    // MakeWaveData(pHeader->pwf.wf.nSamplesPerSec, freq, volume, pWaveBuffer+ WAVE_HEAD_LENGTH, m_samplefreq*durations);//采样点数

    ofstream ocout;
    ocout.open(file, ios::out | ios::binary);//以二进制形式打开文件
    if (ocout)
        ocout.write(pWaveBuffer, totalLen);
    else
        return 0;
    ocout.close();

    delete(pHeader);
    return 1;
}
