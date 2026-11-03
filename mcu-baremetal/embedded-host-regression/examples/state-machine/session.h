#ifndef DEMO_SESSION_H
#define DEMO_SESSION_H
#include <stdint.h>
typedef struct { uint32_t started; uint8_t active; } DemoSession;
void demo_start(DemoSession *s, uint32_t now);
void demo_process(DemoSession *s, uint32_t now);
#endif
