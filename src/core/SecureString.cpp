#include "SecureString.h"
#include <algorithm>

void overwrite_string(QString &str) { std::fill(str.begin(), str.end(), '*'); }