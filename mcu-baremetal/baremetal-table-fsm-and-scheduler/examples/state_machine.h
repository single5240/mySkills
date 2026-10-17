/**
 * @file state_machine.h
 * @brief Generic table-driven state machine without dynamic allocation.
 */
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef uint16_t StateMachineState;
typedef uint16_t StateMachineEvent;

struct StateMachine;

typedef void (*StateMachineAction)(struct StateMachine *machine, void *userContext);

typedef struct
{
    StateMachineEvent  event;
    StateMachineState  currentState;
    StateMachineAction action;
    StateMachineState  nextState;
} StateMachineTransition;

typedef struct
{
    const StateMachineTransition *transitions;
    const StateMachineEvent      *loopEvents;
    uint16_t                      transitionCount;
    uint16_t                      stateCount;
    uint16_t                      eventCount;
} StateMachineConfig;

typedef enum
{
    STATE_MACHINE_PROCESS_HANDLED = 0,
    STATE_MACHINE_PROCESS_IGNORED,
    STATE_MACHINE_PROCESS_INVALID
} StateMachineProcessResult;

typedef struct StateMachine
{
    const StateMachineConfig *config;
    StateMachineEvent        *eventQueue;
    void                     *userContext;
    StateMachineState         currentState;
    uint16_t                  queueCapacity;
    uint16_t                  eventReadIndex;
    uint16_t                  eventWriteIndex;
    uint16_t                  eventCount;
} StateMachine;

/* Config tables and queue storage must outlive machine. Single-context by default. */
bool StateMachine_Init(StateMachine *machine,
                       const StateMachineConfig *config,
                       StateMachineState initialState,
                       StateMachineEvent *eventQueue,
                       uint16_t queueCapacity,
                       void *userContext);
void StateMachine_Reset(StateMachine *machine, StateMachineState initialState);
StateMachineProcessResult StateMachine_Process(StateMachine *machine);
bool StateMachine_PostEvent(StateMachine *machine, StateMachineEvent event);
void StateMachine_ClearEvents(StateMachine *machine);
StateMachineState StateMachine_GetState(const StateMachine *machine);

#ifdef __cplusplus
}
#endif

#endif /* STATE_MACHINE_H */
