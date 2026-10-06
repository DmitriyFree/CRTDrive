#include "hardware.h"

#include <Arduino.h>
#include "common.h"
#include "soc/gpio_struct.h"
#include "soc/ledc_reg.h"
#include "soc/ledc_struct.h"

// ============================================================
// Fast GPIO
// ============================================================

inline void fastWrite(int pin, bool level)
{
    if (level)
        GPIO.out_w1ts = (1UL << pin);
    else
        GPIO.out_w1tc = (1UL << pin);
}


// ============================================================
// Fast LEDC
// ============================================================

inline void fastPwmX(int duty)
{
    LEDC.channel_group[0].channel[0].duty.duty =
        ((uint32_t)duty << 4);

    LEDC.channel_group[0].channel[0].conf1.duty_start = 1;
}

inline void fastPwmY(int duty)
{
    LEDC.channel_group[0].channel[1].duty.duty =
        ((uint32_t)duty << 4);

    LEDC.channel_group[0].channel[1].conf1.duty_start = 1;
}


// ============================================================
// Direction state
// ============================================================

int currentXDirection = 1;
int currentYDirection = 1;


// ============================================================
// X axis
// ============================================================

void setX(int x)
{
    if (x > 0)
    {
        if (currentXDirection != 1)
        {
            fastWrite(X_DIR_PIN, LOW);
            currentXDirection = 1;
        }

        fastPwmX(x);
    }
    else if (x < 0)
    {
        if (currentXDirection != -1)
        {
            fastWrite(X_DIR_PIN, HIGH);
            currentXDirection = -1;
        }

        fastPwmX(-x);
    }
    else
    {
        // Keep a tiny current through zero.
        fastPwmX(0);
    }
}


// ============================================================
// Y axis
// ============================================================

void setY(int y)
{
    // Physical Y axis is inverted.
    y = -y;

    if (y > 0)
    {
        if (currentYDirection != 1)
        {
            fastWrite(Y_DIR_PIN, LOW);
            currentYDirection = 1;
        }

        fastPwmY(y);
    }
    else if (y < 0)
    {
        if (currentYDirection != -1)
        {
            fastWrite(Y_DIR_PIN, HIGH);
            currentYDirection = -1;
        }

        fastPwmY(-y);
    }
    else
    {
        // Keep a tiny current through zero.
        fastPwmY(0);
    }
}


// ============================================================
// 2D position
// ============================================================

void setPoint(int x, int y)
{
    setX(x);
    setY(y);
}


// ============================================================
// Stop
// ============================================================

void stopDeflection()
{
    ledcWrite(X_PWM_PIN, 0);
    ledcWrite(Y_PWM_PIN, 0);
}


// ============================================================
// Beam
// ============================================================

void beamOn()
{
    GPIO.out_w1ts = (1UL << BEAM_ALPHA_A_PIN); // 15 = HIGH
}

void beamOff()
{
    GPIO.out_w1tc = (1UL << BEAM_ALPHA_A_PIN); // 15 = LOW
}


// ============================================================
// Initialization
// ============================================================

void hardwareInit()
{
    pinMode(X_PWM_PIN, OUTPUT);
    pinMode(X_DIR_PIN, OUTPUT);

    pinMode(2, OUTPUT);//led

    pinMode(Y_PWM_PIN, OUTPUT);
    pinMode(Y_DIR_PIN, OUTPUT);

    pinMode(DRIVER_SLEEP_PIN, OUTPUT);
    pinMode(BEAM_ALPHA_A_PIN, OUTPUT);

    // Keep drivers asleep during configuration.
    digitalWrite(DRIVER_SLEEP_PIN, LOW);

    // Beam enabled
    beamOn();

    ledcAttach(
        X_PWM_PIN,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    ledcAttach(
        Y_PWM_PIN,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    stopDeflection();

    // Wake drivers.
    digitalWrite(DRIVER_SLEEP_PIN, HIGH);
}