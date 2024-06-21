/* html.h
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
#include "params.h"
#include "md.h"

/*
 * @HTMLFile
 */
typedef FILE HTMLFile;

typedef enum {
  HTML_TAG_H1,
  HTML_TAG_H2,
  HTML_TAG_H3,
  HTML_TAG_LI,
  HTML_TAG_BLOCKQUOTE,
  HTML_TAG_CODE_BLOCK_START,
  HTML_TAG_CODE_BLOCK_END,
  HTML_TAG_CODE_BLOCK_LINE,
  HTML_TAG_NONE,
  HTML_TAG_NEWLINE,
} HTMLTag;

typedef struct HTMLUnit {
  HTMLTag tag;
  char *content;
  char *uri;
} HTMLUnit;

typedef struct HTML {
  char *file_name;
  char *title;

  /* options */
  bool document;

  /* content */
  uint n_lines;
  HTMLUnit **html;
} HTML;


HTML *html_from_md (MD     *md,
                    Params *params);
void  html_free    (HTML *html);
void  flush_html   (HTML *html);
