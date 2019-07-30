#ifndef _PAINLESS_MESH_BUFFER_HPP_
#define _PAINLESS_MESH_BUFFER_HPP_

#include <list>

#include <Arduino.h>
#include "configuration.hpp"

#ifndef TCP_MSS
#define TCP_MSS 1024
#endif

namespace painlessmesh {
namespace buffer {

// Temporary buffer used by ReceiveBuffer and SentBuffer
struct temp_buffer_t {
  size_t length = TCP_MSS;
  char buffer[TCP_MSS];
};

/**
 * \brief ReceivedBuffer handles cstrings and stores them as strings
 */
template <class T>
class ReceiveBuffer {
 public:
  ReceiveBuffer() { buffer = T(); }

  /**
   * Push a message into the buffer
   */
  void push(const char *cstr, size_t length, temp_buffer_t &buf) {
    auto data_ptr = cstr;
    do {
      auto len = strnlen(data_ptr, length);
      do {
        auto read_len = std::min(len, buf.length);
        memcpy(buf.buffer, data_ptr, read_len);
        buf.buffer[read_len] = '\0';
        auto newBuffer = T(buf.buffer);
        stringAppend(buffer, newBuffer);
        len -= newBuffer.length();
        length -= newBuffer.length();
        data_ptr += newBuffer.length() * sizeof(char);
      } while (len > 0);
      if (length > 0) {
        // Skip/remove the '\0' between the messages
        length -= 1;
        data_ptr += 1 * sizeof(char);
        if (buffer.length() > 0) {  // skip empty buffers
          jsonStrings.push_back(buffer);
          buffer = T();
        }
      }
    } while (length > 0);
  }

  /**
   * Get the oldest message from the buffer
   */
  T front() {
    if (!empty()) return (*jsonStrings.begin());
    return T();
  }

  /**
   * Remove the oldest message from the buffer
   */
  void pop_front() { jsonStrings.pop_front(); }

  /**
   * Is the buffer empty
   */
  bool empty() { return jsonStrings.empty(); }

  /**
   * Clear the buffer
   */
  void clear() {
    jsonStrings.clear();
    buffer = T();
  }

 private:
  T buffer;
  std::list<T> jsonStrings;

  /**
   * Helper function to deal with difference Arduino String
   * and std::string
   */
  inline void stringAppend(T &buffer, T &newBuffer) { buffer.concat(newBuffer); };
};

#ifdef PAINLESSMESH_ENABLE_STD_STRING
template <>
inline void ReceiveBuffer<std::string>::stringAppend(std::string &buffer,
                                              std::string &newBuffer) {
  buffer.append(newBuffer);
}
#endif

/**
 * \brief SentBuffer stores messages (strings) and allows them to be read in any
 * length
 */
template <class T>
class SentBuffer {
 public:
  SentBuffer(){};

  /**
   * push a message into the buffer.
   *
   * \param priority Whether this is a high priority message.
   *
   * High priority messages will be sent to the front of the buffer
   */
  void push(T message, bool priority = false) {
    if (priority) {
      if (clean)
        jsonStrings.push_front(message);
      else
        jsonStrings.insert((++jsonStrings.begin()), message);
    } else
      jsonStrings.push_back(message);
  }

  /**
   * Request whether the passed length is readable
   *
   * Returns the actual length available (<= the requested length
   */
  size_t requestLength(size_t buffer_length) {
    if (jsonStrings.empty())
      return 0;
    else
      // String.toCharArray automatically turns the last character into
      // a \0, we need the extra space to deal with that annoyance
      return std::min(buffer_length - 1, jsonStrings.begin()->length() + 1);
  }

  /**
   * Read the given length into the passed buffer
   *
   * Note the user should first make sure the requested length is available
   * using `SentBuffer.requestLength()`, otherwise this function might fail.
   * Note that if multiple messages are read then they are separated using '\0'.
   */
  void read(size_t length, temp_buffer_t &buf) {
    // TODO: I don't think we actually need to copy here, and/or
    // we should add a non-copy mode, that returns a pointer directly
    // to the data (using c_str()).
    //
    // Note that toCharrArray always null terminates
    // independent of whether the whole string was read so we use one extra
    // space
    jsonStrings.front().toCharArray(buf.buffer, length + 1);
    last_read_size = length;
  }

  /**
   * Returns a pointer directly to the oldest message
   *
   * Note the user should first make sure the requested length is available
   * using `SentBuffer.requestLength()`, otherwise this function might fail.
   * Note that if multiple messages are read then they are separated using '\0'.
   */
  const char* readPtr(size_t length) {
    last_read_size = length;
    return jsonStrings.front().c_str();
  }

  /**
   * Clear the previously read messages from the buffer.
   *
   * Should be called after a call of read() to clear the buffer.
   */
  void freeRead() {
    if (last_read_size == jsonStrings.begin()->length() + 1) {
      jsonStrings.pop_front();
      clean = true;
    } else {
      // jsonStrings.begin()->remove(0, last_read_size);
      stringEraseFront((*jsonStrings.begin()), last_read_size);
      clean = false;
    }
    last_read_size = 0;
  }

  bool empty() { return jsonStrings.empty(); }

  void clear() { jsonStrings.clear(); }

  size_t size() { return jsonStrings.size(); }

 private:
  size_t last_read_size = 0;
  bool clean = true;
  std::list<T> jsonStrings;

  inline void stringEraseFront(T &string, size_t length) { string.remove(0, length); };
};

#ifdef PAINLESSMESH_ENABLE_STD_STRING
template <>
inline void SentBuffer<std::string>::read(size_t length, temp_buffer_t &buf) {
  jsonStrings.front().copy(buf.buffer, length);
  // Mimic String.toCharArray behaviour, which will insert
  // null termination at the end of original string and the last
  // character
  if (length == jsonStrings.front().length() + 1) buf.buffer[length - 1] = '\0';
  buf.buffer[length] = '\0';
  last_read_size = length;
}

template <>
inline void SentBuffer<std::string>::stringEraseFront(std::string &string,
                                               size_t length) {
  string.erase(0, length);
};
#endif

}  // namespace buffer
}  // namespace painlessmesh
#endif
