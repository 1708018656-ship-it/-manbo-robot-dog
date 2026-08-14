#include "ActionTask.h"
#include "Servo.h"
#include "Timebase.h"

typedef struct
{
    uint8_t angle[4];
    uint16_t duration_ms;
} PoseStep;

typedef struct
{
    const PoseStep *steps;
    uint8_t step_count;
    uint8_t step_index;
    uint32_t step_deadline_ms;
    ActionId action;
    uint8_t active;
    uint8_t step_started;
} ActionInstance;

#define BASIC_ACTION_STEP_MS 150U

static const PoseStep g_stand_steps[] =
{
    { {90U, 90U, 90U, 90U}, 500U }
};

static const PoseStep g_forward_steps[] =
{
    { {135U, 90U,  90U,  45U}, BASIC_ACTION_STEP_MS },
    { {135U, 45U, 135U,  45U}, BASIC_ACTION_STEP_MS },
    { { 90U, 45U, 135U,  90U}, BASIC_ACTION_STEP_MS },
    { { 90U, 90U,  90U,  90U}, BASIC_ACTION_STEP_MS },
    { { 90U,135U,  45U,  90U}, BASIC_ACTION_STEP_MS },
    { { 45U,135U,  45U, 135U}, BASIC_ACTION_STEP_MS },
    { { 45U, 90U,  90U, 135U}, BASIC_ACTION_STEP_MS },
    { { 90U, 90U,  90U,  90U}, BASIC_ACTION_STEP_MS }
};

static const PoseStep g_backward_steps[] =
{
    { { 45U, 90U,  90U, 135U}, BASIC_ACTION_STEP_MS },
    { { 45U,135U,  45U, 135U}, BASIC_ACTION_STEP_MS },
    { { 90U,135U,  45U,  90U}, BASIC_ACTION_STEP_MS },
    { { 90U, 90U,  90U,  90U}, BASIC_ACTION_STEP_MS },
    { { 90U, 45U, 135U,  90U}, BASIC_ACTION_STEP_MS },
    { {135U, 45U, 135U,  45U}, BASIC_ACTION_STEP_MS },
    { {135U, 90U,  90U,  45U}, BASIC_ACTION_STEP_MS },
    { { 90U, 90U,  90U,  90U}, BASIC_ACTION_STEP_MS }
};

static const PoseStep g_left_steps[] =
{
    { { 90U,135U,135U, 90U}, BASIC_ACTION_STEP_MS },
    { { 45U,135U,135U, 45U}, BASIC_ACTION_STEP_MS },
    { { 45U, 90U,  90U, 45U}, BASIC_ACTION_STEP_MS },
    { { 90U, 90U,  90U, 90U}, BASIC_ACTION_STEP_MS }
};

static const PoseStep g_right_steps[] =
{
    { { 45U, 90U,  90U, 45U}, BASIC_ACTION_STEP_MS },
    { { 45U,135U,135U, 45U}, BASIC_ACTION_STEP_MS },
    { { 90U,135U,135U, 90U}, BASIC_ACTION_STEP_MS },
    { { 90U, 90U,  90U, 90U}, BASIC_ACTION_STEP_MS }
};

static ActionInstance g_action;

static void Action_GetScript(ActionId action,
                             const PoseStep **steps,
                             uint8_t *step_count)
{
    *steps = 0;
    *step_count = 0U;

    switch (action)
    {
        case ACTION_STAND:
            *steps = g_stand_steps;
            *step_count = (uint8_t)(sizeof(g_stand_steps) / sizeof(g_stand_steps[0]));
            break;
        case ACTION_FORWARD:
            *steps = g_forward_steps;
            *step_count = (uint8_t)(sizeof(g_forward_steps) / sizeof(g_forward_steps[0]));
            break;
        case ACTION_BACKWARD:
            *steps = g_backward_steps;
            *step_count = (uint8_t)(sizeof(g_backward_steps) / sizeof(g_backward_steps[0]));
            break;
        case ACTION_TURN_LEFT:
            *steps = g_left_steps;
            *step_count = (uint8_t)(sizeof(g_left_steps) / sizeof(g_left_steps[0]));
            break;
        case ACTION_TURN_RIGHT:
            *steps = g_right_steps;
            *step_count = (uint8_t)(sizeof(g_right_steps) / sizeof(g_right_steps[0]));
            break;
        default:
            break;
    }
}

