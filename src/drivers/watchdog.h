#ifndef WATCHDOG_H
#define WATCHDOG_H

void watchdog_init(void); /* Call once before starting scheduler */
void watchdog_kick(void); /* Call from health_monitor every 100ms */

#endif
