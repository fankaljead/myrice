#include "CookieParser.h"

#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {

class CookieParserTest:public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CookieParserTest);
  CPPUNIT_TEST(testParse);
  CPPUNIT_TEST_SUITE_END();
public:
  void testParse();
};


CPPUNIT_TEST_SUITE_REGISTRATION( CookieParserTest );

void CookieParserTest::testParse()
{
  std::string str = "JSESSIONID=123456789; expires=Sun, 10-Jun-2007 11:00:00 GMT; path=/; domain=localhost; secure";
  Cookie c = CookieParser().parse(str, "", "");
  CPPUNIT_ASSERT(c.good());
  CPPUNIT_ASSERT_EQUAL(std::string("JSESSIONID"), c.getName());
  CPPUNIT_ASSERT_EQUAL(std::string("123456789"), c.getValue());
  CPPUNIT_ASSERT_EQUAL((time_t)1181473200, c.getExpiry());  
  CPPUNIT_ASSERT_EQUAL(std::string("/"), c.getPath());
  CPPUNIT_ASSERT_EQUAL(std::string(".localhost.local"), c.getDomain());
  CPPUNIT_ASSERT_EQUAL(true, c.isSecureCookie());
  CPPUNIT_ASSERT_EQUAL(false, c.isSessionCookie());

  std::string str2 = "JSESSIONID=123456789";
  c = CookieParser().parse(str2, "default.domain", "/default/path");
  CPPUNIT_ASSERT(c.good());
  CPPUNIT_ASSERT_EQUAL(std::string("JSESSIONID"), c.getName());
  CPPUNIT_ASSERT_EQUAL(std::string("123456789"), c.getValue());
  CPPUNIT_ASSERT_EQUAL((time_t)0, c.getExpiry());
  CPPUNIT_ASSERT_EQUAL(std::string("default.domain"), c.getDomain());
  CPPUNIT_ASSERT_EQUAL(std::string("/default/path"), c.getPath());
  CPPUNIT_ASSERT_EQUAL(false, c.isSecureCookie());
  CPPUNIT_ASSERT_EQUAL(true, c.isSessionCookie());

  std::string str3 = "";
  c = CookieParser().parse(str3, "", "");
  CPPUNIT_ASSERT(!c.good());

#ifndef __MINGW32__
  std::string str4 = "UID=300; expires=Wed, 01-Jan-1960 00:00:00 GMT";
  time_t expire_time = (time_t) -315619200;
#else
  std::string str4 = "UID=300; expires=Wed, 01-Jan-1970 00:00:01 GMT";
  time_t expire_time = (time_t) 1;
#endif
  c = CookieParser().parse(str4, "localhost", "/");
  CPPUNIT_ASSERT(c.good());
  CPPUNIT_ASSERT(!c.isSessionCookie());
  CPPUNIT_ASSERT_EQUAL(expire_time, c.getExpiry());

  std::string str5 = "k=v; expires=Sun, 10-Jun-07 11:00:00 GMT";
  c = CookieParser().parse(str5, "", "");
  CPPUNIT_ASSERT(c.good());
  CPPUNIT_ASSERT_EQUAL(std::string("k"), c.getName());
  CPPUNIT_ASSERT_EQUAL(std::string("v"), c.getValue());
  CPPUNIT_ASSERT_EQUAL((time_t)1181473200, c.getExpiry());
  CPPUNIT_ASSERT_EQUAL(std::string(""), c.getDomain());
  CPPUNIT_ASSERT_EQUAL(std::string(""), c.getPath());
  CPPUNIT_ASSERT(!c.isSecureCookie());
  CPPUNIT_ASSERT(!c.isSessionCookie());
}

} // namespace aria2
