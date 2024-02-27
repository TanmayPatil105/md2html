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
  UNIT_TYPE_H1,
  UNIT_TYPE_H2,
  UNIT_TYPE_H3,
  UNIT_TYPE_BULLET,
  UNIT_TYPE_TEXT,
  UNIT_TYPE_NONE,
} UnitType;

typedef struct MDUnit{
  UnitType type;
  char *content;
  struct MDUnit *next;
} MDUnit;

typedef struct {
  uint n_lines;

  /* Linked List */
  MDUnit *elements;
} MD;


MD  *parse_md (MDFile *file);
void md_free (MD *md);
