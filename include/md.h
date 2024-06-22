/* md.h
 *
 * Copyright 2024 Tanmay Patil <tanmaynpatil105@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */


#pragma once

#include <stdio.h>
#include "lang.h"

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
  UNIT_TYPE_QUOTE,            /* 4 */
  UNIT_TYPE_CODE_BLOCK_START, /* 5 */
  UNIT_TYPE_CODE_BLOCK_END,   /* 6 */
  UNIT_TYPE_CODE_BLOCK_LINE,  /* 7 */
  UNIT_TYPE_TEXT,             /* 8 */
  UNIT_TYPE_NONE,             /* 9 */
} UnitType;

typedef struct MDUnit{
  UnitType type;
  char *content;
  char *uri;

  /* For codeblocks */
  Lang lang;

  struct MDUnit *next;
} MDUnit;

typedef struct {
  uint n_lines;

  /* Linked List */
  MDUnit *elements;
} MD;


MD  *parse_md (MDFile *file);
void md_free (MD *md);
