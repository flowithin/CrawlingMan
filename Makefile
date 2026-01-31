CXX = g++
CXXFLAGS = -std=c++11 -g

all: LinuxGetSsl

LinuxGetSsl: LinuxGetSsl.cpp
	$(CXX) $(CXXFLAGS) $^ -lssl -lcrypto -lz -o LinuxGetSsl

ParsePage:  ParsePage.cpp HtmlTags.h HtmlTags.cpp HtmlParser.h HtmlParser.cpp ParsePage.cpp
	$(CXX) $(CXXFLAGS) $^ -o ParsePage

Crawler:  Crawler.cpp  LinuxGetSsl.cpp  HtmlParser.cpp  HtmlTags.cpp
	$(CXX) $(CXXFLAGS) $^ -o  Crawler -lssl -lcrypto -lz

Parser:  HtmlTags.h HtmlTags.cpp HtmlParser.h HtmlParser.cpp
	$(CXX) $(CXXFLAGS) $^ -o Parser

.PHONY: clean

clean:
	rm -f LinuxGetSsl Parser Crawler