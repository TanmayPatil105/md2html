/* footnotes.h
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

#pragma once

#include "uuid.h"


typedef struct Reference {
  int     index;      /* indexing starts at 1 */
  uuid_t  uuid;
  char   *identifier;
  char   *text;

  int n_referrers;
  uuid_t *referrers;
} Reference;

struct Footnotes;
typedef struct Footnotes Footnotes;


Footnotes *footnotes_new       (void);
void       footnotes_add       (Footnotes *refs,
                                char *identifier,
                                char *text);
void       footnotes_free      (Footnotes *refs);
int        footnotes_get_count (Footnotes *refs);
Reference *footnotes_get_ref   (Footnotes *refs,
                                char *identifier);
Reference *footnotes_get_ref_from_index (Footnotes *refs,
                                         int index);
void       footnotes_add_referrer (Footnotes *notes,
                                   int        index,
                                   uuid_t     uuid);

