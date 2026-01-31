#include "HtmlParser.h"
#include "LinuxGetSsl.h"
#include <fstream>
#include <iostream>

std::string UrlNormalize(std::string &url, std::string &base) {
  if (url.find("http://") == 0 || url.find("https://") == 0) {
    switch (url[0]) {
    case '/':
      return base + url;
    }
  } else {
    return url;
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <number of documents to parse>"
              << std::endl;
    return 1;
  }
  int numDocuments = std::stoi(argv[1]);
  std::ifstream file("list.txt");
  std::string url;
  std::vector<std::string> urls;
  std::vector<HtmlParser> documents;
  while (std::getline(file, url)) {
    urls.push_back(url);
  }
  while (!urls.empty() && documents.size() < numDocuments) {
    std::cout << "-------------------PARSING STARTED-------------------"
              << std::endl;
    std::cout << "Parsing " << urls.front() << std::endl;
    std::string html = getHtml(urls.front());
    HtmlParser parser(html.c_str(), html.size());
    documents.push_back(parser);
    std::cout << "-------------------ADDING LINKS-------------------"
              << std::endl;
    for (auto &link : parser.links) {
      // for now, just skip relative links
      if (link.URL.find("http://") == std::string::npos &&
          link.URL.find("https://") == std::string::npos) {
        continue;
      }
      std::cout << "Adding " << link.URL << std::endl;
      urls.push_back(link.URL);
    }
    urls.erase(urls.begin());
  }
  std::cout << "-------------------PARSING COMPLETED-------------------"
            << std::endl;

  // output
  std::ofstream outputFile("output.txt");
  for (auto document : documents) {
    outputFile << "--------------";
    for (auto w : document.titleWords) {
      outputFile << w << " ";
    }
    outputFile << "--------------" << std::endl;
    outputFile << document.base << std::endl;
    for (int i = 0; i < document.words.size(); i++) {
      outputFile << document.words[i] << " ";
      if (i % 5 == 0) {
        outputFile << "\n";
      }
    }
    outputFile << "\n\n\n";
  }
  outputFile.close();
  return 0;
}