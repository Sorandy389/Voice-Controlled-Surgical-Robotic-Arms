#include <iostream>
#include <windows.h>
#include <Mmsystem.h>
// #pragma comment(lib, "winmm.lib") // use "g++ real-time.cpp -lwinmm" instead

static BYTE*    file = (BYTE*)malloc(sizeof(BYTE) * 512);
static DWORD    hasRecorded = 0;
static BOOL     recurr = TRUE;
class USER
{
public:
	int a = 0;
	char b = 'a';
};
 
void CALLBACK callback(HWAVEIN   hwi,          // 设备句柄
	UINT      uMsg,							   // 消息
	DWORD_PTR dwInstance,					   // 对象
	DWORD_PTR dwParam1,						   // 参数1
	DWORD_PTR dwParam2);					   // 参数2

 
void CALLBACK callback(HWAVEIN   hwi,                              // 设备句柄
						UINT      uMsg,							   // 消息
						DWORD_PTR dwInstance,					   // 对象
						DWORD_PTR dwParam1,						   // 参数1
						DWORD_PTR dwParam2)						   // 参数2
{
	// 获取对象
	USER* user2 = (USER*)dwInstance;
	// 获取音频头
	PWAVEHDR  pwhdr = (PWAVEHDR)dwParam1;
 
	// 处理消息
	switch (uMsg)
	{
	case WIM_OPEN:                                 // 打开录音设备
 
		printf("Succeed in open recorder..\n");
		break;
 
	case WIM_DATA:                                 // 缓冲区已满
	{
		printf("buffer is full..\n");
		printf("a:%d , b:%c \n",user2->a,user2->b);
		// 缓冲池信息
		DWORD buflen = pwhdr->dwBufferLength;
		DWORD bytrecd = pwhdr->dwBytesRecorded;
		hasRecorded += bytrecd;

		// 缓冲扩增
		file = (BYTE*)realloc(file, hasRecorded * sizeof(BYTE));
		// 存储新内容
		if (file)
		{
			memcpy(&file[hasRecorded-bytrecd], pwhdr->lpData, bytrecd);
			printf("Already record:%d byte\n",hasRecorded);
		}
		// 循环 	
		if (recurr)
		{
			// 加入缓存
			waveInAddBuffer(hwi, pwhdr, sizeof(WAVEHDR));
			printf("recurring");
		}
	}
	break;
 
	case WIM_CLOSE:                               // 关闭录音设备
	{
		printf("Stop recording..\n");
		
	}
	break;
 
	default:
		break;
	}
}


 
int main()
{
	HWAVEIN         hWaveIn;		        //输入设备
	HWAVEOUT        hWaveOut;		        //输出设备
	WAVEFORMATEX    waveform;	            //定义音频流格式
	BYTE* pBuffer1, * pBuffer2;				//输入音频缓冲区（左右声道）
	WAVEHDR         whdr_i1, whdr_i2;       //输入音频头
	WAVEHDR         whdr_o;                //输出音频头
	USER*			user = new USER();		//定义用户
	
	// 设备数量
	int count = waveInGetNumDevs();
	printf("\n Wave In Get Num%d\n", count);
 
	// 设备名称
	WAVEINCAPS waveIncaps;
	MMRESULT mmResult = waveInGetDevCaps(0, &waveIncaps, sizeof(WAVEINCAPS));//2
	std::cout <<"device name:"<< waveIncaps.szPname << std::endl;
 
	// 设置音频流格式
	waveform.nSamplesPerSec = 44100;												// 采样率
	waveform.wBitsPerSample = 16;												// 采样精度
	waveform.nChannels = 1;                                                     // 声道个数
	waveform.cbSize = 0;														// 额外空间	
	waveform.wFormatTag = WAVE_FORMAT_PCM;										// 音频格式
	waveform.nBlockAlign = (waveform.wBitsPerSample * waveform.nChannels) / 8;  // 块对齐
	waveform.nAvgBytesPerSec = waveform.nBlockAlign * waveform.nSamplesPerSec;  // 传输速率
 
	//分配内存
	pBuffer1 = new BYTE[1024 * 10000];
	pBuffer2 = new BYTE[1024 * 10000];
	memset(pBuffer1, 0, 1024 * 10000);   // 内存置0
	memset(pBuffer2, 0, 1024 * 10000);   // 内存置0
 
	// 设置音频头
	whdr_i1.lpData = (LPSTR)pBuffer1; // 指向buffer
	whdr_i1.dwBufferLength = 1024 * 10000;     // buffer大小
	whdr_i1.dwBytesRecorded = 0;      // buffer存放大小
	whdr_i1.dwUser = 0;
	whdr_i1.dwFlags = 0;
	whdr_i1.dwLoops = 1;
	whdr_i2.lpData = (LPSTR)pBuffer1; // 指向buffer
	whdr_i2.dwBufferLength = 1024 * 10000;     // buffer大小
	whdr_i2.dwBytesRecorded = 0;      // buffer存放大小
	whdr_i2.dwUser = 0;
	whdr_i2.dwFlags = 0;
	whdr_i2.dwLoops = 1;
 
	// 开启录音
	MMRESULT mRet = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD_PTR)callback, (DWORD_PTR)user, CALLBACK_FUNCTION);
	waveInPrepareHeader(hWaveIn, &whdr_i1, sizeof(WAVEHDR));//准备buffer
	// waveInPrepareHeader(hWaveIn, &whdr_i2, sizeof(WAVEHDR));//准备buffer
	waveInAddBuffer(hWaveIn, &whdr_i1, sizeof(WAVEHDR));    //添加buffer
	// waveInAddBuffer(hWaveIn, &whdr_i2, sizeof(WAVEHDR));    //添加buffer
 
	waveInStart(hWaveIn);
	getchar();
	recurr = FALSE;
	//waveInStop(hWaveIn);
    waveInReset(hWaveIn);
	waveInClose(hWaveIn);
 
	HANDLE wait = CreateEvent(NULL, 0, 0, NULL);
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveform, (DWORD_PTR)wait, 0L, CALLBACK_EVENT);
 
	// 播放录音
	whdr_o.lpData = (LPSTR)file;			// 指向buffer
	whdr_o.dwBufferLength = hasRecorded;    // buffer大小
	whdr_o.dwBytesRecorded = hasRecorded;
	whdr_o.dwFlags = 0;
	whdr_o.dwLoops = 1;
 
	
	ResetEvent(wait);
	waveOutPrepareHeader(hWaveOut, &whdr_o, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &whdr_o, sizeof(WAVEHDR));
	/*Sleep(5000);*/
	DWORD dw = WaitForSingleObject(wait, INFINITE);
	if (dw == WAIT_OBJECT_0)
	{
		std::cout << "end" << std::endl;
		return 0;
	}
}
 
 