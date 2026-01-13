#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <utility>

#include "store.h"


static std::string a="a",b="b",c="c",d="d";
class StoreTest : public ::testing::Test{
protected:
    Store* store;  
    std::string dirName=std::filesystem::temp_directory_path() / "test_dir";
    void SetUp() override {   
        std::filesystem::create_directory(dirName);
        store=new Store(dirName);
        std::pair<std::string,std::string> p1={"a","b"},p2={"b","c"};
        store->PUT(p1);
        store->PUT(p2);
        
    }

    // TearDown() runs after each test (optional)
    void TearDown() override {
        // cleanup code if needed
        delete store;
        std::filesystem::remove_all(dirName);
    }
};
TEST_F(StoreTest, test_put) {
    std::pair<std::string,std::string> p1={"c","d"};
    bool v=store->PUT(p1);
    EXPECT_EQ(v,true);
    std::pair<std::string,bool> p=store->GET(c);
    EXPECT_EQ(p.second,true);
    EXPECT_EQ(p.first,std::string(d));


}
TEST_F(StoreTest, test_get) {
    std::pair<std::string,bool> p=store->GET(c);
    EXPECT_EQ(p.second,false);
    EXPECT_EQ(p.first,std::string(""));
    p=store->GET(a);
    EXPECT_EQ(p.second,true);
    EXPECT_EQ(p.first,b);
    
}
TEST_F(StoreTest, test_delete) {
    std::pair<std::string,bool> p=store->GET(a);
    EXPECT_EQ(p.second,true);
    EXPECT_EQ(p.first,std::string("b"));
    store->DELETE(a);
    p=store->GET(a);
    EXPECT_EQ(p.second,false);
    EXPECT_EQ(p.first,std::string(""));
}