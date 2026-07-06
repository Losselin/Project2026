#ifndef TIMEACCOUNT_H
#define TIMEACCOUNT_H

void timeaccount_reset();
void timeaccount_set_running(bool running);
int timeaccount_get_elapsed_seconds();

#endif
