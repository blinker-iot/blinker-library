// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2022, Benoit BLANCHON
// MIT License

#pragma once

#include "RamString.hpp"
#include "../IsString.hpp"
#include "../String.hpp"

namespace ARDUINOJSON_NAMESPACE {

inline SizedRamString adaptString(const String& s) {
  return SizedRamString(s.c_str(), s.size());
}

template <>
struct IsString<String> : true_type {};

}  // namespace ARDUINOJSON_NAMESPACE
