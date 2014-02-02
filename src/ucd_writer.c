#include "ucd_private.h"
#include <string.h>


void _ucd_writer_simple_sub(ucd_context* c, const ucd_data* d)
{
    int i, j, k, base_col;
    float* buffer;

    if (d == NULL)
        return;

    ucd_write_data_header(c, d->num_comp, d->components, d->labels, d->units);
    if (c->is_binary) {
        ucd_write_data_minmax(c, d->minima, d->maxima);
        base_col = 0;
        for (i = 0; i < d->num_comp; ++i) {
            buffer = (float*) malloc(d->components[i] * d->num_rows * sizeof(float));
            for (j = 0; j < d->num_rows; ++j) {
                for (k = 0; k < d->components[i]; ++k) {
                    buffer[d->components[i] * j + k] = d->data[d->num_data * j + base_col + k];
                }
            }
            ucd_write_data_binary(c, d->components[i], buffer, 0);
            free(buffer);
            base_col += d->components[i];
        }
        ucd_write_data_active_list(c, NULL);
    } else {
        ucd_write_data_ascii_n(c, d->row_id, d->data);
    }
}

int ucd_writer_simple(const ucd_content* ucd, const char* filename, int is_binary)
{
    ucd_context c;
    int i;
    int* cells;

    c.is_binary = is_binary;
    c.num_nodes = ucd->num_nodes;
    c.num_cells = ucd->num_cells;
    c.num_ndata = ucd->ndata != NULL ? ucd->ndata->num_data : 0;
    c.num_cdata = ucd->cdata != NULL ? ucd->cdata->num_data : 0;
    c.num_nlist = 0;

    cells = (int*) malloc(4 * ucd->num_cells * sizeof(int));
    for (i = 0; i < ucd->num_cells; ++i) {
        cells[4*i] = ucd->cell_id[i];
        cells[4*i+1] = ucd->cell_mat_id[i];
        cells[4*i+2] = ucd->cell_type[i];
        cells[4*i+3] = ucd_cell_nlist_size(ucd->cell_type[i]);
        c.num_nlist += cells[4*i+3];
    }
    ucd_writer_open(&c, filename);
    ucd_write_nodes_and_cells(&c,
            ucd->node_id, ucd->node_x, ucd->node_y, ucd->node_z,
            cells, ucd->cell_nlist, ucd->ld_nlist);
    free(cells);

    _ucd_writer_simple_sub(&c, ucd->ndata);
    _ucd_writer_simple_sub(&c, ucd->cdata);

    ucd_close(&c);

    return EXIT_SUCCESS;
}


int ucd_writer_open(ucd_context* ucd, const char* filename)
{
    const char magic_number = UCD_MAGIC_NUMBER;
    const int zero = 0;

    ucd->fp = fopen(filename, ucd->is_binary ? "wb" : "w");
    if (ucd->fp == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", __func__, filename);
        return EXIT_FAILURE;
    }

    if (ucd->is_binary) {
        fwrite(&magic_number, sizeof(char), 1, ucd->fp);
        fwrite(&ucd->num_nodes, sizeof(int), 1, ucd->fp);
        fwrite(&ucd->num_cells, sizeof(int), 1, ucd->fp);
        fwrite(&ucd->num_ndata, sizeof(int), 1, ucd->fp);
        fwrite(&ucd->num_cdata, sizeof(int), 1, ucd->fp);
        fwrite(&zero, sizeof(int), 1, ucd->fp); /* mdata */
        fwrite(&ucd->num_nlist, sizeof(int), 1, ucd->fp);
    } else {
        fprintf(ucd->fp, "%d %d %d %d 0\n",
                ucd->num_nodes, ucd->num_cells, ucd->num_ndata, ucd->num_cdata);
    }

    ucd->_nc = 0;

    return ferror(ucd->fp);
}


int ucd_write_nodes_and_cells(
        ucd_context* ucd,
        const int* nodes,
        const float* x,
        const float* y,
        const float* z,
        const int* cells,
        const int* nlist,
        int ld_nlist)
{
    int i, j;

    if (ucd->is_binary) {
        fwrite(cells, sizeof(int), 4 * ucd->num_cells, ucd->fp);
        fwrite(nlist, sizeof(int), ucd->num_nlist, ucd->fp);
        fwrite(x, sizeof(float), ucd->num_nodes, ucd->fp);
        fwrite(y, sizeof(float), ucd->num_nodes, ucd->fp);
        fwrite(z, sizeof(float), ucd->num_nodes, ucd->fp);
    } else {
        if (nodes != NULL) {
            for (i = 0; i < ucd->num_nodes; ++i) {
                fprintf(ucd->fp, "%d %e %e %e\n", nodes[i], x[i], y[i], z[i]);
            }
        } else {
            for (i = 0; i < ucd->num_nodes; ++i) {
                fprintf(ucd->fp, "%d %e %e %e\n", i, x[i], y[i], z[i]);
            }
        }
        for (i = 0; i < ucd->num_cells; ++i) {
            fprintf(ucd->fp, "%d %d %s",
                    cells[4*i], cells[4*i+1], ucd_cell_type_string(cells[4*i+2]));
            for (j = 0; j < cells[4*i+3]; ++j) {
                fprintf(ucd->fp, " %d", nlist[ld_nlist*i+j]);
            }
            fprintf(ucd->fp, "\n");
        }
    }
    return ferror(ucd->fp);
}


