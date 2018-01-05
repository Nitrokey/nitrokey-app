/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 * Copyright (c) 2012-2018 Nitrokey UG
 *
 * This file is part of Nitrokey App.
 *
 * Nitrokey App is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey App is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "hotpslot.h"
#include "string.h"


OTPSlot::OTPSlot() {
  isProgrammed = false;
  memset(slotName, 0, sizeof(slotName));
  memset(secret, 0, sizeof(secret));
  memset(counter, 0, sizeof(counter));
  memset(tokenID, 0, sizeof(tokenID));
  config = 0;
  slotNumber = 0;
}


#include <cppcodec/base32_crockford.hpp>
#include <cppcodec/base32_default_rfc4648.hpp>
#include <QDebug>
#include <QString>

std::vector<uint8_t> decodeBase32Secret(const std::string secret, const bool debug_mode) {
  std::vector<uint8_t> secret_raw;
  std::string error;
  try {
    secret_raw = base32::decode(secret);
    return secret_raw;
  }
  catch (const cppcodec::parse_error &e){
    if (debug_mode)
      qDebug() << e.what();
    error = error + "base32: " + e.what();
  }
  try {
    auto s = QString::fromStdString(secret);
    s = s.remove('=');
    secret_raw = cppcodec::base32_crockford::decode(s);
    return secret_raw;
  }
  catch (const cppcodec::parse_error &e){
    if (debug_mode)
      qDebug() << e.what();
    error = error + "; crockford: " + e.what();
  }
  throw cppcodec::parse_error(error);
  return secret_raw;
}