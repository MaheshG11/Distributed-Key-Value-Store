#include <rocksdb/db.h>
#include <rocksdb/snapshot.h>
#include <rocksdb/write_batch.h>
#include <string>
#include "utility"

class Store{

    rocksdb::DB* db;
    rocksdb::Options options;
    rocksdb::Status status;
    rocksdb::WriteOptions write_options;
    rocksdb::ReadOptions read_options;
public:
    Store(std::string &path);
    rocksdb::Status PUT(std::pair<std::string,std::string> &key_value);
    rocksdb::Status DELETE(std::string &key);
    rocksdb::Status GET(std::string &key,std::string &value);
    
};


