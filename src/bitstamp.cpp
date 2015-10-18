#include <string.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <sys/time.h>
#include "base64.h"
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <jansson.h>
#include "bitstamp.h"
#include "curl_fun.h"

/*OpenSSL is a software library to be used in applications that need to secure communications against eavesdropping or need to ascertain the identity of the party at the other end. It has found wide use in internet web servers, serving a majority of all web sites.OpenSSL contains an open-source implementation of the SSL and TLS protocols. The core library, written in the C programming language, implements basic cryptographic functions and provides various utility functions.*/
/*Jansson is a C library for encoding, decoding and manipulating JSON data. */
/*iomanip.h : Header providing parametric manipulators like setprecision*/
/*unistd.h is the name of the header file that provides access to the POSIX operating system API. unistd.h is typically made up largely of system call wrapper functions such as fork, pipe and I/O primitives (read, write, close, etc.).*/

namespace Bitstamp {

double getQuote(CURL *curl, bool isBid) {
  /*root declared as a pointer to json_t type data structure in jansson*/
  /*root will point to json_t structure having key:value pairs. Key can be "bid" or ask" among other things as we are reading from bitstamp ticker API*/
  json_t *root = getJsonFromUrl(curl, "https://www.bitstamp.net/api/ticker/", "");
  const char* quote;
  double quoteValue;
  /*isBid is bool being supplied as parameter to function*/
  if (isBid) {
    /*from root pointer, value corresponding to "bid" key retrieved*/
    quote = json_string_value(json_object_get(root, "bid"));
  } else {
    /*from root pointer, value corresponding to "ask" key retrieved*/
    quote = json_string_value(json_object_get(root, "ask"));
  }
  if (quote != NULL) {
      /*atof : Convert string to double*/
    quoteValue = atof(quote);
  } else {
    quoteValue = 0.0;
  }
 /*The reference count is used to track whether a value is still in use or not. When a value is created, it’s reference count is set to 1. If a reference to a value is kept (e.g. a value is stored somewhere for later use), its reference count is incremented, and when the value is no longer needed, the reference count is decremented. When the reference count drops to zero, there are no references left, and the value can be destroyed.*/
 /*void json_decref(json_t *json)
Decrement the reference count of json. As soon as a call to json_decref() drops the reference count to zero, the value is destroyed and it can no longer be used.*/
  json_decref(root);
  return quoteValue;
}


double getAvail(CURL *curl, Parameters params, std::string currency) {
  /*https://www.bitstamp.net/api/balance/ - Account balance*/
  json_t *root = authRequest(curl, params, "https://www.bitstamp.net/api/balance/", "");
  while (json_object_get(root, "message") != NULL) {
    sleep(1.0);
    std::cout << "<Bitstamp> Error with JSON in getAvail: " << json_dumps(root, 0) << ". Retrying..." << std::endl;
    /*root a pointer to json_t data structure would take value returned by authrequest to balance bitstamp API*/
    root = authRequest(curl, params, "https://www.bitstamp.net/api/balance/", "");
  }

  double availability = 0.0;
  /*currency is a string supplied as parameter to the function getAvail*/
  /*http://www.cplusplus.com/reference/string/string/compare/ if <std::string> str1.compare("str2")==0 then strings equal*/
  if (currency.compare("btc") == 0) {
    /*"btc_balance" is a key in the JSON object returned by json_object_get"*/
    availability = atof(json_string_value(json_object_get(root, "btc_balance")));
  }
  else if (currency.compare("usd") == 0) {
        /*"usd_balance" is a key in the JSON object returned by json_object_get"*/
    availability = atof(json_string_value(json_object_get(root, "usd_balance")));
  }
  json_decref(root);
  return availability;
}

/*To send order to bitstamp*/
/*To send order to bitstamp - direction is buy or sell*/
int sendOrder(CURL *curl, Parameters params, std::string direction, double quantity, double price) {
  /*Before sending order to bitstamp API, first get limitprice of the buy or sell order for the specific quantity*/
  double limPrice;  // define limit price to be sure to be executed
  if (direction.compare("buy") == 0) {
    limPrice = getLimitPrice(curl, quantity, false);
  }
  else if (direction.compare("sell") == 0) {
    limPrice = getLimitPrice(curl, quantity, true);
  }

  std::cout << "<Bitstamp> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$" << limPrice << "..." << std::endl;
/*class std::ostringstream : Output stream class to operate on strings.
Objects of this class use a string buffer that contains a sequence of characters. This sequence of characters can be accessed directly as a string object, using member str.*/

  std::ostringstream oss;
  oss << "https://www.bitstamp.net/api/" << direction << "/";
/*
public member function std::ostringstream::str 
1.string str() const;
2.void str (const string& s);
Get/set content
The first form (1) returns a string object with a copy of the current contents of the stream.
The second form (2) sets s as the contents of the stream, discarding any previous contents. The object preserves its open mode: if this includes ios_base::ate, the writing position is moved to the end of the new sequence.
*/
  std::string url = oss.str();
  oss.clear();
  oss.str("");
  
  /*This operator (<<) applied to an output stream is known as insertion operator. It will insert objects into the output stream*/
  oss << "amount=" << quantity << "&price=" << std::fixed << std::setprecision(2) << limPrice;
  std::string options = oss.str(); /*oss will return amount=&price=<limPrice value with precision = 2>*/
  /*url used as argument to function below stores the https://www.bitstamp.net/api/" << direction API address*/
  json_t *root = authRequest(curl, params, url, options);/*root will get assigned with this authRequest*/

  int orderId = json_integer_value(json_object_get(root, "id"));
  if (orderId == 0) {
    std::cout << "<Bitstamp> Order ID = 0. Message: " << json_dumps(root, 0) << std::endl;
  }
  std::cout << "<Bitstamp> Done (order ID: " << orderId << ")\n" << std::endl;
  json_decref(root);
  return orderId;
}


bool isOrderComplete(CURL *curl, Parameters params, int orderId) {
  if (orderId == 0) {
    return true;
  }
  std::ostringstream oss;
  oss << "id=" << orderId;
  std::string options = oss.str();
  json_t *root = authRequest(curl, params, "https://www.bitstamp.net/api/order_status/", options);
  std::string status = json_string_value(json_object_get(root, "status"));
  json_decref(root);
  if (status.compare("Finished") == 0) {
    return true;
  } else {
    return false;
  }
}


double getActivePos(CURL *curl, Parameters params) {
  return getAvail(curl, params, "btc");
}


double getLimitPrice(CURL *curl, double volume, bool isBid) {
  double limPrice;
  json_t *root;
  if (isBid) {
    root = json_object_get(getJsonFromUrl(curl, "https://www.bitstamp.net/api/order_book/", ""), "bids");
  } else {
    root = json_object_get(getJsonFromUrl(curl, "https://www.bitstamp.net/api/order_book/", ""), "asks");
  }

  // loop on volume
  double tmpVol = 0.0;
  int i = 0;
  while (tmpVol < volume) {
    // volumes are added up until the requested volume is reached
    tmpVol += atof(json_string_value(json_array_get(json_array_get(root, i), 1)));
    i++;
  }
  // return the second next offer
  limPrice = atof(json_string_value(json_array_get(json_array_get(root, i+1), 0)));
  json_decref(root);
  return limPrice;
}


json_t* authRequest(CURL *curl, Parameters params, std::string url, std::string options) {
  // nonce
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long nonce = (tv.tv_sec * 1000.0) + (tv.tv_usec * 0.001) + 0.5;

  std::ostringstream oss;
  oss << nonce << params.bitstampClientId << params.bitstampApi;
  unsigned char* digest;

  // Using sha256 hash engine
  digest = HMAC(EVP_sha256(), params.bitstampSecret, strlen(params.bitstampSecret), (unsigned char*)oss.str().c_str(), strlen(oss.str().c_str()), NULL, NULL);

  char mdString[SHA256_DIGEST_LENGTH+100];  // FIXME +100
  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    sprintf(&mdString[i*2], "%02X", (unsigned int)digest[i]);
  }

