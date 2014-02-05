/**
 * @file ucd.h
 * @brief Headers of the ucdtool.
 * @author Shinsuke Ogawa
 * @date 2014
 */

#pragma once

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4820)
#endif

#include <stdio.h>

#ifdef _WIN32
#pragma warning(pop)
#endif

/**
 * @struct ucd_context
 * @brief ...
 *
 */
typedef struct {
    /**
     * An indicator which represents the UCD file format.
     *
     * It is zero if it is ASCII format, otherwise not zero. 
     * When reading a file, it is overwriten in ucd_reader_open().
     * When writing to a file, user should specify preferable value.
     */
    int is_binary;

    /** The number of nodes. */
    int num_nodes;

    /** The number of cells. */
    int num_cells;

    /** The number of node data. */
    int num_ndata;

    /** The number of cell data. */
    int num_cdata;

    /**
     * The number (length) of node list.
     * The member is only referenced in binary format.
     */
    int num_nlist;

    /** @private */
    FILE* _fp;

    /** @private */
    int _nc;
} ucd_context;


typedef struct {
    /** The number of rows (i.e. nodes or cells). */
    int num_rows;

    /** The number of (node or cell) data. */
    int num_data;

    /** The number of components. */
    int num_comp;

    /**
     * Sizes of components.
     * The valid size is #num_comp (actual size may be more).  The total
     * is should be equal to #num_data.
     */
    int* components;

    /**
     * Label of the components.
     * It is splitted by null character in an array.
     */
    char labels[1024];

    /**
     * Unit of the components.
     * It is splitted by null carachter in an array.
     */
    char units[1024];

    /**
     * Minimum values for each data.
     * The size is #num_data.
     */
    float* minima;

    /**
     * Maximum values for each data.
     * The size is #num_data.
     */
    float* maxima;

    /**
     * Node or cell indeces.
     * The size is #num_rows.
     */
    int* row_id;

    /**
     * A data array.
     * It is column-major matrix which the size is #num_rows (as row)
     * times #num_data (as column).  If you access to (i, j) components,
     * please describe [i * num_data + j].
     * */
    float* data;
} ucd_data;


typedef struct {
    /** The number of nodes. */
    int num_nodes;

    /** The number of cells. */
    int num_cells;

    /** The leading dimension size of the #cell_nlist. */
    int ld_nlist;

    /**
     * An array which stores node indeces.
     * The size is #num_nodes.
     */
    int* node_id;

    /**
     * An array which stores x coordinate of nodes.
     * The size is #num_nodes.
     */
    float* node_x;

    /**
     * An array which stores y coordinate of nodes.
     * The size is #num_nodes.
     */
    float* node_y;

    /**
     * An array which stores z coordinate of nodes.
     * The size is #num_nodes.
     */
    float* node_z;

    /**
     * An array which stores cell indeces.
     * The size is #num_cells.
     */
    int* cell_id;

    /**
     * An array which stores material id of cells.
     * The size is #num_cells.
     */
    int* cell_mat_id;

    /**
     * An array which stores type (topology id) of cells.
     * The size is #num_cells.
     */
    int* cell_type;

    /**
     * An array which stores node (vertex) list for each cell.
     * The size is #ld_nlist times #num_cells.
     */
    int* cell_nlist;

    /**
     * A pointer to node data.
     * It is null if it has no data.
     */
    ucd_data* ndata;

    /**
     * A pointer to cell data.
     * It is null if it has no data.
     */
    ucd_data* cdata;
} ucd_content;

/**
 *
 * \param ucd A pointer to content.
 * \param filename A filename to read.
 * \param was_binary It returns ucd_context#is_binary unless NULL.
 * \return EXIT_SUCCESS if success.
 */
int ucd_simple_reader(ucd_content* ucd, const char* filename, int* was_binary);
int ucd_simple_writer(const ucd_content* ucd, const char* filename, int is_binary);
void ucd_simple_free(ucd_content* ucd);

int ucd_reader_open(ucd_context* c, const char* filename);
int ucd_read_nodes_and_cells(ucd_context* c,
        int* nodes, float* x, float* y, float* z,
        int* cells, int* nlist, int ld_nlist);
int ucd_read_data_header(ucd_context* c,
        int* num_comp, int* components, char* labels, char* units);
int ucd_read_data_minmax(ucd_context* c, float* minima, float* maxima);
int ucd_read_data_ascii(ucd_context* c, int* ids, float* data);
int ucd_read_data_binary(ucd_context* c,
        int component_size, float* data, int ld_data);
int ucd_read_data_active_list(ucd_context* ucd, int* active_list);

int ucd_writer_open(ucd_context* c, const char* filename);
int ucd_write_nodes_and_cells(ucd_context* c,
        const int* nodes, const float* x, const float* y, const float* z,
        const int* cells, const int* nlist, int ld_nlist);
int ucd_write_data_header(ucd_context* c,
        int num_comp, const int* components,
        const char* labels, const char* units);
int ucd_write_data_minmax(ucd_context* c,
        const float* minima, const float* maxima);
int ucd_write_data_ascii_1(ucd_context* c, int id, const float* data);
int ucd_write_data_ascii_n(ucd_context* c, const int* ids, const float* data);
int ucd_write_data_binary(ucd_context* ucd,
        int component_size, const float* data, int ld_data);
int ucd_write_data_active_list(ucd_context* c, const int* active_list);

int ucd_cell_nlist_size(int cell_type);
const char* ucd_cell_type_string(int num);
int ucd_cell_type_number(const char* str);
int ucd_binary_filesize(ucd_context* c);

int ucd_close(ucd_context* c);
