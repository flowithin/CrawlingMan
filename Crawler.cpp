#include "HtmlParser.h"
#include "LinuxGetSsl.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <vector>

std::vector<std::string> urls;
std::vector<HtmlParser> documents;
pthread_mutex_t mutex_paths = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_foundpaths = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t done = PTHREAD_COND_INITIALIZER;
int numDocuments = 0;

std::string GetWork() {
  pthread_mutex_lock(&mutex_paths);
  while (urls.empty()) {
    pthread_cond_wait(&cond, &mutex_paths);
  }
  std::string url = urls.front();
  urls.erase(urls.begin());
  pthread_mutex_unlock(&mutex_paths);
  return url;
}

void AddWork(std::string &url) {
  pthread_mutex_lock(&mutex_paths);
  if (url.find("https://") == std::string::npos) {
    pthread_mutex_unlock(&mutex_paths);
    return;
  }
  urls.push_back(url);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex_paths);
}

// std::string UrlNormalize(std::string &url, std::string &base) {
//   if (url.find("http://") == 0 || url.find("https://") == 0) {
//     switch (url[0]) {
//     case '/':
//       return base + url;
//     }
//   } else {
//     return url;
//   }
// }

void *Traverse(std::string &url) {

  // Add the path to found paths
  pthread_mutex_lock(&mutex_foundpaths);
  std::string html = getHtml(url);
  HtmlParser parser(html.c_str(), html.size());
  documents.push_back(parser);
  pthread_mutex_unlock(&mutex_foundpaths);
  for (auto &link : parser.links) {
    AddWork(link.URL);
  }
  return nullptr;
}

void *WorkerThread(void *arg) {
  // get document
  while (1) {
    pthread_mutex_lock(&mutex_foundpaths);
    int numParsedDocuments = documents.size();
    pthread_mutex_unlock(&mutex_foundpaths);
    if (numParsedDocuments >= numDocuments) {
      break;
    }
    std::string url = GetWork();
    Traverse(url);
  }
  return nullptr;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0]
              << " <number of documents to parse> <number of threads>"
              << std::endl;
    return 1;
  }
  numDocuments = std::stoi(argv[1]);
  int numThreads = std::stoi(argv[2]);
  std::ifstream file("list.txt");
  std::string url;
  while (std::getline(file, url)) {
    urls.push_back(url);
  }

  std::vector<pthread_t> threads(numThreads);
  for (int i = 0; i < numThreads; i++) {
    pthread_create(&threads[i], nullptr, WorkerThread, nullptr);
  }
  // todo: we don't need to worry about urls become empty
  for (int i = 0; i < numThreads; i++) {
    pthread_join(threads[i], nullptr);
  }

  // log:
  // while (!urls.empty() && documents.size() < numDocuments) {
  //   std::cout << "-------------------PARSING STARTED-------------------"
  //             << std::endl;
  //   std::cout << "Parsing " << urls.front() << std::endl;
  //   std::string html = getHtml(urls.front());
  //   HtmlParser parser(html.c_str(), html.size());
  //   documents.push_back(parser);
  //   std::cout << "-------------------ADDING LINKS-------------------"
  //             << std::endl;
  //   for (auto &link : parser.links) {
  //     // for now, just skip relative links
  //     if (link.URL.find("http://") == std::string::npos &&
  //         link.URL.find("https://") == std::string::npos) {
  //       continue;
  //     }
  //     std::cout << "Adding " << link.URL << std::endl;
  //     urls.push_back(link.URL);
  //   }
  //   urls.erase(urls.begin());
  // }
  // std::cout << "-------------------PARSING COMPLETED-------------------"
  //           << std::endl;

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