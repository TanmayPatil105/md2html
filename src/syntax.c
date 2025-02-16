/* syntax.c
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

#include "lang.h"
#include "syntax.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


/* Number of languages supported */
#define N_LANGS LANG_NONE

/* keyword maximum string size */
#define MAX_KEYWORD_STR_SIZE 100

/* RGB color for highlighting a string
 *
 * Eg. #C061CB */
#define MAX_ELEMENT_RGB_COLOR  8


/* Keywords */

struct keyword {
  char str[MAX_KEYWORD_STR_SIZE];
  char color[MAX_ELEMENT_RGB_COLOR];
};

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

/**
 * C programming language
 **/
#define N_C_KEYWORDS 2

static struct keyword c_keywords[N_C_KEYWORDS] = {
  { "#include", "#5FD7FF" },
  { "return",  "#E9AD0C" }
};


static char *
highlight_keywords (char           *codeblk,
                    struct keyword  keywords[],
                    int             n_keywords)
{
  int size = 1000;
  int count = 0;
  char *highlighted = NULL;
  char *ptr = NULL;

  highlighted = malloc (sizeof (char) * size);

  ptr = codeblk;

  while (*ptr != '\0')
    {
      int i = n_keywords;

      if (count == size - 1)
        {
          size <<= 2;

          highlighted = realloc (highlighted, size);
        }

      if (isspace (*ptr))
        {
          highlighted[count++] = *ptr++;
          continue;
        }

      for (i = 0; i < n_keywords; i++)
        {
          char *keyword;

          keyword = keywords[i].str;
          if (strncmp (ptr, keyword, strlen (keyword)) == 0)
            break;
        }

      if (i < n_keywords)
        {
          char *cpy, *org;
          char *strs[5] = {
            "<font color=\"", NULL, "\">",
            NULL,
            "</font>"
          };

          strs[1] = keywords[i].color;
          strs[3] = keywords[i].str;

          cpy = &highlighted[count];
          org = cpy;

          for (int i = 0; i < 5; i++)
            {
              strcpy (cpy, strs[i]);
              cpy += strlen (strs[i]);
              count += strlen (strs[i]);
            }

          ptr += strlen (keywords[i].str);
        }
      else /* Not a keyword */
        {
          unsigned int index;

          index = get_char_index (*ptr);

          if (index != N_CHARS)
            {
              char *cpy = NULL;

              cpy = &highlighted[count];
              strcpy (cpy, chars[index].str);

              count += strlen (chars[index].str);
              ptr++;
            }
          else
            {
              highlighted[count++] = *ptr++;
            }
        }
    }

  highlighted[count] = '\0';

  return highlighted;
}

char *
syntax_highlight (char *codeblk,
                  Lang  lang)
{
  char *highlighted = NULL;

  switch (lang)
    {
      case LANG_C:
        highlighted = highlight_keywords (codeblk,
                                          c_keywords, N_C_KEYWORDS);
        break;
      default:
        highlighted = NULL;
    }

  return highlighted;
}
