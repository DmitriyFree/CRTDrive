
#include "hardware.h"

#include <Arduino.h>
#include "common.h"
#include "soc/gpio_struct.h"
#include "soc/ledc_reg.h"
#include "soc/ledc_struct.h"

namespace
{
    struct PwmConfig
    {
        uint32_t frequencyX;
        uint32_t frequencyY;
        uint8_t resolutionX;
        uint8_t resolutionY;
        float scaleX;
        float scaleY;
        int deadZoneX;
        int deadZoneY;
    };

    constexpr PwmConfig RASTER_CONFIG {
        19531, 39062,  // X / Y frequencies
        12, 10,        // X / Y resolutions
        RASTER_SCALE_X,
        RASTER_SCALE_Y,
        30,
        25
    };
    constexpr PwmConfig TEXT_CONFIG {
        156250, 156250,
        9, 9,
        TEXT_SCALE_X,
        TEXT_SCALE_Y,
        72,
        72
    };

    PwmConfig activeConfig = TEXT_CONFIG;
    DisplayProfile activeProfile = DisplayProfile::Text;
    bool profileInitialized = false;

    int currentXDirection = 1;
    int currentYDirection = 1;

    inline void fastWrite(int pin, bool level)
    {
        if (level)
            GPIO.out_w1ts = (1UL << pin);
        else
            GPIO.out_w1tc = (1UL << pin);
    }

    inline void fastPwmX(int duty)
    {
        const int maxDuty = (1 << activeConfig.resolutionX) - 1;

        duty = static_cast<int>(lroundf(duty * activeConfig.scaleX));
        duty = constrain(duty, 0, maxDuty);

        LEDC.channel_group[0].channel[0].duty.duty =
            (static_cast<uint32_t>(duty) << 4);

        LEDC.channel_group[0].channel[0].conf1.duty_start = 1;
    }

    inline void fastPwmY(int duty)
    {
        const int maxDuty = (1 << activeConfig.resolutionY) - 1;

        duty = static_cast<int>(lroundf(duty * activeConfig.scaleY));
        duty = constrain(duty, 0, maxDuty);

        LEDC.channel_group[0].channel[1].duty.duty =
            (static_cast<uint32_t>(duty) << 4);

        LEDC.channel_group[0].channel[1].conf1.duty_start = 1;
    }

    const PwmConfig& configFor(DisplayProfile profile)
    {
        switch (profile)
        {
            case DisplayProfile::Raster:  return RASTER_CONFIG;
            case DisplayProfile::Text:    return TEXT_CONFIG;
        }

        return TEXT_CONFIG;
    }
}

int pwmMaxDutyX()
{
    return (1 << activeConfig.resolutionX) - 1;
}

int pwmMaxDutyY()
{
    return (1 << activeConfig.resolutionY) - 1;
}

float pwmScaleX()
{
    return activeConfig.scaleX;
}

float pwmScaleY()
{
    return activeConfig.scaleY;
}

DisplayProfile getDisplayProfile()
{
    return activeProfile;
}

bool setDisplayProfile(DisplayProfile profile)
{
    if (profileInitialized && profile == activeProfile)
        return true;

    const PwmConfig next = configFor(profile);

    // Do not call this while another task is actively drawing.
    beamOff();
    digitalWrite(DRIVER_SLEEP_PIN, LOW);

    ledcDetach(X_PWM_PIN);
    ledcDetach(Y_PWM_PIN);

    const bool xOk = ledcAttachChannel(
        X_PWM_PIN,
        next.frequencyX,
        next.resolutionX,
        0
    );

    const bool yOk = ledcAttachChannel(
        Y_PWM_PIN,
        next.frequencyY,
        next.resolutionY,
        1
    );

    if (!xOk || !yOk)
    {
        profileInitialized = false;
        return false;
    }

    activeConfig = next;
    activeProfile = profile;
    profileInitialized = true;

    // Restore a known position and direction after reconfiguration.
    currentXDirection = 1;
    currentYDirection = 1;

    fastWrite(X_DIR_PIN, LOW);
    fastWrite(Y_DIR_PIN, LOW);

    fastPwmX(0);
    fastPwmY(0);

    digitalWrite(DRIVER_SLEEP_PIN, HIGH);
    beamOn();

    return true;
}

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
    /*else
    {
        fastPwmX(0);
    }*/
}

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
    /*else
    {
        fastPwmY(0);
    }*/
}

void setPoint(int x, int y)
{
    setX(x);
    setY(y);
}

void beamOn()
{
    fastWrite(BEAM_ALPHA_A_PIN, HIGH);
}

void beamOff()
{
    fastWrite(BEAM_ALPHA_A_PIN, LOW);
}

void hardwareInit()
{
    pinMode(X_PWM_PIN, OUTPUT);
    pinMode(X_DIR_PIN, OUTPUT);
    pinMode(Y_PWM_PIN, OUTPUT);
    pinMode(Y_DIR_PIN, OUTPUT);
    pinMode(DRIVER_SLEEP_PIN, OUTPUT);
    pinMode(BEAM_ALPHA_A_PIN, OUTPUT);
    pinMode(2, OUTPUT);

    digitalWrite(DRIVER_SLEEP_PIN, LOW);
    beamOff();

    profileInitialized = false;

    // Set up the initial drawing profile.
    if (setDisplayProfile(DisplayProfile::Text))
    {
        setPoint(0, 0);
        beamOn();
    }
}
int getDisplayDeadZoneX()
{
    return activeConfig.deadZoneX;
}

int getDisplayDeadZoneY()
{
    return activeConfig.deadZoneY;
}