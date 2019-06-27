#ifndef _PAINLESS_MESH_NTP_HPP_
#define _PAINLESS_MESH_NTP_HPP_

#ifndef TIME_SYNC_INTERVAL
#define TIME_SYNC_INTERVAL 1 * TASK_MINUTE  // Time resync period
#endif

#ifndef TIME_SYNC_ACCURACY
#define TIME_SYNC_ACCURACY 5000  // Minimum time sync accuracy (5ms
#endif

#include "Arduino.h"

namespace painlessmesh {
namespace ntp {
/**
 * Calculate the offset of the local clock using the ntp algorithm
 *
 * See ntp overview for more information
 */
inline int32_t clockOffset(uint32_t time0, uint32_t time1, uint32_t time2,
                           uint32_t time3) {
  uint32_t offset =
      ((int32_t)(time1 - time0) / 2) + ((int32_t)(time2 - time3) / 2);

  // Take small steps to avoid over correction
  if (offset < TASK_SECOND && offset > 4) offset = offset / 4;
  return offset;
}

/**
 * Calculate the time it took to get reply from other node
 *
 * See ntp algorithm for more information
 */
inline int32_t tripDelay(uint32_t time0, uint32_t time1, uint32_t time2,
                         uint32_t time3) {
  return ((time3 - time0) - (time2 - time1)) / 2;
}
}  // namespace ntp
}  // namespace painlessmesh
#endif
