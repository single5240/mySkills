/**
 * @file hardfault_rtt_dump.c
 * @brief ARMCC 5 & GCC compatible HardFault stack frame & SCB register dumper.
 */
#include <stdint.h>

/* Replace with SEGGER_RTT_printf(0, ...) or polling UART print */
extern int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...);

/* Cortex-M3/M4/M7 System Control Block Fault Registers */
#define SCB_CFSR_REG   (*(volatile uint32_t *)0xE000ED28UL)
#define SCB_HFSR_REG   (*(volatile uint32_t *)0xE000ED2CUL)
#define SCB_DFSR_REG   (*(volatile uint32_t *)0xE000ED30UL)
#define SCB_MMFAR_REG  (*(volatile uint32_t *)0xE000ED34UL)
#define SCB_BFAR_REG   (*(volatile uint32_t *)0xE000ED38UL)
#define DHCSR_REG      (*(volatile uint32_t *)0xE000EDF0UL)

void HardFault_DumpC(const uint32_t *sp, uint32_t excReturn)
{
    uint32_t r0   = sp[0];
    uint32_t r1   = sp[1];
    uint32_t r2   = sp[2];
    uint32_t r3   = sp[3];
    uint32_t r12  = sp[4];
    uint32_t lr   = sp[5];
    uint32_t pc   = sp[6];
    uint32_t xpsr = sp[7];

    uint32_t cfsr  = SCB_CFSR_REG;
    uint32_t hfsr  = SCB_HFSR_REG;
    uint32_t mmfar = SCB_MMFAR_REG;
    uint32_t bfar  = SCB_BFAR_REG;

    (void)SEGGER_RTT_printf(0U, "\r\n=== [HARDFAULT DETECTED] ===\r\n");
    (void)SEGGER_RTT_printf(0U, "SP=%08X EXC_RET=%08X (%s)\r\n",
                            (unsigned)(uintptr_t)sp,
                            (unsigned)excReturn,
                            ((excReturn & 0x04UL) != 0UL) ? "PSP" : "MSP");
    (void)SEGGER_RTT_printf(0U, "PC  = 0x%08X  <-- Fault Instruction\r\n", (unsigned)pc);
    (void)SEGGER_RTT_printf(0U, "LR  = 0x%08X  <-- Caller Return Addr\r\n", (unsigned)lr);
    (void)SEGGER_RTT_printf(0U, "xPSR= 0x%08X  R12 = 0x%08X\r\n", (unsigned)xpsr, (unsigned)r12);
    (void)SEGGER_RTT_printf(0U, "R0  = 0x%08X  R1  = 0x%08X\r\n", (unsigned)r0, (unsigned)r1);
    (void)SEGGER_RTT_printf(0U, "R2  = 0x%08X  R3  = 0x%08X\r\n", (unsigned)r2, (unsigned)r3);
    (void)SEGGER_RTT_printf(0U, "HFSR= 0x%08X  CFSR= 0x%08X\r\n", (unsigned)hfsr, (unsigned)cfsr);

    if ((cfsr & 0x00000080UL) != 0UL)
    {
        (void)SEGGER_RTT_printf(0U, "MMFAR = 0x%08X (Valid)\r\n", (unsigned)mmfar);
    }
    if ((cfsr & 0x00008000UL) != 0UL)
    {
        (void)SEGGER_RTT_printf(0U, "BFAR  = 0x%08X (Valid)\r\n", (unsigned)bfar);
    }

    /* Halt in debugger if SWD debugger is attached */
    if ((DHCSR_REG & 0x01UL) != 0UL)
    {
        __breakpoint(0);
    }

    while (1)
    {
        /* Wait for IWDG reset */
    }
}

#if defined(__CC_ARM)
/* Keil ARMCC 5 naked assembly wrapper */
__asm void HardFault_Handler(void)
{
    TST     lr, #4
    ITE     EQ
    MRSEQ   r0, MSP
    MRSNE   r0, PSP
    MOV     r1, lr
    B       __cpp(HardFault_DumpC)
}
#elif defined(__GNUC__) || defined(__clang__)
/* ARMClang 6 / GCC naked assembly wrapper */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4          \n"
        "ite eq              \n"
        "mrseq r0, msp       \n"
        "mrsne r0, psp       \n"
        "mov r1, lr          \n"
        "b HardFault_DumpC   \n"
    );
}
#endif
