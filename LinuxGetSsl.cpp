#include "LinuxGetSsl.h"
class ParsedUrl {
public:
  const char *CompleteUrl;
  char *Service, *Host, *Port, *Path;

  ParsedUrl(const char *url) {
    // Assumes url points to static text but
    // does not check.

    CompleteUrl = url;

    pathBuffer = new char[strlen(url) + 1];
    const char *f;
    char *t;
    for (t = pathBuffer, f = url; *t++ = *f++;)
      ;

    Service = pathBuffer;

    const char Colon = ':', Slash = '/';
    char *p;
    for (p = pathBuffer; *p && *p != Colon; p++)
      ;

    if (*p) {
      // Mark the end of the Service.
      *p++ = 0;

      if (*p == Slash)
        p++;
      if (*p == Slash)
        p++;

      Host = p;

      for (; *p && *p != Slash && *p != Colon; p++)
        ;

      if (*p == Colon) {
        // Port specified.  Skip over the colon and
        // the port number.
        *p++ = 0;
        Port = +p;
        for (; *p && *p != Slash; p++)
          ;
      } else
        Port = p;

      if (*p)
        // Mark the end of the Host and Port.
        *p++ = 0;

      // Whatever remains is the Path.
      Path = p;
    } else
      Host = Path = p;
  }

  ~ParsedUrl() { delete[] pathBuffer; }

private:
  char *pathBuffer;
};

std::string getHeader(std::string &response) {
  size_t headerEnd = response.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    return "";
  }
  return response.substr(0, headerEnd);
}
std::string getBody(std::string &response) {
  size_t headerEnd = response.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    return "";
  }
  return response.substr(headerEnd + 4);
}

std::string getHtml(std::string url_str) {
  // Parse the URL
  ParsedUrl url(url_str.c_str());
  addrinfo *addrInfo;
  if (!url.Port || *url.Port == '\0') {
    url.Port = (char *)"443";
  }
  //   std::cout << "Host: " << url.Host << std::endl;
  //   std::cout << "Port: " << url.Port << std::endl;
  //   std::cout << "Path: " << url.Path << std::endl;

  // Get the host address.
  int status = getaddrinfo(url.Host, url.Port, NULL, &addrInfo);
  if (status != 0) {
    std::cerr << "Error getting host address: " << gai_strerror(status)
              << std::endl;
    return "";
  }

  // Create a TCP/IP socket.
  int sockfd =
      socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
  if (sockfd == -1) {
    std::cerr << "Error creating socket" << std::endl;
    return "";
  }

  // Connect the socket to the host address.
  if (connect(sockfd, addrInfo->ai_addr, addrInfo->ai_addrlen) == -1) {
    std::cerr << "Error connecting to host" << std::endl;
    return "";
  }

  // Initialize SSL
  SSL_library_init();
  SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
  if (ctx == NULL) {
    std::cerr << "Error creating SSL context" << std::endl;
    return "";
  }
  SSL *ssl = SSL_new(ctx);
  if (ssl == NULL) {
    std::cerr << "Error creating SSL" << std::endl;
    return "";
  }
  SSL_set_tlsext_host_name(ssl, url.Host);
  SSL_set_fd(ssl, sockfd);
  if (SSL_connect(ssl) == -1) {
    std::cerr << "Error connecting SSL" << std::endl;
    return "";
  }

  std::string path;

  if (!url.Path || *url.Path == '\0') {
    path = "/";
  } else if (url.Path[0] != '/') {
    path = "/" + std::string(url.Path);
  } else {
    path = url.Path;
  }

  //   std::cout << "Path: " << path << std::endl;

  std::string get11 = "GET " + path +
                      " HTTP/1.1\r\n"
                      "Host: " +
                      url.Host +
                      "\r\n"
                      "Connection: close\r\n"
                      "User-Agent: LinuxGetSsl/1.0\r\n"
                      "\r\n";

  ssize_t sent = SSL_write(ssl, get11.c_str(), get11.size());
  if (sent <= 0) {
    std::cerr << "Error sending GET message\n";
    return "";
  }
  // Read the HTTP response
  std::string response;
  char buffer[1024];
  ssize_t bytesRead;
  while ((bytesRead = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
    response.append(buffer, bytesRead);
  }
  if (bytesRead < 0) {
    std::cerr << "Error reading from socket\n";
    return "";
  }

  // Close the socket and free the address info structure.
  close(sockfd);
  SSL_free(ssl);
  SSL_CTX_free(ctx);
  freeaddrinfo(addrInfo);
  return getBody(response);
}

// int main(int argc, char **argv) {

//   if (argc != 2) {
//     std::cerr << "Usage: " << argv[0] << " url" << std::endl;
//     return 1;
//   }

//   std::string html = getHtml(argv[1]);
//   std::cout << html << std::endl;

//   return 0;
// }
