#include "Option.h"
#include <string>
#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {

class OptionTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(OptionTest);
  CPPUNIT_TEST(testPutAndGet);
  CPPUNIT_TEST(testPutAndGetAsInt);
  CPPUNIT_TEST(testPutAndGetAsDouble);
  CPPUNIT_TEST(testDefined);
  CPPUNIT_TEST(testBlank);
  CPPUNIT_TEST_SUITE_END();
private:

public:
  void setUp() {
  }

  void testPutAndGet();
  void testPutAndGetAsInt();
  void testPutAndGetAsDouble();
  void testDefined();
  void testBlank();
};


CPPUNIT_TEST_SUITE_REGISTRATION( OptionTest );

void OptionTest::testPutAndGet() {
  Option op;
  op.put("key", "value");
  
  CPPUNIT_ASSERT(op.defined("key"));
  CPPUNIT_ASSERT_EQUAL(std::string("value"), op.get("key"));
}

void OptionTest::testPutAndGetAsInt() {
  Option op;
  op.put("key", "1000");

  CPPUNIT_ASSERT(op.defined("key"));
  CPPUNIT_ASSERT_EQUAL((int32_t)1000, op.getAsInt("key"));
}

void OptionTest::testPutAndGetAsDouble() {
  Option op;
  op.put("key", "10.0");
  
  CPPUNIT_ASSERT_EQUAL(10.0, op.getAsDouble("key"));
}

void OptionTest::testDefined()
{
  Option op;
  op.put("k", "v");
  op.put("k1", "");
  CPPUNIT_ASSERT(op.defined("k"));
  CPPUNIT_ASSERT(op.defined("k1"));
  CPPUNIT_ASSERT(!op.defined("undefined"));
}

void OptionTest::testBlank()
{
  Option op;
  op.put("k", "v");
  op.put("k1", "");
  CPPUNIT_ASSERT(!op.blank("k"));
  CPPUNIT_ASSERT(op.blank("k1"));
  CPPUNIT_ASSERT(op.blank("undefined"));
}

} // namespace aria2
