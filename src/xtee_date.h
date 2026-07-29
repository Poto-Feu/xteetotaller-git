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
