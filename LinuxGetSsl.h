#pragma once

#include <netdb.h>
#include <openssl/ssl.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <string>

std::string getHtml(std::string url_str);