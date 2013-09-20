
#include <assert.h>
#include <stdbool.h>

#include "m.h"
#include "ob.h"
#include "s.h"
#include "gc.h"
#include "v.h"
#include "itp.h"
#include "err.h"
#include "ar.h"

/* Allocate an entity with gballoc,
   find the appropriate mtab,
   set the current save level in the "mark" field,
   wrap it up in an object. */
object consarr(mfile *mem, unsigned sz) {
    unsigned ent;
    unsigned rent;
    unsigned cnt;
    mtab *tab;
    object o;
    unsigned i;
    assert(mem->base);

    //unsigned ent = mtalloc(mem, 0, sz * sizeof(object));
    if (sz == 0) {
        ent = 0;
    } else {
        ent = gballoc(mem, (unsigned)(sz * sizeof(object)));
        tab = (void *)(mem->base);
        rent = ent;
        findtabent(mem, &tab, &rent);
        cnt = count(mem, adrent(mem, VS));
        tab->tab[rent].mark = ( (0 << MARKO) | (0 << RFCTO) |
                (cnt << LLEVO) | (cnt << TLEVO) );

        for (i = 0; i < sz; i++)
            put(mem, ent, i, (unsigned)sizeof(object), &null);
    }

    //return (object){ .comp_.tag = arraytype, .comp_.sz = sz, .comp_.ent = ent, .comp_.off = 0};
    o.tag = arraytype | (unlimited << FACCESSO);
    o.comp_.sz = (word)sz;
    o.comp_.ent = (word)ent;
    o.comp_.off = 0;
    return o;
} 

/* Select a memory file according to vmmode,
   call consarr,
   set BANK flag. */
object consbar(context *ctx, unsigned sz) {
    object a = consarr(ctx->vmmode==GLOBAL?
            ctx->gl: ctx->lo, sz);
    if (ctx->vmmode==GLOBAL)
        a.tag |= FBANK;
    return a;
}

/* Copy if necessary,
   call put. */
void arrput(mfile *mem, object a, integer i, object o) {
    if (!stashed(mem, a.comp_.ent)) stash(mem, a.comp_.ent);
    if (i > a.comp_.sz)
        error(rangecheck, "arrput");
    put(mem, a.comp_.ent, (unsigned)(a.comp_.off + i), (unsigned)sizeof(object), &o);
}

/* Select mfile according to BANK flag,
   call arrput. */
void barput(context *ctx, object a, integer i, object o) {
    arrput(/*bank(ctx, a)*/ a.tag&FBANK? ctx->gl: ctx->lo, a, i, o);
}

/* call get. */
object arrget(mfile *mem, object a, integer i) {
    object o;
    get(mem, a.comp_.ent, (unsigned)(a.comp_.off +i), (unsigned)(sizeof(object)), &o);
    return o;
}

/* Select mfile according to BANK flag,
   call arrget. */
object barget(context *ctx, object a, integer i) {
    return arrget(bank(ctx, a) /*a.tag&FBANK? ctx->gl: ctx->lo*/, a, i);
}

/* adjust the offset and size fields in the object. */
object arrgetinterval(object a, integer off, integer sz) {
    if (sz - off > a.comp_.sz) error(rangecheck, "getinterval can only shrink!");
    a.comp_.off += off;
    a.comp_.sz = sz;
    return a;
}

#ifdef TESTMODULE_AR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

context *ctx;
mfile *mem;

int main(void) {
    itpdata = malloc(sizeof*itpdata);
    memset(itpdata, 0, sizeof*itpdata);
    inititp(itpdata);
    ctx = &itpdata->ctab[0];
    mem = ctx->lo;
    //initmem(&mem, "x.mem");
    //(void)initmtab(&mem);
    //initfree(&mem);
    //initsave(&mem);

    enum { SIZE = 10 };
    printf("\n^test ar.c\n");
    printf("allocating array occupying %zu bytes\n", SIZE*sizeof(object));
    object a = consarr(mem, SIZE);

    //printf("the memory table:\n"); dumpmtab(mem, 0);

    printf("test array by filling\n");
    int i;
    for (i=0; i < SIZE; i++) {
        printf("%d ", i+1);
        arrput(mem, a, i, consint( i+1 ));
    }
    puts("");

    printf("and accessing.\n");
    for (i=0; i < SIZE; i++) {
        object t;
        t = arrget(mem, a, i);
        printf("%d: %d\n", i, t.int_.val);
    }

    printf("the memory table:\n");
    dumpmtab(mem, 0);

    return 0;
}

#endif
