#include <sstream>
#include <string>
#include "misc.h"
#include "inttypes.h"
#include <cstdlib>
#include <cstring>
#include "LibraryException.h"
#include <vector>

namespace nitrokey {
namespace misc {



::std::vector<uint8_t> hex_string_to_byte(const char* hexString){
    const size_t big_string_size = 256; //arbitrary 'big' number
    const size_t s_size = strlen(hexString);
    const size_t d_size = s_size/2;
    if (s_size%2!=0 || s_size>big_string_size){
        throw InvalidHexString(0);
    }
    auto data = ::std::vector<uint8_t>();
    data.reserve(d_size);

    char buf[2];
    for(size_t i=0; i<s_size; i++){

        char c = hexString[i];
        bool char_from_range = (('0' <= c && c <='9') || ('A' <= c && c <= 'F') || ('a' <= c && c<= 'f'));
        if (!char_from_range){
            throw InvalidHexString(c);
        }
        buf[i%2] = c;
        if (i%2==1){
            data.push_back( strtoul(buf, NULL, 16) & 0xFF );
        }
    }
    return data;
};

#include <cctype>
::std::string hexdump(const char *p, size_t size, bool print_header,
        bool print_ascii, bool print_empty) {
  ::std::stringstream out;
  char formatbuf[128];
  const char *pstart = p;

  for (const char *pend = p + size; p < pend;) {
      if (print_header){
          snprintf(formatbuf, 128, "%04x\t", static_cast<int> (p - pstart));
          out << formatbuf;
      }

    const char* pp = p;
    for (const char *le = p + 16; p < le; p++) {
      if (p < pend){
        snprintf(formatbuf, 128, "%02x ", uint8_t(*p));
        out << formatbuf;
      } else {
          if(print_empty)
            out << "-- ";
      }

    }
      if(print_ascii){
        out << "\t";
        for (const char *le = pp + 16; pp < le && pp < pend; pp++) {
          if (std::isgraph(*pp))
            out << uint8_t(*pp);
          else
            out << '.';
        }
      }
    out << ::std::endl;
  }
  return out.str();
}

static uint32_t _crc32(uint32_t crc, uint32_t data) {
  int i;
  crc = crc ^ data;

  for (i = 0; i < 32; i++) {
    if (crc & 0x80000000)
      crc = (crc << 1) ^ 0x04C11DB7;  // polynomial used in STM32
    else
      crc = (crc << 1);
  }

  return crc;
}

uint32_t stm_crc32(const uint8_t *data, size_t size) {
  uint32_t crc = 0xffffffff;
  const uint32_t *pend = (const uint32_t *)(data + size);
  for (const uint32_t *p = (const uint32_t *)(data); p < pend; p++)
    crc = _crc32(crc, *p);
  return crc;
}
}
}
