#ifndef DATE_CALCULATIONS
#define DATE_CALCULATIONS
/*****************************************************************************/
/*  MODULE NAME:  lib_date.h                            MODULE TYPE:  (lib)  */
/*****************************************************************************/
/*  Date calculations complying with ISO/R 2015-1971 and DIN 1355 standards  */
/*****************************************************************************/
/*  MODULE IMPORTS:                                                          */
/*****************************************************************************/
#include <stdlib.h>                                 /*  MODULE TYPE:  (sys)  */
#include <string.h>                                 /*  MODULE TYPE:  (sys)  */
#include <ctype.h>                                  /*  MODULE TYPE:  (sys)  */
#include "lib_defs.h"                               /*  MODULE TYPE:  (dat)  */
/*****************************************************************************/
/*  MODULE INTERFACE:                                                        */
/*****************************************************************************/

boolean leap(N_int year);
boolean check_date(N_int year, N_int mm, N_int dd);

N_int   compress(N_int yy, N_int mm, N_int dd);
boolean uncompress(N_int date, N_int *cc, N_int *yy, N_int *mm, N_int *dd);
boolean check_compressed(N_int date);
baseptr compressed_to_short(N_int date);

Z_long  calc_days(N_int year, N_int mm, N_int dd);
N_int   day_of_week(N_int year, N_int mm, N_int dd);
Z_long  dates_difference(N_int year1, N_int mm1, N_int dd1,
                         N_int year2, N_int mm2, N_int dd2);
boolean calc_new_date(N_int *year, N_int *mm, N_int *dd, Z_long offset);

boolean date_time_difference
(
    Z_long *days, Z_int *hh, Z_int *mm, Z_int *ss,
    N_int year1, N_int month1, N_int day1, N_int hh1, N_int mm1, N_int ss1,
    N_int year2, N_int month2, N_int day2, N_int hh2, N_int mm2, N_int ss2
);
boolean calc_new_date_time
(
    N_int *year, N_int *month, N_int *day, N_int *hh, N_int *mm, N_int *ss,
    Z_long days_offset, Z_long hh_offset, Z_long mm_offset, Z_long ss_offset
);

baseptr date_to_short(N_int year, N_int mm, N_int dd);
baseptr date_to_string(N_int year, N_int mm, N_int dd);

N_int   weeks_in_year(N_int year);
N_int   week_number(N_int year, N_int mm, N_int dd);
boolean week_of_year(N_int *week, N_int *year, N_int  mm, N_int  dd);
boolean first_in_week(N_int week, N_int *year, N_int *mm, N_int *dd);

N_int   decode_day(baseptr buffer, N_int len);
N_int   decode_month(baseptr buffer, N_int len);
boolean decode_date(baseptr buffer, N_int *year, N_int *mm, N_int *dd);

void    annihilate(baseptr buffer);

/*****************************************************************************/
/*  MODULE RESOURCES:                                                        */
/*****************************************************************************/

extern N_int   month_length[2][13];
/*
    {
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
    };
*/

extern N_char  day_name[8][16];
/*
    {
        "Error", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday", "Sunday"
    };
*/

extern N_char  month_name[13][16];
/*
    {
        "Error", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
*/

extern const blockdef(rsrc_date_001,16); /* = "<no date>"; exactly 9 chars long */
extern const blockdef(rsrc_date_002,32); /* = "<no date>"; short form */
extern const blockdef(rsrc_date_003,64); /* = "<no date>"; verbose form */

/*****************************************************************************/
/*  MODULE IMPLEMENTATION:                                                   */
/*****************************************************************************/

/*****************************************************************************/
/*  AUTHOR:  Steffen Beyer                                                   */
/*****************************************************************************/
/*  VERSION:  3.2                                                            */
/*****************************************************************************/
/*  VERSION HISTORY:                                                         */
/*****************************************************************************/
/*    01.11.93    Created                                                    */
/*    16.02.97    Version 3.0                                                */
/*    15.06.97    Version 3.2                                                */
/*****************************************************************************/
/*  COPYRIGHT:                                                               */
/*                                                                           */
/*    Copyright (c) 1995, 1996, 1997, 1998 by Steffen Beyer.                 */
/*    All rights reserved.                                                   */
/*                                                                           */
/*    This library is free software; you can redistribute it and/or          */
/*    modify it under the terms of the GNU Library General Public            */
/*    License as published by the Free Software Foundation; either           */
/*    version 2 of the License, or (at your option) any later version.       */
/*                                                                           */
/*    This library is distributed in the hope that it will be useful,        */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       */
/*    Library General Public License for more details.                       */
/*                                                                           */
/*    You should have received a copy of the GNU Library General Public      */
/*    License along with this library; if not, write to the Free Software    */
/*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,   */
/*    USA.                                                                   */
/*                                                                           */
/*****************************************************************************/
#endif
