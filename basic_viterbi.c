//#include "com_inc.h"

//#include "matlab_types.h"

#include "basic_viterbi.h"


//#if ((TEST_PLATFORM == PLATFORM_SIM) || (TEST_PLATFORM == PLATFROM_CHIP))
#if 1


metric_and_decode_int g_metric_and_path_int[2][64];


//unsigned char g_stat[64][2] = {0,32,0,32, 1,33, 1,33, 2,34, 2,34, 3,35, 3,35, 4,36, 4,36, 5,37, 5,37, 6,38, 6,38, 7,39, 7,39, 8,40,8,40,9,41,9,41,10,42,10,42,11,43,11,43,12,44,12,44,13,45,13,45,14,46,14,46,15,47,15,47,16,48,16,48,17,49,17,49,18,50,18,50,19,51,19,51,20,52,20,52,21,53,21,53,22,54,22,54,23,55,23,55,24,56,24,56,25,57,25,57,26,58,26,58,27,59,27,59,28,60,28,60,29,61,29,61,30,62,30,62,31,63,31,63};

CONST_TCM_DATA unsigned char g_stat[64][2] = { {0,32},
								{0,32},
								{1,33},
								{1,33},
								{2,34},
								{2,34},
								{3,35},
								{3,35},
								{4,36},
								{4,36},
								{5,37},
								{5,37},
								{6,38},
								{6,38},
								{7,39},
								{7,39},
								{8,40},
								{8,40},
								{9,41},
								{9,41},
								{10,42},
								{10,42},
								{11,43},
								{11,43},
								{12,44},
								{12,44},
								{13,45},
								{13,45},
								{14,46},
								{14,46},
								{15,47},
								{15,47},
								{16,48},
								{16,48},
								{17,49},
								{17,49},
								{18,50},
								{18,50},
								{19,51},
								{19,51},
								{20,52},
								{20,52},
								{21,53},
								{21,53},
								{22,54},
								{22,54},
								{23,55},
								{23,55},
								{24,56},
								{24,56},
								{25,57},
								{25,57},
								{26,58},
								{26,58},
								{27,59},
								{27,59},
								{28,60},
								{28,60},
								{29,61},
								{29,61},
								{30,62},
								{30,62},
								{31,63},
								{31,63}};

CONST_TCM_DATA unsigned char g_stat_Output[64][2] = {	{0,3},
										{3,0},
										{1,2},
										{2,1},
										{0,3},
										{3,0},
										{1,2},
										{2,1},
										{3,0},
										{0,3},
										{2,1},
										{1,2},
										{3,0},
										{0,3},
										{2,1},
										{1,2},
										{3,0},
										{0,3},
										{2,1},
										{1,2},
										{3,0},
										{0,3},
										{2,1},
										{1,2},
										{0,3},
										{3,0},
										{1,2},
										{2,1},
										{0,3},
										{3,0},
										{1,2},
										{2,1},
										{2,1},
										{1,2},
										{3,0},
										{0,3},
										{2,1},
										{1,2},
										{3,0},
										{0,3},
										{1,2},
										{2,1},
										{0,3},
										{3,0},
										{1,2},
										{2,1},
										{0,3},
										{3,0},
										{1,2},
										{2,1},
										{0,3},
										{3,0},
										{1,2},
										{2,1},
										{0,3},
										{3,0},
										{2,1},
										{1,2},
										{3,0},
										{0,3},
										{2,1},
										{1,2},
										{3,0},
										{0,3} };
	

/* set 1 to a certain bit */
void setbit(uint32 k, uint32 index, uint32 *pDecodedWord)
{
	uint32 wordIdx = (index/32);
	uint32 bitIdx = (index%32);
	*(pDecodedWord + wordIdx) |= (1 << (31 - bitIdx));
}


int32 calMetric_int(uint32 expect0, uint32 expect1, int32 decode0, int32 decode1)
{
	int32 matrics = 0;
	matrics = (decode0 * (1 - 2 * expect0) + decode1 * (1 - 2 * expect1));
	return matrics;
}

/*
Parameter:
int32 *pInput: 		point to input data memory.  
uint32 InputSoftbitNum:    block bit num； the number of input softbit,  should be an even number. such as: 48 * 2；
tailBiteTimes: 		should be 1;  that means, input data will be used two times.
backTrackingLen:    Length of backtracing, it will determin the result;
pDecodeOutput: 		output memory; byte num; should be ((InputSoftbitNum/2 + 7)/8) byte

example: block bit num is 48; 
		 input softbit num is 48 * 2 (code rate),

viterbi_int(*pIn, 48 * 2 , 1, 12, pOut(byte len = 6));

*/

/* 256 * 32bit 是一组；打印出来；I:5bit, Q 5bit; 高位全部填0

*/

void viterbi_int(int32 *pInput, 
			      uint32 InputSoftbitNum, 
			      uint32 tailBiteTimes, 
			      uint32 *pDecodeOutput)
{
	uint32 i, j, k;
	int32  decode0, decode1;
	uint32 pingPangNow;
	uint32 pingPangNext;

	int32 metrics, metricsTotal, metricsMax;

	uint32 NextStateIdx;
	uint8 expectOutBit0, expectOutBit1;

	uint32 CorrectIdx;

	uint32 totalOutBit = 0; 
	/* repeat input 3 times if tailbite */
	totalOutBit = InputSoftbitNum/2 * tailBiteTimes;
	
	memset(&g_metric_and_path_int[0][0], 0, sizeof(g_metric_and_path_int));
	g_metric_and_path_int[0][0].calculatedFlag = 1;

	/* 2 softbits input */
	for (i = 0; i < totalOutBit; i++)
	{
		decode0 = pInput[(i * 2 + 0) % InputSoftbitNum];
		decode1 = pInput[(i * 2 + 1) % InputSoftbitNum];

		/* determin which buffer to use,  ping or pang */
		pingPangNow = (i % 2);
		pingPangNext = (i + 1) % 2;
		/* memset the next metric parameter */
		memset(&g_metric_and_path_int[pingPangNext][0], 0, sizeof(g_metric_and_path_int[pingPangNext]));
		
		/* cal for each state */
		for (j = 0; j < 64; j++)
		{
			/* whether the state has been reached:
						yes: calculate next stat;
						no:  no need to calculate */
			if (1 == g_metric_and_path_int[pingPangNow][j].calculatedFlag)
			{
				/* assume it is 0 or 1, calculate distance value, add, compare and select   */
				for (k = 0; k < 2; k++)
				{
					NextStateIdx = g_stat[j][k];
					expectOutBit1 = g_stat_Output[j][k] & 0x1;
					expectOutBit0 = ((g_stat_Output[j][k] >> 1) & 0x1);

					/* calculate metrics */
					metrics = calMetric_int(expectOutBit0, expectOutBit1, decode0, decode1);

					metricsTotal = metrics + g_metric_and_path_int[pingPangNow][j].metrics;

					/* determine whether metrics update is needed
						(1)has not been accecced, 
						(2)has been accecced, but new metris is larger;
						otherwise, no need to update metris */
					if ( (0 == g_metric_and_path_int[pingPangNext][NextStateIdx].calculatedFlag) ||
						 ((1 == g_metric_and_path_int[pingPangNext][NextStateIdx].calculatedFlag) &&
						 (metricsTotal > g_metric_and_path_int[pingPangNext][NextStateIdx].metrics)))
					{
						/* update calculatedFlag */
						g_metric_and_path_int[pingPangNext][NextStateIdx].calculatedFlag = 1;
						g_metric_and_path_int[pingPangNext][NextStateIdx].metrics = metricsTotal;
						/* copy decodedWord */
						memcpy(g_metric_and_path_int[pingPangNext][NextStateIdx].decodedWord,
							   g_metric_and_path_int[pingPangNow][j].decodedWord,
							   sizeof(g_metric_and_path_int[pingPangNow][j].decodedWord));
						/* update decode result */
						/* if k is 0, no need to set 0; 
						   if k is 1, need to set 1 to decodedWord */
						if (1 == k)
						{
							setbit(k, i, g_metric_and_path_int[pingPangNext][NextStateIdx].decodedWord);
						}
					}
				}
			}
		}

	}


	/* search the biggest metrics path */
	metricsMax = 0;
	CorrectIdx = 0;
	for (j = 0; j < 64; j++)
	{
		metrics = g_metric_and_path_int[pingPangNext][j].metrics;
		if (1 == g_metric_and_path_int[pingPangNext][j].calculatedFlag)
		{
			if (metrics > metricsMax)
			{
				CorrectIdx = j;
				metricsMax = metrics;
			}
		}

	}

	/* copy result to output */
	memcpy(pDecodeOutput,
		g_metric_and_path_int[pingPangNext][CorrectIdx].decodedWord,
		sizeof(g_metric_and_path_int[pingPangNext][CorrectIdx].decodedWord));
}


