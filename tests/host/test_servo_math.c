/* Host tests for ServoMath.h: pulse widths, trim, clamping. */
#include <stdio.h>
#define SERVO_TRIM_DEG { 0, 5, -7, 0 }
#include "ServoMath.h"

static int failures = 0, assertions = 0;
#define CHECK(c) do { assertions++; if (!(c)) { failures++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

int main(void)
{
    /* Untrimmed servo 1: same values as the old float implementation */
    CHECK(ServoMath_AngleToCompare(0, 0) == 500);
    CHECK(ServoMath_AngleToCompare(0, 90) == 1500);
    CHECK(ServoMath_AngleToCompare(0, 180) == 2500);
    CHECK(ServoMath_AngleToCompare(0, 45) == 1000);
    CHECK(ServoMath_AngleToCompare(0, 135) == 2000);

    /* Trim applied per servo */
    CHECK(ServoMath_AngleToCompare(1, 90) == ServoMath_AngleToCompare(0, 95));
    CHECK(ServoMath_AngleToCompare(2, 90) == ServoMath_AngleToCompare(0, 83));

    /* Clamped to the safe range after trim */
    CHECK(ServoMath_AngleToCompare(1, 180) == 2500);
    CHECK(ServoMath_AngleToCompare(2, 3) == 500);
    CHECK(ServoMath_AngleToCompare(0, -20) == 500);
    CHECK(ServoMath_AngleToCompare(0, 300) == 2500);

    /* Round trip */
    CHECK(ServoMath_CompareToAngle(ServoMath_AngleToCompare(0, 37)) == 37);
    CHECK(ServoMath_CompareToAngle(1500) == 90);
    CHECK(ServoMath_CompareToAngle(100) == 0);
    CHECK(ServoMath_CompareToAngle(3000) == 180);

    printf("servo_math_tests: %d assertions, %d failures\n", assertions, failures);
    return failures != 0;
}
