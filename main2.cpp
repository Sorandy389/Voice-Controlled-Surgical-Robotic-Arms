#include <windows.h>
#include <ctime>
#include "WaveFunction.h"

int main(int argc, char *argv[])
{

    // Pre-train the MFCC
    cout<<"start train"<<endl;
    clock_t start = clock();

    vector< vector< vector<float> > > trainset;

    for (int i=0; i<1; i++) {

        string file= "record/train0_"+to_string(i)+".wav";

        WaveFunction* a=new WaveFunction(128,13);//每帧多少个采样点，MFCC参数的维数
        

        // trainset.push_back(a->getMFCCs(file));
        vector< vector<float> > temp = a->getMFCCs(file);//提取mfcc参数
        //mfccs1=a->addFirstOrderDifference(mfccs1);//增加一阶差分系数,此时mfcc参数变为13+13维
        
        trainset.push_back(a->addOrderDifference(temp));//增加一阶差分和二阶差分系数，此时mfcc参数变为13*2+13维

        cout<<' ';
    }

    clock_t end = clock();
    printf("end train: %e\n", (end-start)*1.0/CLOCKS_PER_SEC);

    // float time[16];
    cout<<"start test"<<endl;
    start = clock();
    for (int i=0; i<16; i++) {
        clock_t local = clock();
        string file = "record/test_"+to_string(i)+".wav";
        cout<<to_string(i)+':';
        int cmp[2];
        for (int j=0; j<2; j++) {
            WaveFunction* a=new WaveFunction(128,13);//每帧多少个采样点，MFCC参数的维数
            
            vector<vector<float> > mfccs = a->getMFCCs(file);//提取mfcc参数

            //mfccs1=a->addFirstOrderDifference(mfccs1);//增加一阶差分系数,此时mfcc参数变为13+13维

            mfccs=a->addOrderDifference(mfccs);//增加一阶差分和二阶差分系数，此时mfcc参数变为13*2+13维

            // cout<<to_string(i)+"_1"+"-"+to_string(j/2)+"_"+to_string(j%2+1)+"-";
            // vector<vector<float> > temp = trainset.at(0);
            cmp[j] = a->ComputeDTW(mfccs,trainset[j]);
            // cout<<to_string(j)+":";
            // cout<<cmp[j];//利用动态时间规整算法，计算两个语音的相似度，越小相似度越大
            // cout<<'\t';
        }
        int score = 0;
        // int max = -99999;
        // int min = 99999;
        for (int k=0; k<2; k++) {
            score += cmp[k];
            // cout<<score<<endl;
            // if (cmp[k] > max) max = cmp[k];
            // if (cmp[k] < min) min = cmp[k];
        }
        // score = (score - max - min) / 8;
        score /= 2;
        cout<<to_string(score);
        cout<<'\t';
        printf("%e\n", (clock()-local)*1.0/CLOCKS_PER_SEC);
    }
    end = clock();
    printf("end test: %e\n", (end-start)*1.0/CLOCKS_PER_SEC);

    return 0;
}
