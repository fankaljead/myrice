#include "HttpResponse.h"

#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include "prefs.h"
#include "PiecedSegment.h"
#include "Piece.h"
#include "Request.h"
#include "HttpHeader.h"
#include "HttpRequest.h"
#include "Exception.h"
#include "A2STR.h"
#include "Decoder.h"
#include "DlRetryEx.h"
#include "CookieStorage.h"
#include "AuthConfigFactory.h"
#include "AuthConfig.h"

namespace aria2 {

class HttpResponseTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(HttpResponseTest);
  CPPUNIT_TEST(testGetContentLength_null);
  CPPUNIT_TEST(testGetContentLength_contentLength);
  //CPPUNIT_TEST(testGetContentLength_range);
  CPPUNIT_TEST(testGetEntityLength);
  CPPUNIT_TEST(testGetContentType);
  CPPUNIT_TEST(testDeterminFilename_without_ContentDisposition);
  CPPUNIT_TEST(testDeterminFilename_with_ContentDisposition_zero_length);
  CPPUNIT_TEST(testDeterminFilename_with_ContentDisposition);
  CPPUNIT_TEST(testGetRedirectURI_without_Location);
  CPPUNIT_TEST(testGetRedirectURI_with_Location);
  CPPUNIT_TEST(testIsRedirect);
  CPPUNIT_TEST(testIsTransferEncodingSpecified);
  CPPUNIT_TEST(testGetTransferEncoding);
  CPPUNIT_TEST(testGetTransferEncodingDecoder);
  CPPUNIT_TEST(testIsContentEncodingSpecified);
  CPPUNIT_TEST(testGetContentEncoding);
  CPPUNIT_TEST(testGetContentEncodingDecoder);
  CPPUNIT_TEST(testValidateResponse);
  CPPUNIT_TEST(testValidateResponse_good_range);
  CPPUNIT_TEST(testValidateResponse_bad_range);
  CPPUNIT_TEST(testValidateResponse_chunked);
  CPPUNIT_TEST(testValidateResponse_withIfModifiedSince);
  CPPUNIT_TEST(testHasRetryAfter);
  CPPUNIT_TEST(testProcessRedirect);
  CPPUNIT_TEST(testRetrieveCookie);
  CPPUNIT_TEST(testSupportsPersistentConnection);
  CPPUNIT_TEST_SUITE_END();
private:

public:
  void setUp() {
  }

  void testGetContentLength_null();
  void testGetContentLength_contentLength();
  void testGetEntityLength();
  void testGetContentType();
  void testDeterminFilename_without_ContentDisposition();
  void testDeterminFilename_with_ContentDisposition_zero_length();
  void testDeterminFilename_with_ContentDisposition();
  void testGetRedirectURI_without_Location();
  void testGetRedirectURI_with_Location();
  void testIsRedirect();
  void testIsTransferEncodingSpecified();
  void testGetTransferEncoding();
  void testGetTransferEncodingDecoder();
  void testIsContentEncodingSpecified();
  void testGetContentEncoding();
  void testGetContentEncodingDecoder();
  void testValidateResponse();
  void testValidateResponse_good_range();
  void testValidateResponse_bad_range();
  void testValidateResponse_chunked();
  void testValidateResponse_withIfModifiedSince();
  void testHasRetryAfter();
  void testProcessRedirect();
  void testRetrieveCookie();
  void testSupportsPersistentConnection();
};


CPPUNIT_TEST_SUITE_REGISTRATION( HttpResponseTest );

void HttpResponseTest::testGetContentLength_null()
{
  HttpResponse httpResponse;

  CPPUNIT_ASSERT_EQUAL((uint64_t)0ULL, httpResponse.getContentLength());
}

void HttpResponseTest::testGetContentLength_contentLength()
{
  HttpResponse httpResponse;

  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpHeader->put("Content-Length", "4294967296");

  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT_EQUAL((uint64_t)4294967296ULL, httpResponse.getContentLength());
}

void HttpResponseTest::testGetEntityLength()
{
  HttpResponse httpResponse;

  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpHeader->put("Content-Length", "4294967296");

  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT_EQUAL((uint64_t)4294967296ULL, httpResponse.getEntityLength());

  httpHeader->put("Content-Range", "bytes 1-4294967296/4294967297");

  CPPUNIT_ASSERT_EQUAL((uint64_t)4294967297ULL, httpResponse.getEntityLength());

}

