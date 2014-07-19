#include "http_kit.hpp"
#include <curl/curl.h>
#include <sstream>
#include <cstdio>

namespace citynet {

  using namespace std;

  static const char* user_agent = 
    "curl/7.24.0 (x86_64-apple-darwin12.0) "
    "libcurl/7.24.0 OpenSSL/0.9.8x zlib/1.2.5";

  size_t write_data(void *buffer, size_t size, size_t nmemb, string *userp) {
    auto byte_length = size * nmemb;
    userp->append((const char*)buffer, byte_length);
    return byte_length;
  }

  static bool g_http_debug = false;

  void http_global_init(bool http_debug) {
    curl_global_init(CURL_GLOBAL_ALL);
    g_http_debug = http_debug;
  }

  void http_global_destroy() {
    curl_global_cleanup();
  }

  class HTTPHeaderList {
    public:
      HTTPHeaderList(const HTTPHeaderMap& headers) {

        std::ostringstream os;
        for (auto &h : headers) {
          os << h.first << ": " << h.second;  
          _header_list = curl_slist_append(_header_list, os.str().c_str());
          os.str({});
        }
      }

      ~HTTPHeaderList() {
        curl_slist_free_all(_header_list);
      }

      curl_slist* curl_list() { return _header_list; }


    private:
      curl_slist* _header_list = NULL;
  };


  class Http {
    public:
      Http(const string& url, const map<string, string>& headers):
        _headers(HTTPHeaderList(headers)) {
          _curl = curl_easy_init();
          curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
          curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 1L);
          curl_easy_setopt(_curl, CURLOPT_USERAGENT, user_agent);
          curl_easy_setopt(_curl, CURLOPT_MAXREDIRS, 50L);
          curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 1L);

          if (g_http_debug) curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1);
          if (g_http_debug) curl_easy_setopt(_curl, CURLOPT_HEADER, 1);

          curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers.curl_list());
        }
      ~Http() {
        curl_easy_cleanup(_curl);
      }

      CURL* curl() { return _curl; }

    private:
      CURL* _curl;
      HTTPHeaderList _headers;
  };

  int http_get(const string& url, const map<string, string>& headers, string& response) {
    auto client = Http(url, headers);
    curl_easy_setopt(client.curl(), CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(client.curl(), CURLOPT_WRITEDATA, &response);
    return curl_easy_perform(client.curl());
  }

  int http_get(const std::string& url, std::string& response) {
    return http_get(url, {{}}, response);
  }

  int http_post(const string& url, 
      const map<string, string>& headers, 
      const string& data,
      string& response) {
    auto c = Http(url, headers);
    curl_easy_setopt(c.curl(), CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(c.curl(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(c.curl(), CURLOPT_POST, 1);
    curl_easy_setopt(c.curl(), CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(c.curl(), CURLOPT_POSTFIELDSIZE, data.size());
    return curl_easy_perform(c.curl());
  }

  int http_put(const string& url, 
      const map<string, string>& headers, 
      const string& data, 
      string& response) {
    auto c = Http(url, headers);
    curl_easy_setopt(c.curl(), CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(c.curl(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(c.curl(), CURLOPT_PUT, 1);
    curl_easy_setopt(c.curl(), CURLOPT_READDATA, data.c_str());
    curl_easy_setopt(c.curl(), CURLOPT_INFILESIZE, data.size());
    return curl_easy_perform(c.curl());
  }

  const char* http_error_str(int code) {
    return curl_easy_strerror((CURLcode)code);
  }
}