/* viterbi out is 96bit:
				  	(original 1st)16bit  + (original 2end)16bit  
				  + (original 3rd)16 bit + (backtrace 1st)16bit 
				  + (backtrace 2end)16bit + (backtrace 3rd)16 bit 
	select 48 bits as output:
					(backtrace 1st)16bit + (backtrace 2end)16bit 
				  + (original 3rd)16 bit

*/

/* viterbi out is 96bit:
				  	(original 1st)8bit  + (original 2nd)8bit  +  (original 3rd)8bit   + (original 4th)8bit  
				  + (original 5th)8 bit + (original 6th)8 bit +  (backtrace 1st)8bit  +  (backtrace 2nd)8bit
				  + (backtrace 3rd)8bit + (backtrace 4th)8 bit + (backtrace 5th)8 bit + (backtrace 6th)8 bit
	select 48 bits as output:
					(backtrace 1st)8bit  +  (backtrace 2nd)8bit +   (backtrace 3rd)8bit + (original 4th)8bit
				  + (original 5th)8 bit + (original 6th)8 bit

*/

MINI_ITCM_CODE_SECTION void Viterbi_BackTracking_repeat_3(uint32 *pViterbiOut, uint32 *pDataOut)
{

	*(pDataOut + 0) = ((*(pViterbiOut + 1) & 0xffff) << 16);  //  67
	*(pDataOut + 0) += ((*(pViterbiOut + 2) & 0xffff0000) >> 16 );  //89

	*(pDataOut + 1) += ((*(pViterbiOut + 2) & 0xffff) << 16);

}

MINI_ITCM_CODE_SECTION void Viterbi_BackTracking_repeat_2(uint32 *pViterbiOut, uint32 *pDataOut)
{

	*(pDataOut + 0) = ((*(pViterbiOut + 1) & 0xffff) << 16);
	*(pDataOut + 0) += ((*(pViterbiOut + 2) & 0xff000000) >> 16 );
	*(pDataOut + 0) += (*(pViterbiOut + 0) & 0x000000ff);
	
	*(pDataOut + 1) += (*(pViterbiOut + 1) & 0xffff0000);

}

uint32 g_viterbiOut[6] = {0};
uint32 g_viterbiOutBackTrace[2] = {0};
uint32 g_viterbiBlockOut[2] = {0};
uint8 g_viterbiOut_little[2 * 4] = {0};

extern void print_int32_hex(char *s[], int32 * pData, uint32 pointNum);

MINI_ITCM_CODE_SECTION void viterbi(int32 *pInput, 
			      uint32 InputSoftbitNum, 
			      uint32 tailBiteTimes, 
			      uint8 *pDecodeOutput)
{


	memset(&g_viterbiOut[0], 0, sizeof(g_viterbiOut));
	memset(&g_viterbiBlockOut[0], 0, sizeof(g_viterbiBlockOut));
	memset(&g_viterbiOutBackTrace[0], 0, sizeof(g_viterbiOutBackTrace));

	/* 5. Viterbi decode */
	viterbi_int_new(pInput, 
				InputSoftbitNum, /* code rate is 2; tail bite is 2 */
				tailBiteTimes,
				&g_viterbiOut[0]);

	//print_int32_hex("matlab decode: g_viterbiOut", g_viterbiOut, 10);

	if(2 == tailBiteTimes)
	{
		Viterbi_BackTracking_repeat_2(&g_viterbiOut[0], &g_viterbiOutBackTrace[0]);
	}
	else if(3 == tailBiteTimes)
	{
		Viterbi_BackTracking_repeat_3(&g_viterbiOut[0], &g_viterbiOutBackTrace[0]);
	}
	//print_int32_hex("matlab decode: g_viterbiOutBackTrace", g_viterbiOutBackTrace, 10);


	/* add 1st 48bit to output */
	g_viterbiBlockOut[0] = g_viterbiOutBackTrace[0];
	g_viterbiBlockOut[1] = (g_viterbiOutBackTrace[1] & 0xffff0000);

	WordToByte_To_LittleEnd(g_viterbiBlockOut, &g_viterbiOut_little[0], 8);
	memcpy(pDecodeOutput, &g_viterbiOut_little[0], 6);
	
}


#endif


typedef struct
{
	int32 metrics;
	uint32 decodedWord[4];
}viterbi_new_stru;


MINI_DTCM_DATA_SECTION __attribute__((aligned (16)))  viterbi_new_stru  vitStruPingPong[2][64];
MINI_DTCM_DATA_SECTION __attribute__((aligned (16)))  RAM_DATA int32 metrics_all[96][4] = { 0 };

typedef struct
{
	uint8 new_state;
	uint8 old_state;
	uint8 value0Or1;
	uint8 IqIndex;
}viterbi_stru_all;

