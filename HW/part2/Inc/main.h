#ifndef MAIN_H_
#define MAIN_H_


//#include "stm32f1xx.h"
#include "stm32f100xb.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "math.h"

#endif /* MAIN_H_ */

#define RX_BUFF_SIZE 512
#define TX_BUFF_SIZE 128


void initTIM2(void);
void initUSART1(void);
void txStr(char *str, bool crlf);
void ExecuteCommand(void);
bool checkString(char* str);
int findSemicolon(char* str);
int checkDigit(char* str);
void voltValues(char* str, float* res);
void initDAC(void);
float max(float* array);
