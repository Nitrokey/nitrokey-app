//
// Created by sz on 09.01.17.
//

#include "libada.h"

std::shared_ptr <libada> libada::_instance = nullptr;

std::shared_ptr<libada> libada::i() {
  if (_instance == nullptr){
    _instance = std::make_shared<libada>();
  }
  return _instance;
}
