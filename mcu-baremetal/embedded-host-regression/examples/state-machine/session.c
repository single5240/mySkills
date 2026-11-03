#include "session.h"
/* 教学模块：服务间隔需小于计数器一整圈；不代表目标实际计时。 */
void demo_start(DemoSession *s, uint32_t now)
{
    s->started = now;
    s->active = 1U;
}
void demo_process(DemoSession *s, uint32_t now)
{
    if(s->active && (uint32_t)(now - s->started) >= 12000U)
        s->active = 0U;
}
