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

#include "libtbl.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*****************************************************************************
 * Helpers
 ****************************************************************************/
static char *
libtbl_asprintf(char const *fmt, ...)
{
	/* Obviously this is not ideal but it works */
	char tmp = 0, *result = NULL;
	size_t actual = 0;
	va_list vp;

	va_start(vp, fmt);
	actual = vsnprintf(&tmp, 1, fmt, vp);
	va_end(vp);

	result = calloc(1, actual + 1);
	if (!result)
		return NULL;
 
	va_start(vp, fmt);
	vsnprintf(result, actual + 1, fmt, vp);
	va_end(vp);

	return result;
}

/*****************************************************************************
 * ANSI Colour handling 
 ****************************************************************************/
static int have_colours = -1;

static int
libtbl_have_colours(void)
{
	if (have_colours < 0) {
		have_colours = isatty(STDOUT_FILENO);
	}

	return have_colours;
}

static struct {
        uint32_t code;
        char *sequence;
} colour_table[1024];
static size_t colour_table_size;

static void
clean_colour_table(void)
{
        for (size_t i = 0; i < colour_table_size; ++i)
                free(colour_table[i].sequence);
}

static char const *
colour_cache_lookup(uint32_t const code)
{
        for (size_t i = 0; i < colour_table_size; ++i) {
                if (colour_table[i].code == code)
                        return colour_table[i].sequence;
        }
        return NULL;
}

static void
colour_cache_insert(uint32_t const code, char *sequence)
{
        colour_table[colour_table_size].code = code;
        colour_table[colour_table_size].sequence = sequence;
        colour_table_size++;
}

static char const *const
libtbl_setcolour(int code)
{
	if (!libtbl_have_colours())
		return "";

	switch (code) {
	case LIBTBL_COLOUR_BLACK:   return "\033[30m";
	case LIBTBL_COLOUR_RED:     return "\033[31m";
	case LIBTBL_COLOUR_GREEN:   return "\033[32m";
	case LIBTBL_COLOUR_YELLOW:  return "\033[33m";
	case LIBTBL_COLOUR_BLUE:    return "\033[34m";
	case LIBTBL_COLOUR_MAGENTA: return "\033[35m";
	case LIBTBL_COLOUR_CYAN:    return "\033[36m";
	case LIBTBL_COLOUR_WHITE:   return "\033[37m";
	case LIBTBL_COLOUR_DEFAULT: return "\033[39m";
	default:
		return NULL;
	}
	return NULL;
}

static char const *const
libtbl_setcolour256(uint64_t const code)
{
	char *result = NULL;
	char const *oldresult = NULL;

        if (!libtbl_have_colours())
                return "";

	if (colour_table_size == 0)
		atexit(clean_colour_table);

	oldresult = colour_cache_lookup(code);
	if (oldresult)
		return oldresult;

        /* TODO: This is inherently screwed */
	result = libtbl_asprintf("\033[38;2;%02d;%02d;%02dm",
	                         (code & 0xFF000000) >> 24,
	                         (code & 0x00FF0000) >> 16,
	                         (code & 0x0000FF00) >>  8);

	colour_cache_insert(code, result);

	return result;
}

static char const *const
libtbl_setbold(void)
{
	if (!libtbl_have_colours())
		return "";
	else
		return "\033[1m";
}

static char const *const
libtbl_resetcolour(void)
{
	if (!libtbl_have_colours())
		return "";
	else
		return "\033[m";
}

static char const *const
libtbl_resetbold(void)
{
	if (!libtbl_have_colours())
		return "";
	else
		return "\033[22m";
}

int
libtbl_set_colours_enabled(int enabled)
{
	return (have_colours = enabled);
}

/*****************************************************************************
 * Table State
 ****************************************************************************/
/* A row */
struct libtbl_tblrow;

/* Internal state of a table printer. We return a handle to it in
 * gcli_table_init. */
struct libtbl_tbl {
	libtbl_tblcoldef const *cols; /* user provided column definitons */
	int *col_widths;        /* minimum width of the columns */
	size_t cols_size;       /* size of above arrays */

	struct libtbl_tblrow *rows; /* list of rows */
	size_t rows_size;       /* number of rows */
};

