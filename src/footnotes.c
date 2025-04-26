/* footnotes.c
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

#include "footnotes.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>


struct Footnotes {
  int n_refs; /* number of references */
  Reference *refs;  /* refs */
};


Footnotes *
footnotes_new (void)
{
  Footnotes *notes;

  notes = calloc (1, sizeof (Footnotes));

  notes->n_refs = 0;
  notes->refs = NULL;

  return notes;
}

static void
reference_init (Reference *ref,
                int        index,
                char      *identifier,
                char      *text)
{
  ref->index = index;
  ref->identifier = strdup (identifier);
  ref->text = strdup (text);
  uuid_generate_random (ref->uuid);
  ref->n_referrers = 0;
  ref->referrers = NULL;
}

void
footnotes_add (Footnotes *notes,
               char *identifier,
               char *text)
{
  int index;

  index = ++notes->n_refs;

  notes->refs = realloc (notes->refs,
                        sizeof (Reference) * index);

  reference_init (&notes->refs[index - 1],
                  index, identifier, text);
}

void
footnotes_free (Footnotes *notes)
{
  for (int i = 0; i < notes->n_refs; i++)
    {
      Reference *ref = &notes->refs[i];

      free (ref->identifier);
      free (ref->referrers);
      free (ref->text);
    }

  free (notes->refs);
  free (notes);
}

int
footnotes_get_count (Footnotes *notes)
{
  return notes->n_refs;
}

Reference *
footnotes_get_ref (Footnotes *notes,
                   char *identifier)
{
  if (identifier == NULL)
    return NULL;

  for (int i = 0; i < notes->n_refs; i++)
    {
      Reference *ref = &notes->refs[i];

      if (strcmp (identifier, ref->identifier) == 0)
        return ref;
    }

  return NULL;
}

Reference *
footnotes_get_ref_from_index (Footnotes *notes,
                              int        index)
{
  if (index > notes->n_refs)
    return NULL;

  return &notes->refs[index];
}

void
footnotes_add_referrer (Footnotes *notes,
                        int        index,
                        uuid_t     uuid)
{
  Reference *ref = NULL;

  ref = footnotes_get_ref_from_index (notes, index - 1);

  ref->referrers = realloc (ref->referrers,
                            sizeof (uuid_t) * ++ref->n_referrers);

  strncpy (ref->referrers[ref->n_referrers - 1], uuid, sizeof (uuid_t));
}
