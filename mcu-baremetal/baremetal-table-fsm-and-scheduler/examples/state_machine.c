/**
 * @file state_machine.c
 * @brief Generic table-driven state machine without dynamic allocation.
 */
#include "state_machine.h"

#include <stddef.h>

static bool StateMachine_IsConfigValid(const StateMachineConfig *config)
{
    uint16_t index;
    uint16_t otherIndex;
    uint16_t state;
    bool loopTransitionFound;

    if ((config == NULL) || (config->transitions == NULL) ||
        (config->loopEvents == NULL) || (config->transitionCount == 0U) ||
        (config->stateCount == 0U) || (config->eventCount == 0U))
    {
        return false;
    }

    for (index = 0U; index < config->stateCount; index++)
    {
        if (config->loopEvents[index] >= config->eventCount)
        {
            return false;
        }
    }

    for (index = 0U; index < config->transitionCount; index++)
    {
        const StateMachineTransition *transition = &config->transitions[index];

        if ((transition->event >= config->eventCount) ||
            (transition->currentState >= config->stateCount) ||
            (transition->nextState >= config->stateCount))
        {
            return false;
        }

        for (otherIndex = (uint16_t)(index + 1U);
             otherIndex < config->transitionCount;
             otherIndex++)
        {
            const StateMachineTransition *otherTransition = &config->transitions[otherIndex];

            if ((transition->event == otherTransition->event) &&
                (transition->currentState == otherTransition->currentState))
            {
                return false;
            }
        }
    }

    for (state = 0U; state < config->stateCount; state++)
    {
        loopTransitionFound = false;
        for (index = 0U; index < config->transitionCount; index++)
        {
            if ((config->transitions[index].currentState == state) &&
                (config->transitions[index].event == config->loopEvents[state]))
            {
                loopTransitionFound = true;
                break;
            }
        }

        if (!loopTransitionFound)
        {
            return false;
        }
    }

    return true;
}

static bool StateMachine_TakeEvent(StateMachine *machine, StateMachineEvent *event)
{
    if ((machine == NULL) || (event == NULL) || (machine->eventCount == 0U))
    {
        return false;
    }

    *event = machine->eventQueue[machine->eventReadIndex];
    machine->eventReadIndex++;
    if (machine->eventReadIndex >= machine->queueCapacity)
    {
        machine->eventReadIndex = 0U;
    }
    machine->eventCount--;
    return true;
}

bool StateMachine_Init(StateMachine *machine,
                       const StateMachineConfig *config,
                       StateMachineState initialState,
                       StateMachineEvent *eventQueue,
                       uint16_t queueCapacity,
                       void *userContext)
{
    if ((machine == NULL) || (eventQueue == NULL) || (queueCapacity == 0U) ||
        !StateMachine_IsConfigValid(config) || (initialState >= config->stateCount))
    {
        return false;
    }

    machine->config = config;
    machine->eventQueue = eventQueue;
    machine->userContext = userContext;
    machine->queueCapacity = queueCapacity;
    machine->currentState = initialState;
    StateMachine_ClearEvents(machine);
    return true;
}

void StateMachine_Reset(StateMachine *machine, StateMachineState initialState)
{
    if ((machine == NULL) || (machine->config == NULL) ||
        (initialState >= machine->config->stateCount))
    {
        return;
    }

    StateMachine_ClearEvents(machine);
    machine->currentState = initialState;
}

StateMachineProcessResult StateMachine_Process(StateMachine *machine)
{
    StateMachineEvent event;
    uint16_t index;

    if ((machine == NULL) || (machine->config == NULL) ||
        (machine->eventQueue == NULL) ||
        (machine->currentState >= machine->config->stateCount))
    {
        return STATE_MACHINE_PROCESS_INVALID;
    }

    /* Idle processing uses the current state's loop event without queueing it. */
    if (!StateMachine_TakeEvent(machine, &event))
    {
        event = machine->config->loopEvents[machine->currentState];
    }

    for (index = 0U; index < machine->config->transitionCount; index++)
    {
        const StateMachineTransition *transition = &machine->config->transitions[index];

        if ((transition->event == event) &&
            (transition->currentState == machine->currentState))
        {
            if (transition->action != NULL)
            {
                transition->action(machine, machine->userContext);
            }
            machine->currentState = transition->nextState;
            return STATE_MACHINE_PROCESS_HANDLED;
        }
    }

    return STATE_MACHINE_PROCESS_IGNORED;
}

bool StateMachine_PostEvent(StateMachine *machine, StateMachineEvent event)
{
    if ((machine == NULL) || (machine->config == NULL) ||
        (machine->eventQueue == NULL) || (event >= machine->config->eventCount) ||
        (machine->eventCount >= machine->queueCapacity))
    {
        return false;
    }

    machine->eventQueue[machine->eventWriteIndex] = event;
    machine->eventWriteIndex++;
    if (machine->eventWriteIndex >= machine->queueCapacity)
    {
        machine->eventWriteIndex = 0U;
    }
    machine->eventCount++;
    return true;
}

void StateMachine_ClearEvents(StateMachine *machine)
{
    if (machine == NULL)
    {
        return;
    }

    machine->eventReadIndex = 0U;
    machine->eventWriteIndex = 0U;
    machine->eventCount = 0U;
}

StateMachineState StateMachine_GetState(const StateMachine *machine)
{
    if (machine == NULL)
    {
        return 0U;
    }

    return machine->currentState;
}
