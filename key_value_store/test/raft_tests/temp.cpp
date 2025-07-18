// processor_test.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "echo.grpc.pb.h"
#include "mock_echo_service.h"
#include "processor.h"

struct ProcessTestCase {
    std::string input_message;
    grpc::Status grpc_status;
    std::string grpc_response;
    std::string expected_result;
};

class ProcessMessageTest : public ::testing::TestWithParam<ProcessTestCase> {
protected:
    MockEchoServiceStub* mock_stub = nullptr;
    std::unique_ptr<MessageProcessor> processor;

    void SetUp() override {
        auto stub = std::make_unique<MockEchoServiceStub>();
        mock_stub = stub.get();

        EXPECT_CALL(*mock_stub, Echo)
            .WillRepeatedly([&](grpc::ClientContext*, const EchoRequest& req, EchoResponse* res) {
                const auto& param = GetParam();
                if (req.message() == param.input_message) {
                    if (param.grpc_status.ok()) {
                        res->set_message(param.grpc_response);
                    }
                    return param.grpc_status;
                }
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "no match");
            });

        processor = std::make_unique<MessageProcessor>(std::move(stub));
    }
};

TEST_P(ProcessMessageTest, HandlesDifferentInputs) {
    const auto& param = GetParam();
    std::string result = processor->process_message(param.input_message);
    EXPECT_EQ(result, param.expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    AllCases,
    ProcessMessageTest,
    ::testing::Values(
        ProcessTestCase{"ping", grpc::Status::OK, "pong", "ACK"},
        ProcessTestCase{"hello", grpc::Status::OK, "hi there", "GREET"},
        ProcessTestCase{"hi", grpc::Status::OK, "something", "UNKNOWN_RESPONSE"},
        ProcessTestCase{"fail", grpc::Status(grpc::StatusCode::UNAVAILABLE, "server down"), "", "error: server down"}
    )
);
