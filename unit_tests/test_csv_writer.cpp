#include "gtest/gtest.h"
#include "../CsvWriter/csvWriter.h"
#include <filesystem>

class CsvWriterTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        removeTestFiles();
    }

    void removeTestFiles() {
        std::filesystem::remove("./Monotonicity.csv");
        std::filesystem::remove("./Balance.csv");
        std::filesystem::remove("./MemoryUsage.csv");
        std::filesystem::remove("./LookupTime.csv");
        std::filesystem::remove("./ResizeTime.csv");
        std::filesystem::remove("./InitTime.csv");
    }
};

TEST(CsvWriterTest, MonotonicityWriteTest) {
    CsvWriter<Monotonicity>& writer = CsvWriter<Monotonicity>::getInstance();

    Monotonicity data1("hash", "algo", 3., 100, "uniform", 4);
    Monotonicity data2("hash2", "algo2", 3., 100, "uniform", 4);
    writer.add(data1);
    writer.add(data2);

    writer.write("./");

    std::filesystem::path filePath = "./Monotonicity.csv";
    ASSERT_TRUE(std::filesystem::exists(filePath));
}

TEST(CsvWriterTest, BalanceWriteTest) {
    CsvWriter<Balance>& writer = CsvWriter<Balance>::getInstance();

    Balance data1("hash", "algo", 10, "uniform", 10, 5);
    Balance data2("hash2", "algo2", 10, "uniform", 10, 5);
    writer.add(data1);
    writer.add(data2);

    writer.write("./");

    std::filesystem::path filePath = "./Balance.csv";
    ASSERT_TRUE(std::filesystem::exists(filePath));

}

TEST(CsvWriterTest, MemoryUsageWriteTest) {
    CsvWriter<MemoryUsage>& writer = CsvWriter<MemoryUsage>::getInstance();

    MemoryUsage data1;
    MemoryUsage data2;
    writer.add(data1);
    writer.add(data2);

    writer.write("./");

    std::filesystem::path filePath = "./MemoryUsage.csv";
    ASSERT_TRUE(std::filesystem::exists(filePath));
}

TEST(CsvWriterTest, LookupTimeWriteTest) {
    CsvWriter<LookupTime>& writer = CsvWriter<LookupTime>::getInstance();

    LookupTime data1("test", "test", 2, 3, "unit", "algo", "bench", "distr", "func", 100);
    LookupTime data2("test", "test", 2, 3, "unit", "algo", "bench", "distr", "func", 100);
    writer.add(data1);
    writer.add(data2);

    writer.write("./");

    std::filesystem::path filePath = "./LookupTime.csv";
    ASSERT_TRUE(std::filesystem::exists(filePath));
}

TEST(CsvWriterTest, ResizeAndInitTimeWriteTest) {
    CsvWriter<ResizeTime>& resizeWriter = CsvWriter<ResizeTime>::getInstance();
    CsvWriter<InitTime>& initWriter = CsvWriter<InitTime>::getInstance();

    ResizeTime resizeData1("test", "test", 3, 4, "unit", "algo", "fun", 2);
    ResizeTime resizeData2("test", "test", 3, 4, "unit", "algo", "fun", 2);
    resizeWriter.add(resizeData1);
    resizeWriter.add(resizeData2);

    resizeWriter.write("./");

    std::filesystem::path resizeFilePath = "./ResizeTime.csv";
    ASSERT_TRUE(std::filesystem::exists(resizeFilePath));

    InitTime initData1("test", "test", 3, 4, "unit", "algo", "fun", 2);
    InitTime initData2("test", "test", 3, 4, "unit", "algo", "fun", 2);
    initWriter.add(initData1);
    initWriter.add(initData2);

    initWriter.write("./");

    std::filesystem::path initFilePath = "./InitTime.csv";
    ASSERT_TRUE(std::filesystem::exists(initFilePath));

}


