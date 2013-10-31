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
#include <string.h>

#include "xpost_memory.h"  // save/restore works with mtabs
#include "xpost_object.h"  // save/restore examines objects
#include "xpost_stack.h"  // save/restore manipulates (internal) stacks
#include "xpost_interpreter.h"  //
#include "xpost_garbage.h"  //
#include "xpost_save.h"  // double-check prototypes

/*
typedef struct {
    word tag;
    word lev;
    unsigned stk;
} save_;

typedef struct {
    word tag;
    word pad;
    word src;
    word cpy;
} saverec_;
*/

/* create a stack in slot VS.
   sz is 0 so gc will ignore it. */
void initsave (mfile *mem)
{
    unsigned t;
    unsigned ent;
    mtab *tab;

    ent = mtalloc(mem, 0, 0, 0); /* allocate an entry of zero length */
    assert(ent == VS);

    t = initstack(mem);
    tab = (void *)mem->base;
    tab->tab[ent].adr = t;
}

/* push a new save object on the save stack
   this object is itself a stack (contains a stackadr) */
Xpost_Object save (mfile *mem)
{
    Xpost_Object v;
    v.tag = savetype;
    v.save_.lev = count(mem, adrent(mem, VS));
    v.save_.stk = initstack(mem);
    push(mem, adrent(mem, VS), v);
    return v;
}

/* check ent's tlev against current save level (save-stack count) */
unsigned stashed (mfile *mem,
                  unsigned ent)
{
    //object sav = top(mem, adrent(mem, VS), 0);
    mtab *tab;
    unsigned cnt;
    unsigned tlev;

    cnt = count(mem, adrent(mem, VS));
    findtabent(mem, &tab, &ent);
    tlev = (tab->tab[ent].mark & TLEVM) >> TLEVO;

    return tlev == cnt;
}

/* make a clone of ent, return new ent */
unsigned copy(mfile *mem,
              unsigned ent)
{
    mtab *tab;
    unsigned new;
    unsigned tent = ent;

    findtabent(mem, &tab, &ent);
    new = gballoc(mem, tab->tab[ent].sz, tab->tab[ent].tag);
    ent = tent;
    findtabent(mem, &tab, &ent); //recalc
    memcpy(mem->base + adrent(mem, new),
            mem->base + tab->tab[ent].adr,
            tab->tab[ent].sz);

    return new;
}

/* set tlev for ent to current save level
   push saverec relating ent to saved copy */
void stash(mfile *mem,
           unsigned tag,
           unsigned pad,
           unsigned ent)
{
    mtab *tab;
    Xpost_Object o;
    unsigned tlev;
    unsigned rent = ent;
    Xpost_Object sav = top(mem, adrent(mem, VS), 0);

    findtabent(mem, &tab, &rent);
    tlev = sav.save_.lev;
    tab->tab[rent].mark &= ~TLEVM; // clear TLEV field
    tab->tab[rent].mark |= (tlev << TLEVO);  // set TLEV field

    o.saverec_.tag = tag;
    o.saverec_.pad = pad;
    o.saverec_.src = ent;
    o.saverec_.cpy = copy(mem, ent);
    push(mem, sav.save_.stk, o);
}

/* for each saverec from current save stack
        exchange adrs between src and cpy
        pop saverec
    pop save stack */
void restore(mfile *mem)
{
    unsigned v;
    Xpost_Object sav;
    mtab *stab, *ctab;
    unsigned cnt;
    unsigned sent, cent;

    v = adrent(mem, VS); // save-stack address
    sav = pop(mem, v); // save-object (stack of saverec_'s)
    cnt = count(mem, sav.save_.stk);
    while (cnt--) {
        Xpost_Object rec;
        unsigned hold;
        rec = pop(mem, sav.save_.stk);
        sent = rec.saverec_.src;
        cent = rec.saverec_.cpy;
        findtabent(mem, &stab, &sent);
        findtabent(mem, &ctab, &cent);
        hold = stab->tab[sent].adr;                 // tmp = src
        stab->tab[sent].adr = ctab->tab[cent].adr;  // src = cpy
        ctab->tab[cent].adr = hold;                 // cpy = tmp
    }
    sfree(mem, sav.save_.stk);
}

#ifdef TESTMODULE_V
#include "xpost_array.h"
#include <stdio.h>

mfile mf;

void init (mfile *mem)
{
    initmem(mem, "x.mem");
    (void)initmtab(mem);
    initfree(mem);
    initsave(mem);
}

void show (char *msg, mfile *mem, Xpost_Object a)
{
    printf("%s ", msg);
    printf("%d ", arrget(mem, a, 0).int_.val);
    printf("%d\n", arrget(mem, a, 1).int_.val);
}

int main (void)
{
    mfile *mem = &mf;
    Xpost_Object a;
    printf("\n^test v\n");
    init(mem);

    a = consarr(mem, 2);
    arrput(mem, a, 0, xpost_cons_int(33));
    arrput(mem, a, 1, xpost_cons_int(66));
    show("initial", mem, a);

    //object v = 
    (void)save(mem);
    arrput(mem, a, 0, xpost_cons_int(77));
    show("save and alter", mem, a);

    restore(mem);
    show("restored", mem, a);

    puts("");
    return 0;
}

#endif


