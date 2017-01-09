//
// Created by sz on 09.01.17.
//

#include "libnitrokey_adapter.h"

std::shared_ptr <libnitrokey_adapter> libnitrokey_adapter::_instance = nullptr;

std::shared_ptr<libnitrokey_adapter> libnitrokey_adapter::instance() {
  if (_instance == nullptr){
    _instance = std::make_shared<libnitrokey_adapter>();
  }
  return _instance;
}
