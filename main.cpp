#include <iostream>
#include <iomanip> // for formatting output
#include <algorithm> // for transform
#include <cctype> // various character functions 
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "libxml/HTMLParser.h"
#include "libxml/xpath.h"
#include <string>

using namespace std;

struct memory {
	char* response;
	size_t size;
};

typedef struct stat_table {
	string stats[19];
};

static size_t write_cb(char* data, size_t size, size_t nmemb, void *clientp) {
	size_t realsize = size * nmemb;
	struct memory* mem = (struct memory*)clientp;
	char* ptr = (char *) realloc(mem->response, mem->size + realsize + 1);
	if (!ptr) {
		free(ptr);
		return 0;
	}

	mem->response = ptr;
	memcpy(&(mem->response[mem->size]), data, realsize);
	mem->size += realsize;
	mem->response[mem->size] = 0;

	return realsize;
}

struct memory GetRequest(CURL* handle, const char* url) {
	CURLcode res;
	struct memory mem;

	mem.response = (char *) malloc(1);
	mem.size = 0;

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&mem);
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla / 5.0 (Windows NT 10.0; Win64; x64)");
	res = curl_easy_perform(handle);

	if (res != CURLE_OK) {
		cout << "GET request failed: " << curl_easy_strerror(res) << endl;
	}
	
	return mem;
}

bool is_num(string s) {
	try {
		int x = stoi("");
		return true;
	}
	catch (...) {
		return false;
	}
}

void printStatRow(string stat, string base, string mn, string mx)
{
	cout << left << setw(10) << setfill(' ') << stat;
	cout << left << setw(5) << setfill(' ') << base;
	cout << left << setw(5) << setfill(' ') << mn;
	cout << left << setw(5) << setfill(' ') << mx;
	cout << endl;
}

int main() {
	curl_global_init(CURL_GLOBAL_ALL);
	
	CURL* handle = curl_easy_init();
	string p = "";
	cout << "View pokemon stats ('q' to quit): ";
	cin >> p;
	while (p != "q") {
		std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c) { return std::tolower(c); });
		string url = "https://pokemondb.net/pokedex/" + p;
		struct memory res = GetRequest(handle, url.c_str());

		htmlDocPtr doc = htmlReadMemory(res.response, (unsigned long)res.size, NULL, NULL, HTML_PARSE_NOERROR);
		xmlXPathContextPtr context = xmlXPathNewContext(doc);
		xmlXPathObjectPtr elements = xmlXPathEvalExpression((xmlChar*)"//table[contains(@class,'vitals-table')]", context);

		xmlXPathSetContextNode(elements->nodesetval->nodeTab[3], context);
		xmlNodePtr* stats = xmlXPathEvalExpression((xmlChar*)".//td[contains(@class, 'cell-num')]", context)->nodesetval->nodeTab;

		stat_table s;
		for (int i = 0; i < 19; i++) {
			s.stats[i] = (char*)(xmlNodeGetContent(stats[i]));
		}

		free(stats);
		free(res.response);
		xmlXPathFreeContext(context);
		xmlFreeDoc(doc);

		printStatRow(p +"'s Base Stats", "", "", "");
		printStatRow("", "Base", "Min", "Max");
		printStatRow("HP", s.stats[0], s.stats[1], s.stats[2]);
		printStatRow("Atk", s.stats[3], s.stats[4], s.stats[5]);
		printStatRow("Def", s.stats[6], s.stats[7], s.stats[8]);
		printStatRow("Sp. Atk", s.stats[9], s.stats[10], s.stats[11]);
		printStatRow("Sp. Def", s.stats[12], s.stats[13], s.stats[14]);
		printStatRow("Spd", s.stats[15], s.stats[16], s.stats[17]);
		printStatRow("Total", s.stats[18], "", "");

		cout << "View pokemon stats ('q' to quit): ";
		cin >> p;
	}
	xmlCleanupParser();
	curl_easy_cleanup(handle);
	curl_global_cleanup();
	return 0;
}