#ifndef __GLOBAL_H
#define __GLOBAL_H

#define TAB2_LEN 	4

#define TIMER_START() 	(T0TCR = 0x01)
#define TIMER_STOP()  	(T0TCR = 0x00)
#define TIMER_RESET()	(T0TCR = 0x02)

#define NO_BITS 0x00
#define BIT0    0x01
#define BIT1    0x02
#define BIT2    0x04
#define BIT3    0x08
#define BIT4    0x10
#define BIT5    0x20
#define BIT6    0x40
#define BIT7    0x80
#define BIT8    0x0100
#define BIT9    0x0200
#define BIT10   0x0400
#define BIT11   0x0800
#define BIT12   0x1000
#define BIT13   0x2000
#define BIT14   0x4000
#define BIT15   0x8000
#define BIT16   0x010000
#define BIT17   0x020000
#define BIT18   0x040000
#define BIT19   0x080000
#define BIT20   0x100000
#define BIT21   0x200000
#define BIT22   0x400000
#define BIT23   0x800000
#define BIT24   0x01000000
#define BIT25   0x02000000
#define BIT26   0x04000000
#define BIT27   0x08000000
#define BIT28   0x10000000
#define BIT29   0x20000000
#define BIT30   0x40000000
#define BIT31   0x80000000

extern const unsigned char tab_lsb2[];
extern const unsigned char tab_lsb3[];
extern unsigned int height;
extern unsigned int width;
extern unsigned long int time;
extern unsigned long int image_size;

extern unsigned int decodedSize;
extern unsigned int decodedHeight;
extern unsigned int decodedWidth;
extern unsigned char *image;
extern unsigned char *image2;

void initTimer(void);
 
#endif
