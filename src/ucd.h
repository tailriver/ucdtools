#pragma once

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4820)
#endif

#include <stdio.h>

#ifdef _WIN32
#pragma warning(pop)
#endif


typedef struct {
    int is_binary;
    int num_nodes;
    int num_cells;
    int num_ndata;
    int num_cdata;
    int num_nlist; /* binary only */
    FILE* fp;
    int _nc;
} ucd_context;


typedef struct {
    int num_rows;
    int num_data;
    int num_comp;
    int* components;
    char labels[1024];
    char units[1024];
    float* minima;
    float* maxima;
    int* row_id;
    float* data;
} ucd_data;


typedef struct {
    int num_nodes;
    int num_cells;
    int ld_nlist;
    int* node_id;
    float* node_x;
    float* node_y;
    float* node_z;
    int* cell_id;
    int* cell_mat_id;
    int* cell_type;
    int* cell_nlist;
    ucd_data* ndata;
    ucd_data* cdata;
} ucd_content;

int ucd_reader_simple(ucd_content* ucd, const char* filename, int* was_binary);
int ucd_writer_simple(const ucd_content* ucd, const char* filename, int is_binary);

int ucd_reader_open(ucd_context* ucd, const char* filename);
int ucd_read_nodes_and_cells(ucd_context* ucd,
        int* nodes, float* x, float* y, float* z,
        int* cells, int* nlist, int ld_nlist);
int ucd_read_data_header(ucd_context* ucd,
        int* num_comp, int* components, char* labels, char* units);
int ucd_read_data_minmax(ucd_context* ucd, float* minima, float* maxima);
int ucd_read_data_ascii(ucd_context* ucd, int* ids, float* data);
int ucd_read_data_binary(ucd_context* ucd, int component_size, float* data);
int ucd_read_data_active_list(ucd_context* ucd, int* active_list);

int ucd_writer_open(ucd_context* ucd, const char* filename);
int ucd_write_nodes_and_cells(ucd_context* ucd,
        const int* nodes, const float* x, const float* y, const float* z,
        const int* cells, const int* nlist, int ld_nlist);
int ucd_write_data_header(ucd_context* ucd,
        int num_comp, const int* components, const char* labels, const char* units);
int ucd_write_data_minmax(ucd_context* ucd, const float* minima, const float* maxima);
int ucd_write_data_ascii_1(ucd_context* ucd, int id, const float* data);
int ucd_write_data_ascii_n(ucd_context* ucd, const int* ids, const float* data);
int ucd_write_data_binary(ucd_context* ucd,
        int component_size, const float* data, int need_transpose);
int ucd_write_data_active_list(ucd_context* ucd, const int* active_list);

int ucd_cell_nlist_size(int cell_type);
const char* ucd_cell_type_string(int num);
int ucd_cell_type_number(const char* str);
int ucd_binary_filesize(ucd_context* ucd);

int ucd_close(ucd_context* ucd);
