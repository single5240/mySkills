#include "link.h"
void demo_frame(uint8_t out[5], uint8_t target, uint16_t token, uint8_t command)
{
    out[0] = target; out[1] = (uint8_t)token;
    out[2] = (uint8_t)(token >> 8); out[3] = command;
    out[4] = (uint8_t)(out[0] + out[1] + out[2] + out[3]);
}
void demo_receive(DemoReceiver *r, const uint8_t *frame, size_t size)
{
    uint8_t sum;
    uint16_t token;
    if(!frame || size != 5U) return;
    sum = (uint8_t)(frame[0] + frame[1] + frame[2] + frame[3]);
    if(sum != frame[4]) return;
    token = (uint16_t)(frame[1] | ((uint16_t)frame[2] << 8));
    if(!token) return;
    if(frame[3] == DEMO_SWITCH) {
        if(frame[0] < 1U || frame[0] > 10U) return;
        r->token = token; r->current = frame[0];
    } else if(token == r->token && frame[3] == DEMO_SAVE && frame[0] == r->current) {
        if(r->saved != r->current) { r->saved = r->current; r->writes++; }
    } else if(token == r->token && frame[3] == DEMO_RETURN) {
        r->current = r->saved;
    }
}
