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

#include <stdio.h>

#include "xpost.h"
#include "xpost_log.h"
#include "xpost_object.h"

/**
 * @file xpost_object.c
 * @brief Simple object constructors and functions.
*/

/**
 * @cond LOCAL
 */

#ifdef WANT_LARGE_OBJECT
# define XPOST_FMT_WORD(_)    "u"
# define XPOST_FMT_DWORD(_)   "lu"
# define XPOST_FMT_INTEGER(_) "ld"
# define XPOST_FMT_REAL       "f"
#else
# define XPOST_FMT_WORD(_)    "u"
# define XPOST_FMT_DWORD(_)   "u"
# define XPOST_FMT_INTEGER(_) "d"
# define XPOST_FMT_REAL       "f"
#endif
# define XPOST_FMT_ADDR       XPOST_FMT_DWORD

/**
 * @endcond
 */

/*
 *
 * Variables
 *
 */

/* null and mark are both global objects */
XPOST_OBJECT_SINGLETONS(XPOST_OBJECT_DEFINE_SINGLETON)

/**
 * @var xpost_object_type_names
 * @brief Printable strings corresponding to #Xpost_Object_Type.
*/
char *xpost_object_type_names[] =
{
    XPOST_OBJECT_TYPES(XPOST_OBJECT_AS_TYPE_STR)
    "invalid"
};


/*
   Constructors for simple types
*/

Xpost_Object xpost_bool_cons (int b)
{
    Xpost_Object obj;

    obj.int_.tag = booleantype
        | (XPOST_OBJECT_TAG_ACCESS_UNLIMITED
            << XPOST_OBJECT_TAG_DATA_FLAG_ACCESS_OFFSET);
    obj.int_.pad = 0;
    obj.int_.val = !!b;

    return xpost_object_cvlit(obj);
}

Xpost_Object xpost_int_cons (integer i)
{
    Xpost_Object obj;

    obj.int_.tag = integertype
        | (XPOST_OBJECT_TAG_ACCESS_UNLIMITED
                << XPOST_OBJECT_TAG_DATA_FLAG_ACCESS_OFFSET);
    obj.int_.pad = 0;
    obj.int_.val = i;

    return xpost_object_cvlit(obj);
}

Xpost_Object xpost_real_cons (real r)
{
    Xpost_Object obj;

    obj.real_.tag = realtype
        | (XPOST_OBJECT_TAG_ACCESS_UNLIMITED
                << XPOST_OBJECT_TAG_DATA_FLAG_ACCESS_OFFSET);
    obj.real_.pad = 0;
    obj.real_.val = r;

    return xpost_object_cvlit(obj);
}


/*
   Type and Tag Manipulation
*/

Xpost_Object_Type xpost_object_get_type (Xpost_Object obj)
{
    return (Xpost_Object_Type)(obj.tag & XPOST_OBJECT_TAG_DATA_TYPE_MASK);
}

int xpost_object_is_composite (Xpost_Object obj)
{
    switch (xpost_object_get_type(obj))
    {
        case stringtype: /*@fallthrough@*/
        case arraytype: /*@fallthrough@*/
        case dicttype:
            return 1;
        default: break;
    }
    return 0;
}

int xpost_object_get_ent(Xpost_Object obj)
{
    if (!xpost_object_is_composite(obj))
        return -1;
    return (unsigned int)obj.comp_.ent +
        ((obj.comp_.tag >> XPOST_OBJECT_TAG_DATA_EXTRA_BITS)
         << (8*sizeof(word)));
}

Xpost_Object xpost_object_set_ent(Xpost_Object obj,
                                  unsigned int ent)
{
    if (!xpost_object_is_composite(obj))
        return invalid;
    obj.comp_.ent = ent;
    obj.comp_.tag &= (1 << XPOST_OBJECT_TAG_DATA_EXTRA_BITS) - 1;
    obj.comp_.tag |= (ent >> (8*sizeof(word)))
        << XPOST_OBJECT_TAG_DATA_EXTRA_BITS;
    return obj;
}

