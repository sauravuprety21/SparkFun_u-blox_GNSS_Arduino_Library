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
#ifndef GTIME_H
#define GTIME_H
#include "constant.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
    int64_t  time;   /* time in whole second */
    float64_t sec;  /* fractional part */
}gtime_t;


/* function delaration */
extern gtime_t gtime_epoch2time(const float64_t *ep);
extern void gtime_time2epoch(const gtime_t t, float64_t* ep);
extern gtime_t gtime_gpst2time(int32_t week,float64_t sec);
extern float64_t gtime_time2gpst(gtime_t t,int32_t *week);
extern gtime_t gtime_timeadd(gtime_t t,float64_t sec);
extern float64_t gtime_timediff(gtime_t t1,gtime_t t2);
extern gtime_t gtime_gpst2utc(gtime_t t);
extern gtime_t gtime_utc2gpst(gtime_t t);

#ifdef __cplusplus
}
#endif
#endif /* GTIME_H */