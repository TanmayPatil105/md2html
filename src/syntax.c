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

#define NULL_CHAR '\0'

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
  struct keyword *keywords;
};

const int font_tag_size = 30; /* pre-calculated */

/**
 * C programming language
 **/

#define STRING_TOKEN(c) (c == '\"')

#define CHAR_TOKEN(c) (c == '\'')

#define STRING_CHAR_TOKEN(c) \
        (STRING_TOKEN (c) || \
         CHAR_TOKEN (c))

#define SINGLE_LINE_COMMENT_TOKEN(ptr) \
        (ptr[0] == '/' &&  \
         ptr[1] && ptr[1] == '/')

#define MULTI_LINE_COMMENT_TOKEN(ptr) \
        (ptr[0] == '/' &&  \
         ptr[1] && ptr[1] == '*')

#define NUMBER_TOKEN(c) \
        (c >= '0' && c <= '9')

#define BINARY_TOKEN(c) \
        (c == '0' || c == '1')

#define HEX_TOKEN(c) \
        (NUMBER_TOKEN (c)       || \
         (c >= 'a' && c <= 'f') || \
         (c >= 'A' && c <= 'F'))

static struct keywords_set *c_keywords[4] = {
  [0] = & (struct keywords_set) {
    (struct keyword[]) {
      { "#include",   "#E91E63" },
      { "#define",    "#E91E63" },
      { "#if",        "#E91E63" },
      { "#ifndef",    "#E91E63" },
      { "#endif",     "#E91E63" },
      { NULL, NULL }
    }
  },
  [1] = & (struct keywords_set) {
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
      { "case",      "#D84315" },
      { "default",   "#D84315" },
      { NULL, NULL }
    }
  },
  [2] = & (struct keywords_set) {
    (struct keyword[]) {
      { "int",       "#0000bb" },
      { "char",      "#0000bb" },
      { "float",     "#0000bb" },
      { "double",    "#0000bb" },
      { "long",      "#0000bb" },
      { "short",     "#0000bb" },
      { "unsigned",  "#0000bb" },
      { "bool",      "#0000bb" },
      { "signed",    "#0000bb" },
      { "static",    "#0000bb" },
      { "const",     "#0000bb" },
      { "struct",    "#0000bb" },
      { "void",      "#0000bb" },
      { "size_t",    "#0000bb" },
      { NULL, NULL }
    }
	},
  [3] = & (struct keywords_set) {
     (struct keyword[]) {
      { "union ",    "#0000bb" },
      { "enum",      "#0000bb" },
      { "sizeof",    "#D84315" },
      { "typedef",   "#D84315" },
      { "enum",      "#D84315" },
      { "true",      "#6A1B9A" },
      { "false",     "#6A1B9A" },
      { "NULL",      "#6A1B9A" },
      { NULL, NULL }
    }
  }
};

static bool
__isalnum (char c)
{
  if (c == '_')
    return true;

  return isalnum (c);
}

static bool
__isoperator (char c)
{
  bool ret = false;
  char operators[] = "+-*/%&|^!=<>?:()[]";

  for (int i = 0; operators[i] != '\0'; i++)
    {
      if (c == operators[i])
        {
          ret = true;
          break;
        }
    }

  return ret;
}

static bool
isescape_sequence (char *ptr)
{
  if (* (ptr) != '\\')
    return false;
  else if (* (ptr - 1) == '\\')
    return false;

  return true;
}

static size_t
handle_binary (char *str)
{
  size_t size = 0;

  while (*str != '\0')
    {
      if (!BINARY_TOKEN (*str))
        {
          if (isspace (*str) || *str == ';'
              || __isoperator (*str))
            break;
          else
            return 0;
        }

      str++;
      size++;
    }

  return size;
}

static size_t
handle_hex (char *str)
{
  size_t size = 0;

  while (*str != '\0')
    {
      if (!HEX_TOKEN (*str))
        {
          if (isspace (*str) || *str == ';'
              || __isoperator (*str))
            break;
          else
            return 0;
        }

      str++;
      size++;
    }

  return size;
}

static size_t
get_number_length (char *str)
{
  /* we have already processed first character */
  size_t size = 1;

  if (*str == '0')
    {
      size_t ret = -1;

      if (* (str + 1) == 'b')
        {
          ret = handle_binary (str + 2);
        }
      else if (* (str + 1) == 'x')
        {
          ret = handle_hex (str + 2);
        }

      if (ret != -1UL)
        return ret + 2;
    }

  str++;

  while (*str != '\0')
    {
      if (!NUMBER_TOKEN (*str))
        {
          if (isspace (*str) || *str == ';'
              || __isoperator (*str))
            break;
          else
            return 0;
        }

      str++;
      size++;
    }

  return size;
}

static size_t
extract_number (char   *str,
                char   *buf)
{
  size_t size;

  size = get_number_length (str);

  if (size != 0)
    {
      strncpy (buf, str, size);
    }

  return size;
}

