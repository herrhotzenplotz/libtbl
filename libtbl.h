/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBTBL_TABLE_H
#define LIBTBL_TABLE_H

#include <stdlib.h>

typedef struct libtbl_tblcoldef libtbl_tblcoldef;
typedef void *libtbl_tbl;

/** Flags for table column definitions */
enum libtbl_tblcol_flags {
	/* Custom formatter (function pointer + data) */
	LIBTBL_TBLCOL_CUSTOM  = 1,
	/* Right-justify the column */
	LIBTBL_TBLCOL_JUSTIFYR       = 2,
	/* Make it bold */
	LIBTBL_TBLCOL_BOLD           = 4,
	/* Explicit colour - provide the colour to libtbl_tbl_add_row first
	 * and second the content of the cell. */
	LIBTBL_TBLCOL_COLOUREXPL     = 8,
	/* 256 color handling. Just like the above */
	LIBTBL_TBLCOL_256COLOUR      = 16,
};

enum libtbl_tblcoltype {
	LIBTBL_TBLCOLTYPE_INT,        /* integer */
	LIBTBL_TBLCOLTYPE_LONG,       /* signed long int */
	LIBTBL_TBLCOLTYPE_STRING,     /* C string */
	LIBTBL_TBLCOLTYPE_DOUBLE,     /* double precision float */
	LIBTBL_TBLCOLTYPE_BOOL,       /* yes/no */
};

/** A single table column */
struct libtbl_tblcoldef {
	char const *name;           /* name of the column, also displayed in first row */
	int type;                   /* type of values in this column */
	int flags;                  /* flags about this column */
};

/* Init a table printer */
libtbl_tbl libtbl_tbl_begin(libtbl_tblcoldef const *const cols,
			    size_t const cols_size);

/* Print the table contents and free all the resources allocated in
 * the table */
void libtbl_tbl_end(libtbl_tbl table);

/* Add a single to an initialized table */
int libtbl_tbl_add_row(libtbl_tbl table, ...);

#endif /* LIBTBL_TABLE_H */
