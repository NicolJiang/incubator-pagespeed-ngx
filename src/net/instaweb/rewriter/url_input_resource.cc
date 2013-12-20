/*
 * Copyright 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: sligocki@google.com (Shawn Ligocki)
//         jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/url_input_resource.h"

#include "base/logging.h"               // for COMPACT_GOOGLE_LOG_FATAL, etc
#include "net/instaweb/http/public/response_headers.h"
#include "net/instaweb/rewriter/public/rewrite_options.h"
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/util/public/google_url.h"

namespace net_instaweb {

namespace {

// Constructs a cache key that is
// a) the URL itself if is_authorized_domain is true
// b) the URL prefixed with unauth:// or unauths://, after removing the existing
//    protocol prefix, if is_authorized_domain is false.
GoogleString GetCacheKey(const StringPiece& url, bool is_authorized_domain) {
  GoogleUrl gurl(url);
  DCHECK(gurl.IsWebValid());
  if (!is_authorized_domain) {
    GoogleString url_prefix = "unauth://";
    if (gurl.SchemeIs("https")) {
      url_prefix = "unauths://";
    } else if (!gurl.SchemeIs("http")) {
      // This should really not happen! Crash now.
      CHECK(false);
    }
    return StrCat(url_prefix, gurl.HostAndPort(), gurl.PathAndLeaf());
  }
  return url.as_string();
}

}  // namespace

UrlInputResource::UrlInputResource(RewriteDriver* rewrite_driver,
                                   const ContentType* type,
                                   const StringPiece& url,
                                   bool is_authorized_domain)
    : CacheableResourceBase("url_input_resource", url,
                            GetCacheKey(url, is_authorized_domain),
                            type, rewrite_driver) {
  set_is_authorized_domain(is_authorized_domain);
  if (!is_authorized_domain) {
    GoogleUrl tmp_url(url);
    if (tmp_url.IsWebValid()) {
      // Store away the domain so that it can be authorized in PrepareRequest
      // before the actual Fetch is issued.
      // TODO(anupama): What should we do if the URL turns out to be invalid?
      tmp_url.Origin().CopyToString(&origin_);
    }
  }
  response_headers()->set_implicit_cache_ttl_ms(
      rewrite_options()->implicit_cache_ttl_ms());
  response_headers()->set_min_cache_ttl_ms(
      rewrite_options()->min_cache_ttl_ms());
  set_disable_rewrite_on_no_transform(
      rewrite_options()->disable_rewrite_on_no_transform());
}

UrlInputResource::~UrlInputResource() {
}

void UrlInputResource::InitStats(Statistics* stats) {
  CacheableResourceBase::InitStats("url_input_resource", stats);
}

void UrlInputResource::PrepareRequest(
    const RequestContextPtr& request_context, RequestHeaders* headers) {
  if (!is_authorized_domain() && !origin_.empty()) {
    request_context->AddSessionAuthorizedFetchOrigin(origin_);
  }
}

}  // namespace net_instaweb
