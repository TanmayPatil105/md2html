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
#include "xml.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/* Number of languages supported */
#define N_LANGS LANG_NONE

/* keyword maximum string size */
#define MAX_KEYWORD_STR_SIZE 100

/* RGB color for highlighting a string
 *
 * Eg. #C061CB */
#define MAX_ELEMENT_RGB_COLOR  8

/*
 * Utility macros
 */
#define ARRAY_SIZE(arr) \
        sizeof (arr) / sizeof (arr[0])

/* Keywords */

struct keyword {
  const char *str;
  const char *color;
};

struct keywords_set {
  const char *prev_allowed;
  const char *next_allowed;

  struct keyword *keywords;
};

/**
 * C programming language
 **/

#define STRING_TOKEN(c) (c == '\"')

#define COMMENT_TOKEN(ptr) \
        (ptr[0] == '/' &&  \
         ptr[1] && ptr[1] == '*')

static struct keywords_set *c_keywords[4] = {
  [0] = & (struct keywords_set) {
    NULL,
    NULL,
    (struct keyword[]) {
      { "#include",   "#E91E63" },
      { "#define ",   "#E91E63" },
      { NULL, NULL }
    }
  },
  [1] = & (struct keywords_set) {
    NULL,
    NULL,
    (struct keyword[]) {
      { "for",       "#D84315" },
      { "while",     "#D84315" },
      { "do",        "#D84315" },
      { "break",     "#D84315" },
      { "if",        "#D84315" },
      { "else",      "#D84315" },
      { "switch",    "#D84315" },
      { "continue",  "#D84315" },
      { "return",    "#D84315" },
      { NULL, NULL }
    }
  },
  [2] = & (struct keywords_set) {
    NULL,
    NULL,
    (struct keyword[]) {
      { "int",       "#0000bb" },
      { "char",      "#0000bb" },
      { "float",     "#0000bb" },
      { "double",    "#0000bb" },
      { "long",      "#0000bb" },
      { "short",     "#0000bb" },
      { "unsigned",  "#0000bb" },
      { "signed",    "#0000bb" },
      { "void",      "#0000bb" },
      { NULL, NULL }
    }
	},
  [3] = & (struct keywords_set) {
     NULL,
     NULL,
     (struct keyword[]) {
      { "static",    "#0000bb" },
      { "struct",    "#0000bb" },
      { "union ",    "#0000bb" },
      { "enum",      "#0000bb" },
      { "sizeof",    "#D84315" },
      { "typedef",   "#D84315" },
      { "enum",      "#D84315" },
      { NULL, NULL }
    }
  }
};


static char *
highlight_keywords (char                 *codeblk,
                    struct keywords_set **set,
                    int                   n_keyword_types)
{
  int size = 1000;
  int count = 0;
  char *highlighted = NULL;
  char *ptr = NULL;
  struct keyword string;

  highlighted = malloc (sizeof (char) * size);

  ptr = codeblk;

  while (*ptr != '\0')
    {
      int i;
      struct keyword *match = NULL;
      char buf[300] = { 0 }; /* FIXME */
      bool advance_ptr = true;

      if (count == size - 1)
        {
          size <<= 2;

          highlighted = realloc (highlighted, size);
        }

      if (STRING_TOKEN (*ptr))
        {
          char *str_start, *needle;

          str_start = ptr;
          needle = str_start;

          do {
            needle = strchr (needle + 1, '\"');
            /* skip escape sequences */
            if (needle && * (needle - 1) !=  '\\')
               break;

          } while (needle != NULL);

          if (needle != NULL)
            {
              size_t size;

              size = needle - str_start + 1;

              xml_sanitize_strcpy (buf, str_start, size);
              string.str = buf;

              match = &string;
              ptr += size;
              advance_ptr = false;
           }

          string.color = "#6A1B9A";
        }
      else if (COMMENT_TOKEN (ptr))
        {
          char *str_start, *needle;

          str_start = ptr;
          needle = str_start;

          do {
            needle = strstr (needle + 1, "*/");
            /* skip escape sequences */
            if (needle && * (needle - 1) !=  '\\')
               break;

          } while (needle != NULL);

          if (needle != NULL)
            {
              size_t size;

              size = needle - str_start + 1;

              xml_sanitize_strcpy (buf, str_start, size);
              string.str = buf;

              match = &string;
              ptr += size;
              advance_ptr = false;
           }

          string.color = "#006400";
        }
      else if (isspace (*ptr))
        {
          highlighted[count++] = *ptr++;
          continue;
        }

      for (i = 0; i < n_keyword_types; i++)
        {
          struct keywords_set *type_set;
          struct keyword *type;

          type_set = set[i];
          type = set[i]->keywords;

          for (int j = 0; type[j].str != NULL; j++)
            {
              const char *keyword;

              keyword = type[j].str;

              if (strncmp (ptr, keyword, strlen (keyword)) == 0)
                {
                  match = &type[j];
                  break;
                }
            }
        }

      if (match != NULL)
        {
          char *cpy, *org;
          const char *strs[5] = {
            "<font color=\"", NULL, "\">",
            NULL,
            "</font>"
          };

          strs[1] = match->color;
          strs[3] = match->str;

          cpy = &highlighted[count];
          org = cpy;

          for (int i = 0; i < 5; i++)
            {
              strcpy (cpy, strs[i]);
              cpy += strlen (strs[i]);
              count += strlen (strs[i]);
            }

          if (advance_ptr)
            ptr += strlen (match->str);
        }
      else /* Not a keyword */
        {
          const char *str;

          str = xml_char_replace (*ptr);

          if (str != NULL)
            {
              char *cpy = NULL;

              cpy = &highlighted[count];
              strcpy (cpy, str);

              count += strlen (str);
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
  int n_types;

  switch (lang)
    {
      case LANG_C:
        n_types = ARRAY_SIZE (c_keywords);
        highlighted = highlight_keywords (codeblk,
                                          c_keywords, n_types);
        break;
      default:
        highlighted = NULL;
    }

  return highlighted;
}
