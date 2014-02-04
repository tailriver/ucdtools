#include "ucd_private.h"
#include <string.h>


void _ucd_simple_writer_sub(ucd_context* c, const ucd_data* d)
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
            buffer = malloc(d->components[i] * d->num_rows * sizeof(*buffer));
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

int ucd_simple_writer(const ucd_content* ucd, const char* filename, int is_binary)
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

    cells = malloc(4 * ucd->num_cells * sizeof(*cells));
    for (i = 0; i < ucd->num_cells; ++i) {
        cells[4*i] = ucd->cell_id[i];
        cells[4*i+1] = ucd->cell_mat_id[i];
        cells[4*i+2] = ucd_cell_nlist_size(ucd->cell_type[i]);
        cells[4*i+3] = ucd->cell_type[i];
        c.num_nlist += cells[4*i+2];
    }
    ucd_writer_open(&c, filename);
    ucd_write_nodes_and_cells(&c,
            ucd->node_id, ucd->node_x, ucd->node_y, ucd->node_z,
            cells, ucd->cell_nlist, ucd->ld_nlist);
    free(cells);

    _ucd_simple_writer_sub(&c, ucd->ndata);
    _ucd_simple_writer_sub(&c, ucd->cdata);

    ucd_close(&c);

    return EXIT_SUCCESS;
}


int ucd_writer_open(ucd_context* c, const char* filename)
{
    const char magic_number = UCD_MAGIC_NUMBER;
    const int zero = 0;

    c->_fp = fopen(filename, c->is_binary ? "wb" : "w");
    if (c->_fp == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", __func__, filename);
        return EXIT_FAILURE;
    }

    if (c->is_binary) {
        fwrite(&magic_number, sizeof(char), 1, c->_fp);
        fwrite(&c->num_nodes, sizeof(int), 1, c->_fp);
        fwrite(&c->num_cells, sizeof(int), 1, c->_fp);
        fwrite(&c->num_ndata, sizeof(int), 1, c->_fp);
        fwrite(&c->num_cdata, sizeof(int), 1, c->_fp);
        fwrite(&zero, sizeof(int), 1, c->_fp); /* mdata */
        fwrite(&c->num_nlist, sizeof(int), 1, c->_fp);
    } else {
        fprintf(c->_fp, "%d %d %d %d 0\n",
                c->num_nodes, c->num_cells, c->num_ndata, c->num_cdata);
    }

    c->_nc = 0;

    return ferror(c->_fp);
}


int ucd_write_nodes_and_cells(
        ucd_context* c,
        const int* nodes, const float* x, const float* y, const float* z,
        const int* cells, const int* nlist, int ld_nlist)
{
    int i, j;

    if (c->is_binary) {
        fwrite(cells, sizeof(int), 4 * c->num_cells, c->_fp);
        for (i = 0; i < c->num_cells; ++i) {
            fwrite(&nlist[ld_nlist*i], sizeof(int), cells[4*i+2], c->_fp);
        }
        fwrite(x, sizeof(float), c->num_nodes, c->_fp);
        fwrite(y, sizeof(float), c->num_nodes, c->_fp);
        fwrite(z, sizeof(float), c->num_nodes, c->_fp);
    } else {
        if (nodes != NULL) {
            for (i = 0; i < c->num_nodes; ++i) {
                fprintf(c->_fp, "%d %e %e %e\n", nodes[i], x[i], y[i], z[i]);
            }
        } else {
            for (i = 0; i < c->num_nodes; ++i) {
                fprintf(c->_fp, "%d %e %e %e\n", i, x[i], y[i], z[i]);
            }
        }
        for (i = 0; i < c->num_cells; ++i) {
            fprintf(c->_fp, "%d %d %s",
                    cells[4*i], cells[4*i+1], ucd_cell_type_string(cells[4*i+2]));
            for (j = 0; j < cells[4*i+3]; ++j) {
                fprintf(c->_fp, " %d", nlist[ld_nlist*i+j]);
            }
            fprintf(c->_fp, "\n");
        }
    }
    return ferror(c->_fp);
}


