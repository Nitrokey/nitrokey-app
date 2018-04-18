/*
 * Copyright (c) 2017-2018 Nitrokey UG
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

static const char *const default_admin_pin = "12345678";
static const char *const default_user_pin = "123456";
const char * temporary_password = "123456789012345678901234";
const char * RFC_SECRET = "12345678901234567890";

#include "catch.hpp"

#include <iostream>
#include <NitrokeyManager.h>
#include <stick20_commands.h>

using namespace nitrokey;


TEST_CASE("List devices", "[BASIC]") {
    shared_ptr<Stick20> d = make_shared<Stick20>();
    auto v = d->enumerate();
    REQUIRE(v.size() > 0);
    for (auto a : v){
        std::cout << a;
        d->set_path(a);
        d->connect();
        auto res = GetStatus::CommandTransaction::run(d);
        auto res2 = GetDeviceStatus::CommandTransaction::run(d);
        std::cout << " " << res.data().card_serial_u32 << " "
                  << res.data().get_card_serial_hex()
                  << " " << std::to_string(res2.data().versionInfo.minor)
                  << std::endl;
        d->disconnect();
    }
}

TEST_CASE("Regenerate AES keys", "[BASIC]") {
    shared_ptr<Stick20> d = make_shared<Stick20>();
    auto v = d->enumerate();
    REQUIRE(v.size() > 0);

    std::vector<shared_ptr<Stick20>> devices;
    for (auto a : v){
        std::cout << a << endl;
        d = make_shared<Stick20>();
        d->set_path(a);
        d->connect();
        devices.push_back(d);
    }

    for (auto d : devices){
        auto res2 = GetDeviceStatus::CommandTransaction::run(d);
        std::cout << std::to_string(res2.data().versionInfo.minor) << std::endl;
//        nitrokey::proto::stick20::CreateNewKeys::CommandPayload p;
//        p.set_defaults();
//        memcpy(p.password, "12345678", 8);
//        auto res3 = nitrokey::proto::stick20::CreateNewKeys::CommandTransaction::run(d, p);
    }

    for (auto d : devices){
        //TODO watch out for multiple hid_exit calls
        d->disconnect();
    }
}


TEST_CASE("Use API", "[BASIC]") {
    auto nm = NitrokeyManager::instance();
    nm->set_loglevel(2);
    auto v = nm->list_devices();
    REQUIRE(v.size() > 0);

    for (int i=0; i<10; i++){
        for (auto a : v) {
            std::cout <<"Connect with: " << a <<
            " " << std::boolalpha << nm->connect_with_path(a) << " ";
            try{
                auto status_storage = nm->get_status_storage();
                std::cout << status_storage.ActiveSmartCardID_u32
                          << " " << status_storage.ActiveSD_CardID_u32
                          << std::endl;

//                nm->fill_SD_card_with_random_data("12345678");
            }
            catch (const LongOperationInProgressException &e){
                std::cout << "long operation in progress on " << a
                        << " " << std::to_string(e.progress_bar_value) << std::endl;
//                this_thread::sleep_for(1000ms);
            }
        }
        std::cout <<"Iteration: " << i << std::endl;
    }

}


TEST_CASE("Use API ID", "[BASIC]") {
    auto nm = NitrokeyManager::instance();
    nm->set_loglevel(2);

    auto v = nm->list_devices_by_cpuID();
    REQUIRE(v.size() > 0);

    //no refresh - should not reconnect to new devices
    // Scenario:
    // 1. Run test
    // 2. Remove one of the devices and reinsert it after a while
    // 3. Device should not be reconnected and test should not crash
    // 4. Remove all devices - test should continue

    for (int j = 0; j < 100; j++) {
        for (auto i : v) {
            if (!nm->connect_with_ID(i)) continue;
            int retry_count = 99;
            try {
                retry_count = nm->get_admin_retry_count();
                std::cout << j << " " << i << " " << to_string(retry_count) << std::endl;
            }
            catch (...) {
                retry_count = 99;
                //pass
            }
        }
    }
    std::cout << "finished" << std::endl;
}

TEST_CASE("Use API ID refresh", "[BASIC]") {
    auto nm = NitrokeyManager::instance();
    nm->set_loglevel(2);

    //refresh in each iteration - should reconnect to new devices
    // Scenario:
    // 1. Run test
    // 2. Remove one of the devices and reinsert it after a while
    // 3. Device should be reconnected

    for(int j=0; j<100; j++) {
        auto v = nm->list_devices_by_cpuID();
        REQUIRE(v.size() > 0);
        for (auto i : v) {
            nm->connect_with_ID(i);
            int retry_count = 99;
            try {
                retry_count = nm->get_admin_retry_count();
                std::cout << j <<" " << i << " " << to_string(retry_count) << std::endl;
            }
            catch (...){
                retry_count = 99;
                //pass
            }
        }
    }
    std::cout << "finished" << std::endl;
}
