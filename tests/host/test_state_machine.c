#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ActionTask.h"
#include "SafetyTask.h"
#include "AudioTask.h"

static float g_angles[4];
static unsigned g_servo_writes;
static unsigned g_audio_requests;
static unsigned g_audio_runs;
static unsigned char g_audio_busy;
static char g_last_audio[64];

static void reset_observations(void)
{
    g_angles[0] = 90.0f;
    g_angles[1] = 90.0f;
    g_angles[2] = 90.0f;
    g_angles[3] = 90.0f;
    g_servo_writes = 0U;
    g_audio_requests = 0U;
    g_audio_runs = 0U;
    g_audio_busy = 0U;
    g_last_audio[0] = '\0';
}

void Servo_SetAngle1(float angle) { g_angles[0] = angle; g_servo_writes++; }
void Servo_SetAngle2(float angle) { g_angles[1] = angle; g_servo_writes++; }
void Servo_SetAngle3(float angle) { g_angles[2] = angle; g_servo_writes++; }
void Servo_SetAngle4(float angle) { g_angles[3] = angle; g_servo_writes++; }

void AudioTask_Init(void)
{
    g_audio_busy = 0U;
}

uint8_t AudioTask_RequestText(const char *text)
{
    if ((text == NULL) || (g_audio_busy != 0U))
    {
        return 0U;
    }
    g_audio_requests++;
    (void)snprintf(g_last_audio, sizeof(g_last_audio), "%s", text);
    g_audio_busy = 1U;
    return 1U;
}

void AudioTask_Run(uint32_t now_ms)
{
    (void)now_ms;
    g_audio_runs++;
    g_audio_busy = 0U;
}

uint8_t AudioTask_IsBusy(void)
{
    return g_audio_busy;
}

static unsigned g_tests;
static unsigned g_failures;

#define EXPECT_TRUE(condition, message) do { \
    g_tests++; \
    if (!(condition)) { \
        g_failures++; \
        fprintf(stderr, "FAIL: %s (%s:%d)\n", message, __FILE__, __LINE__); \
    } \
} while (0)

#define EXPECT_EQ_UINT(actual, expected, message) \
    EXPECT_TRUE((actual) == (expected), message)

#define EXPECT_EQ_FLOAT(actual, expected, message) \
    EXPECT_TRUE(fabs((actual) - (expected)) < 0.01, message)

static void test_pose_progression_and_preemption(void)
{
    reset_observations();
    ActionTask_Init();
    AudioTask_Init();

    EXPECT_EQ_UINT(ActionTask_RequestMotion(ACTION_FORWARD), 1U,
                   "forward request is accepted");
    ActionTask_Run(0U);
    EXPECT_EQ_UINT(ActionTask_Current(), ACTION_FORWARD,
                   "forward action becomes current");
    EXPECT_EQ_FLOAT(g_angles[0], 135.0f,
                    "first forward pose is applied immediately");
    EXPECT_EQ_UINT(g_servo_writes, 4U,
                   "one PoseStep writes all four servos");

    ActionTask_Run(149U);
    EXPECT_EQ_FLOAT(g_angles[0], 135.0f,
                    "step does not advance before its deadline");

    EXPECT_EQ_UINT(ActionTask_RequestMotion(ACTION_DANCE), 1U,
                   "new action request is accepted during motion");
    ActionTask_Run(149U);
    EXPECT_EQ_UINT(ActionTask_Current(), ACTION_DANCE,
                   "new action preempts the old action");
    EXPECT_EQ_FLOAT(g_angles[0], 135.0f,
                    "preempting action applies its first pose");

    ActionTask_Run(298U);
    EXPECT_EQ_FLOAT(g_angles[0], 135.0f,
                    "dance remains on first pose before deadline");
    ActionTask_Run(299U);
    EXPECT_EQ_FLOAT(g_angles[0], 90.0f,
                    "dance advances at the PoseStep deadline");
}

