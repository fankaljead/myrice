/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "HttpRequest.h"

#include <cassert>
#include <numeric>
#include <vector>

#include "Segment.h"
#include "Range.h"
#include "CookieStorage.h"
#include "Option.h"
#include "util.h"
#include "Base64.h"
#include "prefs.h"
#include "AuthConfigFactory.h"
#include "AuthConfig.h"
#include "a2functional.h"
#include "TimeA2.h"
#include "array_fun.h"

namespace aria2 {

const std::string HttpRequest::USER_AGENT("aria2");

HttpRequest::HttpRequest():contentEncodingEnabled_(true),
                           userAgent_(USER_AGENT),
                           noCache_(true),
                           acceptGzip_(false),
                           endOffsetOverride_(false)
{}

void HttpRequest::setSegment(const SharedHandle<Segment>& segment)
{
  segment_ = segment;
}

void HttpRequest::setRequest(const SharedHandle<Request>& request)
{
  request_ = request;
}

off_t HttpRequest::getStartByte() const
{
  if(segment_.isNull()) {
    return 0;
  } else {
    return fileEntry_->gtoloff(segment_->getPositionToWrite());
  }
}

off_t HttpRequest::getEndByte() const
{
  if(segment_.isNull() || request_.isNull()) {
    return 0;
  } else {
    if(request_->isPipeliningEnabled()) {
      off_t endByte =
        fileEntry_->gtoloff(segment_->getPosition()+segment_->getLength()-1);
      return std::min(endByte, static_cast<off_t>(fileEntry_->getLength()-1));
    } else {
      return 0;
    }
  }
}

RangeHandle HttpRequest::getRange() const
{
  // content-length is always 0
  if(segment_.isNull()) {
    return SharedHandle<Range>(new Range());
  } else {
    return SharedHandle<Range>(new Range(getStartByte(), getEndByte(),
                                         fileEntry_->getLength()));
  }
}

bool HttpRequest::isRangeSatisfied(const RangeHandle& range) const
{
  if(segment_.isNull()) {
    return true;
  }
  if((getStartByte() == range->getStartByte()) &&
     ((getEndByte() == 0) ||
      (getEndByte() == range->getEndByte())) &&
     ((fileEntry_->getLength() == 0) ||
      (fileEntry_->getLength() == range->getEntityLength()))) {
    return true;
  } else {
    return false;
  }  
}

static std::string getHostText(const std::string& host, uint16_t port)
{
  std::string hosttext = host;
  if(!(port == 80 || port == 443)) {
    strappend(hosttext, ":", util::uitos(port));
  }
  return hosttext;
}

std::string HttpRequest::createRequest()
{
  authConfig_ = authConfigFactory_->createAuthConfig(request_, option_);
  std::string requestLine = request_->getMethod();
  requestLine += " ";
  if(!proxyRequest_.isNull()) {
    if(getProtocol() == Request::PROTO_FTP &&
       request_->getUsername().empty() && !authConfig_.isNull()) {
      // Insert user into URI, like ftp://USER@host/
      std::string uri = getCurrentURI();
      assert(uri.size() >= 6);
      uri.insert(6, util::percentEncode(authConfig_->getUser())+"@");
      requestLine += uri;
    } else {
      requestLine += getCurrentURI();
    }
  } else {
    if(getDir() == A2STR::SLASH_C) {
      requestLine += getDir();
    } else {
      requestLine += getDir();
      requestLine += A2STR::SLASH_C;
    }
    requestLine += getFile();
    requestLine += getQuery();
  }
  requestLine += " HTTP/1.1\r\n";

  std::vector<std::pair<std::string, std::string> > builtinHds;
  builtinHds.reserve(20);
  builtinHds.push_back(std::make_pair("User-Agent:", userAgent_));
  std::string acceptTypes = "*/*";
  for(std::vector<std::string>::const_iterator i = acceptTypes_.begin(),
        eoi = acceptTypes_.end(); i != eoi; ++i) {
    strappend(acceptTypes, ",", (*i));
  }
  builtinHds.push_back(std::make_pair("Accept:", acceptTypes));
  if(contentEncodingEnabled_) {
    std::string acceptableEncodings;
#ifdef HAVE_LIBZ
    if(acceptGzip_) {
      acceptableEncodings += "deflate, gzip";
    }
#endif // HAVE_LIBZ
    if(!acceptableEncodings.empty()) {
      builtinHds.push_back
        (std::make_pair("Accept-Encoding:", acceptableEncodings));
    }
  }
  builtinHds.push_back
    (std::make_pair("Host:", getHostText(getURIHost(), getPort())));
  if(noCache_) {
    builtinHds.push_back(std::make_pair("Pragma:", "no-cache"));
    builtinHds.push_back(std::make_pair("Cache-Control:", "no-cache"));
  }
  if(!request_->isKeepAliveEnabled() && !request_->isPipeliningEnabled()) {
    builtinHds.push_back(std::make_pair("Connection:", "close"));
  }
  if(!segment_.isNull() && segment_->getLength() > 0 && 
     (request_->isPipeliningEnabled() || getStartByte() > 0)) {
    std::string rangeHeader = "bytes=";
    rangeHeader += util::itos(getStartByte());
    rangeHeader += "-";
    if(request_->isPipeliningEnabled()) {
      rangeHeader += util::itos(getEndByte());
    } else if(getProtocol() != Request::PROTO_FTP && endOffsetOverride_ > 0) {
      // FTP via http proxy does not support endbytes 
      rangeHeader += util::itos(endOffsetOverride_-1);
    }
    builtinHds.push_back(std::make_pair("Range:", rangeHeader));
  }
  if(!proxyRequest_.isNull()) {
    if(request_->isKeepAliveEnabled() || request_->isPipeliningEnabled()) {
      builtinHds.push_back(std::make_pair("Proxy-Connection:", "Keep-Alive"));
    } else {
      builtinHds.push_back(std::make_pair("Proxy-Connection:", "close"));
    }
  }
  if(!proxyRequest_.isNull() && !proxyRequest_->getUsername().empty()) {
    builtinHds.push_back(getProxyAuthString());
  }
  if(!authConfig_.isNull()) {
    builtinHds.push_back
      (std::make_pair("Authorization:",
                      strconcat("Basic ",
                                Base64::encode(authConfig_->getAuthText()))));
  }
  if(getPreviousURI().size()) {
    builtinHds.push_back(std::make_pair("Referer:", getPreviousURI()));
  }
  if(!cookieStorage_.isNull()) {
    std::string cookiesValue;
    std::vector<Cookie> cookies =
      cookieStorage_->criteriaFind(getHost(),
                                   getDir(),
                                   Time().getTime(),
                                   getProtocol() == Request::PROTO_HTTPS ?
                                   true : false);
    for(std::vector<Cookie>::const_iterator itr = cookies.begin(),
          eoi = cookies.end(); itr != eoi; ++itr) {
      strappend(cookiesValue, (*itr).toString(), ";");
    }
    if(!cookiesValue.empty()) {
      builtinHds.push_back(std::make_pair("Cookie:", cookiesValue));
    }
  }
  if(!ifModSinceHeader_.empty()) {
    builtinHds.push_back
      (std::make_pair("If-Modified-Since:", ifModSinceHeader_));
  }
  for(std::vector<std::pair<std::string, std::string> >::const_iterator i =
        builtinHds.begin(), eoi = builtinHds.end(); i != eoi; ++i) {
    std::vector<std::string>::const_iterator j = headers_.begin();
    std::vector<std::string>::const_iterator jend = headers_.end();
    for(; j != jend; ++j) {
      if(util::startsWith(*j, (*i).first)) {
        break;
      }
    }
    if(j == jend) {
      strappend(requestLine, (*i).first, " ", (*i).second, A2STR::CRLF);
    }
  }
  // append additional headers given by user.
  for(std::vector<std::string>::const_iterator i = headers_.begin(),
        eoi = headers_.end(); i != eoi; ++i) {
    strappend(requestLine, (*i), A2STR::CRLF);
  }
  requestLine += A2STR::CRLF;
  return requestLine;
}

std::string HttpRequest::createProxyRequest() const
{
  assert(!proxyRequest_.isNull());
  std::string hostport = getURIHost();
  strappend(hostport, ":", util::uitos(getPort()));

  std::string requestLine = "CONNECT ";
  strappend(requestLine, hostport, " HTTP/1.1\r\n");
  strappend(requestLine, "User-Agent: ", userAgent_, "\r\n");
  strappend(requestLine, "Host: ", hostport, "\r\n");
  // TODO Is "Proxy-Connection" needed here?
  //   if(request->isKeepAliveEnabled() || request->isPipeliningEnabled()) {
  //     requestLine += "Proxy-Connection: Keep-Alive\r\n";
  //   }else {
  //     requestLine += "Proxy-Connection: close\r\n";
  //   }
  if(!proxyRequest_->getUsername().empty()) {
    std::pair<std::string, std::string> auth = getProxyAuthString();
    strappend(requestLine, auth.first, " ", auth.second, A2STR::CRLF);
  }
  requestLine += A2STR::CRLF;
  return requestLine;
}

std::pair<std::string, std::string> HttpRequest::getProxyAuthString() const
{
  return std::make_pair
    ("Proxy-Authorization:",
     strconcat("Basic ",
               Base64::encode(strconcat(proxyRequest_->getUsername(),
                                        ":",
                                        proxyRequest_->getPassword()))));
}

void HttpRequest::enableContentEncoding()
{
  contentEncodingEnabled_ = true;
}

void HttpRequest::disableContentEncoding()
{
  contentEncodingEnabled_ = false;
}

void HttpRequest::addHeader(const std::string& headersString)
{
  std::vector<std::string> headers;
  util::split(headersString, std::back_inserter(headers), "\n", true);
  headers_.insert(headers_.end(), headers.begin(), headers.end());
}

void HttpRequest::addAcceptType(const std::string& type)
{
  acceptTypes_.push_back(type);
}

void HttpRequest::setCookieStorage
(const SharedHandle<CookieStorage>& cookieStorage)
{
  cookieStorage_ = cookieStorage;
}

void HttpRequest::setAuthConfigFactory
(const SharedHandle<AuthConfigFactory>& factory, const Option* option)
{
  authConfigFactory_ = factory;
  option_ = option;
}

void HttpRequest::setProxyRequest(const SharedHandle<Request>& proxyRequest)
{
  proxyRequest_ = proxyRequest;
}

bool HttpRequest::isProxyRequestSet() const
{
  return !proxyRequest_.isNull();
}

bool HttpRequest::authenticationUsed() const
{
  return !authConfig_.isNull();
}

const SharedHandle<AuthConfig>& HttpRequest::getAuthConfig() const
{
  return authConfig_;
}

} // namespace aria2
