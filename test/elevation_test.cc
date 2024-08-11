#ifdef _WIN32
#include "windows.h"
#endif

#include "gtest/gtest.h"
#include "osr/extract/elevation/elevation.h"


constexpr uint16_t SCALED_VALUE_2 = 2 * osr::elevation::SCALING_FACTOR;
constexpr uint16_t SCALED_VALUE_3 = 3 * osr::elevation::SCALING_FACTOR;
constexpr uint16_t SCALED_VALUE_10 = 10 * osr::elevation::SCALING_FACTOR;
constexpr uint16_t MAX_VALUE = osr::elevation::MAX_VALUE;


TEST(ElevationChangeTest, AddElevation) {
  osr::elevation::ElevationChange ec;

  ec.addElevation(SCALED_VALUE_2);
  EXPECT_EQ(ec.getElevation(), SCALED_VALUE_2);
  EXPECT_EQ(ec.getDescent(), 0);

  ec.addElevation(SCALED_VALUE_3);
  EXPECT_EQ(ec.getElevation(), SCALED_VALUE_2 + SCALED_VALUE_3);
  EXPECT_EQ(ec.getDescent(), 0);

  ec.addElevation(SCALED_VALUE_10);
  //Should have reached 15*SCALING_FACTOR
  EXPECT_EQ(ec.getElevation(), MAX_VALUE);
  EXPECT_EQ(ec.getDescent(), 0);

  //handle overflow
  ec.addElevation(SCALED_VALUE_3);
  EXPECT_EQ(ec.getElevation(), MAX_VALUE);
  EXPECT_EQ(ec.getDescent(), 0);
}


TEST(ElevationChangeTest, AddDescent) {
  osr::elevation::ElevationChange ec;

  
  ec.addDescent(SCALED_VALUE_2);
  EXPECT_EQ(ec.getDescent(), SCALED_VALUE_2);
  EXPECT_EQ(ec.getElevation(), 0);

  ec.addDescent(SCALED_VALUE_3);
  EXPECT_EQ(ec.getDescent(), SCALED_VALUE_2 + SCALED_VALUE_3);
  EXPECT_EQ(ec.getElevation(), 0);

  ec.addDescent(SCALED_VALUE_10);
  EXPECT_EQ(ec.getDescent(), MAX_VALUE);
  EXPECT_EQ(ec.getElevation(), 0);

  ec.addDescent(SCALED_VALUE_3);
  EXPECT_EQ(ec.getDescent(), MAX_VALUE);
  EXPECT_EQ(ec.getElevation(), 0);
}


TEST(ElevationChangeTest, ReverseDirection) {
  osr::elevation::ElevationChange ec;
  ec.addElevation(SCALED_VALUE_3);
  ec.addDescent(SCALED_VALUE_2);

  ec.reverseDirection(); 
  EXPECT_EQ(ec.getElevation(), SCALED_VALUE_2);
  EXPECT_EQ(ec.getDescent(), SCALED_VALUE_3);
}


TEST(ElevationChangeTest, Reset) {
  osr::elevation::ElevationChange ec;
  ec.addElevation(SCALED_VALUE_3);
  ec.addDescent(SCALED_VALUE_2);
  ec.reset(); 

  EXPECT_EQ(ec.getElevation(), 0);
  EXPECT_EQ(ec.getDescent(), 0);
  EXPECT_EQ(ec.getData(), 0);
}


TEST(ElevationChangeTest, BitData) {
  osr::elevation::ElevationChange ec(0b10010110);
  EXPECT_EQ(ec.getData(), 0b10010110);
  ec.reverseDirection();
  EXPECT_EQ(ec.getData(), 0b01101001);
}

//TODO add test case for fractions- e.g. 2.5*SCALING_FACTOR