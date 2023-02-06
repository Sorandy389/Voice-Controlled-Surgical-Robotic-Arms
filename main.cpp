#include<iostream>
#include<windows.h>
#include<Mmsystem.h>
#include<cmath>
#include<ctime>
#include"WaveFunction.h"
#include"wav.h"
// #include "WaveFunction.h"
// #include "wave.h"
// #pragma comment(lib, "winmm.lib") // use "g++ real-time.cpp -lwinmm" instead
using   namespace   std;

typedef int16_t int16;

HWAVEIN         hWaveIn;		        //输入设备
HWAVEOUT        hWaveOut;		        //输出设备
WAVEFORMATEX    waveform;	            //定义音频流格式
// int16* pBuffer1, * pBuffer2;				//输入音频缓冲区（左右声道）
WAVEHDR         whdr_i1, whdr_i2;       //输入音频头
WAVEHDR         whdr_o;                //输出音频头

int16* file = (int16*)malloc(sizeof(int16) * 512);
DWORD hasRecorded = 0;

class USER
{
public:
	int a = 0;
	char b = 'a';
};

// from wave.cpp
#define FRAME_LEN   240
#define FRAME_SHIFT 80
#define FRAMES      300
#define BUFFER_LEN  2400
#define MAX_SAMPLES (FRAMES*FRAME_LEN)

int buff_begin, buff_end, buff_ptr;

bool flag_word_start;
bool flag_word_end;
bool flag_poss_start;
bool flag_user_stop;

int max_silence_time;
int min_word_length;
int silence_time;
int word_length;

double amp, amp_high, amp_low, amp_buf[FRAMES];
double zcr, zcr_high, zcr_low, zcr_buf[FRAMES];
double zcr_thresold;
int16 voice_buff[MAX_SAMPLES];
int16 pBuffer1[BUFFER_LEN];
int16 pBuffer2[BUFFER_LEN];

void wave_reset();

void wave_filter(double* buff);

void wave_energy(double* buff);

void wave_zcr(double* buff);

int wave_vad();

void wave_append(int16* sample, int len);

// void CALLBACK waveInProc(
//     HWAVEIN hwi,
//     UINT uMsg,
//     DWORD lpsample,
//     DWORD lpwvhdr,
//     DWORD reserved);

void wave_stop();

// void wave_start();

int wave_get_len();

int wave_get_frames();

void wave_get_data(double* buff);

void wave_get_zcr(double* buff);

void wave_get_energy(double* buff);

void CALLBACK callback(HWAVEIN   hwi,          // 设备句柄
	UINT      uMsg,							   // 消息
	DWORD_PTR dwInstance,					   // 对象
	DWORD_PTR dwParam1,						   // 参数1
	DWORD_PTR dwParam2);					   // 参数2

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

	memset(pBuffer1, 0, BUFFER_LEN);   // 内存置0
	memset(pBuffer2, 0, BUFFER_LEN);   // 内存置0
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
        word_length -= silence_time / 2;
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
    flag_poss_start = false;
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

        voice_buff[buff_end++] = *sample++;
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

void wave_stop() {
    flag_user_stop = true;
}

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

void CALLBACK callback(HWAVEIN   hwi,                              // 设备句柄
						UINT      uMsg,							   // 消息
						DWORD_PTR dwInstance,					   // 对象
						DWORD_PTR dwParam1,						   // 参数1
						DWORD_PTR dwParam2)						   // 参数2
{
	// 获取对象
	USER* user2 = (USER*)dwInstance;

	// 处理消息
	switch (uMsg)
	{
	case WIM_OPEN:                                 // 打开录音设备
 
		printf("Succeed in open recorder..\n");
		break;
 
	case WIM_DATA:                                 // 缓冲区已满
	{
		// 获取音频头
		PWAVEHDR  pwhdr = (PWAVEHDR)dwParam1;
		int16* pBuff = (int16*)(pwhdr->lpData);
		printf("buffer is full..\n");
		printf("a:%d , b:%c \n",user2->a,user2->b);
		// 缓冲池信息
		DWORD buflen = pwhdr->dwBufferLength;
		DWORD bytrecd = pwhdr->dwBytesRecorded;
		hasRecorded += bytrecd;

		// 缓冲扩增
		file = (int16*)realloc(file, hasRecorded / 2 * sizeof(int16));
		// 存储新内容
		if (file)
		{
			memcpy(&file[(hasRecorded-bytrecd)/2], pwhdr->lpData, bytrecd);
			printf("Already record:%d byte\n",hasRecorded);
		}
        wave_append(pBuff, BUFFER_LEN); //endpoint
		cout<<buff_begin<<","<<buff_ptr<<","<<buff_end<<endl;
        if (flag_word_end | flag_user_stop) {
			cout<<"stop signal:"<<flag_word_end<<','<<BUFFER_LEN<<endl;
            // waveInReset(hwi);
            // waveInUnprepareHeader(hwi, &whdr_i1, sizeof(WAVEHDR));
            // waveInUnprepareHeader(hwi, &whdr_i2, sizeof(WAVEHDR));
            // waveInClose(hwi);
			// cout<<"here"<<endl;
        } else {
        // if (!flag_user_stop) {
			// 加入缓存
			waveInAddBuffer(hwi, pwhdr, sizeof(WAVEHDR));
		}
	}
	break;
 
	case WIM_CLOSE:                               // 关闭录音设备
	{
		printf("Stop recording..\n");
		
	}
	break;
 
	default:
		printf("nothing..\n");
		break;
	}
}



