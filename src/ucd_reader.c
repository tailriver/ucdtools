#include <float.h>
#include <string.h>
#include "ucd_private.h"

#ifdef _WIN32
#pragma warning(disable:4996)
#endif


void _ucd_ignore_lines(ucd_context* c, int count) {
    while (count-- > 0) {
        while (getc(c->_fp) != '\n');
    }
}


void _ucd_simple_reader_sub(ucd_context* c, ucd_data* d)
{
    int i, j, k, base_col;
    float* buffer;

    d->components = malloc(d->num_data * sizeof(*d->components));
    d->minima = malloc(d->num_data * sizeof(*d->maxima));
    d->maxima = malloc(d->num_data * sizeof(*d->minima));
    d->row_id = malloc(d->num_rows * sizeof(*d->row_id));
    d->data = malloc(d->num_rows * d->num_data * sizeof(*d->data));

    ucd_read_data_header(c, &d->num_comp, d->components, d->labels, d->units);

    if (c->is_binary) {
        ucd_read_data_minmax(c, d->minima, d->maxima);

        for (i = 0; i < d->num_rows; ++i) {
            d->row_id[i] = i + 1;
        }

        base_col = 0;
        for (i = 0; i < d->num_comp; ++i) {
            buffer = malloc(d->components[i] * d->num_rows * sizeof(*buffer));
            ucd_read_data_binary(c, d->components[i], buffer);
            for (j = 0; j < d->num_rows; ++j) {
                for (k = 0; k < d->components[i]; ++k) {
                    d->data[d->num_data * j + base_col + k] = buffer[d->components[i] * j + k];
                }
            }
            free(buffer);
            base_col += d->components[i];
        }
        ucd_read_data_active_list(c, NULL);
    } else {
        ucd_read_data_ascii(c, d->row_id, d->data);

        for (i = 0; i < d->num_data; ++i) {
            d->minima[i] = +FLT_MAX;
            d->maxima[i] = -FLT_MAX;
        }
        for (i = 0; i < d->num_rows; ++i) {
            for(j = 0; j < d->num_data; ++j) {
                if (d->data[d->num_data * i + j] < d->minima[j]) {
                    d->minima[j] = d->data[d->num_data * i + j];
                }
                if (d->data[d->num_data * i + j] > d->maxima[j]) {
                    d->maxima[j] = d->data[d->num_data * i + j];
                }
            }
        }
    }
}


int ucd_simple_reader(ucd_content* ucd, const char* filename, int* was_binary)
{
    ucd_context c;
    int i, j, k;
    int *int_buffer1, *int_buffer2;

    /* header */
    ucd_reader_open(&c, filename);

    /* nodes and cells */
    ucd->ld_nlist = 8; /* hex */
    ucd->node_id = malloc(c.num_nodes * sizeof(*ucd->node_id));
    ucd->node_x = malloc(c.num_nodes * sizeof(*ucd->node_x));
    ucd->node_y = malloc(c.num_nodes * sizeof(*ucd->node_y));
    ucd->node_z = malloc(c.num_nodes * sizeof(*ucd->node_z));
    ucd->cell_id = malloc(c.num_cells * sizeof(*ucd->cell_id));
    ucd->cell_mat_id = malloc(c.num_cells * sizeof(*ucd->cell_mat_id));
    ucd->cell_type = malloc(c.num_cells * sizeof(*ucd->cell_type));
    ucd->cell_nlist = malloc(ucd->ld_nlist * c.num_cells * sizeof(*ucd->cell_nlist));

    int_buffer1 = malloc(4 * c.num_cells * sizeof(*int_buffer1));
    if (c.is_binary) {
        int_buffer2 = malloc(c.num_nlist * sizeof(*int_buffer2));
    } else {
        int_buffer2 = ucd->cell_nlist;
    }
    ucd_read_nodes_and_cells(&c,
            ucd->node_id, ucd->node_x, ucd->node_y, ucd->node_z,
            int_buffer1, int_buffer2, ucd->ld_nlist);

    k = 0;
    for (i = 0; i < c.num_cells; ++i) {
        ucd->cell_id[i] = int_buffer1[4*i];
        ucd->cell_mat_id[i] = int_buffer1[4*i+1];
        ucd->cell_type[i] = int_buffer1[4*i+2];
        if (c.is_binary) {
            for (j = 0; j < int_buffer1[4*i+3]; ++j) {
                ucd->cell_nlist[ucd->ld_nlist * i + j] = int_buffer2[k++];
            }
        }
    }
    free(int_buffer1);
    if (c.is_binary) {
        free(int_buffer2);
    }

    /* node data */
    if (c.num_ndata > 0) {
        ucd->ndata = malloc(sizeof(*ucd->ndata));
        ucd->ndata->num_rows = c.num_nodes;
        ucd->ndata->num_data = c.num_ndata;
        _ucd_simple_reader_sub(&c, ucd->ndata);
    } else {
        ucd->ndata = NULL;
    }

    /* cell data */
    if (c.num_cdata > 0) {
        ucd->cdata = malloc(sizeof(*ucd->cdata));
        ucd->cdata->num_rows = c.num_cells;
        ucd->cdata->num_data = c.num_cdata;
        _ucd_simple_reader_sub(&c, ucd->cdata);
    } else {
        ucd->cdata = NULL;
    }

    ucd->num_nodes = c.num_nodes;
    ucd->num_cells = c.num_cells;

    ucd_close(&c);

    if (was_binary != NULL) {
        *was_binary = c.is_binary;
    }

    return EXIT_SUCCESS;
}


