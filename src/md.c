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

static int code_block_count = 0;

#define IS_CODE_BLOCK_START(line)     \
        (line[0] && line[0] == '`' && \
         line[1] && line[1] == '`' && \
         line[2] && line[2] == '`' && \
         (code_block_count % 2 == 0))

#define IS_CODE_BLOCK_END(line)       \
        (line[0] && line[0] == '`' && \
         line[1] && line[1] == '`' && \
         line[2] && line[2] == '`' && \
         (code_block_count % 2 == 1))

#define IS_CODE_BLOCK_LINE(prev)               \
        (prev == UNIT_TYPE_CODE_BLOCK_START || \
         prev == UNIT_TYPE_CODE_BLOCK_LINE)

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
  *unit = malloc (sizeof (MDUnit));

  // malloc fails
  if (*unit == NULL)
    {
      // panic
      return;
    }

  (*unit)->type = UNIT_TYPE_NONE;
  (*unit)->content = NULL;
  (*unit)->uri = NULL;
  (*unit)->next = NULL;
}


static UnitType prev_md_unit = UNIT_TYPE_NONE;

/* Notes:
 *
 * UNIT_TYPE_NONE: empty line
 */
static UnitType
find_md_unit_type (char *line)
{
  /* empty line */
  if (line[0] &&
      line[0] == '\n' &&
      !IS_CODE_BLOCK_LINE (prev_md_unit))
    {
      return UNIT_TYPE_NONE;
    }

  /* not a strict check */
  if (line[0] &&
      line[0] == '!')
    {
      return UNIT_TYPE_IMAGE;
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


  if (IS_CODE_BLOCK_START (line))
    {
      code_block_count++;
      return UNIT_TYPE_CODE_BLOCK_START;
    }

  if (IS_CODE_BLOCK_END (line))
    {
      code_block_count++;
      return UNIT_TYPE_CODE_BLOCK_END;
    }

  if (IS_CODE_BLOCK_LINE (prev_md_unit))
    {
      return UNIT_TYPE_CODE_BLOCK_LINE;
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
      case UNIT_TYPE_IMAGE:
        line += 1;
        break;
      case UNIT_TYPE_CODE_BLOCK_START:
        return NULL;
      case UNIT_TYPE_CODE_BLOCK_END:
        return NULL;
      default:
        break;
    }

  return line;
}

/*
 * @example: content = [Alt text](/images/md.svg)
 */
static char*
find_image_uri (char *content)
{
  size_t len;
  char *uri = NULL;

  uri = strchr (content, '(') + 1;

  len = strlen(uri);
  uri[len - 1 ] = '\0';

  return uri;
}

//FIXME: extract title
static char*
find_image_title ()
{
  return NULL;
}

static void
read_md_unit (char *line,
              MD   *md)
{
  MDUnit *unit = NULL;
  MDUnit *next = NULL;
  UnitType type;
  char *content = NULL;

  md_unit_init (&unit);
  next = md->elements;

  type = find_md_unit_type (line);
  prev_md_unit = type;
  unit->type = type;

  /* make a copy */
  if (content = remove_trailing_new_line (find_md_content (line, type)))
    unit->content = strdup (content);

  if (unit->type == UNIT_TYPE_IMAGE)
    {
      unit->uri = find_image_uri (unit->content);
      unit->content = find_image_title ();
    }

  /* Append to md->elements */
  if (next == NULL)
    {
      md->elements = unit;
    }
  else
    {
      while (next->next)
        {
          next = next->next;
        }
      next->next = unit;
    }
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

  return_val_if_null (file);

  /* parse here */
  md_init (&md);

  /* read file line by line */
  while ((read = getline (&line, &len, file) != -1))
    {
      read_md_unit (line, md);
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
      if (unit->content != NULL)
        free (unit->content);

      free (unit);
      unit = next;
    }

  free (md);
}
