#include "ActionTask.h"
#include "Servo.h"
#include "Timebase.h"
#include "AudioTask.h"

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

static const PoseStep g_swing_forward_back_steps[] =
{
    { {135U,135U, 45U, 45U}, 150U },
    { { 90U, 90U, 90U, 90U}, 150U },
    { { 45U, 45U,135U,135U}, 150U },
    { { 90U, 90U, 90U, 90U}, 150U }
};

static const PoseStep g_swing_left_right_steps[] =
{
    { {135U,135U, 90U, 90U}, 150U },
    { {135U,135U,135U,135U}, 150U },
    { { 90U, 90U, 90U, 90U}, 150U },
    { { 45U, 45U, 90U, 90U}, 150U },
    { { 45U, 45U, 45U, 45U}, 150U },
    { { 90U, 90U, 90U, 90U}, 150U }
};

static const PoseStep g_dance_steps[] =
{
    { {135U,135U,135U,135U}, 150U },
    { { 90U, 90U, 90U, 90U}, 150U },
    { {135U,135U, 45U, 45U}, 150U },
    { { 90U, 90U, 90U, 90U}, 150U },
    { { 45U, 45U, 90U, 90U}, 150U },
    { { 45U, 45U, 45U, 45U}, 150U },
    { { 90U, 90U, 90U, 90U}, 150U },
    { { 45U, 45U,135U,135U}, 150U },
    { { 90U, 90U, 90U, 90U}, 150U }
};

static const PoseStep g_hello_steps[] =
{
    { { 70U,110U, 90U, 70U}, 120U },
    { { 70U,170U, 90U, 10U}, 240U },
    { {130U,170U, 90U, 10U}, 120U },
    { {180U,170U, 90U, 10U}, 500U },
    { {130U,170U, 90U, 10U}, 500U },
    { {180U,170U, 90U, 10U}, 500U },
    { {130U,170U, 90U, 10U}, 500U },
    { { 90U, 90U, 90U, 90U}, 250U }
};

static const PoseStep g_stretch_steps[] =
{
    { { 90U,155U, 90U, 25U}, 325U },
    { { 70U,155U,110U, 25U}, 100U },
    { { 70U,155U,110U, 25U}, 1000U },
    { {130U,155U,110U, 25U}, 240U },
    { {180U,155U,110U, 25U}, 500U },
    { {130U,155U,110U, 25U}, 500U },
    { {180U,155U,110U, 25U}, 500U },
    { { 70U,155U,110U, 25U}, 250U }
};

static const PoseStep g_two_hands_steps[] =
{
    { { 90U, 20U, 20U, 90U}, 200U },
    { { 90U, 90U, 90U, 90U}, 200U },
    { {160U, 90U, 90U,160U}, 200U },
    { { 90U, 90U, 90U, 90U}, 200U }
};

static const PoseStep g_lazy_steps[] =
{
    { {165U,127U, 15U, 53U}, 375U },
    { {165U,127U, 15U, 53U}, 750U },
    { { 90U, 90U, 90U, 90U}, 375U },
    { { 90U, 90U, 90U, 90U}, 150U }
};

static const PoseStep g_head_up_steps[] =
{
    { { 70U, 90U,110U, 90U}, 200U },
    { { 70U,155U,110U, 25U}, 650U },
    { { 90U, 90U, 90U, 90U}, 250U }
};

static const PoseStep g_sleep_prone_steps[] =
{
    { {165U, 90U, 15U, 90U}, 750U },
    { {165U, 15U, 15U,165U}, 750U },
    { {165U, 15U, 15U,165U}, 1000U }
};

static const PoseStep g_sleep_side_steps[] =
{
    { { 90U, 90U,165U, 15U}, 750U },
    { { 90U,165U,165U, 15U}, 750U },
    { { 90U,165U,165U, 15U}, 1000U }
};

static const PoseStep g_declare_love_steps[] =
{
    { {135U,135U, 45U, 45U}, 250U },
    { { 90U, 90U, 90U, 90U}, 200U },
    { {135U, 90U, 45U, 90U}, 350U },
    { { 90U, 90U, 90U, 90U}, 250U }
};

static const PoseStep g_elements_steps[] =
{
    { {135U, 90U, 45U, 90U}, 500U },
    { { 90U,135U, 90U, 45U}, 500U },
    { { 45U, 90U,135U, 90U}, 500U },
    { { 90U, 45U, 90U,135U}, 500U },
    { { 90U, 90U, 90U, 90U}, 250U }
};

static const PoseStep g_training_steps[] =
{
    { {135U, 90U, 90U, 90U}, 500U },
    { {135U, 90U, 45U, 90U}, 500U },
    { {135U, 45U, 45U,135U}, 500U },
    { { 90U, 90U, 90U, 90U}, 250U },
    { { 45U,135U,135U, 45U}, 250U },
    { { 90U, 90U, 90U, 90U}, 250U }
};

static const PoseStep g_world_light_steps[] =
{
    { { 90U,135U, 90U, 45U}, 500U },
    { { 90U,135U, 90U, 45U}, 1500U },
    { { 90U, 90U, 90U, 90U}, 250U }
};

