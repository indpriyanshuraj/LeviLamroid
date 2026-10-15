#pragma once
#include <cstdint>

class Level;

class DataLoadHelper {
public:
    virtual ~DataLoadHelper() = default;
};

class NewUniqueIdsDataLoadHelper : public DataLoadHelper {
public:
    Level* mLevel{nullptr};
    NewUniqueIdsDataLoadHelper() = default;
    ~NewUniqueIdsDataLoadHelper() override = default;
};