static void Action_ApplyPose(const PoseStep *step)
{
    Servo_SetAngle1((float)step->angle[0]);
    Servo_SetAngle2((float)step->angle[1]);
    Servo_SetAngle3((float)step->angle[2]);
    Servo_SetAngle4((float)step->angle[3]);
}

void ActionTask_Init(void)
{
    g_action.steps = 0;
    g_action.step_count = 0U;
    g_action.step_index = 0U;
    g_action.step_deadline_ms = 0U;
    g_action.action = ACTION_NONE;
    g_action.active = 0U;
    g_action.step_started = 0U;
}

uint8_t ActionTask_RequestMotion(ActionId action)
{
    const PoseStep *steps;
    uint8_t step_count;

    Action_GetScript(action, &steps, &step_count);
    if ((steps == 0) || (step_count == 0U))
    {
        return 0U;
    }

    /* 同一动作正在运行时只保持当前实例，避免主循环重复重启脚本。 */
    if ((g_action.active != 0U) && (g_action.action == action))
    {
        return 1U;
    }

    /* 新移动命令直接替换旧动作，实现协作式抢占。 */
    g_action.steps = steps;
    g_action.step_count = step_count;
    g_action.step_index = 0U;
    g_action.step_deadline_ms = 0U;
    g_action.action = action;
    g_action.active = 1U;
    g_action.step_started = 0U;
    return 1U;
}

void ActionTask_Run(uint32_t now_ms)
{
    const PoseStep *step;

    if (g_action.active == 0U)
    {
        return;
    }

    if (g_action.step_started == 0U)
    {
        step = &g_action.steps[g_action.step_index];
        Action_ApplyPose(step);
        g_action.step_deadline_ms = now_ms + step->duration_ms;
        g_action.step_started = 1U;
        return;
    }

    if ((int32_t)(now_ms - g_action.step_deadline_ms) < 0)
    {
        return;
    }

    g_action.step_index++;
    if (g_action.step_index >= g_action.step_count)
    {
        g_action.active = 0U;
        g_action.action = ACTION_NONE;
        return;
    }

    step = &g_action.steps[g_action.step_index];
    Action_ApplyPose(step);
    g_action.step_deadline_ms = now_ms + step->duration_ms;
}

uint8_t ActionTask_HandleCommand(uint8_t command)
{
    switch (command)
    {
        case '5':
            ActionTask_AbortToSafeStop();
            return 1U;
        case 'f':
            return ActionTask_RequestMotion(ACTION_FORWARD);
        case 'b':
            return ActionTask_RequestMotion(ACTION_BACKWARD);
        case 'l':
            return ActionTask_RequestMotion(ACTION_TURN_LEFT);
        case 'r':
            return ActionTask_RequestMotion(ACTION_TURN_RIGHT);
        default:
            return 0U;
    }
}

void ActionTask_AbortToSafeStop(void)
{
    const PoseStep safe_pose = { {90U, 90U, 90U, 90U}, 0U };

    Action_ApplyPose(&safe_pose);
    g_action.steps = 0;
    g_action.step_count = 0U;
    g_action.step_index = 0U;
    g_action.step_deadline_ms = 0U;
    g_action.action = ACTION_NONE;
    g_action.active = 0U;
    g_action.step_started = 0U;
}

uint8_t ActionTask_IsActive(void)
{
    return g_action.active;
}

ActionId ActionTask_Current(void)
{
    return g_action.action;
}