MINI_DTCM_DATA_SECTION __attribute__((aligned (16)))  viterbi_stru_all  g_viterbiStateAll[128] = 
{

#if 0
//old_state   new_state   01_value   IQ_Value_Idx 
{ 0, 0, 0, 0 },
{ 0, 32, 1, 3 },
{ 1, 0, 0, 3 },
{ 1, 32, 1, 0 },
{ 2, 1, 0, 1 },
{ 2, 33, 1, 2 },
{ 3, 1, 0, 2 },
{ 3, 33, 1, 1 },
{ 4, 2, 0, 0 },
{ 4, 34, 1, 3 },
{ 5, 2, 0, 3 },
{ 5, 34, 1, 0 },
{ 6, 3, 0, 1 },
{ 6, 35, 1, 2 },
{ 7, 3, 0, 2 },
{ 7, 35, 1, 1 },
{ 8, 4, 0, 3 },
{ 8, 36, 1, 0 },
{ 9, 4, 0, 0 },
{ 9, 36, 1, 3 },
{ 10, 5, 0, 2 },
{ 10, 37, 1, 1 },
{ 11, 5, 0, 1 },
{ 11, 37, 1, 2 },
{ 12, 6, 0, 3 },
{ 12, 38, 1, 0 },
{ 13, 6, 0, 0 },
{ 13, 38, 1, 3 },
{ 14, 7, 0, 2 },
{ 14, 39, 1, 1 },
{ 15, 7, 0, 1 },
{ 15, 39, 1, 2 },
{ 16, 8, 0, 3 },
{ 16, 40, 1, 0 },
{ 17, 8, 0, 0 },
{ 17, 40, 1, 3 },
{ 18, 9, 0, 2 },
{ 18, 41, 1, 1 },
{ 19, 9, 0, 1 },
{ 19, 41, 1, 2 },
{ 20, 10, 0, 3 },
{ 20, 42, 1, 0 },
{ 21, 10, 0, 0 },
{ 21, 42, 1, 3 },
{ 22, 11, 0, 2 },
{ 22, 43, 1, 1 },
{ 23, 11, 0, 1 },
{ 23, 43, 1, 2 },
{ 24, 12, 0, 0 },
{ 24, 44, 1, 3 },
{ 25, 12, 0, 3 },
{ 25, 44, 1, 0 },
{ 26, 13, 0, 1 },
{ 26, 45, 1, 2 },
{ 27, 13, 0, 2 },
{ 27, 45, 1, 1 },
{ 28, 14, 0, 0 },
{ 28, 46, 1, 3 },
{ 29, 14, 0, 3 },
{ 29, 46, 1, 0 },
{ 30, 15, 0, 1 },
{ 30, 47, 1, 2 },
{ 31, 15, 0, 2 },
{ 31, 47, 1, 1 },
{ 32, 16, 0, 2 },
{ 32, 48, 1, 1 },
{ 33, 16, 0, 1 },
{ 33, 48, 1, 2 },
{ 34, 17, 0, 3 },
{ 34, 49, 1, 0 },
{ 35, 17, 0, 0 },
{ 35, 49, 1, 3 },
{ 36, 18, 0, 2 },
{ 36, 50, 1, 1 },
{ 37, 18, 0, 1 },
{ 37, 50, 1, 2 },
{ 38, 19, 0, 3 },
{ 38, 51, 1, 0 },
{ 39, 19, 0, 0 },
{ 39, 51, 1, 3 },
{ 40, 20, 0, 1 },
{ 40, 52, 1, 2 },
{ 41, 20, 0, 2 },
{ 41, 52, 1, 1 },
{ 42, 21, 0, 0 },
{ 42, 53, 1, 3 },
{ 43, 21, 0, 3 },
{ 43, 53, 1, 0 },
{ 44, 22, 0, 1 },
{ 44, 54, 1, 2 },
{ 45, 22, 0, 2 },
{ 45, 54, 1, 1 },
{ 46, 23, 0, 0 },
{ 46, 55, 1, 3 },
{ 47, 23, 0, 3 },
{ 47, 55, 1, 0 },
{ 48, 24, 0, 1 },
{ 48, 56, 1, 2 },
{ 49, 24, 0, 2 },
{ 49, 56, 1, 1 },
{ 50, 25, 0, 0 },
{ 50, 57, 1, 3 },
{ 51, 25, 0, 3 },
{ 51, 57, 1, 0 },
{ 52, 26, 0, 1 },
{ 52, 58, 1, 2 },
{ 53, 26, 0, 2 },
{ 53, 58, 1, 1 },
{ 54, 27, 0, 0 },
{ 54, 59, 1, 3 },
{ 55, 27, 0, 3 },
{ 55, 59, 1, 0 },
{ 56, 28, 0, 2 },
{ 56, 60, 1, 1 },
{ 57, 28, 0, 1 },
{ 57, 60, 1, 2 },
{ 58, 29, 0, 3 },
{ 58, 61, 1, 0 },
{ 59, 29, 0, 0 },
{ 59, 61, 1, 3 },
{ 60, 30, 0, 2 },
{ 60, 62, 1, 1 },
{ 61, 30, 0, 1 },
{ 61, 62, 1, 2 },
{ 62, 31, 0, 3 },
{ 62, 63, 1, 0 },
{ 63, 31, 0, 0 },
{ 63, 63, 1, 3 },
#else
//  new_state old_state   01_value   IQ_Value_Idx 

{ 0, 0, 0, 0 },
{ 0, 1, 0, 3 },

{ 1, 2, 0, 1 },
{ 1, 3, 0, 2 },

{ 2, 4, 0, 0 },
{ 2, 5, 0, 3 },

{ 3, 6, 0, 1 },
{ 3, 7, 0, 2 },

{ 4, 8, 0, 3 },
{ 4, 9, 0, 0 },

{ 5, 10, 0, 2 },
{ 5, 11, 0, 1 },

{ 6, 12, 0, 3 },
{ 6, 13, 0, 0 },

{ 7, 14, 0, 2 },
{ 7, 15, 0, 1 },

{ 8, 16, 0, 3 },
{ 8, 17, 0, 0 },

{ 9, 18, 0, 2 },
{ 9, 19, 0, 1 },

{ 10, 20, 0, 3 },
{ 10, 21, 0, 0 },

{ 11, 22, 0, 2 },
{ 11, 23, 0, 1 },

{ 12, 24, 0, 0 },
{ 12, 25, 0, 3 },

{ 13, 26, 0, 1 },
{ 13, 27, 0, 2 },

{ 14, 28, 0, 0 },
{ 14, 29, 0, 3 },

{ 15, 30, 0, 1 },
{ 15, 31, 0, 2 },

{ 16, 32, 0, 2 },
{ 16, 33, 0, 1 },

{ 17, 34, 0, 3 },
{ 17, 35, 0, 0 },

{ 18, 36, 0, 2 },
{ 18, 37, 0, 1 },

{ 19, 38, 0, 3 },
{ 19, 39, 0, 0 },

{ 20, 40, 0, 1 },
{ 20, 41, 0, 2 },

{ 21, 42, 0, 0 },
{ 21, 43, 0, 3 },

{ 22, 44, 0, 1 },
{ 22, 45, 0, 2 },

{ 23, 46, 0, 0 },
{ 23, 47, 0, 3 },

{ 24, 48, 0, 1 },
{ 24, 49, 0, 2 },

{ 25, 50, 0, 0 },
{ 25, 51, 0, 3 },

{ 26, 52, 0, 1 },
{ 26, 53, 0, 2 },

{ 27, 54, 0, 0 },
{ 27, 55, 0, 3 },

{ 28, 56, 0, 2 },
{ 28, 57, 0, 1 },

{ 29, 58, 0, 3 },
{ 29, 59, 0, 0 },

{ 30, 60, 0, 2 },
{ 30, 61, 0, 1 },

{ 31, 62, 0, 3 },
{ 31, 63, 0, 0 },

{ 32, 0, 1, 3 },
{ 32, 1, 1, 0 },

{ 33, 2, 1, 2 },
{ 33, 3, 1, 1 },

{ 34, 4, 1, 3 },
{ 34, 5, 1, 0 },

{ 35, 6, 1, 2 },
{ 35, 7, 1, 1 },

{ 36, 8, 1, 0 },
{ 36, 9, 1, 3 },

{ 37, 10, 1, 1 },
{ 37, 11, 1, 2 },

{ 38, 12, 1, 0 },
{ 38, 13, 1, 3 },

{ 39, 14, 1, 1 },
{ 39, 15, 1, 2 },

{ 40, 16, 1, 0 },
{ 40, 17, 1, 3 },

{ 41, 18, 1, 1 },
{ 41, 19, 1, 2 },

{ 42, 20, 1, 0 },
{ 42, 21, 1, 3 },

{ 43, 22, 1, 1 },
{ 43, 23, 1, 2 },

{ 44, 24, 1, 3 },
{ 44, 25, 1, 0 },

{ 45, 26, 1, 2 },
{ 45, 27, 1, 1 },

{ 46, 28, 1, 3 },
{ 46, 29, 1, 0 },

{ 47, 30, 1, 2 },
{ 47, 31, 1, 1 },

{ 48, 32, 1, 1 },
{ 48, 33, 1, 2 },

{ 49, 34, 1, 0 },
{ 49, 35, 1, 3 },

{ 50, 36, 1, 1 },
{ 50, 37, 1, 2 },

{ 51, 38, 1, 0 },
{ 51, 39, 1, 3 },

{ 52, 40, 1, 2 },
{ 52, 41, 1, 1 },

{ 53, 42, 1, 3 },
{ 53, 43, 1, 0 },

{ 54, 44, 1, 2 },
{ 54, 45, 1, 1 },

{ 55, 46, 1, 3 },
{ 55, 47, 1, 0 },

{ 56, 48, 1, 2 },
{ 56, 49, 1, 1 },

{ 57, 50, 1, 3 },
{ 57, 51, 1, 0 },

{ 58, 52, 1, 2 },
{ 58, 53, 1, 1 },

{ 59, 54, 1, 3 },
{ 59, 55, 1, 0 },

{ 60, 56, 1, 1 },
{ 60, 57, 1, 2 },

{ 61, 58, 1, 0 },
{ 61, 59, 1, 3 },

{ 62, 60, 1, 1 },
{ 62, 61, 1, 2 },

{ 63, 62, 1, 0 },
{ 63, 63, 1, 3 },
#endif
};