void HttpResponseTest::testGetContentType()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpHeader->put("content-type", "application/metalink+xml; charset=UTF-8");
  httpResponse.setHttpHeader(httpHeader);
  // See paramter is ignored.
  CPPUNIT_ASSERT_EQUAL(std::string("application/metalink+xml"),
                       httpResponse.getContentType());
}

void HttpResponseTest::testDeterminFilename_without_ContentDisposition()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  SharedHandle<Request> request(new Request());
  request->setUri("http://localhost/archives/aria2-1.0.0.tar.bz2");
  httpRequest->setRequest(request);

  httpResponse.setHttpHeader(httpHeader);
  httpResponse.setHttpRequest(httpRequest);

  CPPUNIT_ASSERT_EQUAL(std::string("aria2-1.0.0.tar.bz2"),
                       httpResponse.determinFilename());
}

void HttpResponseTest::testDeterminFilename_with_ContentDisposition_zero_length
()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpHeader->put("Content-Disposition", "attachment; filename=\"\"");
  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  SharedHandle<Request> request(new Request());
  request->setUri("http://localhost/archives/aria2-1.0.0.tar.bz2");
  httpRequest->setRequest(request);

  httpResponse.setHttpHeader(httpHeader);
  httpResponse.setHttpRequest(httpRequest);

  CPPUNIT_ASSERT_EQUAL(std::string("aria2-1.0.0.tar.bz2"),
                       httpResponse.determinFilename());
}

void HttpResponseTest::testDeterminFilename_with_ContentDisposition()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpHeader->put("Content-Disposition",
                  "attachment; filename=\"aria2-current.tar.bz2\"");
  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  SharedHandle<Request> request(new Request());
  request->setUri("http://localhost/archives/aria2-1.0.0.tar.bz2");
  httpRequest->setRequest(request);

  httpResponse.setHttpHeader(httpHeader);
  httpResponse.setHttpRequest(httpRequest);

  CPPUNIT_ASSERT_EQUAL(std::string("aria2-current.tar.bz2"),
                       httpResponse.determinFilename());
}

void HttpResponseTest::testGetRedirectURI_without_Location()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());

  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT_EQUAL(std::string(""),
                       httpResponse.getRedirectURI());  
}

void HttpResponseTest::testGetRedirectURI_with_Location()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpHeader->put("Location", "http://localhost/download/aria2-1.0.0.tar.bz2");
  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT_EQUAL
    (std::string("http://localhost/download/aria2-1.0.0.tar.bz2"),
     httpResponse.getRedirectURI());
}

void HttpResponseTest::testIsRedirect()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpHeader->setResponseStatus("200");
  httpHeader->put("Location", "http://localhost/download/aria2-1.0.0.tar.bz2");

  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT(!httpResponse.isRedirect());

  httpHeader->setResponseStatus("301");

  CPPUNIT_ASSERT(httpResponse.isRedirect());  
}

void HttpResponseTest::testIsTransferEncodingSpecified()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());

  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT(!httpResponse.isTransferEncodingSpecified());  

  httpHeader->put("Transfer-Encoding", "chunked");

  CPPUNIT_ASSERT(httpResponse.isTransferEncodingSpecified());
}

void HttpResponseTest::testGetTransferEncoding()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());

  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT_EQUAL(std::string(""), httpResponse.getTransferEncoding());  

  httpHeader->put("Transfer-Encoding", "chunked");

  CPPUNIT_ASSERT_EQUAL(std::string("chunked"),
                       httpResponse.getTransferEncoding());
}

void HttpResponseTest::testGetTransferEncodingDecoder()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());

  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT(httpResponse.getTransferEncodingDecoder().isNull());  

  httpHeader->put("Transfer-Encoding", "chunked");

  CPPUNIT_ASSERT(!httpResponse.getTransferEncodingDecoder().isNull());
}

void HttpResponseTest::testIsContentEncodingSpecified()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  
  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT(!httpResponse.isContentEncodingSpecified());

  httpHeader->put("Content-Encoding", "gzip");

  CPPUNIT_ASSERT(httpResponse.isContentEncodingSpecified());
}

void HttpResponseTest::testGetContentEncoding()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  
  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT_EQUAL(A2STR::NIL, httpResponse.getContentEncoding());

  httpHeader->put("Content-Encoding", "gzip");

  CPPUNIT_ASSERT_EQUAL(std::string("gzip"), httpResponse.getContentEncoding());
}

void HttpResponseTest::testGetContentEncodingDecoder()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  
  httpResponse.setHttpHeader(httpHeader);

  CPPUNIT_ASSERT(httpResponse.getContentEncodingDecoder().isNull());

