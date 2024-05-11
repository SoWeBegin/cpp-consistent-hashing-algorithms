#include "gtest/gtest.h"
#include "../CsvWriter/csv_writer_handler.h"
#include <filesystem>

class CsvWriterHandlerTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        removeTestFiles();
    }

    void removeTestFiles() {
        std::filesystem::remove("./Monotonicity.csv");
        std::filesystem::remove("./Balance.csv");
    }
};

TEST(CsvWriterHandlerTest, MonotonicityWriteTest) {
    CsvWriterHandler<Monotonicity> handler;

    CsvWriter<Monotonicity>& writer = handler.get_writer<Monotonicity>();

    Monotonicity data1("hash", "algo", 3., 100, "uniform", 4);
    Monotonicity data2("hash2", "algo2", 3., 100, "uniform", 4);
    writer.add(data1);
    writer.add(data2);

    handler.write_all("./");

    std::filesystem::path filePath = "./Monotonicity.csv";
    ASSERT_TRUE(std::filesystem::exists(filePath));
}

TEST(CsvWriterHandlerTest, MultipleDataTypeWriteTest) {
    CsvWriterHandler<Monotonicity, Balance, InitTime> handler;

    CsvWriter<Monotonicity>& monotonicityWriter = handler.get_writer<Monotonicity>();

    Monotonicity monotonicityData1("hash", "algo", 3., 100, "uniform", 4);
    Monotonicity monotonicityData2("hash2", "algo2", 3., 100, "uniform", 4);
    monotonicityWriter.add(monotonicityData1);
    monotonicityWriter.add(monotonicityData2);

    CsvWriter<Balance>& balanceWriter = handler.get_writer<Balance>();

    Balance balanceData1("hash", "algo", 10, "uniform", 10, 6);
    Balance balanceData2("hash2", "algo2", 10, "uniform", 10, 5);
    balanceWriter.add(balanceData1);
    balanceWriter.add(balanceData2);

    handler.write_all("./");

    std::filesystem::path monotonicityFilePath = "./Monotonicity.csv";
    ASSERT_TRUE(std::filesystem::exists(monotonicityFilePath));

    std::filesystem::path balanceFilePath = "./Balance.csv";
    ASSERT_TRUE(std::filesystem::exists(balanceFilePath));

    std::filesystem::path inittimeFilePath = "/path/to/directory/InitTime.csv";
    ASSERT_FALSE(std::filesystem::exists(inittimeFilePath));
}

