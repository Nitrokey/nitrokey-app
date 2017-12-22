#ifndef CXX_SEMANTICS_H
#define CXX_SEMANTICS_H

#ifndef _MSC_VER
#define __packed __attribute__((__packed__))
#else
#define __packed 
#endif

#ifdef _MSC_VER
#define strdup _strdup
#endif

/*
 *	There's no need to include Boost for a simple subset this project needs.
 */
namespace semantics {
class non_constructible {
  non_constructible() {}
};
}

#endif
