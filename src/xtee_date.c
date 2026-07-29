#include "xtee_date.h"

#include <time.h>
#include <stdio.h>

void
xtee_date_set_to_current_day(XteeDate *date)
{
        struct tm *current_time;
        time_t t;
        int corrected_year, corrected_month;

        t = time(NULL);
        current_time = localtime(&t);
        corrected_year = 1900 + current_time->tm_year;
        corrected_month = 1 + current_time->tm_mon;
        /* Ignore GCC warnings as the source size will fit. */
#pragma GCC diagnostic ignored "-Wformat-overflow"
        sprintf(date->year, "%d", corrected_year);
        sprintf(date->month, "%02d", corrected_month);
        sprintf(date->day, "%02d", current_time->tm_mday);
#pragma GCC diagnostic pop
}
