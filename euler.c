#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "libtbl.h"

static void
taylor(libtbl_tbl table)
{
	double exp = 0.0;
	double x = 1.0;
	double eps = 1e-6;
	double g = 1.0;
	int i = 0;
	bool converges = false;

	for (;;) {
		exp += g;
		g = g * x / (double)(++i);

		converges = fabs(g) <= eps;
		libtbl_tbl_add_row(table,
		                   i,
		                   g,
		                   converges ? LIBTBL_COLOUR_GREEN : LIBTBL_COLOUR_RED,
		                   converges,
		                   exp);

		if (fabs(g) <= eps) {
			break;
		}
	}
}

int
main(void)
{
	libtbl_tbl table;             /* Table printer state */
	libtbl_tblcoldef cols[] = {   /* Table column definitions */
		{ .name = "ITERATION",
		  .type = LIBTBL_TBLCOLTYPE_INT,
		  .flags = LIBTBL_TBLCOL_JUSTIFYR },
		{ .name = "DIFFERENCE",
		  .type = LIBTBL_TBLCOLTYPE_DOUBLE,
		  .flags = LIBTBL_TBLCOL_JUSTIFYR },
		{ .name = "CONVERGES",
		  .type = LIBTBL_TBLCOLTYPE_BOOL,
		  .flags = LIBTBL_TBLCOL_COLOUREXPL },
		{ .name = "EXPONENTIAL",
		  .type = LIBTBL_TBLCOLTYPE_DOUBLE,
		  .flags = 0 },
	};

	/* Init the table printer */
	table = libtbl_tbl_begin(cols, sizeof(cols) / sizeof(*cols));
	if (!table) {
		fprintf(stderr, "error: could not init table\n");
		return EXIT_FAILURE;
	}

	/* Do a calculation */
	taylor(table);

	/* Dump the table and deallocate the internal allocations */
	libtbl_tbl_end(table);

	return EXIT_SUCCESS;
}

