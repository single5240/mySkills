/**
 * @file rtt_cli_template.c
 * @brief Zero-malloc table-driven J-Link RTT CLI with DWT main-loop profiler.
 */
#include "SEGGER_RTT.h"
#include <stdint.h>
#include <string.h>

#define RTT_CLI_CHANNEL        (0U)
#define RTT_CLI_LINE_CAPACITY  (128U)
#define RTT_CLI_READ_CHUNK     (32U)

typedef void (*RttCliHandler)(const char *args);

typedef struct
{
    const char   *name;
    const char   *help;
    RttCliHandler handler;
} RttCliCommand;

typedef struct
{
    uint64_t loopCount;
    uint64_t totalExecCycles;
    uint32_t minExecCycles;
    uint32_t maxExecCycles;
    uint32_t maxPeriodCycles;
    uint32_t loopStartCycle;
    uint32_t prevStartCycle;
    uint8_t  dwtAvailable;
    uint8_t  prevStartValid;
} RttCliRuntimeStats;

static char               s_line[RTT_CLI_LINE_CAPACITY];
static uint16_t           s_lineLen;
static RttCliRuntimeStats s_stats;

/* External platform hooks */
extern uint32_t PlatformTime_GetMs(void);
extern uint32_t SystemCoreClock;
extern uint8_t  FlashService_IsIdle(void);

static void CmdHelp(const char *args);
static void CmdVer(const char *args);
static void CmdStat(const char *args);

static const RttCliCommand s_commands[] =
{
    { "help", "list all commands",                 CmdHelp },
    { "ver",  "print build timestamp and HCLK",    CmdVer  },
    { "stat", "print main-loop DWT stats (reset)", CmdStat },
};

#define RTT_CLI_CMD_COUNT (sizeof(s_commands) / sizeof(s_commands[0]))

static void RttCli_PrintTimestamp(void)
{
    uint32_t ms = PlatformTime_GetMs();
    uint32_t s  = ms / 1000U;
    (void)SEGGER_RTT_printf(RTT_CLI_CHANNEL,
                            "[%02u:%02u:%02u.%03u] ",
                            (unsigned)((s / 3600U) % 24U),
                            (unsigned)((s / 60U) % 60U),
                            (unsigned)(s % 60U),
                            (unsigned)(ms % 1000U));
}

static char *RttCli_SplitCommand(char *line, const char **argsOut)
{
    char *p = line;
    while ((*p == ' ') || (*p == '\t')) { p++; }
    if (*p == '\0') { *argsOut = p; return NULL; }

    char *name = p;
    while ((*p != '\0') && (*p != ' ') && (*p != '\t')) { p++; }
    if (*p != '\0')
    {
        *p++ = '\0';
        while ((*p == ' ') || (*p == '\t')) { p++; }
    }
    *argsOut = p;
    return name;
}

static void RttCli_DispatchLine(char *line)
{
    const char *args = NULL;
    char *name = RttCli_SplitCommand(line, &args);
    if (name == NULL) { return; }

    RttCli_PrintTimestamp();
    for (uint32_t i = 0U; i < RTT_CLI_CMD_COUNT; i++)
    {
        if (strcmp(name, s_commands[i].name) == 0)
        {
            s_commands[i].handler(args);
            return;
        }
    }
    (void)SEGGER_RTT_printf(RTT_CLI_CHANNEL, "ERR: unknown cmd '%s'\r\n", name);
}

void AppRttCli_Init(void)
{
    s_lineLen = 0U;
    memset(&s_stats, 0, sizeof(s_stats));
    s_stats.minExecCycles = 0xFFFFFFFFUL;
    /* Enable Cortex-M DWT CYCCNT if available */
    /* CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; */
}

void AppRttCli_NoteMainLoopStart(uint32_t currentDwtCycle)
{
    if (s_stats.prevStartValid != 0U)
    {
        uint32_t period = currentDwtCycle - s_stats.prevStartCycle;
        if (period > s_stats.maxPeriodCycles) { s_stats.maxPeriodCycles = period; }
    }
    s_stats.prevStartCycle = currentDwtCycle;
    s_stats.prevStartValid = 1U;
    s_stats.loopStartCycle = currentDwtCycle;
}

void AppRttCli_NoteMainLoopEnd(uint32_t currentDwtCycle)
{
    uint32_t exec = currentDwtCycle - s_stats.loopStartCycle;
    s_stats.loopCount++;
    s_stats.totalExecCycles += exec;
    if (exec < s_stats.minExecCycles) { s_stats.minExecCycles = exec; }
    if (exec > s_stats.maxExecCycles) { s_stats.maxExecCycles = exec; }
}

void AppRttCli_Process(void)
{
    char chunk[RTT_CLI_READ_CHUNK];
    unsigned numRead = SEGGER_RTT_ReadNoLock(RTT_CLI_CHANNEL, chunk, sizeof(chunk));

    for (unsigned i = 0U; i < numRead; i++)
    {
        char c = chunk[i];
        if ((c == '\r') || (c == '\n'))
        {
            if (s_lineLen > 0U)
            {
                s_line[s_lineLen] = '\0';
                RttCli_DispatchLine(s_line);
                s_lineLen = 0U;
            }
        }
        else if (s_lineLen + 1U < RTT_CLI_LINE_CAPACITY)
        {
            s_line[s_lineLen++] = c;
        }
        else
        {
            /* Overflow protection: drop line until next newline */
            s_lineLen = 0U;
        }
    }
}

static void CmdHelp(const char *args)
{
    (void)args;
    (void)SEGGER_RTT_WriteString(RTT_CLI_CHANNEL, "Commands:\r\n");
    for (uint32_t i = 0U; i < RTT_CLI_CMD_COUNT; i++)
    {
        (void)SEGGER_RTT_printf(RTT_CLI_CHANNEL, "  %-14s - %s\r\n",
                                s_commands[i].name, s_commands[i].help);
    }
}

static void CmdVer(const char *args)
{
    (void)args;
    (void)SEGGER_RTT_printf(RTT_CLI_CHANNEL, "Build: %s %s, HCLK=%u Hz\r\n",
                            __DATE__, __TIME__, (unsigned)SystemCoreClock);
}

static void CmdStat(const char *args)
{
    if ((args != NULL) && (strcmp(args, "reset") == 0))
    {
        s_stats.loopCount       = 0U;
        s_stats.totalExecCycles = 0U;
        s_stats.minExecCycles   = 0xFFFFFFFFUL;
        s_stats.maxExecCycles   = 0U;
        s_stats.maxPeriodCycles = 0U;
        s_stats.prevStartValid  = 0U;
        (void)SEGGER_RTT_WriteString(RTT_CLI_CHANNEL, "OK: stat reset\r\n");
        return;
    }
    uint32_t khz = SystemCoreClock / 1000U;
    uint32_t maxExecUs = (khz > 0U) ? (uint32_t)(((uint64_t)s_stats.maxExecCycles * 1000ULL) / khz) : 0U;
    uint32_t maxPerUs  = (khz > 0U) ? (uint32_t)(((uint64_t)s_stats.maxPeriodCycles * 1000ULL) / khz) : 0U;
    (void)SEGGER_RTT_printf(RTT_CLI_CHANNEL,
                            "loops=%u max_exec=%u us max_period=%u us\r\n",
                            (unsigned)s_stats.loopCount,
                            (unsigned)maxExecUs,
                            (unsigned)maxPerUs);
}
