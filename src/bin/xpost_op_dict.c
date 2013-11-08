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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#include "xpost_memory.h"
#include "xpost_object.h"
#include "xpost_stack.h"
#include "xpost_save.h"
#include "xpost_context.h"
#include "xpost_interpreter.h"
#include "xpost_error.h"
#include "xpost_string.h"
#include "xpost_array.h"
#include "xpost_dict.h"
#include "xpost_name.h"
#include "xpost_operator.h"
#include "xpost_op_stack.h"
#include "xpost_op_dict.h"

int DEBUGLOAD = 0;
void Awhere(Xpost_Context *ctx, Xpost_Object K); /* forward decl */

/* int  dict  dict
   create dictionary with capacity for int elements */
static
void Idict(Xpost_Context *ctx,
           Xpost_Object I)
{
    xpost_stack_push(ctx->lo, ctx->os, xpost_object_cvlit(consbdc(ctx, I.int_.val)));
}

/* -  <<  mark
   start dictionary construction */

/* mark k_1 v_1 ... k_N v_N  >>  dict
   construct dictionary from pairs on stack */
static
void dictomark(Xpost_Context *ctx)
{
    int i;
    Xpost_Object d, k, v;
    Zcounttomark(ctx);
    i = xpost_stack_pop(ctx->lo, ctx->os).int_.val;
    d = consbdc(ctx, i);
    for ( ; i > 0; i -= 2){
        v = xpost_stack_pop(ctx->lo, ctx->os);
        k = xpost_stack_pop(ctx->lo, ctx->os);
        bdcput(ctx, d, k, v);
    }
    (void)xpost_stack_pop(ctx->lo, ctx->os); // pop mark
    xpost_stack_push(ctx->lo, ctx->os, d);
}

/* dict  length  int
   number of key-value pairs in dict */
static
void Dlength(Xpost_Context *ctx,
             Xpost_Object D)
{
    xpost_stack_push(ctx->lo, ctx->os, xpost_cons_int(diclength(
                    xpost_context_select_memory(ctx, D) /*D.tag&FBANK?ctx->gl:ctx->lo*/,
                    D)));
}

/* dict  maxlength  int
   capacity of dict */
static
void Dmaxlength(Xpost_Context *ctx,
                Xpost_Object D)
{
    xpost_stack_push(ctx->lo, ctx->os, xpost_cons_int(dicmaxlength(
                    xpost_context_select_memory(ctx, D) /*D.tag&FBANK?ctx->gl:ctx->lo*/,
                    D)));
}

/* dict  begin  -
   push dict on dict stack */
static
void Dbegin(Xpost_Context *ctx,
            Xpost_Object D)
{
    xpost_stack_push(ctx->lo, ctx->ds, D);
}

/* -  end  -
   pop dict stack */
static
void Zend(Xpost_Context *ctx)
{
    if (xpost_stack_count(ctx->lo, ctx->ds) <= 3)
        error(dictstackunderflow, "end");
    (void)xpost_stack_pop(ctx->lo, ctx->ds);
}

/* key value  def  -
   associate key with value in current dict */
static
void Adef(Xpost_Context *ctx,
          Xpost_Object K,
          Xpost_Object V)
{
    //Xpost_Object D = xpost_stack_topdown_fetch(ctx->lo, ctx->ds, 0);
    //dumpdic(xpost_context_select_memory(ctx, D), D); puts("");
    bdcput(ctx, xpost_stack_topdown_fetch(ctx->lo, ctx->ds, 0), K, V);
    //puts("!def!");
    //dumpdic(xpost_context_select_memory(ctx, D), D); puts("");
}

/* key  load  value
   search dict stack for key and return associated value */
void Aload(Xpost_Context *ctx,
           Xpost_Object K)
{
    int i;
    int z = xpost_stack_count(ctx->lo, ctx->ds);
    if (DEBUGLOAD) {
        printf("\nload:");
        xpost_object_dump(K);
        xpost_stack_dump(ctx->lo, ctx->ds);
    }

    for (i = 0; i < z; i++) {
        Xpost_Object D = xpost_stack_topdown_fetch(ctx->lo,ctx->ds,i);

    if (DEBUGLOAD) {
        dumpdic(xpost_context_select_memory(ctx, D), D);
        (void)puts("");
    }

        if (dicknown(ctx, xpost_context_select_memory(ctx, D), D, K)) {
            xpost_stack_push(ctx->lo, ctx->os, bdcget(ctx, D, K));
            return;
        }
    }

    if (DEBUGLOAD) {
        unsigned int names;
        xpost_memory_file_dump(ctx->lo);
        xpost_memory_table_dump(ctx->lo);
        xpost_memory_file_dump(ctx->gl);
        xpost_memory_table_dump(ctx->gl);
        xpost_memory_table_get_addr(ctx->gl,
                XPOST_MEMORY_TABLE_SPECIAL_NAME_STACK, &names);
        xpost_stack_dump(ctx->gl, names);
        xpost_object_dump(K);
    }

    error(undefined, "Aload");
}

