#ifndef TIMER_H
#define TIMER_H

#define TIMER_IRQ_ID 27

void timer_init(void);
void timer_reset(void);
unsigned long timer_get_frequency(void);

#endif