#include <msp430.h>
#include "libTimer.h"
#include "buzzer.h"

char p2val;

void buzzer_init()
{
    /* 
     *       Direct timer A output "TA0.1" to P2.6.  
     *        According to table 21 from data sheet:
     *          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
     *          P2SEL.6 must be 1
     *        Also: P2.6 direction must be output
     */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */
    /* start buzzing!!! */
}

void buzzer_set_period(short cycles)
{
    CCR0 = cycles; 
    CCR1 = cycles >> 1;		/* one half cycle */
}
void note(unsigned int note, unsigned long duartion) {
    switch (duartion) {
        case w:
            buzzer_set_period(note);
            __delay_cycles(w);
            break;
        case h:
            buzzer_set_period(note);
            __delay_cycles(h);
            break;
        case q:
            buzzer_set_period(note);
            __delay_cycles(q);
            break;
        case e:
            buzzer_set_period(note);
            __delay_cycles(e);
            break;
        case s:
            buzzer_set_period(note);
            __delay_cycles(s);
            break;
        case t:
            buzzer_set_period(note);
            __delay_cycles(t);
            break;
    }
    buzzer_set_period(0);
}

void win() {
    note(as5,q);
    note(d4,q);
    note(g4,q);
    note(as5,q);
    note(c5,q);
    note(f4,q);
    note(a5,q);
    note(as5,q);
    note(g4,q);
    note(as5,q);
    note(d5,q);
    note(ds5,q);
    note(g4,q);
    note(d5,q);
}

