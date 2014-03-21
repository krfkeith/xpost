/*
 * Xpost - a Level-2 Postscript interpreter
 * Copyright (C) 2013, Michael Joshua Ryan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the Xpost software product nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* relational, boolean, and bitwise operators */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif !defined alloca
# ifdef __GNUC__
#  define alloca __builtin_alloca
# elif defined _AIX
#  define alloca __alloca
# elif defined _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# elif !defined HAVE_ALLOCA
#  ifdef  __cplusplus
extern "C"
#  endif
void *alloca (size_t);
# endif
#endif

#include <assert.h>
#include <stdlib.h> /* NULL */

#include "xpost_memory.h"
#include "xpost_object.h"
#include "xpost_stack.h"

#include "xpost_context.h"
#include "xpost_interpreter.h"
#include "xpost_name.h"
#include "xpost_dict.h"
#include "xpost_operator.h"
#include "xpost_op_boolean.h"

/* any1 any2  eq  bool
   test equal */
static
int Aeq (Xpost_Context *ctx,
          Xpost_Object x,
          Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(objcmp(ctx,x,y) == 0));
    return 0;
}

/* any1 any2  ne  bool
   test not equal */
static
int Ane (Xpost_Context *ctx,
          Xpost_Object x,
          Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(objcmp(ctx,x,y) != 0));
    return 0;
}

/* any1 any2  ge  bool
   test greater or equal */
static
int Age (Xpost_Context *ctx,
          Xpost_Object x,
          Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(objcmp(ctx,x,y) >= 0));
    return 0;
}

/* any1 any2  gt  bool
   test greater than */
static
int Agt (Xpost_Context *ctx,
          Xpost_Object x,
          Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(objcmp(ctx,x,y) > 0));
    return 0;
}

/* any1 any2  le  bool
   test less or equal */
static
int Ale (Xpost_Context *ctx,
          Xpost_Object x,
          Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(objcmp(ctx,x,y) <= 0));
    return 0;
}

/* any1 any2  lt  bool
   test less than */
static
int Alt (Xpost_Context *ctx,
          Xpost_Object x,
          Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(objcmp(ctx,x,y) < 0));
    return 0;
}

/* bool1|int1 bool2|int2  and  bool3|int3
   logical|bitwise and */
static
int Band (Xpost_Context *ctx,
           Xpost_Object x,
           Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(x.int_.val & y.int_.val));
    return 0;
}

static
int Iand (Xpost_Context *ctx,
           Xpost_Object x,
           Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_int_cons(x.int_.val & y.int_.val));
    return 0;
}

/* bool1|int1  not  bool2|int2
   logical|bitwise not */
static
int Bnot (Xpost_Context *ctx,
           Xpost_Object x)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons( ! x.int_.val ));
    return 0;
}

static
int Inot (Xpost_Context *ctx,
           Xpost_Object x)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_int_cons( ~ x.int_.val ));
    return 0;
}

/* bool1|int1 bool2|int2  or  bool3|int3
   logical|bitwise inclusive or */
static
int Bor (Xpost_Context *ctx,
          Xpost_Object x,
          Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(x.int_.val | y.int_.val));
    return 0;
}

static
int Ior (Xpost_Context *ctx,
          Xpost_Object x,
          Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_int_cons(x.int_.val | y.int_.val));
    return 0;
}

/* bool1|int1 bool2|int2  xor  bool3|int3
   exclusive or */
static
int Bxor (Xpost_Context *ctx,
           Xpost_Object x,
           Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_bool_cons(x.int_.val ^ y.int_.val));
    return 0;
}

static
int Ixor (Xpost_Context *ctx,
           Xpost_Object x,
           Xpost_Object y)
{
    xpost_stack_push(ctx->lo, ctx->os,
            xpost_int_cons(x.int_.val ^ y.int_.val));
    return 0;
}

/* true */
/* false */
/* defined as the booleantype object directly */


/* int1 shift  bitshift  int2
   bitwise shift of int1 (positive is left) */
static
int Ibitshift (Xpost_Context *ctx,
                Xpost_Object x,
                Xpost_Object y)
{
    if (y.int_.val >= 0)
        xpost_stack_push(ctx->lo, ctx->os,
                xpost_int_cons(x.int_.val << y.int_.val));
    else
        xpost_stack_push(ctx->lo, ctx->os,
                xpost_int_cons( (unsigned long)x.int_.val >> -y.int_.val));
    return 0;
}

int initopb(Xpost_Context *ctx,
             Xpost_Object sd)
{
    oper *optab;
    Xpost_Object n,op;
    unsigned int optadr;
    int ret;

    assert(ctx->gl->base);
    xpost_memory_table_get_addr(ctx->gl,
            XPOST_MEMORY_TABLE_SPECIAL_OPERATOR_TABLE, &optadr);
    optab = (void *)(ctx->gl->base + optadr);

    op = consoper(ctx, "eq", Aeq, 1, 2, anytype, anytype); INSTALL;
    op = consoper(ctx, "ne", Ane, 1, 2, anytype, anytype); INSTALL;
    op = consoper(ctx, "ge", Age, 1, 2, anytype, anytype); INSTALL;
    op = consoper(ctx, "gt", Agt, 1, 2, anytype, anytype); INSTALL;
    op = consoper(ctx, "le", Ale, 1, 2, anytype, anytype); INSTALL;
    op = consoper(ctx, "lt", Alt, 1, 2, anytype, anytype); INSTALL;
    op = consoper(ctx, "and", Band, 1, 2, booleantype, booleantype); INSTALL;
    op = consoper(ctx, "and", Iand, 1, 2, integertype, integertype); INSTALL;
    op = consoper(ctx, "not", Bnot, 1, 1, booleantype); INSTALL;
    op = consoper(ctx, "not", Inot, 1, 1, integertype); INSTALL;
    op = consoper(ctx, "or", Bor, 1, 2, booleantype, booleantype); INSTALL;
    op = consoper(ctx, "or", Ior, 1, 2, integertype, integertype); INSTALL;
    op = consoper(ctx, "xor", Bxor, 1, 2, booleantype, booleantype); INSTALL;
    op = consoper(ctx, "xor", Ixor, 1, 2, integertype, integertype); INSTALL;
    ret = xpost_dict_put(ctx, sd, xpost_name_cons(ctx, "true"), xpost_bool_cons(1));
    if (ret)
        return 0;
    ret = xpost_dict_put(ctx, sd, xpost_name_cons(ctx, "false"), xpost_bool_cons(0));
    if (ret)
        return 0;
    op = consoper(ctx, "bitshift", Ibitshift, 1, 2, integertype, integertype); INSTALL;

    /* xpost_dict_dump_memory (ctx->gl, sd); fflush(NULL); */

    return 1;
}
