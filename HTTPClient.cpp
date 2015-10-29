#include "HTTPClient.h"
#include <cstring>
#include <sstream>


struct Bufinfo
{
    /* data */
    char* buf;
    int maxsize;
    int len;
};

size_t WriterCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  Bufinfo *p = (Bufinfo*) userp;
  //std::cout<<"in writer"<<std::endl;
  // assert p->len + realsize < p->maxsize
  memcpy(p->buf + p->len, contents, realsize);
  p->len += realsize;
  return realsize;
}

HTTPClient::HTTPClient(const char* url, size_t cap, OffsetMgr* o)
:BlockingBuffer(url, cap, o)
,urlparser(url)
{
    this->curl = curl_easy_init();
    curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, WriterCallback);
    curl_easy_setopt(this->curl, CURLOPT_FORBID_REUSE, 1L);
    this->AddHeaderField(HOST,urlparser.Host());
}

HTTPClient::~HTTPClient() {
    curl_easy_cleanup(this->curl);
}

bool HTTPClient::SetMethod(Method m) {
    this->method = m;
    return true;
}

bool HTTPClient::AddHeaderField(HeaderField f, const char* v) {
    if(v == NULL) {
        // log warning
        return false;
    } 
    this->fields[f] = std::string(v);
    return true;
}

const char* GetFieldString(HeaderField f) {
    switch(f) {
        case HOST: return "host";
        case RANGE: return "range";
        case DATE:  return "date";
        case CONTENTLENGTH: return "contentlength";
        default:
            return "unknown";
    }
}

// buffer size should be at lease len
// read len data from offest
size_t HTTPClient::fetchdata(size_t offset, char* data, size_t len) {
    if(len == 0)  return 0;

    Bufinfo bi;
    bi.buf = data;
    bi.maxsize = len;
    bi.len = 0;
    
    CURL *curl_handle = this->curl;
    CURLcode res;
    struct curl_slist *chunk = NULL;
    char rangebuf[128];

    curl_easy_setopt(curl_handle, CURLOPT_URL, this->sourceurl);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriterCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&bi);

    sprintf(rangebuf, "%d-%d", offset, offset + len - 1);
    this->AddHeaderField(RANGE, rangebuf);
    std::stringstream sstr;
    
    std::map<HeaderField, std::string>::iterator it;
    for(it = this->fields.begin(); it != this->fields.end(); it++) {
        sstr<<GetFieldString(it->first)<<":"<<it->second;
        chunk = curl_slist_append(chunk, sstr.str().c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    res = curl_easy_perform(curl_handle);

    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
        bi.len = -1;
    }

    return bi.len;
}