int ucd_reader_open(ucd_context* c, const char* filename)
{
    c->_fp = fopen(filename, "rb");
    if (c->_fp == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", __func__, filename);
        return EXIT_FAILURE;
    }

    if (getc(c->_fp) == UCD_MAGIC_NUMBER) {
        c->is_binary = 1;
        fread(&c->num_nodes, sizeof(int), 1, c->_fp);
        fread(&c->num_cells, sizeof(int), 1, c->_fp);
        fread(&c->num_ndata, sizeof(int), 1, c->_fp);
        fread(&c->num_cdata, sizeof(int), 1, c->_fp);
        fseek(c->_fp, sizeof(int), SEEK_CUR); /* skip mdata */
        fread(&c->num_nlist, sizeof(int), 1, c->_fp);

#if 0
        /* TODO check file size */
        expected_filesize = ucd_binary_filesize(c);
        if (actual_filesize != expected_filesize) {
            fclose(c->_fp);
            fprintf(stderr, "%s: wrong file size (byte order issue?)\n", __func__);
            return EXIT_FAILURE;
        }
#endif
    } else {
        fclose(c->_fp);

        c->_fp = fopen(filename, "r");
        if (c->_fp == NULL) {
            fprintf(stderr, "%s cannot open %s\n", __func__, filename);
            return EXIT_FAILURE;
        }

        fscanf(c->_fp, "%d %d %d %d",
                &c->num_nodes, &c->num_cells, &c->num_ndata, &c->num_cdata);
        _ucd_ignore_lines(c, 1); /* skip mdata */

        c->is_binary = 0;
    }

    c->_nc = 0;
    return ferror(c->_fp);
}


int ucd_read_nodes_and_cells(ucd_context* c,
        int* node_id, float* x, float* y, float* z,
        int* cells, int* nlist, int ld_nlist)
{
    int i, j;
    char cell_type[8];

    if (c->is_binary) {
        if (cells != NULL) {
            fread(cells, sizeof(int), 4 * c->num_cells, c->_fp);
        } else {
            fseek(c->_fp, 4 * c->num_cells * sizeof(int), SEEK_CUR);
        }
        if (nlist != NULL) {
            fread(nlist, sizeof(int), c->num_nlist, c->_fp);
        } else {
            fseek(c->_fp, c->num_nlist * sizeof(int), SEEK_CUR);
        }
        if (node_id != NULL) {
            for (i = 0; i < c->num_nodes; ++i) {
                node_id[i] = i + 1;
            }
        }
        if (x != NULL && y != NULL && z != NULL) {
            fread(x, sizeof(float), c->num_nodes, c->_fp);
            fread(y, sizeof(float), c->num_nodes, c->_fp);
            fread(z, sizeof(float), c->num_nodes, c->_fp);
        } else {
            fseek(c->_fp, 3 * c->num_nodes * sizeof(float), SEEK_CUR);
        }
    } else {
        if (node_id != NULL && x != NULL && y != NULL && z != NULL) {
            for (i = 0; i < c->num_nodes; ++i) {
                fscanf(c->_fp, "%d %f %f %f", &node_id[i], &x[i], &y[i], &z[i]);
            }
        } else {
            _ucd_ignore_lines(c, c->num_nodes);
        }
        if (cells != NULL) {
            for (i = 0; i < c->num_cells; ++i) {
                fscanf(c->_fp, "%d %d %s",
                        &cells[4 * i], &cells[4 * i + 1], cell_type);
                cells[4 * i + 2] = ucd_cell_type_number(cell_type);
                cells[4 * i + 3] = ucd_cell_nlist_size(cells[4 * i + 2]);
                if (nlist != NULL) {
                    for (j = 0; j < cells[4 * i + 3]; ++j) {
                        fscanf(c->_fp, "%d", &nlist[ld_nlist * i + j]);
                    }
                } else {
                    _ucd_ignore_lines(c, 1);
                }
            }
        } else {
            _ucd_ignore_lines(c, c->num_cells);
        }
    }
    return ferror(c->_fp);
}


