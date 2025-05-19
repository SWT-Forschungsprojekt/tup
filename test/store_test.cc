#include <gtest/gtest.h>
#include "tup-utils/stopTimeStore.h"

TEST(StoreTest, CompleteTest) {
  stopTimeStore store;

  // Unix-Zeitstempel für den 20. März 2024, 12:00 Uhr
  constexpr int64_t time1 = 1710931200;  // 2024-03-20 12:00:00
  // Unix-Zeitstempel für den 20. März 2024, 12:10 Uhr
  constexpr int64_t time2 = 1710931800;  // 2024-03-20 12:10:00
  // Unix-Zeitstempel für den 20. März 2024, 12:20 Uhr
  constexpr int64_t time3 = 1710932400;  // 2024-03-20 12:20:00

  store.store("trip1", "stop1", time1, "2024-03-20");
  store.store("trip1", "stop1", time2, "2024-03-21");
  store.store("trip2", "stop1", time3, "2024-03-20");

  // Test für trip1/stop1 - Durchschnitt von time1 und time2
  const int64_t averageTime = store.getAverageArrivalTime("trip1", "stop1");
  EXPECT_EQ(averageTime, 1710931500);  // (1710931200 + 1710931800) / 2

  // Test für trip2/stop1 - Sollte exakt time3 sein
  const int64_t singleTime = store.getAverageArrivalTime("trip2", "stop1");
  EXPECT_EQ(singleTime, time3);
}
