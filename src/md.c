/* md.c
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


#include <stdlib.h>
#include <string.h>
#include "md.h"


/* inspired from glib */
#define return_val_if_null(ptr) \
        if (ptr == NULL) return NULL;

/*
 * Markdown Lists:
 *
 * We can make unordered list by preceding one ore more
 * lines of text with `-`, `*` or `+`
 *
 */
#define IS_LIST(line)                           \
        (line[0] && line[1] &&                  \
         ((line[0] == '-' && line[1] != '-') || \
          (line[0] == '*' && line[1] != '*') || \
          (line[0] == '+' && line[1] != '+')))

/*
 * Code blocks in markdown
 *
 * ```
 * int main() {
 * 	printf ("Ok computer!\n");
 * 	return 0;
 * }
 * ```
 */

#define IS_CODE_BLOCK_BOUND(line)     \
        (line[0] && line[0] == '`' && \
         line[1] && line[1] == '`' && \
         line[2] && line[2] == '`')

#define IS_CODE_BLOCK_LINE(prev) \
        (code_block_count == 1)

/*
 * md_init
 * @md: allocates memory for MD object
 *
 */
static void
md_init (MD **md)
{
  *md = malloc (sizeof (MD));

  // malloc fails
  if (*md == NULL)
    {
      // panic
      return;
    }

  (*md)->n_lines = -1;
  (*md)->elements = NULL;
}

/*
 * md_unit_init
 * @unit: allocates memory for MDUnit object
 *
 */
static void
md_unit_init (MDUnit **unit)
{
  *unit = (MDUnit *) malloc (sizeof (MDUnit));
  memset (*unit, 0, sizeof(unit));

  // malloc fails
  if (*unit == NULL)
    {
      // panic
      return;
    }

  (*unit)->type = UNIT_TYPE_NONE;
  (*unit)->content = NULL;
  (*unit)->uri = NULL;
  (*unit)->lang = LANG_NONE;
  (*unit)->next = NULL;
}

/* Notes:
 *
 * UNIT_TYPE_NONE: empty line
 */
static UnitType
find_md_unit_type (char *line)
{
  /* empty line */
  if (line[0] &&
      line[0] == '\n')
    {
      return UNIT_TYPE_NONE;
    }

  if (line[0] &&
      line[0] == '>' &&
      line[1] && line[1] == ' ')
    {
      return UNIT_TYPE_QUOTE;
    }

  if (IS_LIST (line))
    {
      return UNIT_TYPE_BULLET;
    }

  if (IS_CODE_BLOCK_BOUND (line))
    {
      return UNIT_TYPE_CODE_BLOCK_BOUND;
    }


  if (line[0] &&
      line[0] == '#')
    {
      if (line[1] &&
          line[1] == '#')
        {
          if (line[2] &&
              line[2] == '#')
            {
              if (line[3] == ' ')
                {
                  return UNIT_TYPE_H3;
                }
            }
          else if (line[2] &&
                   line[2] == ' ')
            {
              return UNIT_TYPE_H2;
            }
        }
      else if (line[1] &&
               line[1] == ' ')
        {
          return UNIT_TYPE_H1;
        }
    }

  return UNIT_TYPE_TEXT;
}


static char*
remove_trailing_new_line (char *line)
{
  if (line == NULL)
    return NULL;

  size_t len;

  len = strlen(line) - 1;

  if (*line && line[len] == '\n')
    line[len] = '\0';

  return line;
}

static char*
find_md_content (char    *line,
                 UnitType type)
{
  switch(type)
    {
      case UNIT_TYPE_H1:
        line += 2;
        break;
      case UNIT_TYPE_H2:
        line += 3;
        break;
      case UNIT_TYPE_H3:
        line += 4;
        break;
      case UNIT_TYPE_BULLET:
        line += 2;
        break;
      case UNIT_TYPE_QUOTE:
        line += 2;
        break;
      default:
        break;
    }

  return line;
}

static Lang
find_code_block_lang (char *line)
{
  Lang lang = LANG_NONE;
  char *str = NULL;

  /* We already know it's a codeblock start */
  str = line + 3;

  if (strncmp (str, "c", 1) == 0)
    lang = LANG_C;
  else if (strncmp (str, "diff", 4) == 0)
    lang = LANG_DIFF;
  else if (strncmp (str, "html", 4) == 0)
    lang = LANG_HTML;

  return lang;
}

/*
 * Public functions
 */

/*
 * parse_md:
 * @file: markdown file as input
 *
 * parse the markdown file and store it in a data structure
 */
MD*
parse_md (MDFile *file)
{
  uint n_lines = 0;
  char *line = NULL;
  size_t len = 0;
  size_t read;
  MD *md = NULL;
  MDUnit *next = NULL;

  return_val_if_null (file);

  md_init (&md);

  /* read file line by line */
  while ((read = getline (&line, &len, file) != -1))
    {
      MDUnit *unit = NULL;

      md_unit_init (&unit);

      unit->type = find_md_unit_type (line);

      if (unit->type == UNIT_TYPE_CODE_BLOCK_BOUND)
        {
          unit->lang = find_code_block_lang (line);
          unit->type = UNIT_TYPE_CODE_BLOCK;
        }

      if (unit->type == UNIT_TYPE_CODE_BLOCK)
        {
          size_t buf_size = 256;
          size_t count = 1;
          char *buf;

          buf = malloc (sizeof (char) * buf_size);
          buf[0] = '\n'; /* add a newline */

          while ((read = getline (&line, &len, file)) != -1UL)
            {
              if (find_md_unit_type (line) == UNIT_TYPE_CODE_BLOCK_BOUND)
                break;

              if (count + read >= buf_size)
                {
                  buf_size <<= 1;
                  buf = realloc (buf, sizeof (char) * buf_size);
                }

              memcpy (&buf[count], line, read);
              count += read;
            }

          buf[count] = '\0';
          unit->content = buf;
        }
      else
        {
          char *content = NULL;

          content = find_md_content (line, unit->type);
          if (content != NULL)
            unit->content = remove_trailing_new_line (strdup (content));
        }

      /* Append to md->elements */
      if (next == NULL)
        {
          md->elements = unit;
          next = unit;
        }
      else
        {
          next->next = unit;
          next = unit;
        }

      n_lines++;
    }

  md->n_lines = n_lines;

  free (line);
  return md;
}

void
md_free (MD *md)
{
  MDUnit *unit = NULL;
  MDUnit *next = NULL;

  unit = md->elements;
  while (unit != NULL)
    {
      next = unit->next;
      /* free (unit->content); */
      /* HTML takes ownership of the content */
      if (unit->uri != NULL)
        free (unit->uri);

      free (unit);
      unit = next;
    }

  free (md);
}