#define VITBI_MEMCPY(dst, src, size)           *dst = *src;  *(dst + 1) = *(src + 1);  *(dst + 2) = *(src + 2);*(dst + 3) = *(src + 3); 


MINI_ITCM_CODE_SECTION void viterbi_int_new(int32 *pInput, 
			      uint32 InputSoftbitNum, 
			      uint32 tailBiteTimes, 
			      uint32 *pDecodeOutput)
{
	uint32 i, j;
	int32  decode0, decode1;

	int32 metrics,  metricsMax;

	int32 NewMetics0, NewMetics1,NewMetics2, NewMetics3;

	int32 NewMetics_0, NewMetics_1, NewMetics_2, NewMetics_3;

	int32 UpdateMetrics0 = 0;
	int32 UpdateMetrics1 = 0;


	uint32 CorrectIdx;
	uint32 InbitIdx = 0;

	uint32 totalOutBit = 0; 

	viterbi_new_stru *Last;
	viterbi_new_stru *Now;
	viterbi_new_stru *max;

	uint32 wordIdx;
	uint32 bitIdx;
	uint32 value_bit;

	uint32 NowState = 0;

	viterbi_stru_all *p_statePara;

	uint32 value01_bit = 0;

	memset(vitStruPingPong, 0, sizeof(vitStruPingPong));
	//memset(&decodedWord, 0, sizeof(decodedWord));

	InbitIdx = 0;
	Now		= 	&vitStruPingPong[InbitIdx&0x1][0];
	Last	= 	&vitStruPingPong[1 - InbitIdx&0x1][0];
	
	/* cal metrics */   
	for (InbitIdx = 0; InbitIdx < 48; InbitIdx++)  /* InputSoftbitNum / 2  */
	{
		decode0 = pInput[(InbitIdx * 2 + 0)];
		decode1 = pInput[(InbitIdx * 2 + 1)];

		metrics_all[InbitIdx][0] =  decode1 + decode0;
		metrics_all[InbitIdx][1] = -decode1 + decode0;
		metrics_all[InbitIdx][2] =  decode1 - decode0;
		metrics_all[InbitIdx][3] = -decode1 - decode0;
	}
	/* repeat 2 times */
	memcpy(&metrics_all[48][0], &metrics_all[0][0], (sizeof(metrics_all)/2));


	#if 0
	for(InbitIdx = 1; InbitIdx < 6; InbitIdx++)
	{
		Now = 	&vitStruPingPong[InbitIdx&0x1][0];
		Last = 	&vitStruPingPong[1 - InbitIdx&0x1][0];
		memset(Now, 0, sizeof(vitStruPingPong) / 2);

		for(PathIdx = 0; PathIdx < 64; PathIdx++)
		{	
			/* two last state */
			NowState = PathIdx;
			p_statePara = &g_viterbiStateAll[NowState * 2];

			old_state_0 = p_statePara->old_state;
			
			if(Last[old_state_0].metrics != 0)
			{
				value0Or1_0 = p_statePara->value0Or1;
				IqIndex_0 = p_statePara->IqIndex;
		
				NewMetics0 = metrics_all[InbitIdx][IqIndex_0];
				UpdateMetrics0 = Last[old_state_0].metrics + NewMetics0;
			
				Now[PathIdx].metrics = UpdateMetrics0;
				continue;
				//Now[PathIdx].pdecodedWord = Last[g_stat_lastState[PathIdx][0]].pdecodedWord;
				//*(Now[PathIdx].pdecodedWord + InbitIdx) = g_stat_lastState_Value[PathIdx][0];
			}
			


			p_statePara = &g_viterbiStateAll[NowState * 2 + 1];

			old_state_1 = p_statePara->old_state;
			
			if (Last[old_state_1].metrics != 0)
			{
			
				value0Or1_1 = p_statePara->value0Or1;
				IqIndex_1 = p_statePara->IqIndex;
				
				NewMetics1 = metrics_all[InbitIdx][IqIndex_1];
				UpdateMetrics1 = Last[old_state_1].metrics + NewMetics1;

				Now[PathIdx].metrics = UpdateMetrics1;
				
				//Now[PathIdx].pdecodedWord = Last[g_stat_lastState[PathIdx][1]].pdecodedWord;
				//*(Now[PathIdx].pdecodedWord + InbitIdx) = g_stat_lastState_Value[PathIdx][1];
			}
		}
	}
#else

	Now = 	&vitStruPingPong[0x1][0];

	Now[0].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][0] + metrics_all[3][0] + metrics_all[4][0] + metrics_all[5][0];

	Now[1].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][3] + metrics_all[3][3] + metrics_all[4][0] + metrics_all[5][1];

	Now[2].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][2] + metrics_all[3][3] + metrics_all[4][3] + metrics_all[5][0];

	Now[3].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][1] + metrics_all[3][0] + metrics_all[4][3] + metrics_all[5][1];

	Now[4].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][3] + metrics_all[3][2] + metrics_all[4][3] + metrics_all[5][3];

	Now[5].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][0] + metrics_all[3][1] + metrics_all[4][3] + metrics_all[5][2];

	Now[6].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][1] + metrics_all[3][1] + metrics_all[4][0] + metrics_all[5][3];

	Now[7].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][2] + metrics_all[3][2] + metrics_all[4][0] + metrics_all[5][2];

	Now[8].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][0] + metrics_all[3][3] + metrics_all[4][2] + metrics_all[5][3];

	Now[9].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][3] + metrics_all[3][0] + metrics_all[4][2] + metrics_all[5][2];

	Now[10].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][2] + metrics_all[3][0] + metrics_all[4][1] + metrics_all[5][3];

	Now[11].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][1] + metrics_all[3][3] + metrics_all[4][1] + metrics_all[5][2];

	Now[12].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][3] + metrics_all[3][1] + metrics_all[4][1] + metrics_all[5][0];

	Now[13].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][0] + metrics_all[3][2] + metrics_all[4][1] + metrics_all[5][1];

	Now[14].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][1] + metrics_all[3][2] + metrics_all[4][2] + metrics_all[5][0];

	Now[15].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][2] + metrics_all[3][1] + metrics_all[4][2] + metrics_all[5][1];

	Now[16].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][0] + metrics_all[3][0] + metrics_all[4][3] + metrics_all[5][2];

	Now[17].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][3] + metrics_all[3][3] + metrics_all[4][3] + metrics_all[5][3];

	Now[18].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][2] + metrics_all[3][3] + metrics_all[4][0] + metrics_all[5][2];

	Now[19].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][1] + metrics_all[3][0] + metrics_all[4][0] + metrics_all[5][3];

	Now[20].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][3] + metrics_all[3][2] + metrics_all[4][0] + metrics_all[5][1];

	Now[21].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][0] + metrics_all[3][1] + metrics_all[4][0] + metrics_all[5][0];

	Now[22].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][1] + metrics_all[3][1] + metrics_all[4][3] + metrics_all[5][1];

	Now[23].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][2] + metrics_all[3][2] + metrics_all[4][3] + metrics_all[5][0];

	Now[24].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][0] + metrics_all[3][3] + metrics_all[4][1] + metrics_all[5][1];

	Now[25].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][3] + metrics_all[3][0] + metrics_all[4][1] + metrics_all[5][0];

	Now[26].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][2] + metrics_all[3][0] + metrics_all[4][2] + metrics_all[5][1];

	Now[27].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][1] + metrics_all[3][3] + metrics_all[4][2] + metrics_all[5][0];

	Now[28].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][3] + metrics_all[3][1] + metrics_all[4][2] + metrics_all[5][2];

	Now[29].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][0] + metrics_all[3][2] + metrics_all[4][2] + metrics_all[5][3];

	Now[30].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][1] + metrics_all[3][2] + metrics_all[4][1] + metrics_all[5][2];

	Now[31].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][2] + metrics_all[3][1] + metrics_all[4][1] + metrics_all[5][3];

	Now[32].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][0] + metrics_all[3][0] + metrics_all[4][0] + metrics_all[5][3];

	Now[33].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][3] + metrics_all[3][3] + metrics_all[4][0] + metrics_all[5][2];

	Now[34].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][2] + metrics_all[3][3] + metrics_all[4][3] + metrics_all[5][3];

	Now[35].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][1] + metrics_all[3][0] + metrics_all[4][3] + metrics_all[5][2];

	Now[36].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][3] + metrics_all[3][2] + metrics_all[4][3] + metrics_all[5][0];

	Now[37].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][0] + metrics_all[3][1] + metrics_all[4][3] + metrics_all[5][1];

	Now[38].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][1] + metrics_all[3][1] + metrics_all[4][0] + metrics_all[5][0];

	Now[39].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][2] + metrics_all[3][2] + metrics_all[4][0] + metrics_all[5][1];

	Now[40].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][0] + metrics_all[3][3] + metrics_all[4][2] + metrics_all[5][0];

	Now[41].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][3] + metrics_all[3][0] + metrics_all[4][2] + metrics_all[5][1];

	Now[42].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][2] + metrics_all[3][0] + metrics_all[4][1] + metrics_all[5][0];

	Now[43].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][1] + metrics_all[3][3] + metrics_all[4][1] + metrics_all[5][1];

	Now[44].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][3] + metrics_all[3][1] + metrics_all[4][1] + metrics_all[5][3];

	Now[45].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][0] + metrics_all[3][2] + metrics_all[4][1] + metrics_all[5][2];

	Now[46].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][1] + metrics_all[3][2] + metrics_all[4][2] + metrics_all[5][3];

	Now[47].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][2] + metrics_all[3][1] + metrics_all[4][2] + metrics_all[5][2];

	Now[48].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][0] + metrics_all[3][0] + metrics_all[4][3] + metrics_all[5][1];

	Now[49].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][3] + metrics_all[3][3] + metrics_all[4][3] + metrics_all[5][0];

	Now[50].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][2] + metrics_all[3][3] + metrics_all[4][0] + metrics_all[5][1];

	Now[51].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][1] + metrics_all[3][0] + metrics_all[4][0] + metrics_all[5][0];

	Now[52].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][3] + metrics_all[3][2] + metrics_all[4][0] + metrics_all[5][2];

	Now[53].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][0] + metrics_all[3][1] + metrics_all[4][0] + metrics_all[5][3];

	Now[54].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][1] + metrics_all[3][1] + metrics_all[4][3] + metrics_all[5][2];

	Now[55].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][2] + metrics_all[3][2] + metrics_all[4][3] + metrics_all[5][3];

	Now[56].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][0] + metrics_all[3][3] + metrics_all[4][1] + metrics_all[5][2];

	Now[57].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][3] + metrics_all[3][0] + metrics_all[4][1] + metrics_all[5][3];

	Now[58].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][2] + metrics_all[3][0] + metrics_all[4][2] + metrics_all[5][2];

	Now[59].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][1] + metrics_all[3][3] + metrics_all[4][2] + metrics_all[5][3];

	Now[60].metrics = metrics_all[0][0] + metrics_all[1][0] + metrics_all[2][3] + metrics_all[3][1] + metrics_all[4][2] + metrics_all[5][1];

	Now[61].metrics = metrics_all[0][3] + metrics_all[1][2] + metrics_all[2][0] + metrics_all[3][2] + metrics_all[4][2] + metrics_all[5][0];

	Now[62].metrics = metrics_all[0][0] + metrics_all[1][3] + metrics_all[2][1] + metrics_all[3][2] + metrics_all[4][1] + metrics_all[5][1];

	Now[63].metrics = metrics_all[0][3] + metrics_all[1][1] + metrics_all[2][2] + metrics_all[3][1] + metrics_all[4][1] + metrics_all[5][0];


