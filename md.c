/*
 * @file: md.c
 */

#include <stdlib.h>
#include <string.h>
#include "md.h"


/* inspired from glib */
#define return_val_if_null(ptr) \
        if (ptr == NULL) return NULL;

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
  (*unit)->next = NULL;
}

// dirty code
static UnitType
find_md_unit_type (char *line)
{
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
                return UNIT_TYPE_H3;
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

  if (line[0] &&
      line[0] == '-' &&
      line[1] && line[1] != '-')
    {
      return UNIT_TYPE_BULLET;
    }

  return UNIT_TYPE_TEXT;
}


static char*
remove_trailing_new_line (char *line)
{
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
  if (type == UNIT_TYPE_H1)
    {
      line += 2;
      return line;
    }
  else if (type == UNIT_TYPE_H2)
    {
      line += 3;
      return line;
    }
  else if (type == UNIT_TYPE_H3)
    {
      line += 4;
      return line;
    }
  else if (type == UNIT_TYPE_BULLET)
    {
      line += 2;
      return line;
    }

  return line;
}

static void
read_md_unit (char *line,
              MD   *md)
{
  MDUnit *unit = NULL;
  MDUnit *next = NULL;
  UnitType type;

  md_unit_init (&unit);
  next = md->elements;

  type = find_md_unit_type (line);

  unit->type = type;
  /* make a copy */
  unit->content = strdup (remove_trailing_new_line (find_md_content (line, type)));

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
      free (unit);
      unit = next;
    }

  free (md);
}