#ifdef HAVE_LIBZ
  httpHeader->put("Content-Encoding", "gzip");
  {
    SharedHandle<Decoder> decoder = httpResponse.getContentEncodingDecoder();
    CPPUNIT_ASSERT(!decoder.isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("GZipDecoder"), decoder->getName());
  }
  httpHeader.reset(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);
  httpHeader->put("Content-Encoding", "deflate");
  {
    SharedHandle<Decoder> decoder = httpResponse.getContentEncodingDecoder();
    CPPUNIT_ASSERT(!decoder.isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("GZipDecoder"), decoder->getName());
  }
#endif // HAVE_LIBZ
  httpHeader.reset(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);
  httpHeader->put("Content-Encoding", "bzip2");
  {
    SharedHandle<Decoder> decoder = httpResponse.getContentEncodingDecoder();
    CPPUNIT_ASSERT(decoder.isNull());
  }
}

void HttpResponseTest::testValidateResponse()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);

  httpHeader->setResponseStatus("301");

  try {
    httpResponse.validateResponse();
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(Exception& e) {
  }

  httpHeader->put("Location", "http://localhost/archives/aria2-1.0.0.tar.bz2");
  try {
    httpResponse.validateResponse();
  } catch(Exception& e) {
    CPPUNIT_FAIL("exception must not be thrown.");
  }
}
 
void HttpResponseTest::testValidateResponse_good_range()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);

  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  SharedHandle<Piece> p(new Piece(1, 1024*1024));
  SharedHandle<Segment> segment(new PiecedSegment(1024*1024, p));
  httpRequest->setSegment(segment);
  SharedHandle<FileEntry> fileEntry(new FileEntry("file", 1024*1024*10, 0));
  httpRequest->setFileEntry(fileEntry);
  SharedHandle<Request> request(new Request());
  request->setUri("http://localhost/archives/aria2-1.0.0.tar.bz2");
  httpRequest->setRequest(request);
  httpResponse.setHttpRequest(httpRequest);
  httpHeader->setResponseStatus("206");
  httpHeader->put("Content-Range", "bytes 1048576-10485760/10485760");
  
  try {
    httpResponse.validateResponse();
  } catch(Exception& e) {
    std::cerr << e.stackTrace() << std::endl;
    CPPUNIT_FAIL("exception must not be thrown.");
  }
}

void HttpResponseTest::testValidateResponse_bad_range()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);

  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  SharedHandle<Piece> p(new Piece(1, 1024*1024));
  SharedHandle<Segment> segment(new PiecedSegment(1024*1024, p));
  httpRequest->setSegment(segment);
  SharedHandle<FileEntry> fileEntry(new FileEntry("file", 1024*1024*10, 0));
  httpRequest->setFileEntry(fileEntry);
  SharedHandle<Request> request(new Request());
  request->setUri("http://localhost/archives/aria2-1.0.0.tar.bz2");
  httpRequest->setRequest(request);
  httpResponse.setHttpRequest(httpRequest);
  httpHeader->setResponseStatus("206");
  httpHeader->put("Content-Range", "bytes 0-10485760/10485761");

  try {
    httpResponse.validateResponse();
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(Exception& e) {
  }
}

void HttpResponseTest::testValidateResponse_chunked()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);

  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  SharedHandle<Piece> p(new Piece(1, 1024*1024));
  SharedHandle<Segment> segment(new PiecedSegment(1024*1024, p));
  httpRequest->setSegment(segment);
  SharedHandle<FileEntry> fileEntry(new FileEntry("file", 1024*1024*10, 0));
  httpRequest->setFileEntry(fileEntry);
  SharedHandle<Request> request(new Request());
  request->setUri("http://localhost/archives/aria2-1.0.0.tar.bz2");
  httpRequest->setRequest(request);
  httpResponse.setHttpRequest(httpRequest);
  httpHeader->setResponseStatus("206");
  httpHeader->put("Content-Range", "bytes 0-10485760/10485761");
  httpHeader->put("Transfer-Encoding", "chunked");

  // if transfer-encoding is specified, then range validation is skipped.
  try {
    httpResponse.validateResponse();
  } catch(Exception& e) {
    CPPUNIT_FAIL("exception must not be thrown.");
  }
}

void HttpResponseTest::testValidateResponse_withIfModifiedSince()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);
  httpHeader->setResponseStatus("304");
  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  httpResponse.setHttpRequest(httpRequest);
  try {
    httpResponse.validateResponse();
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(Exception& e) {
  }
  httpRequest->setIfModifiedSinceHeader("Fri, 16 Jul 2010 12:56:59 GMT");
  httpResponse.validateResponse();
}

