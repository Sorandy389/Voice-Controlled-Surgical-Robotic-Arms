#include "pch.h"
//#include "MFCLibrary1.h"
#include <mex.h>
#include "wave.h"

#define WAVE_IN_START	1
#define WAVE_IN_STOP	2
#define WAVE_GET_DATA	3
#define WAVE_GET_E		4
#define WAVE_GET_Z		5

void mexFunction(
	int nlhs, mxArray* plhs[],
	int nrhs, const mxArray* prhs[]
) {
	double* ret;
	int cmd, len;

	if (nrhs < 1 || !mxIsNumeric(prhs[0]))
		mexErrMsgTxt("Wrong command format");

	cmd = (int)*mxGetPr(prhs[0]);
	switch (cmd) {
	case WAVE_IN_START:
		wave_start();
		break;

	case WAVE_IN_STOP:
		wave_stop();
		break;

	case WAVE_GET_DATA:
		len = wave_get_len();
		if (len == 0) break;

		plhs[0] = mxCreateDoubleMatrix(len, 1, mxREAL);
		ret = mxGetPr(plhs[0]);

		wave_get_data(ret);
		break;

	case WAVE_GET_Z:
		len = wave_get_frames();
		if (len == 0) break;

		plhs[0] = mxCreateDoubleMatrix(len, 1, mxREAL);
		ret = mxGetPr(plhs[0]);

		wave_get_zcr(ret);
		break;

	case WAVE_GET_E:
		len = wave_get_frames();
		if (len == 0) break;

		plhs[0] = mxCreateDoubleMatrix(len, 1, mxREAL);
		ret = mxGetPr(plhs[0]);

		wave_get_energy(ret);
		break;

	default:
		break;
	}
}