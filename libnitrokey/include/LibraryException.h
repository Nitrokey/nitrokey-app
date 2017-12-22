#ifndef LIBNITROKEY_LIBRARYEXCEPTION_H
#define LIBNITROKEY_LIBRARYEXCEPTION_H

#include <exception>
#include <cstdint>
#include <string>
#include "log.h"

class LibraryException: std::exception {
public:
    virtual uint8_t exception_id()= 0;
};

class TargetBufferSmallerThanSource: public LibraryException {
public:
    virtual uint8_t exception_id() override {
        return 203;
    }

public:
    size_t source_size;
    size_t target_size;

    TargetBufferSmallerThanSource(
            size_t source_size, size_t target_size
            ) : source_size(source_size),  target_size(target_size) {}

    virtual const char *what() const throw() override {
        std::string s = " ";
        auto ts = [](size_t x){ return std::to_string(x); };
        std::string msg = std::string("Target buffer size is smaller than source: [source size, buffer size]")
            +s+ ts(source_size) +s+ ts(target_size);
        return msg.c_str();
    }

};

class InvalidHexString : public LibraryException {
public:
    virtual uint8_t exception_id() override {
        return 202;
    }

public:
    uint8_t invalid_char;

    InvalidHexString (uint8_t invalid_char) : invalid_char( invalid_char) {}

    virtual const char *what() const throw() override {
        return "Invalid character in hex string";
    }

};

class InvalidSlotException : public LibraryException {
public:
    virtual uint8_t exception_id() override {
        return 201;
    }

public:
    uint8_t slot_selected;

    InvalidSlotException(uint8_t slot_selected) : slot_selected(slot_selected) {}

    virtual const char *what() const throw() override {
        return "Wrong slot selected";
    }

};



class TooLongStringException : public LibraryException {
public:
    virtual uint8_t exception_id() override {
        return 200;
    }

    std::size_t size_source;
    std::size_t size_destination;
    std::string message;

    TooLongStringException(size_t size_source, size_t size_destination, const std::string &message = "") : size_source(
            size_source), size_destination(size_destination), message(message) {
      LOG(std::string("TooLongStringException, size diff: ")+ std::to_string(size_source-size_destination), nitrokey::log::Loglevel::DEBUG);

    }

    virtual const char *what() const throw() override {
        //TODO add sizes and message data to final message
        return "Too long string has been supplied as an argument";
    }

};

#endif //LIBNITROKEY_LIBRARYEXCEPTION_H