void HttpResponseTest::testHasRetryAfter()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);

  httpHeader->put("Retry-After", "60");

  CPPUNIT_ASSERT(httpResponse.hasRetryAfter());
  CPPUNIT_ASSERT_EQUAL((time_t)60, httpResponse.getRetryAfter());
}

void HttpResponseTest::testProcessRedirect()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);

  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  SharedHandle<Request> request(new Request());
  request->setUri("http://localhost/archives/aria2-1.0.0.tar.bz2");
  httpRequest->setRequest(request);
  httpResponse.setHttpRequest(httpRequest);
  
  httpHeader->put("Location", "http://mirror/aria2-1.0.0.tar.bz2");
  httpResponse.processRedirect();

  httpHeader->clearField();

  // Give unsupported scheme
  httpHeader->put("Location", "unsupported://mirror/aria2-1.0.0.tar.bz2");
  try {
    httpResponse.processRedirect();
    CPPUNIT_FAIL("DlRetryEx exception must be thrown.");
  } catch(DlRetryEx& e) {
    // Success
  } catch(...) {
    CPPUNIT_FAIL("DlRetryEx exception must be thrown.");
  }
}

void HttpResponseTest::testRetrieveCookie()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);

  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  SharedHandle<Request> request(new Request());
  request->setUri("http://www.aria2.org/archives/aria2-1.0.0.tar.bz2");
  httpRequest->setRequest(request);
  SharedHandle<CookieStorage> st(new CookieStorage());
  httpRequest->setCookieStorage(st);
  httpResponse.setHttpRequest(httpRequest);

  httpHeader->put("Set-Cookie", "k1=v1; expires=Sun, 10-Jun-2007 11:00:00 GMT;"
                  "path=/; domain=.aria2.org;");
  httpHeader->put("Set-Cookie", "k2=v2; expires=Sun, 01-Jan-38 00:00:00 GMT;"
                  "path=/; domain=.aria2.org;");
  httpHeader->put("Set-Cookie", "k3=v3;");

  httpResponse.retrieveCookie();

  CPPUNIT_ASSERT_EQUAL((size_t)2, st->size());

  std::vector<Cookie> cookies;
  st->dumpCookie(std::back_inserter(cookies));
  CPPUNIT_ASSERT_EQUAL(std::string("k2=v2"), cookies[0].toString());
  CPPUNIT_ASSERT_EQUAL(std::string("k3=v3"), cookies[1].toString());
}

void HttpResponseTest::testSupportsPersistentConnection()
{
  HttpResponse httpResponse;
  SharedHandle<HttpHeader> httpHeader(new HttpHeader());
  httpResponse.setHttpHeader(httpHeader);
  SharedHandle<HttpRequest> httpRequest(new HttpRequest());
  httpResponse.setHttpRequest(httpRequest);

  httpHeader->setVersion("HTTP/1.1");
  CPPUNIT_ASSERT(httpResponse.supportsPersistentConnection());
  httpHeader->put("Connection", "close");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
  httpHeader->put("Connection", "keep-alive");
  CPPUNIT_ASSERT(httpResponse.supportsPersistentConnection());
  httpHeader->clearField();

  httpHeader->setVersion("HTTP/1.0");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->put("Connection", "close");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
  httpHeader->put("Connection", "keep-alive");
  CPPUNIT_ASSERT(httpResponse.supportsPersistentConnection());
  httpHeader->clearField();

  // test proxy connection
  SharedHandle<Request> proxyRequest(new Request());
  httpRequest->setProxyRequest(proxyRequest);
  
  httpHeader->setVersion("HTTP/1.1");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->put("Connection", "close");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
  httpHeader->put("Connection", "keep-alive");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
  httpHeader->put("Proxy-Connection", "keep-alive");
  CPPUNIT_ASSERT(httpResponse.supportsPersistentConnection());
  httpHeader->put("Connection", "close");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
  httpHeader->put("Proxy-Connection", "close");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();

  httpHeader->setVersion("HTTP/1.0");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->put("Connection", "close");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
  httpHeader->put("Connection", "keep-alive");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->put("Proxy-Connection", "keep-alive");
  CPPUNIT_ASSERT(httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
  httpHeader->put("Proxy-Connection", "keep-alive");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
  httpHeader->put("Proxy-Connection", "close");
  CPPUNIT_ASSERT(!httpResponse.supportsPersistentConnection());
  httpHeader->clearField();
}

} // namespace aria2
