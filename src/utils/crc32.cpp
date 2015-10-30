/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *
 *
 * This file is part of Nitrokey.
 *
 * Nitrokey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey. If not, see <http://www.gnu.org/licenses/>.
 */

#include "crc32.h"


uint32_t Crc32 (uint32_t Crc, uint32_t Data)
{
int i;

    Crc = Crc ^ Data;

    for (i = 0; i < 32; i++)
        if (Crc & 0x80000000)
            Crc = (Crc << 1) ^ 0x04C11DB7;  // Polynomial used in STM32
        else
            Crc = (Crc << 1);

    return (Crc);
}
