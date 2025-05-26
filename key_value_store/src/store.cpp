#include <rocksdb/db.h>
// #include <rocksdb/snapshot.h>
// #include <rocksdb/write_batch.h>
#include <string>
#include "store.h"
#include "utility"
#include <iostream>
Store::Store(std::string &path){
    options.create_if_missing = true;
    status = rocksdb::DB::Open(options, path, &db);
    std::cout<<status.ToString()<<'\n';
    std::cout<<"db created\n";
}
rocksdb::Status Store::PUT(std::pair<std::string,std::string> &key_value){
    return db->Put(this->write_options, key_value.first, key_value.second);
}
rocksdb::Status Store::DELETE(std::string &key){
    return db->Delete(this->write_options, key);
}
rocksdb::Status Store::GET(std::string &key,std::string &value){
    return db->Get(this->read_options, key, &value);
} 
