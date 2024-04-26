//======== pwmled2.c ========
// For usleep()
#include <unistd.h>
#include <stddef.h>

// Driver Header files
#include <ti/drivers/PWM.h>

// Driver configuration
#include "ti_drivers_config.h"

//======== mainThread ========
//Task periodically increments the PWM duty for the on board LED.
void *mainThread(void *arg0)
{
    // Period and duty in microseconds
    uint16_t pwmPeriod = 1000000;
    uint16_t duty      = 0;
    uint16_t dutyInc   = 100;

    // Sleep time in microseconds
    uint32_t time   = 1000000; // 1 second
    PWM_Handle pwm1 = NULL;
    PWM_Handle pwm2 = NULL;
    PWM_Params params;

    // Call driver init functions.
    PWM_init();
    PWM_Params_init(&params);
    params.dutyUnits   = PWM_DUTY_US;
    params.dutyValue   = 0;
    params.periodUnits = PWM_PERIOD_US;
    params.periodValue = pwmPeriod;
    pwm1               = PWM_open(CONFIG_PWM_0, &params);
    if (pwm1 == NULL)
    {
        while (1) {} // CONFIG_PWM_0 did not open
    }
    PWM_start(pwm1); // start PWM1 with 0% duty cycle

    pwm2 = PWM_open(CONFIG_PWM_1, &params);
    if (pwm2 == NULL)
    {
        while (1) {} // CONFIG_PWM_0 did not open
    }
    PWM_start(pwm2); // start PWM2 with 0% duty cycle

    while (1) // Loop forever incrementing the PWM duty
    {
        PWM_setDuty(pwm1, (pwmPeriod * 0.9));
        PWM_setDuty(pwm2, (pwmPeriod * 0.1));
        usleep(time);
        PWM_setDuty(pwm1, 0);
        PWM_setDuty(pwm2, (pwmPeriod * 0.9));
        usleep(time);

        /* duty = (duty + dutyInc);

        if (duty == pwmPeriod || (!duty))
        {
            dutyInc = -dutyInc;
        } */
    }
}
