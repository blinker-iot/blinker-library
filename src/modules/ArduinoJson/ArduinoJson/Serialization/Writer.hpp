// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2022, Benoit BLANCHON
// MIT License

#pragma once

#include "../Namespace.hpp"

namespace ARDUINOJSON_NAMESPACE {

// The default writer is a simple wrapper for Writers that are not copiable
template <typename TDestination, typename Enable = void>
class Writer {
 public:
  explicit Writer(TDestination& dest) : _dest(&dest) {}

  size_t write(uint8_t c) {
    return _dest->write(c);
  }

  size_t write(const uint8_t* s, size_t n) {
    return _dest->write(s, n);
  }

 private:
  TDestination* _dest;
};

}  // namespace ARDUINOJSON_NAMESPACE

#include "../Serialization/Writers/StaticStringWriter.hpp"

#if ARDUINOJSON_ENABLE_STD_STRING
#  include "../Serialization/Writers/StdStringWriter.hpp"
#endif

#if ARDUINOJSON_ENABLE_ARDUINO_STRING
#  include "../Serialization/Writers/ArduinoStringWriter.hpp"
#endif

#if ARDUINOJSON_ENABLE_STD_STREAM
#  include "../Serialization/Writers/StdStreamWriter.hpp"
#endif

#if ARDUINOJSON_ENABLE_ARDUINO_PRINT
#  include "../Serialization/Writers/PrintWriter.hpp"
#endif