/* adjust the offset and size fields in the object.
   NB. since this function only modifies the fields in the object
   itself, it works for array, string and dict objects which use
   the same comp_ substructure.  It does not touch VM.
 */
Xpost_Object xpost_object_get_interval(Xpost_Object a,
                      integer off,
                      integer sz)
{
    if (sz - off > a.comp_.sz)
        return invalid; /* should be interpreted as a rangecheck error */
    a.comp_.off += off;
    a.comp_.sz = sz;
    return a;
}

int xpost_object_is_exe(Xpost_Object obj)
{
    return !(obj.tag & XPOST_OBJECT_TAG_DATA_FLAG_LIT);
}

int xpost_object_is_lit(Xpost_Object obj)
{
    return !!(obj.tag & XPOST_OBJECT_TAG_DATA_FLAG_LIT);
}

static
Xpost_Object_Tag_Access (*xpost_object_dict_get_access)(Xpost_Context *, Xpost_Object) = NULL;

static
Xpost_Object_Tag_Access (*xpost_object_file_get_access)(Xpost_Context *, Xpost_Object) = NULL;

void xpost_object_install_dict_get_access(Xpost_Object_Tag_Access (*access_func)(Xpost_Context *, Xpost_Object)){
    xpost_object_dict_get_access = access_func;
}

void xpost_object_install_file_get_access(Xpost_Object_Tag_Access (*access_func)(Xpost_Context *, Xpost_Object)){
    xpost_object_file_get_access = access_func;
}

Xpost_Object_Tag_Access xpost_object_get_access (Xpost_Context *ctx, Xpost_Object obj)
{
    word type = xpost_object_get_type(obj);
    if (type == dicttype && xpost_object_dict_get_access)
        return xpost_object_dict_get_access(ctx, obj);
    else if (type == filetype && xpost_object_file_get_access)
        return xpost_object_file_get_access(ctx, obj);
    else
        return (Xpost_Object_Tag_Access)((obj.tag & XPOST_OBJECT_TAG_DATA_FLAG_ACCESS_MASK) >>
                                     XPOST_OBJECT_TAG_DATA_FLAG_ACCESS_OFFSET);
}

static
Xpost_Object (*xpost_object_dict_set_access)(Xpost_Context *, Xpost_Object, Xpost_Object_Tag_Access) = NULL;

static
Xpost_Object (*xpost_object_file_set_access)(Xpost_Context *, Xpost_Object, Xpost_Object_Tag_Access) = NULL;

void xpost_object_install_dict_set_access(Xpost_Object (*set_access_func)(Xpost_Context *, Xpost_Object, Xpost_Object_Tag_Access))
{
    xpost_object_dict_set_access = set_access_func;
}

void xpost_object_install_file_set_access(Xpost_Object (*set_access_func)(Xpost_Context *, Xpost_Object, Xpost_Object_Tag_Access))
{
    xpost_object_file_set_access = set_access_func;
}

Xpost_Object xpost_object_set_access (Xpost_Context *ctx, Xpost_Object obj, Xpost_Object_Tag_Access access)
{
    word type = xpost_object_get_type(obj);
    if (type == dicttype && xpost_object_dict_set_access)
        return xpost_object_dict_set_access(ctx, obj, access);
    else if (type == filetype && xpost_object_file_set_access)
        return xpost_object_file_set_access(ctx, obj, access);
    else {
        obj.tag &= ~XPOST_OBJECT_TAG_DATA_FLAG_ACCESS_MASK;
        obj.tag |= (access << XPOST_OBJECT_TAG_DATA_FLAG_ACCESS_OFFSET);
        return obj;
    }
}

int xpost_object_is_readable(Xpost_Context *ctx, Xpost_Object obj)
{
    if (xpost_object_get_type(obj) == filetype)
    {
        return xpost_object_get_access(ctx, obj)
            & XPOST_OBJECT_TAG_ACCESS_FILE_READ;
    }
    else
    {
        return xpost_object_get_access(ctx, obj)
            >= XPOST_OBJECT_TAG_ACCESS_READ_ONLY;
    }
}