#endif


	for(InbitIdx = 6; InbitIdx < 96; InbitIdx++)
	{
		Now = 	&vitStruPingPong[InbitIdx&0x1][0];
		Last = 	&vitStruPingPong[1 - InbitIdx&0x1][0];
		memset(Now, 0, sizeof(vitStruPingPong) / 2);

#if 1


		NewMetics_0 = metrics_all[InbitIdx][0];
		NewMetics_1 = metrics_all[InbitIdx][1];
		NewMetics_2 = metrics_all[InbitIdx][2];
		NewMetics_3 = metrics_all[InbitIdx][3];
	
		//for(PathIdx = 0; PathIdx < 1; PathIdx++)

		/* path 0 */
		UpdateMetrics0 = Last[0].metrics + NewMetics_0;

		UpdateMetrics1 = Last[1].metrics + NewMetics_3;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[0].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[0].decodedWord,  
				   Last[0].decodedWord, 
				   16);
		}
		else
		{
			Now[0].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[0].decodedWord,
				   Last[1].decodedWord,
				   16);	
		}

		/* path 1 */
		//NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[2].metrics + NewMetics_1;


		//NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[3].metrics + NewMetics_2;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[1].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[1].decodedWord,  
				   Last[2].decodedWord, 
				   16);	
		}
		else
		{
			Now[1].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[1].decodedWord,
				   Last[3].decodedWord,
				   16);	
		}

		/* path 2 */
		//NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[4].metrics + NewMetics_0;


		//NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[5].metrics + NewMetics_3;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[2].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[2].decodedWord,  
				   Last[4].decodedWord, 
				   16);	
		}
		else
		{
			Now[2].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[2].decodedWord,
				   Last[5].decodedWord,
				   16);	
		}

		/* path 3 */
		//NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[6].metrics + NewMetics_1;


		//NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[7].metrics + NewMetics_2;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[3].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[3].decodedWord,  
				   Last[6].decodedWord, 
				   16);	
		}
		else
		{
			Now[3].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[3].decodedWord,
				   Last[7].decodedWord,
				   16);	
		}

		/* path 4 */
		//NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[8].metrics + NewMetics_3;


		//NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[9].metrics + NewMetics_0;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[4].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[4].decodedWord,  
				   Last[8].decodedWord, 
				   16);	
		}
		else
		{
			Now[4].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[4].decodedWord,
				   Last[9].decodedWord,
				   16);	
		}
		

		/* path 5 */
		//NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[10].metrics + NewMetics_2;


		//NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[11].metrics + NewMetics_1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[5].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[5].decodedWord,  
				   Last[10].decodedWord, 
				   16);	
		}
		else
		{
			Now[5].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[5].decodedWord,
				   Last[11].decodedWord,
				   16);	
		}

		/* path 6 */
		//NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[12].metrics + NewMetics_3;


		//NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[13].metrics + NewMetics_0;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[6].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[6].decodedWord,  
				   Last[12].decodedWord, 
				   16);	
		}
		else
		{
			Now[6].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[6].decodedWord,
				   Last[13].decodedWord,
				   16);	
		}

		/* path 7 */
		//NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[14].metrics + NewMetics_2;


		//NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[15].metrics + NewMetics_1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[7].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[7].decodedWord,  
				   Last[14].decodedWord, 
				   16);	
		}
		else
		{
			Now[7].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[7].decodedWord,
				   Last[15].decodedWord,
				   16);	
		}

		/* path 8 */
		//NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[16].metrics + NewMetics_3;


		//NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[17].metrics + NewMetics_0;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[8].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[8].decodedWord,  
				   Last[16].decodedWord, 
				   16);	
		}
		else
		{
			Now[8].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[8].decodedWord,
				   Last[17].decodedWord,
				   16);	
		}

		/* path 9 */
		//NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[18].metrics + NewMetics_2;


		//NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[19].metrics + NewMetics_1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[9].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[9].decodedWord,  
				   Last[18].decodedWord, 
				   16);	
		}
		else
		{
			Now[9].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[9].decodedWord,
				   Last[19].decodedWord,
				   16);	
		}

		/* path `10 */
		//NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[20].metrics + NewMetics_3;


		//NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[21].metrics + NewMetics_0;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[10].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[10].decodedWord,  
				   Last[20].decodedWord, 
				   16);	
		}
		else
		{
			Now[10].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[10].decodedWord,
				   Last[21].decodedWord,
				   16);	
		}

		/* path 11 */
		//NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[22].metrics + NewMetics_2;


		//NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[23].metrics + NewMetics_1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[11].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[11].decodedWord,  
				   Last[22].decodedWord, 
				   16);	
		}
		else
		{
			Now[11].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[11].decodedWord,
				   Last[23].decodedWord,
				   16);	
		}

		/* path 12 */
		//NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[24].metrics + NewMetics_0;


		//NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[25].metrics + NewMetics_3;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[12].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[12].decodedWord,  
				   Last[24].decodedWord, 
				   16);	
		}
		else
		{
			Now[12].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[12].decodedWord,
				   Last[25].decodedWord,
				   16);	
		}

		/* path 13 */
		//NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[26].metrics + NewMetics_1;


		//NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[27].metrics + NewMetics_2;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[13].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[13].decodedWord,  
				   Last[26].decodedWord, 
				   16);	
		}
		else
		{
			Now[13].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[13].decodedWord,
				   Last[27].decodedWord,
				   16);	
		}

		/* path 14 */
		//NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[28].metrics + NewMetics_0;


		//NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[29].metrics + NewMetics_3;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[14].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[14].decodedWord,  
				   Last[28].decodedWord, 
				   16);	
		}
		else
		{
			Now[14].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[14].decodedWord,
				   Last[29].decodedWord,
				   16);	
		}

		/* path 15 */
		//NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[30].metrics + NewMetics_1;


		//NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[31].metrics + NewMetics_2;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[15].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[15].decodedWord,  
				   Last[30].decodedWord, 
				   16);	
		}
		else
		{
			Now[15].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[15].decodedWord,
				   Last[31].decodedWord,
				   16);	
		}

		/* path 16 */
		//NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[32].metrics + NewMetics_2;


		//NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[33].metrics + NewMetics_1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[16].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[16].decodedWord,  
				   Last[32].decodedWord, 
				   16);	
		}
		else
		{
			Now[16].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[16].decodedWord,
				   Last[33].decodedWord,
				   16);	
		}

		/* path 17 */
		//NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[34].metrics + NewMetics_3;


		//NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[35].metrics + NewMetics_0;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[17].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[17].decodedWord,  
				   Last[34].decodedWord, 
				   16);	
		}
		else
		{
			Now[17].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[17].decodedWord,
				   Last[35].decodedWord,
				   16);	
		}

		/* path 18 */
		//NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[36].metrics + NewMetics_2;


		//NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[37].metrics + NewMetics_1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[18].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[18].decodedWord,  
				   Last[36].decodedWord, 
				   16);	
		}
		else
		{
			Now[18].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[18].decodedWord,
				   Last[37].decodedWord,
				   16);	
		}

		/* path 19 */
		//NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[38].metrics + NewMetics_3;


		//NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[39].metrics + NewMetics_0;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[19].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[19].decodedWord,  
				   Last[38].decodedWord, 
				   16);	
		}
		else
		{
			Now[19].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[19].decodedWord,
				   Last[39].decodedWord,
				   16);	
		}

		/* path 20 */
		//NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[40].metrics + NewMetics_1;


		//NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[41].metrics + NewMetics_2;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[20].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[20].decodedWord,  
				   Last[40].decodedWord, 
				   16);	
		}
		else
		{
			Now[20].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[20].decodedWord,
				   Last[41].decodedWord,
				   16);	
		}

		/* path 21 */
		//NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[42].metrics + NewMetics_0;


		//NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[43].metrics + NewMetics_3;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[21].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[21].decodedWord,  
				   Last[42].decodedWord, 
				   16);	
		}
		else
		{
			Now[21].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[21].decodedWord,
				   Last[43].decodedWord,
				   16);	
		}

		/* path 22 */
		//NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[44].metrics + NewMetics_1;


		//NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[45].metrics + NewMetics_2;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[22].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[22].decodedWord,  
				   Last[44].decodedWord, 
				   16);	
		}
		else
		{
			Now[22].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[22].decodedWord,
				   Last[45].decodedWord,
				   16);	
		}

		/* path 23 */
		//NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[46].metrics + NewMetics_0;


		//NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[47].metrics + NewMetics_3;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[23].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[23].decodedWord,  
				   Last[46].decodedWord, 
				   16);	
		}
		else
		{
			Now[23].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[23].decodedWord,
				   Last[47].decodedWord,
				   16);	
		}

		/* path 24 */
		//NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[48].metrics + NewMetics_1;


		//NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[49].metrics + NewMetics_2;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[24].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[24].decodedWord,  
				   Last[48].decodedWord, 
				   16);	
		}
		else
		{
			Now[24].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[24].decodedWord,
				   Last[49].decodedWord,
				   16);	
		}

		/* path 25 */
		//NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[50].metrics + NewMetics_0;


		//NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[51].metrics + NewMetics_3;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[25].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[25].decodedWord,  
				   Last[50].decodedWord, 
				   16);	
		}
		else
		{
			Now[25].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[25].decodedWord,
				   Last[51].decodedWord,
				   16);	
		}

		/* path 26 */
		//NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[52].metrics + NewMetics_1;


		//NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[53].metrics + NewMetics_2;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[26].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[26].decodedWord,  
				   Last[52].decodedWord, 
				   16);	
		}
		else
		{
			Now[26].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[26].decodedWord,
				   Last[53].decodedWord,
				   16);	
		}

		/* path 27 */
		//NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[54].metrics + NewMetics_0;


		//NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[55].metrics + NewMetics_3;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[27].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[27].decodedWord,  
				   Last[54].decodedWord, 
				   16);	
		}
		else
		{
			Now[27].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[27].decodedWord,
				   Last[55].decodedWord,
				   16);	
		}

		/* path 28 */
		//NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[56].metrics + NewMetics_2;


		//NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[57].metrics + NewMetics_1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[28].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[28].decodedWord,  
				   Last[56].decodedWord, 
				   16);	
		}
		else
		{
			Now[28].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[28].decodedWord,
				   Last[57].decodedWord,
				   16);	
		}

		/* path 29 */
		//NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[58].metrics + NewMetics_3;


		//NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[59].metrics + NewMetics_0;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[29].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[29].decodedWord,  
				   Last[58].decodedWord, 
				   16);	
		}
		else
		{
			Now[29].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[29].decodedWord,
				   Last[59].decodedWord,
				   16);	
		}

		/* path 30 */
		//NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[60].metrics + NewMetics_2;


		//NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[61].metrics + NewMetics_1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[30].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[30].decodedWord,  
				   Last[60].decodedWord, 
				   16);	
		}
		else
		{
			Now[30].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[30].decodedWord,
				   Last[61].decodedWord,
				   16);	
		}

		/* path 31 */
		//NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[62].metrics + NewMetics_3;


		//NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[63].metrics + NewMetics_0;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[31].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[31].decodedWord,  
				   Last[62].decodedWord, 
				   16);	
		}
		else
		{
			Now[31].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[31].decodedWord,
				   Last[63].decodedWord,
				   16);	
		}