static size_t
extract_text (char   *start,
              char   *buf,
              size_t  buf_len,
              char   *pattern)
{
  size_t size = 0;
  char *needle;

  needle = start;

  do {
    needle = strstr (needle + 1, pattern);

    /* skip escape sequences */
    if (needle && !isescape_sequence (needle - 1))
       break;

  } while (needle != NULL);

  if (needle != NULL)
    {
      size = needle - start + strlen (pattern);

      xml_sanitize_strcpy (buf, start, size);
    }

  return size;
}

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
      int len = 0;
      struct keyword *match = NULL;
      char buf[30000] = { 0 }; /* FIXME */
      bool advance_ptr = true;

      if (STRING_CHAR_TOKEN (*ptr))
        {
          size_t size;
          char *pattern;

          if (STRING_TOKEN (*ptr))
            {
              pattern = "\"";
            }
          else
            {
              pattern = "\'";
            }

          size = extract_text (ptr, buf, sizeof (buf), pattern);

          if (size != 0)
            {
              string.str = buf;
              string.color = "#6A1B9A";

              match = &string;
              ptr += size;
              len = size;
              advance_ptr = false;
            }
        }
      else if (MULTI_LINE_COMMENT_TOKEN (ptr))
        {
          size_t size;

          size = extract_text (ptr, buf, sizeof (buf), "*/");

          if (size != 0)
            {
              string.str = buf;
              string.color = "#006400";

              match = &string;
              ptr += size;
              len = size;
              advance_ptr = false;
            }
        }
      else if (SINGLE_LINE_COMMENT_TOKEN (ptr))
        {
          size_t size;
          char *newline;

          newline = strchr (ptr + 2, '\n');
          size = newline - ptr;

          strncpy (buf, ptr, size);
          buf[size] = '\0';

          string.str = buf;
          string.color = "#006400";

          match = &string;
          ptr += size;
          len = size;
          advance_ptr = false;
        }
      else if (NUMBER_TOKEN (*ptr)
               && !isalpha (* (ptr - 1)))
        {
          size_t size;

          size = extract_number (ptr, buf);

          if (size != 0)
            {
              string.str = buf;
              string.color = "#9A4EA2";

              match = &string;
              ptr += size;
              len = size;
              advance_ptr = false;
            }
        }

      for (i = 0; i < n_keyword_types && !match; i++)
        {
          struct keyword *type;

          type = set[i]->keywords;

          for (int j = 0; type[j].str != NULL; j++)
            {
              const char *keyword;
              size_t keyword_len;

              keyword = type[j].str;
              keyword_len = strlen (keyword);

              if (strncmp (ptr, keyword, keyword_len) == 0)
                {
                  if (!__isalnum (* (ptr - 1))
                      && !__isalnum (* (ptr + keyword_len)))
                    {
                      match = &type[j];
                      len = keyword_len;
                      i = n_keyword_types;
                      break;
                    }
                }
            }
        }

      if (match != NULL)
        {
          char *cpy;
          const char *strs[5] = {
            "<font color=\"", NULL, "\">",
            NULL,
            "</font>"
          };

          if (count + font_tag_size + len >= size - 1)
            {
              size <<= 1;

              highlighted = realloc (highlighted, size);
            }

          strs[1] = match->color;
          strs[3] = match->str;

          cpy = &highlighted[count];

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
          size_t len, str_size = 0;
          static int max_xml_char_size = 20; /* subject to change */
          char *str = ptr;

          if (isalpha (*str))
            {
              while (__isalnum (*str++))
                str_size++;
            }
          else
            {
              str_size = 1;
            }

          if (count + str_size * max_xml_char_size >= size - 1)
            {
              size <<= 1;

              highlighted = realloc (highlighted, size);
            }

          len = xml_sanitize_strcpy (&highlighted[count],
                                     ptr, str_size);

          count += len;
          ptr += str_size;
        }
    }

  highlighted[count] = '\0';

  return highlighted;
}

/* DIFF */

static bool
diff_keyword (char        *line,
              const char **color)
{
  bool ret = false;

  if (strncmp (line, "+", 1) == 0)
    {
      ret = true;
      *color = "#228B22";
    }
  else if (strncmp (line, "-", 1) == 0)
    {
      ret = true;
      *color = "#DC143C";
    }
	else if (strncmp (line, "@@", 2) == 0)
    {
      ret = true;
      *color = "#4682B4";
    }

  return ret;
}

static char *
highlight_diff (char *diffblk)
{
  size_t size = 1000, count = 0;
  char *highlighted = NULL;
  char *token = NULL;
  char *cpy;
  const char *strs[5] = {
    "<font color=\"", NULL, "\">",
    NULL,
    "</font>"
  };

  highlighted = malloc (sizeof (char) * size);

  token = strtok (diffblk, "\n");

  while (token != NULL)
    {
      size_t len;

      len = strlen (token);

      /* I hate doing this;
       * Implement a string data structure */
      if (2 * len + count + font_tag_size >= size)
        {
          size <<= 1;

          highlighted = realloc (highlighted, size);
        }

      if (diff_keyword (token, &strs[1]))
        {
          cpy = &highlighted[count];

          for (int i = 0; i < 5; i++)
            {
              if (i != 3)
                {
                  cpy = stpcpy (cpy, strs[i]);
                  count += strlen (strs[i]);
                }
              else
                {
                  size_t write;

                  write = xml_sanitize_strcpy (cpy,
                                               token, len);
                  cpy += write;
                  count += write;
                }
            }
        }
      else
        {
          count += xml_sanitize_strcpy (&highlighted[count], token, len);
        }

      highlighted[count++] = '\n';

      token = strtok (NULL, "\n");
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
      case LANG_DIFF:
        /* first character is newline by default */
        highlighted = highlight_diff (codeblk + 1);
        break;
      default:
        highlighted = NULL;
    }

  return highlighted;
}
