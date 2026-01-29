#ifndef STORE
#define STORE
#include <rocksdb/db.h>
#include <rocksdb/snapshot.h>
#include <rocksdb/write_batch.h>
#include <string>
#include "utility"

class Store {
 public:
  Store(std::string& path);
  ~Store();
  bool PUT(std::pair<std::string, std::string>& key_value);
  bool DELETE(std::string& key);
  std::pair<std::string, bool> GET(const std::string& key);

 private:
  rocksdb::DB* db;
  rocksdb::Options options;
  rocksdb::WriteOptions write_options;
  rocksdb::ReadOptions read_options;

 public:
  rocksdb::Status status;
};
#endif