int main()
{	
    
    USER* user = new USER();		//定义用户
	// 设备数量
	int count = waveInGetNumDevs();
	printf("\n Wave In Get Num%d\n", count);
 
	// 设备名称
	WAVEINCAPS waveIncaps;
	MMRESULT mmResult = waveInGetDevCaps(0, &waveIncaps, sizeof(WAVEINCAPS));//2
	std::cout <<"device name:"<< waveIncaps.szPname << std::endl;
 
    // Reset
    wave_reset();
    
	// 设置音频流格式
	waveform.nSamplesPerSec = 8000;												// 采样率
	waveform.wBitsPerSample = 16;												// 采样精度
	waveform.nChannels = 1;                                                     // 声道个数
	waveform.cbSize = 0;														// 额外空间	
	waveform.wFormatTag = WAVE_FORMAT_PCM;										// 音频格式
	waveform.nBlockAlign = (waveform.wBitsPerSample * waveform.nChannels) / 8;  // 块对齐
	waveform.nAvgBytesPerSec = waveform.nBlockAlign * waveform.nSamplesPerSec;  // 传输速率
	
	// test
	MMRESULT err = waveInOpen(NULL, 0, &waveform, 0, 0, WAVE_FORMAT_QUERY);
	if (err != MMSYSERR_NOERROR) {
		cout<<"not support 8kHz 16bit PCM"<<endl;
		return -1;
	}
	//分配内存
	// pBuffer1 = new int16[BUFFER_LEN];
	// pBuffer2 = new int16[BUFFER_LEN];
 
	// 设置音频头
	whdr_i1.lpData = (char*)&pBuffer1; // 指向buffer
	whdr_i1.dwBufferLength = BUFFER_LEN * 2;     // buffer大小
	whdr_i1.dwBytesRecorded = 0;      // buffer存放大小
	whdr_i1.dwUser = 0;
	whdr_i1.dwFlags = 0;
	whdr_i1.dwLoops = 0;
	whdr_i2.lpData = (char*)&pBuffer1; // 指向buffer
	whdr_i2.dwBufferLength = BUFFER_LEN * 2;     // buffer大小
	whdr_i2.dwBytesRecorded = 0;      // buffer存放大小
	whdr_i2.dwUser = 0;
	whdr_i2.dwFlags = 0;
	whdr_i2.dwLoops = 0;
 
	// 开启录音
	MMRESULT mRet = waveInOpen(&hWaveIn, 0, &waveform, (DWORD_PTR)callback, (DWORD_PTR)user, CALLBACK_FUNCTION);
	if (mRet != MMSYSERR_NOERROR) {
		cout<<"input port cannot open"<<endl;
		return -1;
	}
	waveInPrepareHeader(hWaveIn, &whdr_i1, sizeof(WAVEHDR));//准备buffer
	waveInPrepareHeader(hWaveIn, &whdr_i2, sizeof(WAVEHDR));//准备buffer
	waveInAddBuffer(hWaveIn, &whdr_i1, sizeof(WAVEHDR));    //添加buffer
	waveInAddBuffer(hWaveIn, &whdr_i2, sizeof(WAVEHDR));    //添加buffer
	
	cout<<"begin to record"<<endl;
	waveInStart(hWaveIn);
	getchar();
	flag_user_stop = true;
	cout<<"end record"<<hasRecorded<<endl;
	//waveInStop(hWaveIn);
    waveInReset(hWaveIn);
	waveInClose(hWaveIn);
 
	HANDLE wait = CreateEvent(NULL, 0, 0, NULL);
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveform, (DWORD_PTR)wait, 0L, CALLBACK_EVENT);
 
	// int8_t *array = ByteToInt(file, hasRecorded);
	// WaveFunction* a = new WaveFunction(128,13);//每帧多少个采样点，MFCC参数的维数
	// a->endpoint(file, hasRecorded);
	// cout<<buff_begin<<','<<buff_end<<endl;
	// int16* result = (int16*)malloc(sizeof(int16) * word_length * FRAME_LEN);
	// memcpy(result, file+buff_begin, word_length * FRAME_LEN * 2);
	// for (int i=0; i<buff_begin-buff_end; i++) {
	// 	result[i] = voice_buff[buff_begin+i];
	// }
	
	// 播放录音
	// whdr_o.lpData = (char*)&file;			// 指向buffer
	// whdr_o.dwBufferLength = word_length * FRAME_LEN * 2;    // buffer大小
	whdr_o.dwBytesRecorded = MAX_SAMPLES * 2;
	whdr_o.lpData = (char*)&voice_buff+buff_begin*2;			// 指向buffer
	whdr_o.dwBufferLength = (buff_end - buff_begin) * 2;    // buffer大小
	// whdr_o.dwBytesRecorded = word_length * FRAME_LEN * 2;
	// whdr_o.lpData = (LPSTR)(file+(a->x1)*80);			// 指向buffer
	// whdr_o.dwBufferLength = (a->x2 - a->x1)*80;    // buffer大小
	// whdr_o.dwBytesRecorded = (a->x2 - a->x1)*80;

	whdr_o.dwFlags = 0;
	whdr_o.dwLoops = 0;
 
	
	ResetEvent(wait);
	waveOutPrepareHeader(hWaveOut, &whdr_o, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &whdr_o, sizeof(WAVEHDR));
	/*Sleep(5000);*/
	DWORD dw = WaitForSingleObject(wait, INFINITE);
	// if (dw == WAIT_OBJECT_0)
	// {
	// 	std::cout << "end" << std::endl;
	// 	return 0;
	// }

	if (!Create((char*)&voice_buff+buff_begin * 2, (buff_end - buff_begin) * 2, "record/input.wav")) {
		cout<<"failed to create .wav file"<<endl;
		return -1;
	}

    // Pre-train the MFCC
    cout<<"start train"<<endl;
    clock_t start = clock();

    vector< vector< vector<float> > > trainset;

    for (int i=0; i<10; i++) {

        string file= "record/ximin_"+to_string(i)+".wav";

        WaveFunction* b=new WaveFunction(128,13);//每帧多少个采样点，MFCC参数的维数
        

        // trainset.push_back(a->getMFCCs(file));
        vector< vector<float> > temp = b->getMFCCs(file);//提取mfcc参数
        //mfccs1=a->addFirstOrderDifference(mfccs1);//增加一阶差分系数,此时mfcc参数变为13+13维
        
        trainset.push_back(b->addOrderDifference(temp));//增加一阶差分和二阶差分系数，此时mfcc参数变为13*2+13维

        // cout<<' ';
		// vector< vector<float> > nums;
		// temp.swap(nums);
		// delete []b;
    }

    clock_t end = clock();
    printf("end train: %e\n", (end-start)*1.0/CLOCKS_PER_SEC);

    // float time[16];
    cout<<"start test"<<endl;
    start = clock();

	// input mfcc
	WaveFunction* a=new WaveFunction(128,13);//每帧多少个采样点，MFCC参数的维数
		
	// vector<vector<float> > mfccs = a->getMFCCs(voice_buff+buff_begin, buff_end - buff_begin);//提取mfcc参数
	vector<vector<float> > mfccs = a->getMFCCs("record/input.wav");//提取mfcc参数

	//mfccs1=a->addFirstOrderDifference(mfccs1);//增加一阶差分系数,此时mfcc参数变为13+13维

	mfccs=a->addOrderDifference(mfccs);//增加一阶差分和二阶差分系数，此时mfcc参数变为13*2+13维
	float cmp[10];
	for (int i=0; i<10; i++) {
		// cout<<to_string(i)+"_1"+"-"+to_string(j/2)+"_"+to_string(j%2+1)+"-";
		// vector<vector<float> > temp = trainset.at(0);
		vector<vector<float> > temp = trainset[i];
		cmp[i] = a->ComputeDTW(mfccs,temp);
		cout<<to_string(i)+":";
		cout<<cmp[i];//利用动态时间规整算法，计算两个语音的相似度，越小相似度越大
		cout<<'\t';
	}
	cout<<'\n';
	float score = 0;
	int max = -99999;
	int min = 99999;
	for (int k=0; k<10; k++) {
		score += cmp[k];
		// cout<<score<<endl;
		if (cmp[k] > max) max = cmp[k];
		if (cmp[k] < min) min = cmp[k];
	}
	score = (score - max - min) / 8;
	// score /= 10;
	cout<<"score:"<<score<<endl;
    
    end = clock();
    printf("end test: %e\n", (end-start)*1.0/CLOCKS_PER_SEC);

	vector< vector<float> > nums1;
	vector< vector< vector<float> > > nums2;
	mfccs.swap(nums1);
	trainset.swap(nums2);
	// delete []a;

}
 
