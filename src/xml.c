/* xml.c
 *
 * Copyright 2025 Tanmay Patil <tanmaynpatil105@gmail.com>
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

#include "xml.h"

#include <string.h>

enum
{
  CHAR_LESS_THAN = 0,
  CHAR_GREATER_THAN,
  CHAR_AMPERSAND,
  CHAR_DOUBLE_QUOTE,
  CHAR_SINGLE_QUOTE,
  N_CHARS
};

static struct {
  char c;
  char str[20];
} chars[N_CHARS] = {
  { '<',  "&lt;" },
  { '>',  "&gt;" },
  { '&',  "&amp;" },
  { '"',  "&quot;" },
  { '\'', "&apos;" },
};

static inline unsigned int
get_char_index (char c)
{
  for (unsigned int i = 0; i < N_CHARS; i++)
    {
      if (c == chars[i].c)
        return i;
    }

  return N_CHARS;
}

const char *
xml_char_replace (char c)
{
  const char *ret;
  unsigned int index;

  index = get_char_index (c);

  if (index != N_CHARS)
    {
      ret = chars[index].str;
    }
  else
    {
      ret = NULL;
    }

  return ret;
}

void
xml_sanitize_strcpy (char   *dest,
                     char   *src,
                     size_t  n)
{
  size_t count = 0;
  char *ptr = dest;

  while (count < n)
    {
      const char *repl;

      repl = xml_char_replace (*src);

      if (repl != NULL)
        {
          ptr = stpcpy (ptr, repl);
        }
      else
        {
          *ptr = *src;
          ptr++;
        }

      src++;
      count++;
    }
}