static void test_duplicate_command_does_not_restart(void)
{
    unsigned writes_after_start;

    reset_observations();
    ActionTask_Init();
    ActionTask_RequestMotion(ACTION_HELLO);
    ActionTask_Run(0U);
    writes_after_start = g_servo_writes;

    EXPECT_EQ_UINT(ActionTask_RequestMotion(ACTION_HELLO), 1U,
                   "duplicate action request is accepted as a no-op");
    ActionTask_Run(1U);
    EXPECT_EQ_UINT(g_servo_writes, writes_after_start,
                   "duplicate request does not restart the first PoseStep");
}

static void test_emergency_stop_restores_safe_pose(void)
{
    reset_observations();
    ActionTask_Init();
    SafetyTask_Init();
    ActionTask_RequestMotion(ACTION_SLEEP_PRONE);
    ActionTask_Run(0U);

    SafetyTask_RequestEmergencyStop();
    SafetyTask_Run(1U);

    EXPECT_EQ_UINT(ActionTask_Current(), ACTION_NONE,
                   "emergency stop clears current action");
    EXPECT_EQ_UINT(ActionTask_IsActive(), 0U,
                   "emergency stop clears active flag");
    EXPECT_EQ_FLOAT(g_angles[0], 90.0f, "safe stop sets servo 1 to 90 degrees");
    EXPECT_EQ_FLOAT(g_angles[1], 90.0f, "safe stop sets servo 2 to 90 degrees");
    EXPECT_EQ_FLOAT(g_angles[2], 90.0f, "safe stop sets servo 3 to 90 degrees");
    EXPECT_EQ_FLOAT(g_angles[3], 90.0f, "safe stop sets servo 4 to 90 degrees");
    EXPECT_EQ_UINT(SafetyTask_ConsumeStopRequest(), 0U,
                   "safety task consumes the stop request internally");
}

static void test_communication_fault_latches_and_stops(void)
{
    reset_observations();
    ActionTask_Init();
    SafetyTask_Init();
    ActionTask_RequestMotion(ACTION_TRAINING);
    ActionTask_Run(0U);

    SafetyTask_RequestCommunicationFault();
    EXPECT_EQ_UINT(SafetyTask_IsFaultActive(), 1U,
                   "communication fault is latched");
    SafetyTask_Run(1U);
    EXPECT_EQ_UINT(ActionTask_IsActive(), 0U,
                   "communication fault stops the active action");
    EXPECT_EQ_FLOAT(g_angles[0], 90.0f,
                    "communication fault restores the safe pose");
}

static void test_advanced_action_requests_speech(void)
{
    reset_observations();
    ActionTask_Init();
    AudioTask_Init();

    EXPECT_EQ_UINT(ActionTask_HandleCommand('B'), 1U,
                   "B command starts the advanced action");
    EXPECT_EQ_UINT(ActionTask_Current(), ACTION_DECLARE_LOVE,
                   "B maps to the declaration action");
    EXPECT_EQ_UINT(g_audio_requests, 1U,
                   "B requests one speech frame");
    EXPECT_TRUE(strstr(g_last_audio, "wo ai ni") != NULL,
                 "B speech text is queued");

    EXPECT_EQ_UINT(ActionTask_HandleCommand('Y'), 1U,
                   "Y action can preempt B while audio is busy");
    EXPECT_EQ_UINT(ActionTask_Current(), ACTION_ELEMENTS,
                   "Y maps to the elements action");
    EXPECT_EQ_UINT(g_audio_requests, 1U,
                   "busy audio does not block or duplicate the request");

    AudioTask_Run(1U);
    EXPECT_EQ_UINT(g_audio_runs, 1U,
                   "audio task advances independently");
    EXPECT_EQ_UINT(ActionTask_HandleCommand('X'), 1U,
                   "X action starts after audio becomes available");
    EXPECT_EQ_UINT(g_audio_requests, 2U,
                   "new speech request is accepted after previous frame");
}

int main(void)
{
    test_pose_progression_and_preemption();
    test_duplicate_command_does_not_restart();
    test_emergency_stop_restores_safe_pose();
    test_communication_fault_latches_and_stops();
    test_advanced_action_requests_speech();

    printf("state_machine_tests: %u assertions, %u failures\n",
           g_tests, g_failures);
    return (g_failures == 0U) ? 0 : 1;
}
