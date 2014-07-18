#ifndef INCLUDE_COV_HTTP_KIT
#define INCLUDE_COV_HTTP_KIT

#include <string>
#include <map>

namespace cov {

void http_global_init(bool verbose = false);
void http_global_destroy();

typedef std::pair<std::string, std::string> HTTPHeader;
typedef std::map<std::string, std::string> HTTPHeaderMap;

int http_get(const std::string& url, 
    const std::map<std::string, std::string>& headers, 
    std::string& response);
int http_get(const std::string& url, std::string& response);

int http_post(const std::string& url, 
    const std::map<std::string, std::string>& headers, 
    const std::string& data, 
    std::string& response);


int http_put(const std::string& url, 
    const std::map<std::string, std::string>& headers, 
    const std::string& data, 
    std::string& response);
const char* http_error_str( int code);

}

#endif

