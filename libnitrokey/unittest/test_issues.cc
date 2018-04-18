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


const char * const default_admin_pin = "12345678";
const char * const default_user_pin = "123456";
const char * const temporary_password = "123456789012345678901234";
const char * const RFC_SECRET = "12345678901234567890";
const char * const hidden_volume_pass = "123456789012345";

#include "catch.hpp"

#include <NitrokeyManager.h>

using namespace std;
using namespace nitrokey;


bool test_36(){
  auto i = NitrokeyManager::instance();
  i->set_loglevel(3);
  REQUIRE(i->connect());

  for (int j = 0; j < 200; ++j) {
    i->get_status();
    i->get_status_storage_as_string();
    INFO( "Iteration: " << j);
  }
  return true;
}

bool test_31(){
  auto i = NitrokeyManager::instance();
  i->set_loglevel(3);
  REQUIRE(i->connect());

//  i->unlock_encrypted_volume(default_user_pin);
//  i->create_hidden_volume(0, 70, 80, hidden_volume_pass);
//  i->lock_device();

  try{
    i->get_password_safe_slot_status();
  }
  catch (...){
    //pass
  }

  i->get_status_storage();
  i->get_admin_retry_count();
  i->get_status_storage();
  i->get_user_retry_count();
  i->unlock_encrypted_volume(default_user_pin);
  i->get_status_storage();
  i->get_password_safe_slot_status();
  i->get_status_storage();
  i->get_user_retry_count();
  i->get_password_safe_slot_status();
  i->get_status();
  i->get_status_storage();
  i->get_admin_retry_count();
  i->get_status();
  i->get_user_retry_count();
  i->unlock_hidden_volume(hidden_volume_pass);
  i->get_status_storage();
  i->get_password_safe_slot_status();


  return true;
}

TEST_CASE("issue 31", "[issue]"){
  {
    auto i = NitrokeyManager::instance();
    i->set_loglevel(4);
    REQUIRE(i->connect());

    i->unlock_encrypted_volume(default_user_pin);
    i->create_hidden_volume(0, 70, 80, hidden_volume_pass);
    i->lock_device();
  }

  for(int i=0; i<20; i++){
    REQUIRE(test_31());
  }
}



TEST_CASE("issue 36", "[issue]"){
  REQUIRE(test_36());
}
