/*
 * Copyright (c) 2015-2018 Nitrokey UG
 *
 * This file is part of libnitrokey.
 *
 * libnitrokey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libnitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libnitrokey. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#ifndef CXX_SEMANTICS_H
#define CXX_SEMANTICS_H

#ifndef _MSC_VER
#define __packed __attribute__((__packed__))
#else
#define __packed 
#endif

#ifdef _MSC_VER
#define strdup _strdup
#endif

/*
 *	There's no need to include Boost for a simple subset this project needs.
 */
namespace semantics {
class non_constructible {
  non_constructible() {}
};
}

#endif