static const PoseStep g_wake_up_steps[] =
{
    { { 90U, 90U, 90U, 90U}, 250U },
    { { 75U,105U,105U, 75U}, 300U },
    { { 90U, 90U, 90U, 90U}, 250U }
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
        case ACTION_SWING_FORWARD_BACK:
            *steps = g_swing_forward_back_steps;
            *step_count = (uint8_t)(sizeof(g_swing_forward_back_steps) / sizeof(g_swing_forward_back_steps[0]));
            break;
        case ACTION_SWING_LEFT_RIGHT:
            *steps = g_swing_left_right_steps;
            *step_count = (uint8_t)(sizeof(g_swing_left_right_steps) / sizeof(g_swing_left_right_steps[0]));
            break;
        case ACTION_DANCE:
            *steps = g_dance_steps;
            *step_count = (uint8_t)(sizeof(g_dance_steps) / sizeof(g_dance_steps[0]));
            break;
        case ACTION_HELLO:
            *steps = g_hello_steps;
            *step_count = (uint8_t)(sizeof(g_hello_steps) / sizeof(g_hello_steps[0]));
            break;
        case ACTION_STRETCH:
            *steps = g_stretch_steps;
            *step_count = (uint8_t)(sizeof(g_stretch_steps) / sizeof(g_stretch_steps[0]));
            break;
        case ACTION_TWO_HANDS:
            *steps = g_two_hands_steps;
            *step_count = (uint8_t)(sizeof(g_two_hands_steps) / sizeof(g_two_hands_steps[0]));
            break;
        case ACTION_LAZY:
            *steps = g_lazy_steps;
            *step_count = (uint8_t)(sizeof(g_lazy_steps) / sizeof(g_lazy_steps[0]));
            break;
        case ACTION_HEAD_UP:
            *steps = g_head_up_steps;
            *step_count = (uint8_t)(sizeof(g_head_up_steps) / sizeof(g_head_up_steps[0]));
            break;
        case ACTION_SLEEP_PRONE:
            *steps = g_sleep_prone_steps;
            *step_count = (uint8_t)(sizeof(g_sleep_prone_steps) / sizeof(g_sleep_prone_steps[0]));
            break;
        case ACTION_SLEEP_SIDE:
            *steps = g_sleep_side_steps;
            *step_count = (uint8_t)(sizeof(g_sleep_side_steps) / sizeof(g_sleep_side_steps[0]));
            break;
        case ACTION_DECLARE_LOVE:
            *steps = g_declare_love_steps;
            *step_count = (uint8_t)(sizeof(g_declare_love_steps) / sizeof(g_declare_love_steps[0]));
            break;
        case ACTION_ELEMENTS:
            *steps = g_elements_steps;
            *step_count = (uint8_t)(sizeof(g_elements_steps) / sizeof(g_elements_steps[0]));
            break;
        case ACTION_TRAINING:
            *steps = g_training_steps;
            *step_count = (uint8_t)(sizeof(g_training_steps) / sizeof(g_training_steps[0]));
            break;
        case ACTION_WORLD_LIGHT:
            *steps = g_world_light_steps;
            *step_count = (uint8_t)(sizeof(g_world_light_steps) / sizeof(g_world_light_steps[0]));
            break;
        case ACTION_WAKE_UP:
            *steps = g_wake_up_steps;
            *step_count = (uint8_t)(sizeof(g_wake_up_steps) / sizeof(g_wake_up_steps[0]));
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

static uint8_t Action_RequestWithSpeech(ActionId action, const char *text)
{
    uint8_t accepted;

    accepted = ActionTask_RequestMotion(action);
    if (accepted != 0U)
    {
        AudioTask_RequestText(text);
    }
    return accepted;
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
        case 'w':
            return ActionTask_RequestMotion(ACTION_SWING_FORWARD_BACK);
        case 'z':
            return ActionTask_RequestMotion(ACTION_SWING_LEFT_RIGHT);
        case 'd':
            return ActionTask_RequestMotion(ACTION_DANCE);
        case 'o':
            return ActionTask_RequestMotion(ACTION_HELLO);
        case 's':
            return ActionTask_RequestMotion(ACTION_STRETCH);
        case 'j':
            return ActionTask_RequestMotion(ACTION_TWO_HANDS);
        case 'y':
            return ActionTask_RequestMotion(ACTION_LAZY);
        case '1':
            return ActionTask_RequestMotion(ACTION_HEAD_UP);
        case 'p':
            return ActionTask_RequestMotion(ACTION_SLEEP_PRONE);
        case '2':
            return ActionTask_RequestMotion(ACTION_SLEEP_SIDE);
        case 'q':
            return ActionTask_RequestMotion(ACTION_STAND);
        case 'B':
            return Action_RequestWithSpeech(ACTION_DECLARE_LOVE,
                                            "[v12][m0][t5]wo ai ni");
        case 'Y':
            return Action_RequestWithSpeech(ACTION_ELEMENTS,
                                            "[v12][m0][t5]yuan su");
        case 'X':
            return Action_RequestWithSpeech(ACTION_TRAINING,
                                            "[v12][m0][t5]xiao xun lian");
        case 'W':
            return Action_RequestWithSpeech(ACTION_WORLD_LIGHT,
                                            "[v12][m0][t5]shi jie zhi guang");
        case 'U':
            return Action_RequestWithSpeech(ACTION_WAKE_UP,
                                            "[v12][m0][t5]xiao dai");
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