struct libtbl_tblrow {
	struct {
		char *text;     /* the text in the cell */
		char const *colour;     /* colour (ansi escape sequence) if
		                         * explicit fixed colour was given */

		custom_formatter formatter_fn;
		void *userdata;         /* user data to pass to the function */
	} *cells;
};

/* Push a row into the table state */
static int
table_pushrow(struct libtbl_tbl *const table, struct libtbl_tblrow row)
{
	table->rows = realloc(table->rows, sizeof(*table->rows) * (table->rows_size + 1));
	if (!table->rows)
		return -1;

	table->rows[table->rows_size++] = row;

	return 0;
}

/** Initialize the internal state structure of the table printer. */
libtbl_tbl
libtbl_tbl_begin(libtbl_tblcoldef const *const cols, size_t const cols_size)
{
	struct libtbl_tbl *tbl;

	/* Allocate the structure and fill in the handle */
	tbl = calloc(sizeof(*tbl), 1);
	if (!tbl)
		return NULL;

	/* Reserve memory for the column sizes */
	tbl->col_widths = calloc(sizeof(*tbl->col_widths), cols_size);
	if (!tbl->col_widths) {
		free(tbl);
		return NULL;
	}

	/* Store the list of columns */
	tbl->cols = cols;
	tbl->cols_size = cols_size;

	/* Check the headers */
	for (size_t i = 0; i < cols_size; ++i) {

		/* Compute the header's length and use these as initial
		 * values */
		tbl->col_widths[i] = strlen(cols[i].name);
	}

	return tbl;
}

static void
table_freerow(struct libtbl_tblrow *row, size_t const cols)
{
	for (size_t i = 0; i < cols; ++i)
		free(row->cells[i].text);

	free(row->cells);
	row->cells = NULL;
}

static int
tablerow_add_cell(struct libtbl_tbl *const table,
                  struct libtbl_tblrow *const row,
                  size_t const col, va_list vp)
{
	int cell_size = 0;

	/* Extract the explicit colour code */
	if (table->cols[col].flags & LIBTBL_TBLCOL_COLOUREXPL) {
		int code = va_arg(vp, int);

		/* don't free that! it's allocated and free'ed up in the colour
		 * handling code. */
		row->cells[col].colour = libtbl_setcolour(code);
	} else if (table->cols[col].flags & LIBTBL_TBLCOL_256COLOUR) {
		uint64_t hexcode = va_arg(vp, uint64_t);

		/* see comment above */
		row->cells[col].colour = libtbl_setcolour256(hexcode);
	} else if (table->cols[col].flags & LIBTBL_TBLCOL_CUSTOM) {
		row->cells[col].formatter_fn = va_arg(vp, custom_formatter);
		row->cells[col].userdata = va_arg(vp, void *);
	}


	/* Process the content */
	switch (table->cols[col].type) {
	case LIBTBL_TBLCOLTYPE_INT: {
		row->cells[col].text = libtbl_asprintf("%d", va_arg(vp, int));
		cell_size = strlen(row->cells[col].text);
	} break;
	case LIBTBL_TBLCOLTYPE_LONG: {
		row->cells[col].text = libtbl_asprintf("%ld", va_arg(vp, long));
		cell_size = strlen(row->cells[col].text);
	} break;
	case LIBTBL_TBLCOLTYPE_STRING: {
		char *it = va_arg(vp, char *);
		row->cells[col].text = strdup(it);
		cell_size = strlen(it);
	} break;
	case LIBTBL_TBLCOLTYPE_DOUBLE: {
		row->cells[col].text = libtbl_asprintf("%lf", va_arg(vp, double));
		cell_size = strlen(row->cells[col].text);
	} break;
	case LIBTBL_TBLCOLTYPE_BOOL: {
		/* Do not use real _Bool type as it triggers a compiler bug in
		 * LLVM clang 13 */
		int val = va_arg(vp, int);
		if (val) {
			row->cells[col].text = strdup("yes");
			cell_size = 3;
		} else {
			row->cells[col].text = strdup("no");
			cell_size = 2;
		}
	} break;
	default:
		return -1;
	}

	/* Update the column width if needed */
	if (table->col_widths[col] < cell_size)
		table->col_widths[col] = cell_size;

	return 0;
}

