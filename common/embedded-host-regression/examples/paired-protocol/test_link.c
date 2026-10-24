#include <stdio.h>
#include <string.h>
#include "link.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while(0)
int main(void)
{
    DemoReceiver r = {3, 3, 0, 0};
    uint8_t tx[5], bad[5], controller_saved;
    /* 独立已知向量，不调用组帧函数计算期望值。 */
    const uint8_t golden[5] = {7, 0x34, 0x12, 0x31, 0x7e};
    demo_frame(tx, 7, 0x1234, DEMO_SWITCH);
    CHECK(memcmp(tx, golden, 5) == 0);
    demo_receive(&r, tx, 4); CHECK(r.current == 3);
    demo_receive(&r, NULL, 5); CHECK(r.current == 3);
    memcpy(bad, tx, 5); bad[4] ^= 1;
    demo_receive(&r, bad, 5); CHECK(r.current == 3);
    /* 丢失切换通知：保存命令不能让未参与的接收端自行换信道。 */
    demo_frame(tx, 7, 0x1234, DEMO_SAVE);
    demo_receive(&r, tx, 5); CHECK(r.saved == 3 && r.writes == 0);
    demo_frame(tx, 7, 0x1234, DEMO_SWITCH);
    demo_receive(&r, tx, 5); CHECK(r.current == 7 && r.saved == 3);
    demo_receive(&r, tx, 5); CHECK(r.current == 7 && r.writes == 0);
    demo_frame(tx, 11, 0x1235, DEMO_SWITCH);
    demo_receive(&r, tx, 5); CHECK(r.current == 7 && r.token == 0x1234);
    demo_frame(tx, 7, 0x1233, DEMO_SAVE);
    demo_receive(&r, tx, 5); CHECK(r.saved == 3);
    /* 发送端本地保存，接收端漏收：不虚构双端保存成功。 */
    controller_saved = 7;
    demo_frame(tx, 7, 0x1234, DEMO_SAVE); /* 此时故意不交付。 */
    CHECK(controller_saved == 7 && r.saved == 3);
    demo_receive(&r, tx, 5); CHECK(r.saved == 7 && r.writes == 1);
    demo_receive(&r, tx, 5); CHECK(r.writes == 1);
    demo_frame(tx, 8, 0x1235, DEMO_SWITCH); demo_receive(&r, tx, 5);
    demo_frame(tx, 0, 0x1235, DEMO_RETURN); demo_receive(&r, tx, 5);
    CHECK(r.current == 7 && r.saved == 7);
    puts("PASS: known frame, length, checksum, loss, token, duplicate save, return");
    return 0;
}
