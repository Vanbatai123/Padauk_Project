
#include "extern.h"

//				P? VDD PA0 PA3 PA4 PA5 PA6 PA7 GND mask1 mask2 shift
.writer package 6, 5,  0,  6,  1,  4,  3,  0,  2,  0x00FF, 0x00FF, 0

// Delay
#define _delay_ms(x) .delay x*System_Clock/1000
#define _delay_us(x) .delay x*System_Clock/1000000

/*
2.a. assign variables
chustv=1000ms
t1=50ms
t2=100ms
t3=150ms
t4=300ms
t5=500ms
t6=1000ms
t7=2000ms
*/

// ms
#define chustv          1000
#define t1              50
#define t2              100
#define t3              150
#define t4              300
#define t5              500
#define t6              1000
#define t7              2000

#define MAX_STEP5       3

/*
3. assign input/output ports
PA3 - PWM output (initial state 0)
PA4 - Output -1 – (initial state 0)
PA5 - input -1 (ball vibration sensor)
PA6 - input - 1 (spring vibration sensor)
*/
// Pin definitions
#define PIN_A4              PA.4
#define PIN_BALL_A5         PA.5
#define PIN_SPRING_A6       PA.6

#define PWM_DISABLE()       tm2c = 0b0000_10_1_1 // disable clock, PA3, PWM mode, inverse polarity
#define PWM_ENABLE()        tm2c = 0b0001_10_1_1 // system clock, output=PA3, PWM mode, inverse polarity


