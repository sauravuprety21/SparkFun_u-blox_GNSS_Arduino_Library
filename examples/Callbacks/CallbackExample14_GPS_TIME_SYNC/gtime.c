/*------------------------------------------------------------------------------
* gtime.h : P3 time definition and function declaration
*
*          Copyright (C) 2018-2018, All rights reserved to
*          Dr. Yang Gao's group.
*
* author : Zhitao Lyu
* options :
*
* references :
*     [1]
*
* version : $Revision: 1.0 $ $Date: 2018/07/18 21:07:00 $
* history : 2018/07/18 1.0  P3 ver.1.0.0
*-----------------------------------------------------------------------------*/
#include "gtime.h"

#define MAXLEAPS    64                  /* max number of leap seconds table */

const static float64_t gpst0[]={1980,1, 6,0,0,0}; /* gps time reference */


/* leap seconds (y,m,d,h,m,s,utc-gpst) */
static float64_t leaps[MAXLEAPS+1][7]={
    {2017,1,1,0,0,0,-18},
{2015,7,1,0,0,0,-17},
{2012,7,1,0,0,0,-16},
{2009,1,1,0,0,0,-15},
{2006,1,1,0,0,0,-14},
{1999,1,1,0,0,0,-13},
{1997,7,1,0,0,0,-12},
{1996,1,1,0,0,0,-11},
{1994,7,1,0,0,0,-10},
{1993,7,1,0,0,0, -9},
{1992,7,1,0,0,0, -8},
{1991,1,1,0,0,0, -7},
{1990,1,1,0,0,0, -6},
{1988,1,1,0,0,0, -5},
{1985,7,1,0,0,0, -4},
{1983,7,1,0,0,0, -3},
{1982,7,1,0,0,0, -2},
{1981,7,1,0,0,0, -1},
{0}
};

/* add time --------------------------------------------------------------------
* add time to gtime_t struct
* args   : gtime_t t        I   gtime_t struct
*          float64_t sec    I   time to add (s)
* return : gtime_t struct (t+sec)
*-----------------------------------------------------------------------------*/
extern gtime_t gtime_timeadd(gtime_t t,float64_t sec)
{
    float64_t tt;

    t.sec+=sec; tt=floor(t.sec); t.time+=(int32_t)tt; t.sec-=tt;
    return t;
}
/* time difference -------------------------------------------------------------
* difference between gtime_t structs
* args   : gtime_t t1,t2    I   gtime_t structs
* return : time difference (t1-t2) (s)
*-----------------------------------------------------------------------------*/
extern float64_t gtime_timediff(gtime_t t1,gtime_t t2)
{
    return (t1.time-t2.time)+t1.sec-t2.sec;
}

/* convert calendar day/time to time -------------------------------------------
* convert calendar day/time to gtime_t struct
* args   : float64_t *ep       I   day/time {year,month,day,hour,min,sec}
* return : gtime_t struct
* notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
*-----------------------------------------------------------------------------*/
extern gtime_t gtime_epoch2time(const float64_t *ep)
{
    const int32_t doy[]={1,32,60,91,121,152,182,213,244,274,305,335};
    gtime_t time={0};
    int32_t days,sec,year=(int32_t)ep[0],mon=(int32_t)ep[1],day=(int32_t)ep[2];

    if(year<1970||2099<year||mon<1||12<mon) return time;

    /* leap year if year%4==0 in 1901-2099 */
    days=(year-1970)*365+(year-1969)/4+doy[mon-1]+day-2+(year%4==0&&mon>=3?1:0);
    sec=(int32_t)floor(ep[5]);
    time.time=(int64_t)days*86400+(int64_t)ep[3]*3600+(int64_t)ep[4]*60+sec;
    time.sec=ep[5]-sec;
    return time;
}

/* convert time to calendar day/time -------------------------------------------
* convert gtime_t struct to calendar day/time 
* args   : gtime_t t           I   gtime
*          float64_t *ep       O   day/time {year,month,day,hour,min,sec}
* return : none
*-----------------------------------------------------------------------------*/
extern void gtime_time2epoch(const gtime_t t,float64_t* ep)
{
	const int32_t mday[] = { /* # of days in a month */
		31,28,31,30,31,30,31,31,30,31,30,31,31,28,31,30,31,30,31,31,30,31,30,31,
		31,29,31,30,31,30,31,31,30,31,30,31,31,28,31,30,31,30,31,31,30,31,30,31
	};
	int32_t days, sec, mon, day;

	/* leap year if year%4==0 in 1901-2099 */
	days = (int32_t)(t.time / 86400);
	sec = (int32_t)(t.time - (int64_t)days * 86400);
	for (day = days % 1461, mon = 0; mon<48; mon++) {
		if (day >= mday[mon]) day -= mday[mon]; else break;
	}
	ep[0] = 1970 + (float64_t)(days / 1461) * 4 + (float64_t)(mon / 12);
	ep[1] = (float64_t)(mon % 12) + 1; 
	ep[2] = (float64_t)day + 1;
	ep[3] = sec / 3600; 
	ep[4] = sec % 3600 / 60; 
	ep[5] = sec % 60 + t.sec;
}

/* get time struct with gps time ----------------------------------------------
* convert week and tow in gps time to gtime_t struct
* args   : int32_t    week  I   week number in gps time
*          float64_t  sec   I   time of week in gps time (s)
* return : gtime_t struct
*-----------------------------------------------------------------------------*/
extern gtime_t gtime_gpst2time(int32_t week,float64_t sec){
    gtime_t t=gtime_epoch2time(gpst0);

    if(sec<-1E9||1E9<sec) sec=0.0;
    t.time+=(int64_t)86400*7*week+(int32_t)sec;
    t.sec=sec-(int32_t)sec;
    return t;
}
/* time to gps time ------------------------------------------------------------
* convert gtime_t struct to week and tow in gps time
* args   : gtime_t t        I   gtime_t struct
*          int32_t  *week   IO  week number in gps time (NULL: no output)
* return : time of week in gps time (s)
*-----------------------------------------------------------------------------*/
extern float64_t gtime_time2gpst(gtime_t t,int32_t *week){
    gtime_t t0=gtime_epoch2time(gpst0);
    int64_t sec=t.time-t0.time;
    int32_t w=(int32_t)(sec/(86400*7));

    if(week) *week=w;
    return (float64_t)(sec-(float64_t)w*86400*7)+t.sec;
}

/* gpstime to utc --------------------------------------------------------------
* convert gpstime to utc considering leap seconds
* args   : gtime_t t        I   time expressed in gpstime
* return : time expressed in utc
* notes  : ignore slight time offset under 100 ns
*-----------------------------------------------------------------------------*/
extern gtime_t gtime_gpst2utc(gtime_t t)
{
    gtime_t tu;
    int32_t i;

    for(i=0;leaps[i][0]>0;i++) {
        tu=gtime_timeadd(t,leaps[i][6]);
        if(gtime_timediff(tu,gtime_epoch2time(leaps[i]))>=0.0) return tu;
    }
		
		
    return t;
}
/* utc to gpstime --------------------------------------------------------------
* convert utc to gpstime considering leap seconds
* args   : gtime_t t        I   time expressed in utc
* return : time expressed in gpstime
* notes  : ignore slight time offset under 100 ns
*-----------------------------------------------------------------------------*/
extern gtime_t gtime_utc2gpst(gtime_t t)
{
    int32_t i;

    for(i=0;leaps[i][0]>0;i++) {
        if(gtime_timediff(t,gtime_epoch2time(leaps[i]))>=0.0) return gtime_timeadd(t,-leaps[i][6]);
    }
    return t;
}