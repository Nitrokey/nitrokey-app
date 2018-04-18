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

#ifndef COMMAND_H
#define COMMAND_H
#include <string>
#include "command_id.h"
#include "cxx_semantics.h"

#define print_to_ss(x) ( ss << " " << (#x) <<":\t" << (x) << std::endl );
#ifdef LOG_VOLATILE_DATA
#define print_to_ss_volatile(x) print_to_ss(x);
#else
#define print_to_ss_volatile(x) ( ss << " " << (#x) <<":\t" << "***********" << std::endl );
#endif
#define hexdump_to_ss(x) (ss << #x":\n"\
                          << ::nitrokey::misc::hexdump((const uint8_t *) (&x), sizeof x, false));

namespace nitrokey {
    namespace proto {

        template<CommandID cmd_id>
        class Command : semantics::non_constructible {
        public:
            constexpr static CommandID command_id() { return cmd_id; }

            template<typename T>
            std::string dissect(const T &) {
              return std::string("Payload dissection is unavailable");
            }
        };

namespace stick20{
        enum class PasswordKind : uint8_t {
            User = 'P',
            Admin = 'A',
            AdminPrefixed
        };

        template<CommandID cmd_id, PasswordKind Tpassword_kind = PasswordKind::User, int password_length = 20>
        class PasswordCommand : public Command<cmd_id> {
			constexpr static CommandID _command_id() { return cmd_id; }
        public:
            struct CommandPayload {
                uint8_t kind;
                uint8_t password[password_length];

                std::string dissect() const {
                  std::stringstream ss;
                  print_to_ss( kind );
                  print_to_ss_volatile(password);
                  return ss.str();
                }
                void set_kind_admin() {
                  kind = (uint8_t) 'A';
                }
                void set_kind_admin_prefixed() {
                  kind = (uint8_t) 'P';
                }
                void set_kind_user() {
                  kind = (uint8_t) 'P';
                }

                void set_defaults(){
                  set_kind(Tpassword_kind);
                }

                void set_kind(PasswordKind password_kind){
                  switch (password_kind){
                    case PasswordKind::Admin:
                      set_kind_admin();
                    break;
                    case PasswordKind::User:
                      set_kind_user();
                    break;
                    case PasswordKind::AdminPrefixed:
                      set_kind_admin_prefixed();
                      break;
                  }
                };

            } __packed;

            //typedef Transaction<Command<cmd_id>::command_id(), struct CommandPayload, struct EmptyPayload>
            //    CommandTransaction;
			using CommandTransaction = Transaction<cmd_id,  CommandPayload,  EmptyPayload>;
			//using CommandTransaction = Transaction<_command_id(), CommandPayload, EmptyPayload>;

        };
    }
    }
}

#endif
