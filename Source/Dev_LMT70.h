

#ifndef DEV_LMT70_H
#define DEV_LMT70_H

#define LMT_MEASURE_STOP      0x00
#define LMT_MEASURE_START     0x01


extern void Lmt70_Init();

extern void Lmt70_Start();

extern uint16 Lmt70_ReadTemperature();

extern void Lmt70_Stop();



#endif