  oss.clear();
  oss.str("");

  oss << "key=" << params.bitstampApi << "&signature=" << mdString << "&nonce=" << nonce << "&" << options;
  std::string postParams = oss.str().c_str();

  CURLcode resCurl;  // cURL request
  // curl = curl_easy_init();
  if (curl) {
    std::string readBuffer;
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_POST,1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParams.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    resCurl = curl_easy_perform(curl);
    json_t *root;
    json_error_t error;

    while (resCurl != CURLE_OK) {
      std::cout << "<Bitstamp> Error with cURL. Retry in 2 sec...\n" << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);

    while (!root) {
      std::cout << "<Bitstamp> Error with JSON in authRequest:\n" << "Error: : " << error.text << ".  Retrying..." << std::endl;
      readBuffer = "";
      resCurl = curl_easy_perform(curl);
      while (resCurl != CURLE_OK) {
        std::cout << "<Bitstamp> Error with cURL. Retry in 2 sec...\n" << std::endl;
        sleep(2.0);
        readBuffer = "";
        resCurl = curl_easy_perform(curl);
      }
      root = json_loads(readBuffer.c_str(), 0, &error);
    }
    curl_easy_reset(curl);
    return root;
  }
  else {
    std::cout << "<Bitstamp> Error with cURL init." << std::endl;
    return NULL;
  }
}

}
