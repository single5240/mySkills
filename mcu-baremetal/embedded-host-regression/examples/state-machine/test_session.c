#include <stdio.h>
#include "session.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while(0)
int main(void)
{
    DemoSession s;
    uint32_t start;
    demo_start(&s, 100U);
    demo_process(&s, 12099U); CHECK(s.active);
    demo_process(&s, 12100U); CHECK(!s.active);
    demo_process(&s, 20000U); CHECK(!s.active);
    /* 重新启动应建立新一轮；不需要在电脑上真的等待12秒。 */
    demo_start(&s, 30000U);
    demo_start(&s, 31000U);
    demo_process(&s, 42000U); CHECK(s.active);
    demo_process(&s, 43001U); CHECK(!s.active);
    start = UINT32_MAX - 500U;
    demo_start(&s, start);
    demo_process(&s, (uint32_t)(start + 11999U)); CHECK(s.active);
    demo_process(&s, (uint32_t)(start + 12000U)); CHECK(!s.active);
    puts("PASS: session deadline, restart, delayed service, uint32 wrap");
    return 0;
}
