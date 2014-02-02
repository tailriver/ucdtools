#include "ucd_private.h"
#include <string.h>


/*
 * Cell type
 *
 * ID        Name        #nodes
 * 0 pt    | point          1
 * 1 line  | line           2
 * 2 tri   | triangle       3
 * 3 quad  | quadrilateral  4
 * 4 tet   | tetrahedron    4
 * 5 pyr   | pyramid        5
 * 6 prism | prism          6
 * 7 hex   | hexahedron     8
 *
 */

int ucd_cell_nlist_size(int cell_type)
{
    return cell_type + (int)(cell_type < 4 || cell_type == 7);
}


const char* ucd_cell_type_string(int num)
{
    if (num < 0 || num > 7) {
        fprintf(stderr, "cell type number %d is invalid.\n", num);
        return NULL;
    } else if (num % 2) {
        /* 1, 3, 5, 7 */
        if (num < 4) {
            return (num == 1) ? "line" : "quad";
        } else {
            return (num == 5) ? "pyr" : "hex";
        }
    } else {
        /* 0, 2, 4, 6 */
        if (num % 4) {
            return (num == 2) ? "tri" : "prism";
        } else {
            return (num == 0) ? "pt" : "tet";
        }
    }
}


int ucd_cell_type_number(const char* str)
{
    if (str[0] < 'p') {
        /* hex, line */
        if (strcmp(str, "hex") == 0) {
            return 7;
        }
        if (strcmp(str, "line") == 0) {
            return 1;
        }
    } else if (str[0] == 'p') {
        /* prism, pt, pyr */
        if (strcmp(str, "prism") == 0) {
            return 6;
        }
        if (strcmp(str, "pt") == 0) {
            return 0;
        }
        if (strcmp(str, "pyr") == 0) {
            return 5;
        }
    } else {
        /* quad, tet, tri */
        if (strcmp(str, "quad") == 0) {
            return 3;
        }
        if (strcmp(str, "tet") == 0) {
            return 4;
        }
        if (strcmp(str, "tri") == 0) {
            return 2;
        }
    }

    fprintf(stderr, "cell type string %s is invalid.\n", str);
    return -1;
}


int ucd_binary_filesize(ucd_context* ucd)
{
    int size;

    size = sizeof(char) + 6 * sizeof(int); /* header */
    size += 4 * ucd->num_cells * sizeof(int); /* cell information */
    size += ucd->num_nlist * sizeof(int); /* node list of cells */
    size += 3 * ucd->num_nodes * sizeof(float); /* node coordinates */

    if (ucd->num_ndata > 0) {
        size += 2 * 1024 * sizeof(char); /* labels and units */
        size += sizeof(int) + ucd->num_ndata * sizeof(int); /* component size */
        size += 2 * ucd->num_ndata * sizeof(float); /* minimum and maximum */
        size += ucd->num_ndata * ucd->num_nodes * sizeof(float); /* data body */
        size += ucd->num_ndata * sizeof(int); /* active list */
    }

    if (ucd->num_cdata > 0) {
        size += 2 * 1024 * sizeof(char);
        size += sizeof(int) + ucd->num_cdata * sizeof(int);
        size += 2 * ucd->num_cdata * sizeof(float);
        size += ucd->num_cdata * ucd->num_cells * sizeof(float);
        size += ucd->num_cdata * sizeof(int);
    }

    return size;
}


int ucd_close(ucd_context* ucd)
{
    return fclose(ucd->fp);
}