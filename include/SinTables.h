#ifndef SINTABLE_H__INCLUDED
#define SINTABLE_H__INCLUDED

// from https://forum.arduino.cc/index.php?topic=69723.0 with thanks
// only skimmed before i yoinked, might be a faster/more accurate version in the thread

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cinttypes>
#endif

extern unsigned int isinTable16[];
extern uint8_t isinTable8[];

float isin(long x);
float icos(long x);
float itan(long x);
float fsin(float d);

#endif