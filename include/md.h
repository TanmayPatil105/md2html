/*
 * @file: md.h
 */

#pragma once

#include <stdio.h>

typedef unsigned int uint;

/*
 * @MDFile
 */
typedef FILE MDFile;

typedef enum {
  UNIT_TYPE_H1,               /* 0 */
  UNIT_TYPE_H2,               /* 1 */
  UNIT_TYPE_H3,               /* 2 */
  UNIT_TYPE_BULLET,           /* 3 */
  UNIT_TYPE_IMAGE,            /* 4 */
  UNIT_TYPE_QUOTE,            /* 5 */
  UNIT_TYPE_CODE_BLOCK_START, /* 6 */
  UNIT_TYPE_CODE_BLOCK_END,   /* 7 */
  UNIT_TYPE_CODE_BLOCK_LINE,  /* 8 */
  UNIT_TYPE_TEXT,             /* 9 */
  UNIT_TYPE_NONE,             /* 10 */
} UnitType;

typedef struct MDUnit{
  UnitType type;
  char *content;
  char *uri;
  struct MDUnit *next;
} MDUnit;

typedef struct {
  uint n_lines;

  /* Linked List */
  MDUnit *elements;
} MD;


MD  *parse_md (MDFile *file);
void md_free (MD *md);
