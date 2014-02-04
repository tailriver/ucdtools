#pragma once
#include "ucd.h"
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define UCD_MAGIC_NUMBER 0x07
#define UCD_TEXT_FIELD_SIZE 1024
