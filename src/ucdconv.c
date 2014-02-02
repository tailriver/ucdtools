#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ucd.h"

/*
 * a -> b : ascii to binary
 * b -> a : binary to ascii
 * a -> a : ascii to ascii (to shrink size)
 */

int main(int argc, char** argv) {
    ucd_content ucd;
    int is_binary_input, keep_format, has_error, i;
    char* input_file;
    char* output_file;
    char *anchor_l, *anchor_u;

    if (argc < 3) {
        fprintf(stderr, "usage exec [options] input.inp output.inp\n");
        return EXIT_FAILURE;
    }

    keep_format = 0;
    input_file = argv[argc-2];
    output_file = argv[argc-1];

    /* header */
    has_error = ucd_simple_reader(&ucd, input_file, &is_binary_input);
    if (has_error) {
        return has_error;
    }

    printf("Number of nodes: %d\n", ucd.num_nodes);
    printf("Number of cells: %d\n", ucd.num_cells);
    if (ucd.ndata != NULL) {
        printf("Number of node data: %d\n", ucd.ndata->num_data);
        printf("  Number of components: %d\n", ucd.ndata->num_comp);
        anchor_l = ucd.ndata->labels;
        anchor_u = ucd.ndata->units;
        for (i = 0; i < ucd.ndata->num_comp; ++i) {
            printf("  Comp. %d: %s (%s) [%d]\n",
                    i, anchor_l, anchor_u, ucd.ndata->components[i]);
            anchor_l += strlen(anchor_l) + 1;
            anchor_u += strlen(anchor_u) + 1;
        }
    } else {
        printf("Number of node data: (none)\n");
    }
    if (ucd.cdata != NULL) {
        printf("Number of cell data: %d\n", ucd.cdata->num_data);
        printf("  Number of components: %d\n", ucd.cdata->num_comp);
        anchor_l = ucd.cdata->labels;
        anchor_u = ucd.cdata->units;
        for (i = 0; i < ucd.cdata->num_comp; ++i) {
            printf("  Comp. %d: %s (%s) [%d]\n",
                    i, anchor_l, anchor_u, ucd.cdata->components[i]);
            anchor_l += strlen(anchor_l) + 1;
            anchor_u += strlen(anchor_u) + 1;
        }
    } else {
        printf("Number of cell data: (none)\n");
    }

    if (is_binary_input) {
        if (keep_format) {
            fprintf(stderr, "input file is binary format.\n");
            return EXIT_FAILURE;
        } else {
            has_error = ucd_simple_writer(&ucd, output_file, 0);
            /* return ucd_write_ascii(&ucd_, stdout); */
        }
    } else {
        if (keep_format) {
            has_error = ucd_simple_writer(&ucd, output_file, 0);
            /* return ucd_write_ascii(&ucd_, stdout); */
        } else {
            has_error = ucd_simple_writer(&ucd, output_file, 1);
            /* return ucd_write_binary(&ucd_, stdout); */
        }
    }

    ucd_simple_free(&ucd);

    return has_error;
}