int ucd_write_data_header(ucd_context* ucd,
        int num_comp, const int* components, const char* labels, const char* units)
{
    const int zero = 0;

    int num_data, i;
    char buffer[1024];
    const char *anchor_l, *anchor_u;

    if (ucd->_nc == 0 && ucd->num_ndata > 0) {
        ucd->_nc = 1;
        num_data = ucd->num_ndata;
    } else if (ucd->_nc == 1 || ucd->num_cdata > 0) {
        ucd->_nc = 2;
        num_data = ucd->num_cdata;
    } else {
        fprintf(stderr, "%s: wrong call\n", __func__);
        return EXIT_FAILURE;
    }

    if (ucd->is_binary) {
        for (i = 0; i < sizeof(buffer); ++i) {
            buffer[i] = labels[i] == '\0' ? '.' : labels[i];
        }
        buffer[sizeof(buffer)-1] = '0';
        fwrite(buffer, sizeof(char), sizeof(buffer), ucd->fp);

        for (i = 0; i < sizeof(buffer); ++i) {
            buffer[i] = units[i] == '\0' ? '.' : units[i];
        }
        buffer[sizeof(buffer)-1] = '0';
        fwrite(buffer, sizeof(char), sizeof(buffer), ucd->fp);

        fwrite(&num_comp, sizeof(int), 1, ucd->fp);
        fwrite(components, sizeof(int), num_comp, ucd->fp);
        for (i = num_comp; i < num_data; ++i) {
            fwrite(&zero, sizeof(int), 1, ucd->fp);
        }
    } else {
        fprintf(ucd->fp, "%d", num_comp);
        for (i = 0; i < num_comp; ++i) {
            fprintf(ucd->fp, " %d", components[i]);
        }
        fprintf(ucd->fp, "\n");

        anchor_l = labels;
        anchor_u = units;
        for (i = 0; i < num_comp; ++i) {
            fprintf(ucd->fp, "%s,%s\n", anchor_l, anchor_u);
            anchor_l += strlen(anchor_l) + 1;
            anchor_u += strlen(anchor_u) + 1;
        }
    }
    return ferror(ucd->fp);
}


int ucd_write_data_minmax(
        ucd_context* ucd, const float* minima, const float* maxima)
{
    int num_data = ucd->_nc == 1 ? ucd->num_ndata : ucd->num_cdata;

    fwrite(minima, sizeof(float), num_data, ucd->fp);
    fwrite(maxima, sizeof(float), num_data, ucd->fp);

    return ferror(ucd->fp);
}


int ucd_write_data_ascii_1(
        ucd_context* ucd,
        int id,
        const float* data)
{
    int i;
    int num_data = ucd->_nc == 1 ? ucd->num_ndata : ucd->num_cdata;

    fprintf(ucd->fp, "%d", id);
    for (i = 0; i < num_data; ++i) {
        fprintf(ucd->fp, " %e", data[i]);
    }
    fprintf(ucd->fp, "\n");

    return ferror(ucd->fp);
}


int ucd_write_data_ascii_n(
        ucd_context* ucd,
        const int* ids,
        const float* data)
{
    int i, j;
    int num_rows = ucd->_nc == 1 ? ucd->num_nodes : ucd->num_cells;
    int num_data = ucd->_nc == 1 ? ucd->num_ndata : ucd->num_cdata;

    for (i = 0; i < num_rows; ++i) {
        fprintf(ucd->fp, "%d", ids[i]);
        for (j = 0; j < num_data; ++j) {
            fprintf(ucd->fp, " %e", data[num_data * i + j]);
        }
        fprintf(ucd->fp, "\n");
    }

    return ferror(ucd->fp);
}


int ucd_write_data_binary(
        ucd_context* ucd,
        int component_size,
        const float* data,
        int need_transpose)
{
    int i, j;
    int num_rows = ucd->_nc == 1 ? ucd->num_nodes : ucd->num_cells;

    if (!need_transpose) {
        fwrite(data, sizeof(float), component_size * num_rows, ucd->fp);
    } else {
        for (i = 0; i < num_rows; ++i) {
            for (j = 0; j < component_size; ++j) {
                fwrite(&data[num_rows*j+i], sizeof(float), 1, ucd->fp);
            }
        }
    }

    return ferror(ucd->fp);
}


int ucd_write_data_active_list(ucd_context* ucd, const int* active_list)
{
    const int zero = 0;

    int i;
    int num_data = ucd->_nc == 1 ? ucd->num_ndata : ucd->num_cdata;

    for (i = 0; i < num_data; ++i) {
        fwrite(&zero, sizeof(int), 1, ucd->fp);
    }

    return ferror(ucd->fp);
}