int
libtbl_tbl_add_row(libtbl_tbl _table, ...)
{
	va_list vp;
	struct libtbl_tblrow row = {0};
	struct libtbl_tbl *table = (struct libtbl_tbl *)(_table);

	/* reserve array of cells */
	row.cells = calloc(sizeof(*row.cells), table->cols_size);
	if (!row.cells)
		return -1;

	va_start(vp, _table);

	/* Step through all the columns and print the cells */
	for (size_t i = 0; i < table->cols_size; ++i) {
		if (tablerow_add_cell(table, &row, i, vp) < 0) {
			table_freerow(&row, table->cols_size);
			va_end(vp);
			return -1;
		}
	}

	va_end(vp);

	/* Push the row into the table */
	if (table_pushrow(table, row) < 0) {
		table_freerow(&row, table->cols_size);
		return -1;
	}

	return 0;
}

static void
pad(size_t const n)
{
	for (size_t p = 0; p < n; ++p)
		putchar(' ');
}

static void
dump_row(struct libtbl_tbl const *const table, size_t const i)
{
	struct libtbl_tblrow const *const row = &table->rows[i];

	for (size_t col = 0; col < table->cols_size; ++col) {
		/* If right justified and not last column, print padding */
		if ((table->cols[col].flags & LIBTBL_TBLCOL_JUSTIFYR) &&
		    (col + 1) < table->cols_size)
			pad(table->col_widths[col] - strlen(row->cells[col].text));

		/* Colour Processing */
		if (table->cols[col].flags &
		         (LIBTBL_TBLCOL_COLOUREXPL|LIBTBL_TBLCOL_256COLOUR))
			printf("%s", row->cells[col].colour);

		/* Bold */
		if (table->cols[col].flags & LIBTBL_TBLCOL_BOLD)
			printf("%s", libtbl_setbold());

		/* Custom formatter handling */
		if (table->cols[col].flags & LIBTBL_TBLCOL_CUSTOM)
			row->cells[col].formatter_fn(1, row->cells[col].userdata);

		/* Print cell if it is not NULL, otherwise indicate it by
		 * printing <empty> */
		printf("%s  ", row->cells[col].text ? row->cells[col].text : "<empty>");

		/* Custom formatter end */
		if (table->cols[col].flags & LIBTBL_TBLCOL_CUSTOM)
			row->cells[col].formatter_fn(0, row->cells[col].userdata);

		/* End colour */
		if (table->cols[col].flags & (LIBTBL_TBLCOL_COLOUREXPL|LIBTBL_TBLCOL_256COLOUR))
			printf("%s", libtbl_resetcolour());

		/* Stop printing in bold */
		if (table->cols[col].flags & LIBTBL_TBLCOL_BOLD)
			printf("%s", libtbl_resetbold());

		/* If left-justified and not last column, print padding */
		if (!(table->cols[col].flags & LIBTBL_TBLCOL_JUSTIFYR) &&
		    (col + 1) < table->cols_size)
			pad(table->col_widths[col] - strlen(row->cells[col].text));
	}
	putchar('\n');
}

static int
libtbl_tbl_dump(libtbl_tbl const _table)
{
	struct libtbl_tbl const *const table = (struct libtbl_tbl const *const)_table;

	for (size_t i = 0; i < table->cols_size; ++i) {
		printf("%s  ", table->cols[i].name);

		if ((i + 1) < table->cols_size)
			pad(table->col_widths[i] - strlen(table->cols[i].name));
	}
	printf("\n");

	for (size_t i = 0; i < table->rows_size; ++i) {
		dump_row(table, i);
	}

	return 0;
}

static void
libtbl_tbl_free(libtbl_tbl _table)
{
	struct libtbl_tbl *tbl = (struct libtbl_tbl *)_table;

	for (size_t row = 0; row < tbl->rows_size; ++row)
		table_freerow(&tbl->rows[row], tbl->cols_size);

	free(tbl->rows);
	free(tbl->col_widths);
	free(tbl);
}

void
libtbl_tbl_end(libtbl_tbl tbl)
{
	libtbl_tbl_dump(tbl);
	libtbl_tbl_free(tbl);
}
