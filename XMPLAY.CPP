// DPMI only.. May not run in more RealMode environments..
// Works good with PMODE/W and seems not to work with pure DOS4GW
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include "modplug.h"

void (__interrupt __far timerIrqHandler) (void);
typedef void (__interrupt __far* TimerIrqHandler)();

const int HARDWARE_TIMER_FREQUENCY = 1193180; // 1193180 / 65536  = 18.2
TimerIrqHandler oldTimerIrqHandler;
volatile unsigned int timerIrqMultiplier = 4096; // 18.2 * 128 = 2330.43 hz (65536 must be dividable by this)
volatile unsigned int timerIrqMulCounter = 0;
const double baseFreq = (double)HARDWARE_TIMER_FREQUENCY / 65536 * timerIrqMultiplier;

#define SOUND_RING_BUFFER_SIZE 1024
volatile unsigned char sound_ringbuffer[SOUND_RING_BUFFER_SIZE];
int soundRingBufferReadPos = 0;
int soundLastVolume = 0;

void enableTimerIrq();
void disableTimerIrq();

ModPlugFile *xm;

bool loadMusic(const char *fileName) {
  ModPlug_Settings s;
  ModPlug_GetSettings(&s);
  s.mChannels = 1;
  s.mFrequency = baseFreq;
  s.mBits = 8;
  ModPlug_SetSettings(&s);
  FILE *in = fopen(fileName,"rb");
  if (in == NULL) 
    return false;
  fseek(in,0,SEEK_END);
  int fileLen = ftell(in);
  fseek(in,0,SEEK_SET);
  unsigned char *data = new unsigned char[fileLen];
  fread(data,1,fileLen,in);
  xm =  ModPlug_Load(data, fileLen);
  delete[] data;
  fclose(in);

  soundRingBufferReadPos = 0;
  soundLastVolume = 0;
  memset((void*)sound_ringbuffer,0,sizeof(sound_ringbuffer));

  return true;
}

void unloadMusic() {
  ModPlug_Unload(xm);
}
                  
void (__interrupt __far timerIrqHandler) (void)
{
  static char *vidmem = (char *)0xb8000;
  vidmem[158]++;

  // do other timer stuff
  int a = inp(0x61) & (~2);
  int volume = sound_ringbuffer[soundRingBufferReadPos];
  int k = volume - soundLastVolume > 0 ? 1 : 0;
  soundLastVolume = volume;
  outp(0x61,a|(k<<1));
  soundRingBufferReadPos++;
  soundRingBufferReadPos %= SOUND_RING_BUFFER_SIZE;
  if (soundRingBufferReadPos == 0x00) {
    ModPlug_Read(xm, (void*)&sound_ringbuffer[SOUND_RING_BUFFER_SIZE/2], SOUND_RING_BUFFER_SIZE/2);
  }                                                     
  if (soundRingBufferReadPos == SOUND_RING_BUFFER_SIZE/2) {
    ModPlug_Read(xm, (void*)&sound_ringbuffer[0], SOUND_RING_BUFFER_SIZE/2);
  }                                                     

  timerIrqMulCounter++;
  if (timerIrqMulCounter>=timerIrqMultiplier) {
    timerIrqMulCounter = 0;
    oldTimerIrqHandler();
  } else {
    outp(0x20,0x20);
  }
}

void enableTimerIrq() {
  union REGS r;
  unsigned   rmvector;

  r.x.eax = 0x204;
  r.h.bl = 0x08;
  int386 (0x31, &r, &r);
  oldTimerIrqHandler = (TimerIrqHandler)MK_FP(r.x.ecx,r.x.edx);

  r.x.eax = 0x205;
  r.h.bl = 0x08;
  r.x.ecx = FP_SEG(timerIrqHandler);
  r.x.edx = FP_OFF(timerIrqHandler);
  int386 (0x31, &r, &r);

  double pitBase = HARDWARE_TIMER_FREQUENCY/65536.0; // around 18.2
  double freq = pitBase*timerIrqMultiplier;
  double val = HARDWARE_TIMER_FREQUENCY/freq;
  int here = (int)floor(val);
  timerIrqMulCounter = 0;
  outp(0x43,0x36);
  outp(0x40,here & 255);
  outp(0x40,here / 256);
  printf("freq:%f\n",freq);
}

void disableTimerIrq() {
  union REGS r;
  unsigned   rmvector;

  r.x.eax = 0x205;
  r.h.bl = 0x08;
  r.x.ecx = FP_SEG(oldTimerIrqHandler);
  r.x.edx = FP_OFF(oldTimerIrqHandler);
  int386 (0x31, &r, &r);
  outp(0x43,0x36);
  outp(0x40,0); // both 0 = 65536 = 1193180.0/65536.0 = ~18.2 hz
  outp(0x40,0);
}

void main (void)         
{
  if (!loadMusic("0002.XM"))
    exit(0);

  enableTimerIrq();
  getch();
  disableTimerIrq();

  unloadMusic();
}
