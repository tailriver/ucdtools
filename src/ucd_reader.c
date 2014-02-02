#include <float.h>
#include <string.h>
#include "ucd_private.h"

#ifdef _WIN32
#pragma warning(disable:4996)
#endif


void _ucd_ignore_lines(FILE* fp, int count) {
    while (count-- > 0) {
        while (getc(fp) != '\n');
    }
}


void _ucd_reader_simple_sub(ucd_context* c, ucd_data* d)
{
    int i, j, k, base_col;
    float* buffer;

    d->components = (int*) malloc(d->num_data * sizeof(int));
    d->minima = (float*) malloc(d->num_data * sizeof(float));
    d->maxima = (float*) malloc(d->num_data * sizeof(float));
    d->row_id = (int*) malloc(d->num_rows * sizeof(int));
    d->data = (float*) malloc(d->num_rows * d->num_data * sizeof(float));

    ucd_read_data_header(c, &d->num_comp, d->components, d->labels, d->units);

    if (c->is_binary) {
        ucd_read_data_minmax(c, d->minima, d->maxima);

        for (i = 0; i < d->num_rows; ++i) {
            d->row_id[i] = i + 1;
        }

        base_col = 0;
        for (i = 0; i < d->num_comp; ++i) {
            buffer = (float*) malloc(d->components[i] * d->num_rows * sizeof(float));
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


int ucd_reader_simple(ucd_content* ucd, const char* filename, int* was_binary)
{
    ucd_context c;
    int i, j, k;
    int *int_buffer1, *int_buffer2;

    /* header */
    ucd_reader_open(&c, filename);

    /* nodes and cells */
    ucd->ld_nlist = 8; /* hex */
    ucd->node_id = (int*) malloc(c.num_nodes * sizeof(int));
    ucd->node_x = (float*) malloc(c.num_nodes * sizeof(float));
    ucd->node_y = (float*) malloc(c.num_nodes * sizeof(float));
    ucd->node_z = (float*) malloc(c.num_nodes * sizeof(float));
    ucd->cell_id = (int*) malloc(c.num_cells * sizeof(int));
    ucd->cell_mat_id = (int*) malloc(c.num_cells * sizeof(int));
    ucd->cell_type = (int*) malloc(c.num_cells * sizeof(int));
    ucd->cell_nlist = (int*) malloc(ucd->ld_nlist * c.num_cells * sizeof(int));

    int_buffer1 = (int*) malloc(4 * c.num_cells * sizeof(int));
    if (c.is_binary) {
        int_buffer2 = (int*) malloc(c.num_nlist * sizeof(int));
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
        ucd->ndata = (ucd_data*) malloc(sizeof(ucd_data));
        ucd->ndata->num_rows = c.num_nodes;
        ucd->ndata->num_data = c.num_ndata;
        _ucd_reader_simple_sub(&c, ucd->ndata);
    } else {
        ucd->ndata = NULL;
    }

    /* cell data */
    if (c.num_cdata > 0) {
        ucd->cdata = (ucd_data*) malloc(sizeof(ucd_data));
        ucd->cdata->num_rows = c.num_cells;
        ucd->cdata->num_data = c.num_cdata;
        _ucd_reader_simple_sub(&c, ucd->cdata);
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


int ucd_reader_open(ucd_context* ucd, const char* filename)
{
    ucd->fp = fopen(filename, "rb");
    if (ucd->fp == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", __func__, filename);
        return EXIT_FAILURE;
    }

    if (getc(ucd->fp) == UCD_MAGIC_NUMBER) {
        ucd->is_binary = 1;
        fread(&ucd->num_nodes, sizeof(int), 1, ucd->fp);
        fread(&ucd->num_cells, sizeof(int), 1, ucd->fp);
        fread(&ucd->num_ndata, sizeof(int), 1, ucd->fp);
        fread(&ucd->num_cdata, sizeof(int), 1, ucd->fp);
        fseek(ucd->fp, sizeof(int), SEEK_CUR); /* skip mdata */
        fread(&ucd->num_nlist, sizeof(int), 1, ucd->fp);

#if 0
        /* TODO check file size */
        expected_filesize = ucd_binary_filesize(ucd);
        if (actual_filesize != expected_filesize) {
            fclose(ucd->fp);
            fprintf(stderr, "%s: wrong file size (byte order issue?)\n", __func__);
            return EXIT_FAILURE;
        }
#endif
    } else {
        fclose(ucd->fp);

        ucd->fp = fopen(filename, "r");
        if (ucd->fp == NULL) {
            fprintf(stderr, "%s cannot open %s\n", __func__, filename);
            return EXIT_FAILURE;
        }

        fscanf(ucd->fp, "%d %d %d %d",
                &ucd->num_nodes,
                &ucd->num_cells,
                &ucd->num_ndata,
                &ucd->num_cdata);
        _ucd_ignore_lines(ucd->fp, 1); /* skip mdata */

        ucd->is_binary = 0;
    }

    ucd->_nc = 0;
    return ferror(ucd->fp);
}


int ucd_read_nodes_and_cells(ucd_context* ucd,
        int* id, float* x, float* y, float* z, int* cells, int* nlist, int ld_nlist)
{
    int i, j;
    char cell_type[8];

    if (ucd->is_binary) {
        if (cells != NULL) {
            fread(cells, sizeof(int), 4 * ucd->num_cells, ucd->fp);
        } else {
            fseek(ucd->fp, 4 * ucd->num_cells * sizeof(int), SEEK_CUR);
        }
        if (nlist != NULL) {
            fread(nlist, sizeof(int), ucd->num_nlist, ucd->fp);
        } else {
            fseek(ucd->fp, ucd->num_nlist * sizeof(int), SEEK_CUR);
        }
        if (id != NULL) {
            for (i = 0; i < ucd->num_nodes; ++i) {
                id[i] = i + 1;
            }
        }
        if (x != NULL && y != NULL && z != NULL) {
            fread(x, sizeof(float), ucd->num_nodes, ucd->fp);
            fread(y, sizeof(float), ucd->num_nodes, ucd->fp);
            fread(z, sizeof(float), ucd->num_nodes, ucd->fp);
        } else {
            fseek(ucd->fp, 3 * ucd->num_nodes * sizeof(float), SEEK_CUR);
        }
    } else {
        if (id != NULL && x != NULL && y != NULL && z != NULL) {
            for (i = 0; i < ucd->num_nodes; ++i) {
                fscanf(ucd->fp, "%d %f %f %f", &id[i], &x[i], &y[i], &z[i]);
            }
        } else {
            _ucd_ignore_lines(ucd->fp, ucd->num_nodes);
        }
        if (cells != NULL) {
            for (i = 0; i < ucd->num_cells; ++i) {
                fscanf(ucd->fp, "%d %d %s",
                        &cells[4 * i], &cells[4 * i + 1], cell_type);
                cells[4 * i + 2] = ucd_cell_type_number(cell_type);
                cells[4 * i + 3] = ucd_cell_nlist_size(cells[4 * i + 2]);
                if (nlist != NULL) {
                    for (j = 0; j < cells[4 * i + 3]; ++j) {
                        fscanf(ucd->fp, "%d", &nlist[ld_nlist * i + j]);
                    }
                } else {
                    _ucd_ignore_lines(ucd->fp, 1);
                }
            }
        } else {
            _ucd_ignore_lines(ucd->fp, ucd->num_cells);
        }
    }
    return ferror(ucd->fp);
}


int ucd_read_data_header(ucd_context* ucd,
        int* num_comp, int* components, char* labels, char* units)
{
    const int text_field_size = 1024;

    int num_data, i;
    char *anchor_l, *anchor_u;

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
        if (labels != NULL) {
            fread(labels, sizeof(char), text_field_size, ucd->fp);
            for (i = 0; i < text_field_size; ++i) {
                if (labels[i] == '.') {
                    labels[i] = '\0';
                }
            }
        } else {
            fseek(ucd->fp, text_field_size, SEEK_CUR);
        }

        if (units != NULL) {
            fread(units, sizeof(char), text_field_size, ucd->fp);
            for (i = 0; i < text_field_size; ++i) {
                if (units[i] == '.') {
                    units[i] = '\0';
                }
            }
        } else {
            fseek(ucd->fp, text_field_size, SEEK_CUR);
        }

        fread(num_comp, sizeof(int), 1, ucd->fp);

        if (components != NULL) {
            fread(components, sizeof(int), num_data, ucd->fp);
        } else {
            fseek(ucd->fp, num_data * sizeof(int), SEEK_CUR);
        }
    } else {
        fscanf(ucd->fp, "%d", num_comp);
        if (components != NULL) {
            for (i = 0; i < *num_comp; ++i) {
                fscanf(ucd->fp, "%d", &components[i]);
            }
        }
        _ucd_ignore_lines(ucd->fp, 1);
        if (labels != NULL && units != NULL) {
            anchor_l = labels;
            anchor_u = units;
            for (i = 0; i < *num_comp; ++i) {
                fscanf(ucd->fp, "%[^,],%s", anchor_l, anchor_u);
                anchor_l[strlen(anchor_l)] = '\0';
                anchor_u[strlen(anchor_u)] = '\0';
                anchor_l += strlen(anchor_l) + 1;
                anchor_u += strlen(anchor_u) + 1;
                _ucd_ignore_lines(ucd->fp, 1);
            }
        } else {
            _ucd_ignore_lines(ucd->fp, *num_comp);
        }
    }

    return ferror(ucd->fp);
}


int ucd_read_data_minmax(ucd_context* ucd, float* minima, float* maxima)
{
    int num_data = ucd->_nc == 1 ? ucd->num_ndata : ucd->num_cdata;

    if (!ucd->is_binary) {
        fprintf(stderr, "%s: there are no such fields in ascii format\n", __func__);
        return EXIT_FAILURE;
    }

    if (minima != NULL) {
        fread(minima, sizeof(float), num_data, ucd->fp);
    } else {
        fseek(ucd->fp, num_data * sizeof(float), SEEK_CUR);
    }
    if (maxima != NULL) {
        fread(maxima, sizeof(float), num_data, ucd->fp);
    } else {
        fseek(ucd->fp, num_data * sizeof(float), SEEK_CUR);
    }
    return ferror(ucd->fp);
}


int ucd_read_data_ascii(ucd_context* ucd, int* ids, float* data)
{
    int i, j;
    int num_rows = ucd->_nc == 1 ? ucd->num_nodes : ucd->num_cells;
    int num_data = ucd->_nc == 1 ? ucd->num_ndata : ucd->num_cdata;

    if (ucd->is_binary) {
        fprintf(stderr, "%s: it is binary format\n", __func__);
        return EXIT_FAILURE;
    }

    if (ids != NULL && data != NULL) {
        for (i = 0; i < num_rows; ++i) {
            fscanf(ucd->fp, "%d", &ids[i]);
            for (j = 0; j < num_data; ++j) {
                fscanf(ucd->fp, "%f", &data[i * num_data + j]);
            }
            _ucd_ignore_lines(ucd->fp, 1);
        }
    } else {
        _ucd_ignore_lines(ucd->fp, num_rows);
    }
    return ferror(ucd->fp);
}


int ucd_read_data_binary(ucd_context* ucd, int component_size, float* data)
{
    int num_rows = ucd->_nc == 1 ? ucd->num_nodes : ucd->num_cells;

    if (!ucd->is_binary) {
        fprintf(stderr, "%s: it is ascii format\n", __func__);
        return EXIT_FAILURE;
    }

    if (data != NULL) {
        fread(data, sizeof(float), component_size * num_rows, ucd->fp);
    } else {
        fseek(ucd->fp, component_size * num_rows * sizeof(float), SEEK_CUR);
    }
    return ferror(ucd->fp);
}


int ucd_read_data_active_list(ucd_context* ucd, int* active_list)
{
    int num_data = ucd->_nc == 1 ? ucd->num_ndata : ucd->num_cdata;

    if (!ucd->is_binary) {
        fprintf(stderr, "%s: it is ascii format\n", __func__);
        return EXIT_FAILURE;
    }

    if (active_list != NULL) {
        fread(active_list, sizeof(int), num_data, ucd->fp);
    } else {
        fseek(ucd->fp, num_data * sizeof(int), SEEK_CUR);
    }
    return ferror(ucd->fp);
}
