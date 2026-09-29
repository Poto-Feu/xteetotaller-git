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

#ifndef XTEE_DATE_H
#define XTEE_DATE_H

/* All char fields have a NUL-terminator, hence their size. */
/* As Earth will be absorbed by the Sun in about 7.5 billion years, 9 digits
 * for the year field should be OK. */
typedef struct XteeDate {
        char year[10];
        char month[3];
        char day[3];
} XteeDate;

void xtee_date_set_to_current_day(XteeDate *date);

#endif