void FPPA0 (void)
{
    .ADJUST_IC SYSCLK=IHRC/2 // SYSCLK=IHRC/2
    /*2.a. assign variables
    chustv=1000ms
    t1=50ms
    t2=100ms
    t3=150ms
    t4=300ms
    t5=500ms
    t6=1000ms
    t7=2000ms
    */
    // defined above

    /*2.b. config PWM
    "PWM"=40%=H/L
    1 + L/H = 1 + 100/40
    (L+H)/H = 7/2
    => PWM = 2/7 = 28.57%
        
    chast=2048Hz
    
    datasheet page 34
    Frequency of Output = Y ÷ [256 × S1 × (S2+1) ]
    Duty of Output = ( K+1 ) ÷ 256
    
    Where, Y = tm2c[7:4] : frequency of selected clock source
    K = tm2b[7:0] : bound register in decimal
    S1= tm2s[6:5] : pre-scalar (1, 4, 16, 64)
    S2 = tm2s[4:0] : scalar register in decimal (1 ~ 31)
    
    K = 256*28.57%-1=72.14 => 72
    => PWM = 39.84%
    
    Y = 8MHz
    S1 * (S2+1) = 8MHz/2048Hz/256 = 15.259
    S1 = 1
    S2 = 14
    => chast = 1953.125 Hz
    */
    tm2ct = 0x0;// restart counter
    // tm2b = 73;
    tm2b = 182;//inverse
    tm2s = 0b0_00_01110; // 8-bit PWM, pre-scalar = 4, scalar = 3
    tm2c = 0b0000_10_1_1; // PA3, PWM mode, inverse polarity

    /*
    3. assign input/output ports
    PA3 - PWM output (initial state 0)
    PA4 - Output -1 – (initial state 0)
    PA5 - input -1 (ball vibration sensor)
    PA6 - input - 1 (spring vibration sensor)
    */
// step3:
    $ PIN_A4        Out, Low
    $ PIN_SPRING_A6 In, Pull
    $ PIN_BALL_A5   In, Pull

    word cnt_timer = 0;
    word cnt_step = 0;
    int sleep = 0;
    int value_A5 = 0;
    int value_A6 = 0;

    while (1)
    {
step4:
        // 4. read PA5 for 1,
        // if PA5 shows “1” for more than 2000ms, then go to step 5
        // if not, continue reading
        if (PIN_BALL_A5 == 1)
        {
            _delay_ms(2000);
            if (PIN_BALL_A5 == 1)
            {
                goto step5; // go to step 5
            }
        }
        goto step4; // continue reading

step5:
        cnt_step = 0;
        while (cnt_step < MAX_STEP5)
        {
            // 5.a PA4 = 1, delay(t6), PA4 = 0
            PIN_A4 = 1;
            _delay_ms(t6);
            PIN_A4 = 0;
            //5.b. Out PWM A3 = chast during t6
            PWM_ENABLE();
            _delay_ms(t6); // output PWM during t6
            PWM_DISABLE(); // stop pwm

            cnt_step++;
        }
// step6:
        // 6. sleep = 1
        sleep = 1;
        goto step7_sleep;

step7_sleep:
        // 7. enter sleep mode
        stopsys;
        goto exit_sleep;// goto exit sleep after wake up to check sleep value
step8:
        // 8. Wakeup, read PA6 == 1 more than t6(ms)?
        if (PIN_SPRING_A6 == 1)
        {
            _delay_ms(t6);
            if (PIN_SPRING_A6 == 1)
                goto step10; // PA6 = 1 more than t6, go to step 10
            else
                goto step9; // PA6 = 1 less than t6, go to step 9
        }
        else
            goto step7_sleep; // wakeup not by PA6, sleep again
step9:
        // 9.a PA4 = 1, delay(t1), PA4 = 0
        PIN_A4 = 1;
        _delay_ms(t1);
        PIN_A4 = 0;
        // 9.b. Out PWM A3 = chast during t6
        PWM_ENABLE();
        _delay_ms(t6); // output PWM during t6
        PWM_DISABLE(); // stop pwm

        goto step7_sleep;

step10:
        // 10.a PA4 = 1, read A5,A6 during t3, PA4 = 0
        PIN_A4 = 1;
        // reset vaule, read, update
        value_A5 = 0;
        value_A6 = 0;
        cnt_timer = 0;
        while (cnt_timer < t3)
        {
            if (value_A5 == 0 && PIN_BALL_A5 == 1) 
                value_A5 = 1;
            if (value_A6 == 0 && PIN_SPRING_A6 == 1) 
                value_A6 = 1;
            _delay_ms(1);
            cnt_timer++;
        }
        PIN_A4 = 0;

        //10.b. Out PWM A3 = chast, read A5,A6 during t3
        PWM_ENABLE();

        cnt_timer = 0;
        while (cnt_timer < t3)
        {
            if (value_A5 == 0 && PIN_BALL_A5 == 1) 
                value_A5 = 1;
            if (value_A6 == 0 && PIN_SPRING_A6 == 1) 
                value_A6 = 1;
            _delay_ms(1);
            cnt_timer++;
        }
        
        PWM_DISABLE(); // stop pwm

        if(value_A5 == 1)
        {
            sleep = 2;
            goto step7_sleep;
        }
        else if(value_A6 == 1)
            goto step10; // back to step 10
        else
            goto step11; // go to step 11
step11:
        // 11. 
        // PA4 = 1, Read PA6, during t4
        // PA4 = 0, Read PA6, during t7
        // perform 10 times
        // if PA6 == 1, jump to step 10 immediately
        // if not, jump to step 7

        cnt_step = 0;
        // perform 10 times
        while(cnt_step < 10)
        {
            // PA4 = 1, Read PA6, during t4
            PIN_A4 = 1;
            cnt_timer = 0;
            while (cnt_timer < t4)
            {
                // if PA6 == 1, jump to step 10 immediately
                if (PIN_SPRING_A6 == 1)
                    goto step10;

                _delay_ms(1);
                cnt_timer++;
            }
            
            // PA4 = 0, Read PA6, during t7
            PIN_A4 = 0;
            cnt_timer = 0;
            while (cnt_timer < t7)
            {
                // if PA6 == 1, jump to step 10 immediately
                if (PIN_SPRING_A6 == 1)
                    goto step10;

                _delay_ms(1);
                cnt_timer++;
            }

            cnt_step++;// increase times counter
        }
        // PA6 not show 1 after 10 times, jump to step 7
        goto step7_sleep;

step12:
        // 12. Wakeup, read PA5 == 1 more than 2000(ms)?
        if (PIN_BALL_A5 == 1)
        {
            _delay_ms(2000);
            if (PIN_BALL_A5 == 1)
                goto step5; // PA5 = 1 more than 2000(ms), go to step 5
            else
                goto step7_sleep; // PA5 = 1 less than 2000(ms), sleep again
        }
        else
            goto step7_sleep; // wakeup not by PA5, sleep again
        
exit_sleep:
        if (sleep == 1) goto step8;
        else if (sleep == 2) goto step12;
        else goto step7_sleep;
    }
}