/* key value  store  -
   replace topmost definition of key */
static
void Astore(Xpost_Context *ctx,
            Xpost_Object K,
            Xpost_Object V)
{
    Xpost_Object D;
    Awhere(ctx, K);
    if (xpost_stack_pop(ctx->lo, ctx->os).int_.val) {
        D = xpost_stack_pop(ctx->lo, ctx->os);
    } else {
        D = xpost_stack_topdown_fetch(ctx->lo, ctx->ds, 0);
    }
    bdcput(ctx, D, K, V);
}

/* dict key  get  any
   get value associated with key in dict */
static
void DAget(Xpost_Context *ctx,
           Xpost_Object D,
           Xpost_Object K)
{
    xpost_stack_push(ctx->lo, ctx->os, bdcget(ctx, D, K));
}

/* dict key value  put  -
   associate key with value in dict */
static
void DAAput(Xpost_Context *ctx,
            Xpost_Object D,
            Xpost_Object K,
            Xpost_Object V)
{
    bdcput(ctx, D, K, V);
}

/* dict key  undef  -
   remove key and its value in dict */
static
void DAundef(Xpost_Context *ctx,
             Xpost_Object D,
             Xpost_Object K)
{
    bdcundef(ctx, D, K);
}

/* dict key  known  bool
   test whether key is in dict */
static
void DAknown(Xpost_Context *ctx,
             Xpost_Object D,
             Xpost_Object K)
{
#if 0
    printf("\nknown: ");
    xpost_object_dump(D);
    dumpdic(xpost_context_select_memory(ctx, D), D); puts("");
    xpost_object_dump(K);
#endif
    xpost_stack_push(ctx->lo, ctx->os, xpost_cons_bool(dicknown(ctx, xpost_context_select_memory(ctx, D), D, K)));
}


/* key  where  dict true -or- false
   find dict in which key is defined */
void Awhere(Xpost_Context *ctx,
            Xpost_Object K)
{
    int i;
    int z = xpost_stack_count(ctx->lo, ctx->ds);
    for (i = 0; i < z; i++) {
        Xpost_Object D = xpost_stack_topdown_fetch(ctx->lo, ctx->ds, i);
        if (dicknown(ctx, xpost_context_select_memory(ctx, D), D, K)) {
            xpost_stack_push(ctx->lo, ctx->os, D);
            xpost_stack_push(ctx->lo, ctx->os, xpost_cons_bool(1));
            return;
        }
    }
    xpost_stack_push(ctx->lo, ctx->os, xpost_cons_bool(0));
}

/* dict1 dict2  copy  dict2
   copy contents of dict1 to dict2 */
static
void Dcopy(Xpost_Context *ctx,
           Xpost_Object S,
           Xpost_Object D)
{
    int i, sz;
    Xpost_Memory_File *mem;
    unsigned ad;
    Xpost_Object *tp;
    mem = xpost_context_select_memory(ctx, S);
    sz = dicmaxlength(mem, S);
    xpost_memory_table_get_addr(mem, S.comp_.ent, &ad);
    tp = (void *)(mem->base + ad + sizeof(dichead));
    for (i=0; i < sz+1; i++) {
        if (xpost_object_get_type(tp[2 * i]) != nulltype) {
            bdcput(ctx, D, tp[2*i], tp[2*i+1]);
        }
    }
    xpost_stack_push(ctx->lo, ctx->os, D);
}

static
void DPforall (Xpost_Context *ctx,
               Xpost_Object D,
               Xpost_Object P)
{
    Xpost_Memory_File *mem = xpost_context_select_memory(ctx, D);
    assert(mem->base);
    D.comp_.sz = dicmaxlength(mem, D); // stash size locally
    if (D.comp_.off <= D.comp_.sz) { // not finished?
        unsigned ad;
        Xpost_Object *tp;
        xpost_memory_table_get_addr(mem, D.comp_.ent, &ad);
        tp = (void *)(mem->base + ad + sizeof(dichead)); 

        for ( ; D.comp_.off <= D.comp_.sz; ++D.comp_.off) { // find next pair
            if (xpost_object_get_type(tp[2 * D.comp_.off]) != nulltype) { // found
                Xpost_Object k,v;

                k = tp[2 * D.comp_.off];
                if (xpost_object_get_type(k) == extendedtype)
                    k = unextend(k);
                v = tp[2 * D.comp_.off + 1];
                xpost_stack_push(ctx->lo, ctx->os, k);
                xpost_stack_push(ctx->lo, ctx->os, v);

                //xpost_stack_push(ctx->lo, ctx->es, consoper(ctx, "forall", NULL,0,0));
                xpost_stack_push(ctx->lo, ctx->es, operfromcode(ctx->opcode_shortcuts.forall));
                //xpost_stack_push(ctx->lo, ctx->es, consoper(ctx, "cvx", NULL,0,0));
                xpost_stack_push(ctx->lo, ctx->es, operfromcode(ctx->opcode_shortcuts.cvx));
                xpost_stack_push(ctx->lo, ctx->es, xpost_object_cvlit(P));
                ++D.comp_.off;
                xpost_stack_push(ctx->lo, ctx->es, D);

                xpost_stack_push(ctx->lo, ctx->es, P);
                return;
            }
        }
    }
}

