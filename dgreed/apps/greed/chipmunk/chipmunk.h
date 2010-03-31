/* Copyright (c) 2007 Scott Lembcke
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CHIPMUNK_HEADER
#define CHIPMUNK_HEADER

#ifdef __cplusplus
extern "C" {
#endif
	
typedef double cpFloat;
	
static inline cpFloat
cpfmax(cpFloat a, cpFloat b)
{
	return (a > b) ? a : b;
}

static inline cpFloat
cpfmin(cpFloat a, cpFloat b)
{
	return (a < b) ? a : b;
}

static inline cpFloat
cpfclamp(cpFloat f, cpFloat min, cpFloat max){
	return cpfmin(cpfmax(f, min), max);
}

#ifndef INFINITY
	#ifdef _MSC_VER
		union MSVC_EVIL_FLOAT_HACK
		{
			unsigned __int8 Bytes[4];
			float Value;
		};
		static union MSVC_EVIL_FLOAT_HACK INFINITY_HACK = {{0x00, 0x00, 0x80, 0x7F}};
		#define INFINITY (INFINITY_HACK.Value)
	#else
		#define INFINITY (1e1000)
	#endif
#endif

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#ifdef _DEBUG

#include <stdlib.h>

extern void* mem_alloc(size_t size, const char* file, int line);
extern void* mem_calloc(size_t num, size_t size, const char* file, int line);
extern void* mem_realloc(void* p, size_t size, const char* file, int line);
extern void mem_free(void* ptr);

#define cpmalloc(size) mem_alloc((size), "chipmunk", 0) 
#define cpcalloc(num, size) mem_calloc((num), (size), "chipmunk", 0)
#define cprealloc(p, size) mem_realloc((p), (size), "chipmunk", 0)
#define cpfree(p) mem_free((p))

#else

#define cpmalloc malloc
#define cpcalloc calloc
#define cprealloc realloc
#define cpfree free

#endif

#include "cpVect.h"
#include "cpBB.h"
#include "cpBody.h"
#include "cpArray.h"
#include "cpHashSet.h"
#include "cpSpaceHash.h"

#include "cpShape.h"
#include "cpPolyShape.h"

#include "cpArbiter.h"
#include "cpCollision.h"
	
#include "cpJoint.h"

#include "cpSpace.h"

#define CP_HASH_COEF (3344921057ul)
#define CP_HASH_PAIR(A, B) ((size_t)(A)*CP_HASH_COEF ^ (size_t)(B)*CP_HASH_COEF)

void cpInitChipmunk(void);

cpFloat cpMomentForCircle(cpFloat m, cpFloat r1, cpFloat r2, cpVect offset);
cpFloat cpMomentForPoly(cpFloat m, int numVerts, cpVect *verts, cpVect offset);

#ifdef __cplusplus
}
#endif

#endif
