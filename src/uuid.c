/* uuid.c
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

#include "uuid.h"

#include <stdlib.h>
#include <time.h>

#define NUM_CHARACTERS 16

void
uuid_init ()
{
  srand (time (NULL));
}

void
uuid_generate_random (uuid_t uuid)
{
  char v[NUM_CHARACTERS] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                             'a', 'b', 'c', 'd', 'e', 'f' };

  for (int i = 0; i < UUID_NUM_CHARACTERS; i++)
    {
      uuid[i] = v[rand () % NUM_CHARACTERS];
    }

  uuid[8] = '-';
  uuid[13] = '-';
  uuid[18] = '-';
  uuid[23] = '-';

  uuid[36] = '\0';
}
