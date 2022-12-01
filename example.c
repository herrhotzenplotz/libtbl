#include <stdio.h>
#include <stdlib.h>

#include "libtbl.h"

static void
magic_formatter(int is_start, void *data)
{
	(void) data;

	if (is_start)
		printf("\033[6m");
	else
		printf("\033[25m");
}

int
main(void)
{
	libtbl_tbl table;             /* Table printer state */
	libtbl_tblcoldef cols[] = {   /* Table column definitions */
		{ .name = "FOO",      .type = LIBTBL_TBLCOLTYPE_INT,    .flags = LIBTBL_TBLCOL_JUSTIFYR },
		{ .name = "IS GREEN", .type = LIBTBL_TBLCOLTYPE_BOOL,   .flags = LIBTBL_TBLCOL_BOLD|LIBTBL_TBLCOL_JUSTIFYR|LIBTBL_TBLCOL_COLOUREXPL },
		{ .name = "BAR",      .type = LIBTBL_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "MAGIC",    .type = LIBTBL_TBLCOLTYPE_LONG,   .flags = LIBTBL_TBLCOL_CUSTOM },
	};

	/* Init the table printer */
	table = libtbl_tbl_begin(cols, sizeof(cols) / sizeof(*cols));
	if (!table) {
		fprintf(stderr, "error: could not init table\n");
		return EXIT_FAILURE;
	}

	/* Add rows */
	for (int i = 0; i < 10; ++i) {
		char buf[32] = {0};

		snprintf(buf, sizeof buf, "Testing 123 -> %d", i + 42);
		libtbl_tbl_add_row(table, 
		                   i + 1, 
		                   (i%2) ? LIBTBL_COLOUR_GREEN : LIBTBL_COLOUR_RED, i % 2, 
		                   buf,
		                   magic_formatter, NULL, 42);
	}

	/* Dump the table and deallocate the internal allocations */
	libtbl_tbl_end(table);

	return EXIT_SUCCESS;
}

