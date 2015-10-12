#include "curl_fun.h"
#include <unistd.h>
#include <iostream>

/*https://en.wikipedia.org/wiki/Unistd.h
In the C and C++ programming languages, unistd.h is the name of the header file that provides access to the POSIX operating system API. On Unix-like systems, the interface defined by unistd.h is typically made up largely of system call wrapper functions such as fork, pipe and I/O primitives (read, write, close, etc.).*/

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  /*Append is a public method of string class. We cast void pointer type userp to std::string pointer type and then access its append method : http://www.cplusplus.com/reference/string/string/append/ */
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

json_t* getJsonFromUrl(CURL* curl, std::string url, std::string postFields) {
  /*CURLOPT_URL - provide the URL to use in the request : CURLcode curl_easy_setopt(CURL *handle, CURLOPT_URL, char *URL);*/
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  /*CURLOPT_CONNECTTIMEOUT - timeout for the connect phase - CURLcode curl_easy_setopt(CURL *handle, CURLOPT_CONNECTTIMEOUT, long timeout); Pass a long (timeout). It should contain the maximum time in seconds that you allow the connection phase to the server to take.*/
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  if (!postFields.empty()) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  std::string readBuffer;  // data in readBuffer
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
  curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 3600);
  CURLcode resCurl = curl_easy_perform(curl);

  while (resCurl != CURLE_OK) {
    std::cout << "Error with cURL. Retry in 2 sec...\n" << std::endl;
    sleep(2.0);
    readBuffer = "";
    curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 0);
    resCurl = curl_easy_perform(curl);
  }
  // JSON infomation
  json_t *root;
  json_error_t error;
  root = json_loads(readBuffer.c_str(), 0, &error);

  while (!root) {
    std::cout << "Error with JSON:\n" << error.text << ". Retrying..." << std::endl;
    readBuffer = "";
    resCurl = curl_easy_perform(curl);
    while (resCurl != CURLE_OK) {
      std::cout << "Error with cURL. Retry in 2 sec...\n" << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);
  }
  return root;
}


