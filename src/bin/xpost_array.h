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

#ifndef XPOST_AR_H
#define XPOST_AR_H

/** \file xpost_array.h
   \brief array functions

   an array object is 8 bytes,
   consisting of 4 16bit fields common to all composite objects
     tag, type enum and flags
     sz, count of objects in array
     ent, entity number
     off, offset into allocation
   the entity data is a "C" array of objects

   "arr" functions require an mfile.
   "bar" functions select the mfile from a context, using the FBANK flag.
*/

/**
 * @brief consarr - construct an array object
 * in the mtab of specified mfile
*/
Xpost_Object consarr(Xpost_Memory_File *mem, unsigned sz);

/**
 * @brief consbar - construct an array object
 * selecting mfile according to ctx->vmmode
*/
Xpost_Object consbar(Xpost_Context *ctx, unsigned sz);

/** 
   * @brief store value in an array
*/
int arrput(Xpost_Memory_File *mem, Xpost_Object a, integer i, Xpost_Object o);

/** 
 * @brief store value in a banked array
*/
int barput(Xpost_Context *ctx, Xpost_Object a, integer i, Xpost_Object o);

/** 
 * @brief extract value from an array
*/
Xpost_Object arrget(Xpost_Memory_File *mem, Xpost_Object a, integer i);

/** 
 * @brief extract value from a banked array
*/
Xpost_Object barget(Xpost_Context *ctx, Xpost_Object a, integer i);

#endif
