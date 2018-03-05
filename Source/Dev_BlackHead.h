

#ifndef DEV_BLACKHEAD_H
#define DEV_BLACKHEAD_H

#define BLACKHEAD_MEASURE_STOP      0x00
#define BLACKHEAD_MEASURE_START     0x01


extern void BlackHead_Init();

extern void BlackHead_Start();

extern uint16 BlackHead_ReadTemperature();

extern void BlackHead_Stop();



#endif
