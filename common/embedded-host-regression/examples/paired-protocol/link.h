#ifndef DEMO_LINK_H
#define DEMO_LINK_H
#include <stddef.h>
#include <stdint.h>
/* 教学私有协议，与任何产品或芯片协议无关。 */
#define DEMO_SWITCH 0x31U
#define DEMO_SAVE   0x32U
#define DEMO_RETURN 0x33U
typedef struct {
    uint8_t current, saved;
    uint16_t token;
    unsigned int writes;
} DemoReceiver;
void demo_frame(uint8_t out[5], uint8_t target, uint16_t token, uint8_t command);
void demo_receive(DemoReceiver *r, const uint8_t *frame, size_t size);
#endif
