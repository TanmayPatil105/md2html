/* params.h
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

#include <stdbool.h>


typedef struct Params {
  char *i_file;     /* input MD file */

  char *o_file;     /* output HTML file; defaults to index.html */
  char *title;      /* HTML doc title */

  bool  inline_css;  /* add inline css */
  char *i_css_file;  /* set inline css equal to contents of .css file */

  bool version;      /* output version information */
  bool help;         /* display usage message */
  char *error;       /* error while parsing arguments */
} Params;


Params *params_parse (int     argc,
                      char   *argv[]);
void    params_free  (Params *params);
