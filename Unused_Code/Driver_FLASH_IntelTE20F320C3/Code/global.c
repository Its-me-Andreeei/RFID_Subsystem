#include <lpc22xx.h>
#include "global.h"




//const unsigned char tab_lsb2[] = { 0, 255 / 3, 510 / 3, 255 };
//const unsigned char tab_lsb3[] = { 0, 255 / 7, 2 * 255 / 7, 3 * 255 / 7, 4 * 255 / 7, 5 * 255 / 7, 6 * 255 / 7, 255 };

unsigned int height = 0;			// image height
unsigned int width = 0;				// image width
unsigned long int time = 0;			// decode time
unsigned long int image_size = 0; 	// image size in bytes


// variabile YUV
unsigned int decodedSize = 0;
unsigned int decodedHeight = 0;
unsigned int decodedWidth = 0;

unsigned char *image;
unsigned char *image2;

void initTimer(void)
{
	T0IR = 0;
	T0PR = 0;
	T0PC = 0;
	T0MCR = 0;
}
