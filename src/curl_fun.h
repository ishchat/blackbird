#ifndef CURL_FUN_H
#define CURL_FUN_H

#include <string>
#include <jansson.h>
#include <curl/curl.h>

//jansson is C library for working with JSON data

/*In computer programming, a callback is a piece of executable code that is passed as an argument to other code, 
which is expected to call back (execute) the argument at some convenient time.
A callback method is one which is passed as an argument in another method and which is invoked after some kind of event. The 'call back' nature of the argument is that, once its parent method completes, the function which this argument represents is then called; that is to say that the parent method 'calls back' and executes the method provided as an argument.*/
// general curl callback
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);


// return JSON data from not authentificated address
json_t* getJsonFromUrl(CURL* curl, std::string url, std::string postField);


#endif 