int ucd_write_data_header(ucd_context* c,
        int num_comp, const int* components, const char* labels, const char* units)
{
    const int zero = 0;

    int num_data, comp_count, i;
    char buffer[1024];
    const char *anchor_l, *anchor_u;

    if (c->_nc == 0 && c->num_ndata > 0) {
        c->_nc = 1;
        num_data = c->num_ndata;
    } else if (c->_nc == 1 || c->num_cdata > 0) {
        c->_nc = 2;
        num_data = c->num_cdata;
    } else {
        fprintf(stderr, "%s: wrong call\n", __func__);
        return EXIT_FAILURE;
    }

    if (c->is_binary) {
        memset(buffer, '0', sizeof(buffer));
        for (i = 0, comp_count = 0; i < sizeof(buffer) - 1 && comp_count < num_comp; ++i) {
            if (labels[i] == '\0') {
                buffer[i] = '.';
                ++comp_count;
            } else {
                buffer[i] = labels[i];
            }
        }
        fwrite(buffer, sizeof(char), sizeof(buffer), c->_fp);

        memset(buffer, '0', sizeof(buffer));
        for (i = 0, comp_count = 0; i < sizeof(buffer) - 1 && comp_count < num_comp; ++i) {
            if (units[i] == '\0') {
                buffer[i] = '.';
                ++comp_count;
            } else {
                buffer[i] = units[i];
            }
        }
        fwrite(buffer, sizeof(char), sizeof(buffer), c->_fp);

        fwrite(&num_comp, sizeof(int), 1, c->_fp);
        fwrite(components, sizeof(int), num_comp, c->_fp);
        for (i = num_comp; i < num_data; ++i) {
            fwrite(&zero, sizeof(int), 1, c->_fp);
        }
    } else {
        fprintf(c->_fp, "%d", num_comp);
        for (i = 0; i < num_comp; ++i) {
            fprintf(c->_fp, " %d", components[i]);
        }
        fprintf(c->_fp, "\n");

        anchor_l = labels;
        anchor_u = units;
        for (i = 0; i < num_comp; ++i) {
            fprintf(c->_fp, "%s,%s\n", anchor_l, anchor_u);
            anchor_l += strlen(anchor_l) + 1;
            anchor_u += strlen(anchor_u) + 1;
        }
    }
    return ferror(c->_fp);
}


int ucd_write_data_minmax(ucd_context* c, const float* minima, const float* maxima)
{
    int num_data = c->_nc == 1 ? c->num_ndata : c->num_cdata;

    fwrite(minima, sizeof(float), num_data, c->_fp);
    fwrite(maxima, sizeof(float), num_data, c->_fp);

    return ferror(c->_fp);
}


int ucd_write_data_ascii_1(ucd_context* c, int id, const float* data)
{
    int i;
    int num_data = c->_nc == 1 ? c->num_ndata : c->num_cdata;

    fprintf(c->_fp, "%d", id);
    for (i = 0; i < num_data; ++i) {
        fprintf(c->_fp, " %e", data[i]);
    }
    fprintf(c->_fp, "\n");

    return ferror(c->_fp);
}


int ucd_write_data_ascii_n(ucd_context* c, const int* ids, const float* data)
{
    int i, j;
    int num_rows = c->_nc == 1 ? c->num_nodes : c->num_cells;
    int num_data = c->_nc == 1 ? c->num_ndata : c->num_cdata;

    for (i = 0; i < num_rows; ++i) {
        fprintf(c->_fp, "%d", ids[i]);
        for (j = 0; j < num_data; ++j) {
            fprintf(c->_fp, " %e", data[num_data * i + j]);
        }
        fprintf(c->_fp, "\n");
    }

    return ferror(c->_fp);
}


int ucd_write_data_binary(ucd_context* c,
        int component_size, const float* data, int need_transpose)
{
    int i, j;
    int num_rows = c->_nc == 1 ? c->num_nodes : c->num_cells;

    if (!need_transpose) {
        fwrite(data, sizeof(float), component_size * num_rows, c->_fp);
    } else {
        for (i = 0; i < num_rows; ++i) {
            for (j = 0; j < component_size; ++j) {
                fwrite(&data[num_rows*j+i], sizeof(float), 1, c->_fp);
            }
        }
    }

    return ferror(c->_fp);
}


int ucd_write_data_active_list(ucd_context* c, const int* active_list)
{
    const int zero = 0;

    int i;
    int num_data = c->_nc == 1 ? c->num_ndata : c->num_cdata;

    for (i = 0; i < num_data; ++i) {
        fwrite(&zero, sizeof(int), 1, c->_fp);
    }

    return ferror(c->_fp);
}
