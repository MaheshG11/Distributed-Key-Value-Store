#include <rocksdb/db.h>
// #include <rocksdb/snapshot.h>
// #include <rocksdb/write_batch.h>
#include <iostream>
#include <string>
#include "store.h"
#include "utility"
Store::Store(std::string& path) {
  options.create_if_missing = true;
  std::cout << "creating db ...\n";
  status = rocksdb::DB::Open(options, path, &db);
}
Store::~Store() {
  delete db;
}
bool Store::PUT(std::pair<std::string, std::string>& key_value) {
  return (db->Put(this->write_options, key_value.first, key_value.second)).ok();
}
bool Store::DELETE(std::string& key) {
  return (db->Delete(this->write_options, key)).ok();
}
std::pair<std::string, bool> Store::GET(const std::string& key) {
  std::string value;
  bool ok = (db->Get(this->read_options, key, &value)).ok();
  return std::make_pair(value, ok);
}