#endif

		wordIdx = (InbitIdx>>5);
		bitIdx = (InbitIdx&0x1F);
		value01_bit = (1 << (31 - bitIdx));
	
		/* path 32 */
		NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[0].metrics + NewMetics0;

		NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[1].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[32].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[32].decodedWord,  
				   Last[0].decodedWord, 
				   16);	
		}
		else
		{
			Now[32].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[32].decodedWord,
				   Last[1].decodedWord,
				   16);	
		}
		//Now[32].decodedWord[wordIdx] += value01_bit;
		
		/* path 33 */
		NewMetics0 = metrics_all[InbitIdx][2];
			UpdateMetrics0 = Last[2].metrics + NewMetics0;


			NewMetics1 = metrics_all[InbitIdx][1];
			UpdateMetrics1 = Last[3].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[33].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[33].decodedWord,  
				   Last[2].decodedWord, 
				   16);	
		}
		else
		{
			Now[33].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[33].decodedWord,
				   Last[3].decodedWord,
				   16);	
		}
		//Now[33].decodedWord[wordIdx] += value01_bit;

		/* path 34 */
		NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[4].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[5].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[34].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[34].decodedWord,  
				   Last[4].decodedWord, 
				   16);	
		}
		else
		{
			Now[34].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[34].decodedWord,
				   Last[5].decodedWord,
				   16);	
		}
		//Now[34].decodedWord[wordIdx] += value01_bit;


		/* path 35 */
		NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[6].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[7].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[35].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[35].decodedWord,  
				   Last[6].decodedWord, 
				   16);	
		}
		else
		{
			Now[35].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[35].decodedWord,
				   Last[7].decodedWord,
				   16);	
		}
		//Now[35].decodedWord[wordIdx] += value01_bit;


		/* path 36 */
		NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[8].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[9].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[36].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[36].decodedWord,  
				   Last[8].decodedWord, 
				   16);	
		}
		else
		{
			Now[36].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[36].decodedWord,
				   Last[9].decodedWord,
				   16);	
		}
		//Now[36].decodedWord[wordIdx] += value01_bit;


		/* path 37 */
		NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[10].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[11].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[37].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[37].decodedWord,  
				   Last[10].decodedWord, 
				   16);	
		}
		else
		{
			Now[37].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[37].decodedWord,
				   Last[11].decodedWord,
				   16);	
		}
		//Now[37].decodedWord[wordIdx] += value01_bit;


		/* path 38 */
		NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[12].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[13].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[38].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[38].decodedWord,  
				   Last[12].decodedWord, 
				   16);	
		}
		else
		{
			Now[38].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[38].decodedWord,
				   Last[13].decodedWord,
				   16);	
		}
		//Now[38].decodedWord[wordIdx] += value01_bit;


		/* path 39 */
		NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[14].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[15].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[39].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[39].decodedWord,  
				   Last[14].decodedWord, 
				   16);	
		}
		else
		{
			Now[39].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[39].decodedWord,
				   Last[15].decodedWord,
				   16);	
		}
		//Now[39].decodedWord[wordIdx] += value01_bit;


		/* path 40 */
		NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[16].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[17].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[40].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[40].decodedWord,  
				   Last[16].decodedWord, 
				   16);	
		}
		else
		{
			Now[40].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[40].decodedWord,
				   Last[17].decodedWord,
				   16);	
		}
		//Now[40].decodedWord[wordIdx] += value01_bit;


		/* path 41 */
		NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[18].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[19].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[41].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[41].decodedWord,  
				   Last[18].decodedWord, 
				   16);	
		}
		else
		{
			Now[41].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[41].decodedWord,
				   Last[19].decodedWord,
				   16);	
		}
		//Now[41].decodedWord[wordIdx] += value01_bit;


		/* path 42 */
		NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[20].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[21].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[42].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[42].decodedWord,  
				   Last[20].decodedWord, 
				   16);	
		}
		else
		{
			Now[42].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[42].decodedWord,
				   Last[21].decodedWord,
				   16);	
		}
		//Now[42].decodedWord[wordIdx] += value01_bit;


		/* path 43 */
		NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[22].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[23].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[43].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[43].decodedWord,  
				   Last[22].decodedWord, 
				   16);	
		}
		else
		{
			Now[43].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[43].decodedWord,
				   Last[23].decodedWord,
				   16);	
		}
		//Now[43].decodedWord[wordIdx] += value01_bit;

		/* path 44 */
		NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[24].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[25].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[44].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[44].decodedWord,  
				   Last[24].decodedWord, 
				   16);	
		}
		else
		{
			Now[44].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[44].decodedWord,
				   Last[25].decodedWord,
				   16);	
		}
		//Now[44].decodedWord[wordIdx] += value01_bit;


		/* path 45 */
		NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[26].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[27].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[45].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[45].decodedWord,  
				   Last[26].decodedWord, 
				   16);	
		}
		else
		{
			Now[45].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[45].decodedWord,
				   Last[27].decodedWord,
				   16);	
		}
		//Now[45].decodedWord[wordIdx] += value01_bit;

		/* path 46 */
		NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[28].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[29].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[46].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[46].decodedWord,  
				   Last[28].decodedWord, 
				   16);	
		}
		else
		{
			Now[46].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[46].decodedWord,
				   Last[29].decodedWord,
				   16);	
		}
		//Now[46].decodedWord[wordIdx] += value01_bit;


		/* path 47 */
		NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[30].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[31].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[47].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[47].decodedWord,  
				   Last[30].decodedWord, 
				   16);	
		}
		else
		{
			Now[47].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[47].decodedWord,
				   Last[31].decodedWord,
				   16);	
		}
		//Now[47].decodedWord[wordIdx] += value01_bit;


		/* path 48 */
		NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[32].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[33].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[48].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[48].decodedWord,  
				   Last[32].decodedWord, 
				   16);	
		}
		else
		{
			Now[48].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[48].decodedWord,
				   Last[33].decodedWord,
				   16);	
		}
		//Now[48].decodedWord[wordIdx] += value01_bit;


		/* path 49 */
		NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[34].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[35].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[49].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[49].decodedWord,  
				   Last[34].decodedWord, 
				   16);	
		}
		else
		{
			Now[49].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[49].decodedWord,
				   Last[35].decodedWord,
				   16);	
		}
		//Now[49].decodedWord[wordIdx] += value01_bit;


		/* path 50 */
		NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[36].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[37].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[50].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[50].decodedWord,  
				   Last[36].decodedWord, 
				   16);	
		}
		else
		{
			Now[50].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[50].decodedWord,
				   Last[37].decodedWord,
				   16);	
		}
		//Now[50].decodedWord[wordIdx] += value01_bit;


		/* path 51 */
		NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[38].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[39].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[51].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[51].decodedWord,  
				   Last[38].decodedWord, 
				   16);	
		}
		else
		{
			Now[51].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[51].decodedWord,
				   Last[39].decodedWord,
				   16);	
		}
		//Now[51].decodedWord[wordIdx] += value01_bit;


		/* path 52 */
		NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[40].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[41].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[52].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[52].decodedWord,  
				   Last[40].decodedWord, 
				   16);	
		}
		else
		{
			Now[52].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[52].decodedWord,
				   Last[41].decodedWord,
				   16);	
		}
		//Now[52].decodedWord[wordIdx] += value01_bit;


		/* path 53 */
		NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[42].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[43].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[53].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[53].decodedWord,  
				   Last[42].decodedWord, 
				   16);	
		}
		else
		{
			Now[53].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[53].decodedWord,
				   Last[43].decodedWord,
				   16);	
		}
		//Now[53].decodedWord[wordIdx] += value01_bit;


		/* path 54 */
		NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[44].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[45].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[54].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[54].decodedWord,  
				   Last[44].decodedWord, 
				   16);	
		}
		else
		{
			Now[54].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[54].decodedWord,
				   Last[45].decodedWord,
				   16);	
		}
		//Now[54].decodedWord[wordIdx] += value01_bit;


		/* path 55 */
		NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[46].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[47].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[55].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[55].decodedWord,  
				   Last[46].decodedWord, 
				   16);	
		}
		else
		{
			Now[55].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[55].decodedWord,
				   Last[47].decodedWord,
				   16);	
		}
		//Now[55].decodedWord[wordIdx] += value01_bit;


		/* path 56 */
		NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[48].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[49].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[56].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[56].decodedWord,  
				   Last[48].decodedWord, 
				   16);	
		}
		else
		{
			Now[56].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[56].decodedWord,
				   Last[49].decodedWord,
				   16);	
		}
		//Now[56].decodedWord[wordIdx] += value01_bit;


		/* path 57 */
		NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[50].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[51].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[57].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[57].decodedWord,  
				   Last[50].decodedWord, 
				   16);	
		}
		else
		{
			Now[57].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[57].decodedWord,
				   Last[51].decodedWord,
				   16);	
		}
		//Now[57].decodedWord[wordIdx] += value01_bit;


		/* path 58 */
		NewMetics0 = metrics_all[InbitIdx][2];
		UpdateMetrics0 = Last[52].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][1];
		UpdateMetrics1 = Last[53].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[58].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[58].decodedWord,  
				   Last[52].decodedWord, 
				   16);	
		}
		else
		{
			Now[58].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[58].decodedWord,
				   Last[53].decodedWord,
				   16);	
		}
		//Now[58].decodedWord[wordIdx] += value01_bit;


		/* path 59 */
		NewMetics0 = metrics_all[InbitIdx][3];
		UpdateMetrics0 = Last[54].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][0];
		UpdateMetrics1 = Last[55].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[59].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[59].decodedWord,  
				   Last[54].decodedWord, 
				   16);	
		}
		else
		{
			Now[59].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[59].decodedWord,
				   Last[55].decodedWord,
				   16);	
		}
		//Now[59].decodedWord[wordIdx] += value01_bit;


		/* path 60 */
		NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[56].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[57].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[60].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[60].decodedWord,  
				   Last[56].decodedWord, 
				   16);	
		}
		else
		{
			Now[60].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[60].decodedWord,
				   Last[57].decodedWord,
				   16);	
		}
		//Now[60].decodedWord[wordIdx] += value01_bit;


		/* path 61 */
		NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[58].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[59].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[61].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[61].decodedWord,  
				   Last[58].decodedWord, 
				   16);	
		}
		else
		{
			Now[61].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[61].decodedWord,
				   Last[59].decodedWord,
				   16);	
		}
		//Now[61].decodedWord[wordIdx] += value01_bit;


		/* path 62 */
		NewMetics0 = metrics_all[InbitIdx][1];
		UpdateMetrics0 = Last[60].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][2];
		UpdateMetrics1 = Last[61].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[62].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[62].decodedWord,  
				   Last[60].decodedWord, 
				   16);	
		}
		else
		{
			Now[62].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[62].decodedWord,
				   Last[61].decodedWord,
				   16);	
		}
		//Now[62].decodedWord[wordIdx] += value01_bit;


		/* path 63 */
		NewMetics0 = metrics_all[InbitIdx][0];
		UpdateMetrics0 = Last[62].metrics + NewMetics0;


		NewMetics1 = metrics_all[InbitIdx][3];
		UpdateMetrics1 = Last[63].metrics + NewMetics1;

		if(UpdateMetrics0 > UpdateMetrics1)
		{
			Now[63].metrics = UpdateMetrics0;
			VITBI_MEMCPY(Now[63].decodedWord,  
				   Last[62].decodedWord, 
				   16);	
		}
		else
		{
			Now[63].metrics = UpdateMetrics1;
			VITBI_MEMCPY(Now[63].decodedWord,
				   Last[63].decodedWord,
				   16);	
		}
		//Now[63].decodedWord[wordIdx] += value01_bit;

		if((InbitIdx > 23) && (InbitIdx < 72))
		{
			for(i = 32; i < 64; i++)
			{
				Now[i].decodedWord[wordIdx] += value01_bit;
			}
		}

		#if 0
		for(PathIdx = 32; PathIdx < 64; PathIdx++)
		{	
			/* two last state */
			NowState = PathIdx;
			p_statePara = &g_viterbiStateAll[NowState * 2];

			old_state_0 = p_statePara->old_state;
			IqIndex_0 = p_statePara->IqIndex;
	
			NewMetics0 = metrics_all[InbitIdx][IqIndex_0];
			UpdateMetrics0 = Last[old_state_0].metrics + NewMetics0;

			p_statePara = &g_viterbiStateAll[NowState * 2 + 1];

			old_state_1 = p_statePara->old_state;
			value0Or1_1 = p_statePara->value0Or1;
			IqIndex_1 = p_statePara->IqIndex;
			
			NewMetics1 = metrics_all[InbitIdx][IqIndex_1];
			UpdateMetrics1 = Last[old_state_1].metrics + NewMetics1;

		
			#if 1
			if(UpdateMetrics0 > UpdateMetrics1)
			{
				Now[PathIdx].metrics = UpdateMetrics0;

				VITBI_MEMCPY(Now[PathIdx].decodedWord,  
					   Last[old_state_0].decodedWord, 
					   16);

			}
			else
			{
				Now[PathIdx].metrics = UpdateMetrics1;

				VITBI_MEMCPY(Now[PathIdx].decodedWord,  
					   Last[old_state_1].decodedWord, 
					   16);
				
			}
			Now[PathIdx].decodedWord[wordIdx] += value01_bit;
			
			#endif
			
		}
		#endif

			
	}

	


	/* search the biggest metrics path */
	metricsMax = 0;
	CorrectIdx = 0;
	for (j = 0; j < 64; j++)
	{
		max = (Now + j);
		metrics = max->metrics;
		if (metrics > metricsMax)
		{
			CorrectIdx = j;
			metricsMax = metrics;
		}
	}

	//uint32 MaxPathIdx = Now[CorrectIdx].PathIdx;

		/* copy result to output */
	memcpy(pDecodeOutput,
			Now[CorrectIdx].decodedWord,
			12);
}





