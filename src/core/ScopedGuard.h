/*
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

#ifndef NITROKEYAPP_SCOPEDGUARD_H
#define NITROKEYAPP_SCOPEDGUARD_H
#include <functional>

class ScopedGuard{
public:
    explicit ScopedGuard(const std::function<void()> &function_to_run) : function_to_run(function_to_run), cancel_action(false) {}
    ~ScopedGuard(){
        if (!cancel_action){
            function_to_run();
        }
    }
    void cancel(){
        cancel_action = true;
    }
    ScopedGuard(const ScopedGuard&) = delete;
    ScopedGuard& operator=(const ScopedGuard &) = delete;
private:
    std::function<void()> function_to_run;
    bool cancel_action;
};

#endif //NITROKEYAPP_SCOPEDGUARD_H
