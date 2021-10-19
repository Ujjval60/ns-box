#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

namespace util {

using namespace std;

char *get_shell() {
  char *sh = getenv("SHELL");
  if (sh == NULL) {
    sh = "/bin/sh";
  }
  return sh;
}

void fail(const char *msg) {
  perror(msg);
  exit(11);
}
void fail2(const char *msg) {
  cerr << msg << endl;
  exit(11);
}

void arg_err(const char *what) {
  cerr << what << ". See --help for more details\n";
  exit(10);
}

vector<string> split_string(const std::string &str, char delim) {
  auto it = str.begin();
  auto next_it = str.end();
  vector<string> res;
  while ((next_it = find(it, str.end(), delim)) != str.end()) {
    res.emplace_back(it, next_it);
    it = next_it + 1;
  }
  if (it < str.end())
    res.emplace_back(it, str.end());
  return res;
}
} // namespace util
