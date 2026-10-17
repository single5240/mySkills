/**
 * @file serial_ring_frame_parser.c
 * @brief Lock-free SPSC RingBuffer + Timeout-protected Frame Parser (C99).
 *
 * Frame Format Example:
 *   [0xAA] [0x55] [CMD:1B] [LEN:1B] [PAYLOAD:0..64B] [CRC16_LE:2B]
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SERIAL_RING_CAPACITY    (256U) /* Must be power of 2 */
#define SERIAL_RING_MASK        (SERIAL_RING_CAPACITY - 1U)
#define FRAME_MAX_PAYLOAD       (64U)
#define FRAME_INTERBYTE_TIMEOUT (20U)  /* ms */

typedef struct
{
    uint8_t           buf[SERIAL_RING_CAPACITY];
    volatile uint16_t head; /* Written only by producer (ISR) */
    volatile uint16_t tail; /* Written only by consumer (Main loop) */
    volatile uint32_t dropCount;
} SerialRingBuffer;

typedef enum
{
    PARSE_SYNC_AA = 0,
    PARSE_SYNC_55,
    PARSE_CMD,
    PARSE_LEN,
    PARSE_PAYLOAD,
    PARSE_CRC_LO,
    PARSE_CRC_HI
} ParseState;

typedef struct
{
    ParseState state;
    uint8_t    cmd;
    uint8_t    len;
    uint8_t    payIdx;
    uint8_t    payload[FRAME_MAX_PAYLOAD];
    uint16_t   rxCrc;
    uint32_t   lastByteTick;
    uint32_t   validFrameCount;
    uint32_t   crcErrorCount;
    uint32_t   timeoutErrorCount;
} FrameParserCtx;

/* ---- Lock-Free SPSC Ring Buffer ---- */

void SerialRing_Init(SerialRingBuffer *rb)
{
    memset(rb, 0, sizeof(*rb));
}

/* Call from UART IDLE / DMA HT / DMA TC ISR */
void SerialRing_PushFromIsr(SerialRingBuffer *rb, const uint8_t *data, uint16_t len)
{
    uint16_t h = rb->head;
    uint16_t t = rb->tail;

    for (uint16_t i = 0U; i < len; i++)
    {
        uint16_t next = (uint16_t)((h + 1U) & SERIAL_RING_MASK);
        if (next == t)
        {
            rb->dropCount++;
            break;
        }
        rb->buf[h] = data[i];
        h = next;
    }
    rb->head = h;
}

/* Call from Main Loop */
bool SerialRing_Pop(SerialRingBuffer *rb, uint8_t *outByte)
{
    uint16_t t = rb->tail;
    if (t == rb->head)
    {
        return false;
    }
    *outByte = rb->buf[t];
    rb->tail = (uint16_t)((t + 1U) & SERIAL_RING_MASK);
    return true;
}

/* ---- CRC-16/MODBUS ---- */

static uint16_t CalcCrc16Modbus(const uint8_t *data, uint16_t len, uint16_t initCrc)
{
    uint16_t crc = initCrc;
    for (uint16_t i = 0U; i < len; i++)
    {
        crc ^= (uint16_t)data[i];
        for (uint8_t b = 0U; b < 8U; b++)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            }
            else
            {
                crc >>= 1U;
            }
        }
    }
    return crc;
}

/* ---- Non-Blocking Frame Parser with Inter-Byte Timeout ---- */

void FrameParser_Init(FrameParserCtx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = PARSE_SYNC_AA;
}

void FrameParser_Process(FrameParserCtx *ctx,
                         SerialRingBuffer *rb,
                         uint32_t currentTickMs,
                         void (*onFrame)(uint8_t cmd, const uint8_t *payload, uint8_t len))
{
    uint8_t byte = 0U;

    /* Check inter-byte timeout when mid-frame */
    if ((ctx->state != PARSE_SYNC_AA) &&
        ((uint32_t)(currentTickMs - ctx->lastByteTick) >= FRAME_INTERBYTE_TIMEOUT))
    {
        ctx->state = PARSE_SYNC_AA;
        ctx->timeoutErrorCount++;
    }

    while (SerialRing_Pop(rb, &byte))
    {
        ctx->lastByteTick = currentTickMs;

        switch (ctx->state)
        {
        case PARSE_SYNC_AA:
            if (byte == 0xAAU) { ctx->state = PARSE_SYNC_55; }
            break;

        case PARSE_SYNC_55:
            if (byte == 0x55U)      { ctx->state = PARSE_CMD; }
            else if (byte == 0xAAU) { ctx->state = PARSE_SYNC_55; } /* Consecutive 0xAA */
            else                    { ctx->state = PARSE_SYNC_AA; }
            break;

        case PARSE_CMD:
            ctx->cmd   = byte;
            ctx->state = PARSE_LEN;
            break;

        case PARSE_LEN:
            if (byte > FRAME_MAX_PAYLOAD)
            {
                ctx->state = PARSE_SYNC_AA;
                ctx->crcErrorCount++;
            }
            else
            {
                ctx->len    = byte;
                ctx->payIdx = 0U;
                ctx->state  = (byte == 0U) ? PARSE_CRC_LO : PARSE_PAYLOAD;
            }
            break;

        case PARSE_PAYLOAD:
            ctx->payload[ctx->payIdx++] = byte;
            if (ctx->payIdx >= ctx->len)
            {
                ctx->state = PARSE_CRC_LO;
            }
            break;

        case PARSE_CRC_LO:
            ctx->rxCrc = (uint16_t)byte;
            ctx->state = PARSE_CRC_HI;
            break;

        case PARSE_CRC_HI:
        {
            ctx->rxCrc |= (uint16_t)((uint16_t)byte << 8U);
            uint8_t hdr[4] = { 0xAAU, 0x55U, ctx->cmd, ctx->len };
            uint16_t calc  = CalcCrc16Modbus(hdr, 4U, 0xFFFFU);
            calc           = CalcCrc16Modbus(ctx->payload, ctx->len, calc);

            if (calc == ctx->rxCrc)
            {
                ctx->validFrameCount++;
                if (onFrame != NULL)
                {
                    onFrame(ctx->cmd, ctx->payload, ctx->len);
                }
            }
            else
            {
                ctx->crcErrorCount++;
            }
            ctx->state = PARSE_SYNC_AA;
            break;
        }

        default:
            ctx->state = PARSE_SYNC_AA;
            break;
        }
    }
}