int xpost_object_is_writeable (Xpost_Context *ctx, Xpost_Object obj)
{
    if (xpost_object_get_type(obj) == filetype)
    {
        return xpost_object_get_access(ctx, obj)
            & XPOST_OBJECT_TAG_ACCESS_FILE_WRITE;
    }
    else
    {
        return xpost_object_get_access(ctx, obj)
            == XPOST_OBJECT_TAG_ACCESS_UNLIMITED;
    }
}

Xpost_Object xpost_object_cvx (Xpost_Object obj)
{
    obj.tag &= ~ XPOST_OBJECT_TAG_DATA_FLAG_LIT;

    return obj;
}

Xpost_Object xpost_object_cvlit (Xpost_Object obj)
{
    obj.tag |= XPOST_OBJECT_TAG_DATA_FLAG_LIT;

    return obj;
}

/**
 * @cond LOCAL
 */

static
void _xpost_object_dump_composite (Xpost_Object obj)
{
    XPOST_LOG_DUMP(" %c "
            "%" XPOST_FMT_WORD(u) " "
            "%" XPOST_FMT_WORD(u) " "
            "%" XPOST_FMT_WORD(u) " "
            "%" XPOST_FMT_WORD(u) ">",
            obj.comp_.tag & XPOST_OBJECT_TAG_DATA_FLAG_BANK ? 'G' : 'L',
            obj.comp_.tag,
            obj.comp_.sz,
            obj.comp_.ent,
            obj.comp_.off);
}

/**
 * @endcond
 */

void xpost_object_dump (Xpost_Object obj)
{
    switch (xpost_object_get_type(obj))
    {
    default: /*@fallthrough@*/
    case invalidtype: XPOST_LOG_DUMP("<invalid object "
                              "%04" XPOST_FMT_WORD(x) " "
                              "%04" XPOST_FMT_WORD(x) " "
                              "%04" XPOST_FMT_WORD(x) " "
                              "%04" XPOST_FMT_WORD(x) " >",
                              obj.comp_.tag,
                              obj.comp_.sz,
                              obj.comp_.ent,
                              obj.comp_.off);
                      break;

    case nulltype: XPOST_LOG_DUMP("<null>");
                   break;
    case marktype: XPOST_LOG_DUMP("<mark>");
                   break;

    case booleantype: XPOST_LOG_DUMP("<boolean %s>",
                              obj.int_.val ? "true" : "false");
                      break;
    case integertype: XPOST_LOG_DUMP("<integer %" XPOST_FMT_INTEGER(d) ">",
                              obj.int_.val);
                      break;
    case realtype: XPOST_LOG_DUMP("<real %" XPOST_FMT_REAL ">",
                           obj.real_.val);
                   break;

    case stringtype: XPOST_LOG_DUMP("<string");
                     _xpost_object_dump_composite(obj);
                     break;
    case arraytype: XPOST_LOG_DUMP("<array");
                    _xpost_object_dump_composite(obj);
                    break;
    case dicttype: XPOST_LOG_DUMP("<dict");
                   _xpost_object_dump_composite(obj);
                   break;

    case nametype: XPOST_LOG_DUMP("<name %c "
                           "%" XPOST_FMT_WORD(u) " "
                           "%" XPOST_FMT_WORD(u) " "
                           "%" XPOST_FMT_DWORD(u) ">",
                           obj.mark_.tag & XPOST_OBJECT_TAG_DATA_FLAG_BANK ?
                               'G' : 'L',
                           obj.mark_.tag,
                           obj.mark_.pad0,
                           obj.mark_.padw);
                   break;

    case operatortype: XPOST_LOG_DUMP("<operator "
                               "%" XPOST_FMT_DWORD(u) ">",
                               obj.mark_.padw);
                       break;
    case filetype: XPOST_LOG_DUMP("<file "
                           "%" XPOST_FMT_DWORD(u) ">",
                           obj.mark_.padw);
                   break;

    case savetype: XPOST_LOG_DUMP("<save>");
                   break;
    case contexttype: XPOST_LOG_DUMP("<context>");
                      break;
    case extendedtype: XPOST_LOG_DUMP("<extended>");
                       break;
    case globtype: XPOST_LOG_DUMP("<glob>");
                   break;
    }
}

