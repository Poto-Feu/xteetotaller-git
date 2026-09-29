/*
 * Copyright (C) 2026 Adrien Saad
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "xtee_date.h"

#include <time.h>
#include <stdio.h>

void
xtee_date_set_to_current_day(XteeDate *date)
{
        const struct tm *current_time;
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
