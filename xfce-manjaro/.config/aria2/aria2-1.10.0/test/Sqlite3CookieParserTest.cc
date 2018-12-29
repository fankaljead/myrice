#include "Sqlite3CookieParserImpl.h"

#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include "RecoverableException.h"
#include "util.h"

namespace aria2 {

class Sqlite3CookieParserTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(Sqlite3CookieParserTest);
  CPPUNIT_TEST(testMozParse);
  CPPUNIT_TEST(testMozParse_fileNotFound);
  CPPUNIT_TEST(testMozParse_badfile);
  CPPUNIT_TEST(testChromumParse);
  CPPUNIT_TEST_SUITE_END();
public:
  void setUp() {}

  void tearDown() {}

  void testMozParse();
  void testMozParse_fileNotFound();
  void testMozParse_badfile();
  void testChromumParse();
};


CPPUNIT_TEST_SUITE_REGISTRATION(Sqlite3CookieParserTest);

void Sqlite3CookieParserTest::testMozParse()
{
  Sqlite3MozCookieParser parser("cookies.sqlite");
  std::vector<Cookie> cookies;
  parser.parse(cookies);
  CPPUNIT_ASSERT_EQUAL((size_t)3, cookies.size());

  const Cookie& localhost = cookies[0];
  CPPUNIT_ASSERT_EQUAL(std::string("localhost.local"), localhost.getDomain());
  CPPUNIT_ASSERT_EQUAL(std::string("/"), localhost.getPath());
  CPPUNIT_ASSERT_EQUAL(std::string("JSESSIONID"), localhost.getName());
  CPPUNIT_ASSERT_EQUAL(std::string("123456789"), localhost.getValue());
  CPPUNIT_ASSERT_EQUAL((time_t)INT32_MAX, localhost.getExpiry());
  CPPUNIT_ASSERT_EQUAL(true, localhost.isSecureCookie());

  const Cookie& nullValue = cookies[1];
  CPPUNIT_ASSERT_EQUAL(std::string(".null_value.com"), nullValue.getDomain());
  CPPUNIT_ASSERT_EQUAL(std::string("/path/to"), nullValue.getPath());
  CPPUNIT_ASSERT_EQUAL(std::string("uid"), nullValue.getName());
  CPPUNIT_ASSERT_EQUAL(std::string(""), nullValue.getValue());
  CPPUNIT_ASSERT_EQUAL((time_t)0, nullValue.getExpiry());
  CPPUNIT_ASSERT_EQUAL(false, nullValue.isSecureCookie());

  // See row id=3 has no name, so it is skipped.

  const Cookie& overflowTime = cookies[2];
  CPPUNIT_ASSERT_EQUAL(std::string(".overflow.time_t.org"),
                       overflowTime.getDomain());
  CPPUNIT_ASSERT_EQUAL(std::string("/path/to"), overflowTime.getPath());
  CPPUNIT_ASSERT_EQUAL(std::string("foo"), overflowTime.getName());
  CPPUNIT_ASSERT_EQUAL(std::string("bar"), overflowTime.getValue());
  CPPUNIT_ASSERT_EQUAL((time_t)INT32_MAX, overflowTime.getExpiry());
  CPPUNIT_ASSERT_EQUAL(false, overflowTime.isSecureCookie());
}

void Sqlite3CookieParserTest::testMozParse_fileNotFound()
{
  Sqlite3MozCookieParser parser("fileNotFound");
  try {
    std::vector<Cookie> cookies;
    parser.parse(cookies);
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(RecoverableException& e) {
    // SUCCESS
    CPPUNIT_ASSERT(util::startsWith(e.what(),
                                    "SQLite3 database is not opened"));
  }
}

void Sqlite3CookieParserTest::testMozParse_badfile()
{
  Sqlite3MozCookieParser parser("badcookies.sqlite");
  try {
    std::vector<Cookie> cookies;
    parser.parse(cookies);
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(RecoverableException& e) {
    // SUCCESS
  }
}

void Sqlite3CookieParserTest::testChromumParse()
{
  Sqlite3ChromiumCookieParser parser("chromium_cookies.sqlite");
  std::vector<Cookie> cookies;
  parser.parse(cookies);
  const Cookie& sfnet = cookies[0];
  CPPUNIT_ASSERT_EQUAL(std::string(".aria2.sourceforge.net"),
                       sfnet.getDomain());
  CPPUNIT_ASSERT_EQUAL(std::string("/"), sfnet.getPath());
  CPPUNIT_ASSERT_EQUAL(std::string("mykey"), sfnet.getName());
  CPPUNIT_ASSERT_EQUAL(std::string("pass"), sfnet.getValue());
  CPPUNIT_ASSERT_EQUAL((time_t)12345679, sfnet.getExpiry());
  CPPUNIT_ASSERT_EQUAL(false, sfnet.isSecureCookie());

  const Cookie& sfjp = cookies[1];
  CPPUNIT_ASSERT_EQUAL(std::string(".aria2.sourceforge.jp"), sfjp.getDomain());
  CPPUNIT_ASSERT_EQUAL(std::string("/profile"), sfjp.getPath());
  CPPUNIT_ASSERT_EQUAL(std::string("myseckey"), sfjp.getName());
  CPPUNIT_ASSERT_EQUAL(std::string("pass2"), sfjp.getValue());
  CPPUNIT_ASSERT_EQUAL((time_t)0, sfjp.getExpiry());
  CPPUNIT_ASSERT_EQUAL(true, sfjp.isSecureCookie());
}

} // namespace aria2
