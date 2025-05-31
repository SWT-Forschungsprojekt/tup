#include <gtest/gtest.h>
#include "tup-utils/stopTimeStore.h"

TEST(StoreTest, CompleteTest) {
  stopTimeStore store;

  // Zeitstempel für 12:00 Uhr
  constexpr uint32_t time1 = 38400;  // 12:00:00
  // Zeitstempel für 12:10 Uhr
  constexpr uint32_t time2 = 39000;  // 12:10:00
  // Zeitstempel für 12:20 Uhr
  constexpr uint32_t time3 = 39600;  // 12:20:00

  store.store("trip1", "stop1", time1, "2024-03-20");
  store.store("trip1", "stop1", time2, "2024-03-21");
  store.store("trip2", "stop1", time3, "2024-03-20");

  // Test für trip1/stop1 - Durchschnitt von time1 und time2
  const uint32_t averageTime = store.getAverageArrivalTime("trip1", "stop1");
  EXPECT_EQ(averageTime, 38700);  // (39000 + 38400) / 2

  // Test für trip2/stop1 - Sollte exakt time3 sein
  const uint32_t singleTime = store.getAverageArrivalTime("trip2", "stop1");
  EXPECT_EQ(singleTime, time3);
}
