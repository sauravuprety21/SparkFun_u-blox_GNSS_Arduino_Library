/*------------------------------------------------------------------------------
* constant.h : P3 constant definition and compile configuration
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
#ifndef CONSTANT_H
#define CONSTANT_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

/* type redefinition */
/* note: pleas use int32_t and uint32_t to replace int and uint in x86 and x64 
*  CPU(PC and ARM) */
typedef float               float32_t;
typedef double              float64_t;



#ifdef __cplusplus
}
#endif
#endif /* CONSTANT_H */