/* -  currentdict  dict
   push current dict on operand stack */
static
void Zcurrentdict(Xpost_Context *ctx)
{
    xpost_stack_push(ctx->lo, ctx->os, xpost_stack_topdown_fetch(ctx->lo, ctx->ds, 0));
}

/* -  errordict  dict   % error handler dictionary : err.ps
   -  $error  dict      % error control and status dictionary : err.ps
   -  systemdict  dict  % system dictionary : op.c init.ps
   -  userdict  dict    % writeable dictionary in local VM : itp.c
   %-  globaldict  dict  % writeable dictionary in global VM
   %-  statusdict  dict  % product-dependent dictionary
   */

/* -  countdictstack  int
   count elements on dict stack */
static
void Zcountdictstack(Xpost_Context *ctx)
{
    xpost_stack_push(ctx->lo, ctx->os, xpost_cons_int(xpost_stack_count(ctx->lo, ctx->ds)));
}

/* array  dictstack  subarray
   copy dict stack into array */
static
void Adictstack(Xpost_Context *ctx,
                Xpost_Object A)
{
    int z = xpost_stack_count(ctx->lo, ctx->ds);
    int i;
    for (i=0; i < z; i++)
        barput(ctx, A, i, xpost_stack_bottomup_fetch(ctx->lo, ctx->ds, i));
    xpost_stack_push(ctx->lo, ctx->os, arrgetinterval(A, 0, z));
}

static
void cleardictstack(Xpost_Context *ctx)
{
    int z = xpost_stack_count(ctx->lo, ctx->ds);
    while (z-- > 3) {
        (void)xpost_stack_pop(ctx->lo, ctx->ds);
    }
}

void initopdi(Xpost_Context *ctx,
              Xpost_Object sd)
{
    oper *optab;
    Xpost_Object n,op;
    unsigned int optadr;

    assert(ctx->gl->base);
    xpost_memory_table_get_addr(ctx->gl,
            XPOST_MEMORY_TABLE_SPECIAL_OPERATOR_TABLE, &optadr);
    optab = (void *)(ctx->gl->base + optadr);
    op = consoper(ctx, "dict", Idict, 1, 1, integertype); INSTALL;
    bdcput(ctx, sd, consname(ctx, "<<"), mark);
    op = consoper(ctx, ">>", dictomark, 1, 0); INSTALL;
    op = consoper(ctx, "length", Dlength, 1, 1, dicttype); INSTALL;
    op = consoper(ctx, "maxlength", Dmaxlength, 1, 1, dicttype); INSTALL;
    op = consoper(ctx, "begin", Dbegin, 0, 1, dicttype); INSTALL;
    op = consoper(ctx, "end", Zend, 0, 0); INSTALL;
    op = consoper(ctx, "def", Adef, 0, 2, anytype, anytype); INSTALL;
    op = consoper(ctx, "load", Aload, 1, 1, anytype); INSTALL;
    ctx->opcode_shortcuts.load = op.mark_.padw;
    op = consoper(ctx, "store", Astore, 0, 2, anytype, anytype); INSTALL;
    op = consoper(ctx, "get", DAget, 1, 2, dicttype, anytype); INSTALL;
    op = consoper(ctx, "put", DAAput, 1, 3,
            dicttype, anytype, anytype); INSTALL;
    op = consoper(ctx, "undef", DAundef, 0, 2, dicttype, anytype); INSTALL;
    op = consoper(ctx, "known", DAknown, 1, 2, dicttype, anytype); INSTALL;
    op = consoper(ctx, "where", Awhere, 2, 1, anytype); INSTALL;
    op = consoper(ctx, "copy", Dcopy, 1, 2, dicttype, dicttype); INSTALL;
    op = consoper(ctx, "forall", DPforall, 0, 2, dicttype, proctype); INSTALL;
    ctx->opcode_shortcuts.forall = op.mark_.padw;
    op = consoper(ctx, "currentdict", Zcurrentdict, 1, 0); INSTALL;
    op = consoper(ctx, "countdictstack", Zcountdictstack, 1, 0); INSTALL;
    op = consoper(ctx, "dictstack", Adictstack, 1, 1, arraytype); INSTALL;
    op = consoper(ctx, "cleardictstack", cleardictstack, 0, 0); INSTALL;
}

