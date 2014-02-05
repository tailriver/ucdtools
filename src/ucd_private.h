/**
 * @file ucd_private.h
 * @brief Private header.
 * @author Shinsuke Ogawa
 * @date 2014
 */

#pragma once
#include "ucd.h"
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/** @cond */
#ifndef __func__
#define __func__ __FUNCTION__
#endif
/** @endcond */

/**
 * The magic number of UCD binary file format.
 */
#define UCD_MAGIC_NUMBER 0x07

/**
 * Length of label and unit fields in data section.
 */
#define UCD_TEXT_FIELD_SIZE 1024


static void ucd_data_dimension(
        const ucd_context* c, int* num_rows, int* num_data)
{
    if (num_rows != NULL) {
        *num_rows = c->_nc == 1 ? c->num_nodes : c->num_cells;
    }
    if (num_data != NULL) {
        *num_data = c->_nc == 1 ? c->num_ndata : c->num_cdata;
    }
}