int ucd_read_data_header(ucd_context* c,
        int* num_comp, int* components, char* labels, char* units)
{
    const int text_field_size = 1024;

    int num_data, i;
    char *anchor_l, *anchor_u;

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
        if (labels != NULL) {
            fread(labels, sizeof(char), text_field_size, c->_fp);
            for (i = 0; i < text_field_size; ++i) {
                if (labels[i] == '.') {
                    labels[i] = '\0';
                }
            }
        } else {
            fseek(c->_fp, text_field_size, SEEK_CUR);
        }

        if (units != NULL) {
            fread(units, sizeof(char), text_field_size, c->_fp);
            for (i = 0; i < text_field_size; ++i) {
                if (units[i] == '.') {
                    units[i] = '\0';
                }
            }
        } else {
            fseek(c->_fp, text_field_size, SEEK_CUR);
        }

        fread(num_comp, sizeof(int), 1, c->_fp);

        if (components != NULL) {
            fread(components, sizeof(int), num_data, c->_fp);
        } else {
            fseek(c->_fp, num_data * sizeof(int), SEEK_CUR);
        }
    } else {
        fscanf(c->_fp, "%d", num_comp);
        if (components != NULL) {
            for (i = 0; i < *num_comp; ++i) {
                fscanf(c->_fp, "%d", &components[i]);
            }
        }
        _ucd_ignore_lines(c, 1);
        if (labels != NULL && units != NULL) {
            anchor_l = labels;
            anchor_u = units;
            for (i = 0; i < *num_comp; ++i) {
                fscanf(c->_fp, "%[^,],%s", anchor_l, anchor_u);
                anchor_l[strlen(anchor_l)] = '\0';
                anchor_u[strlen(anchor_u)] = '\0';
                anchor_l += strlen(anchor_l) + 1;
                anchor_u += strlen(anchor_u) + 1;
                _ucd_ignore_lines(c, 1);
            }
        } else {
            _ucd_ignore_lines(c, *num_comp);
        }
    }

    return ferror(c->_fp);
}


int ucd_read_data_minmax(ucd_context* c, float* minima, float* maxima)
{
    int num_data = c->_nc == 1 ? c->num_ndata : c->num_cdata;

    if (!c->is_binary) {
        fprintf(stderr, "%s: there are no such fields in ascii format\n", __func__);
        return EXIT_FAILURE;
    }

    if (minima != NULL) {
        fread(minima, sizeof(float), num_data, c->_fp);
    } else {
        fseek(c->_fp, num_data * sizeof(float), SEEK_CUR);
    }
    if (maxima != NULL) {
        fread(maxima, sizeof(float), num_data, c->_fp);
    } else {
        fseek(c->_fp, num_data * sizeof(float), SEEK_CUR);
    }
    return ferror(c->_fp);
}


int ucd_read_data_ascii(ucd_context* c, int* ids, float* data)
{
    int i, j;
    int num_rows = c->_nc == 1 ? c->num_nodes : c->num_cells;
    int num_data = c->_nc == 1 ? c->num_ndata : c->num_cdata;

    if (c->is_binary) {
        fprintf(stderr, "%s: it is binary format\n", __func__);
        return EXIT_FAILURE;
    }

    if (ids != NULL && data != NULL) {
        for (i = 0; i < num_rows; ++i) {
            fscanf(c->_fp, "%d", &ids[i]);
            for (j = 0; j < num_data; ++j) {
                fscanf(c->_fp, "%f", &data[i * num_data + j]);
            }
            _ucd_ignore_lines(c, 1);
        }
    } else {
        _ucd_ignore_lines(c, num_rows);
    }
    return ferror(c->_fp);
}


int ucd_read_data_binary(ucd_context* c, int component_size, float* data)
{
    int num_rows = c->_nc == 1 ? c->num_nodes : c->num_cells;

    if (!c->is_binary) {
        fprintf(stderr, "%s: it is ascii format\n", __func__);
        return EXIT_FAILURE;
    }

    if (data != NULL) {
        fread(data, sizeof(float), component_size * num_rows, c->_fp);
    } else {
        fseek(c->_fp, component_size * num_rows * sizeof(float), SEEK_CUR);
    }
    return ferror(c->_fp);
}


int ucd_read_data_active_list(ucd_context* c, int* active_list)
{
    int num_data = c->_nc == 1 ? c->num_ndata : c->num_cdata;

    if (!c->is_binary) {
        fprintf(stderr, "%s: it is ascii format\n", __func__);
        return EXIT_FAILURE;
    }

    if (active_list != NULL) {
        fread(active_list, sizeof(int), num_data, c->_fp);
    } else {
        fseek(c->_fp, num_data * sizeof(int), SEEK_CUR);
    }
    return ferror(c->_fp);
}
