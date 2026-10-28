#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <functional>

namespace fabric::container {

struct ContainerSlotInfo {
    int slot;
    std::string itemIdentifier;
    int count;
    int damage;
    std::string customName;
};

using ContainerPreviewCallback = void (*)(const char* containerType, int x, int y, int z, int slotCount, const char* jsonItems);

class ContainerPreviewEngine {
public:
    static ContainerPreviewEngine& getInstance() {
        static ContainerPreviewEngine instance;
        return instance;
    }

    void setTargetedBlock(int x, int y, int z, const std::string& blockName) {
        std::lock_guard lock(mMutex);
        mTargetedX = x;
        mTargetedY = y;
        mTargetedZ = z;
        mTargetedBlock = blockName;
    }

    bool getTargetedBlock(int* x, int* y, int* z, std::string& blockName) {
        std::lock_guard lock(mMutex);
        if (mTargetedBlock.empty()) return false;
        if (x) *x = mTargetedX;
        if (y) *y = mTargetedY;
        if (z) *z = mTargetedZ;
        blockName = mTargetedBlock;
        return true;
    }

    void registerCallback(ContainerPreviewCallback cb) {
        if (!cb) return;
        std::lock_guard lock(mMutex);
        mCallbacks.push_back(cb);
    }

    void notifyPreview(const char* containerType, int x, int y, int z, int slotCount, const char* jsonItems) {
        std::lock_guard lock(mMutex);
        for (auto cb : mCallbacks) {
            if (cb) {
                try {
                    cb(containerType, x, y, z, slotCount, jsonItems);
                } catch (...) {}
            }
        }
    }

    static bool isContainerBlock(const std::string& blockName) {
        if (blockName.find("chest") != std::string::npos) return true;
        if (blockName.find("barrel") != std::string::npos) return true;
        if (blockName.find("shulker_box") != std::string::npos) return true;
        if (blockName.find("furnace") != std::string::npos) return true;
        if (blockName.find("smoker") != std::string::npos) return true;
        if (blockName.find("blast_furnace") != std::string::npos) return true;
        if (blockName.find("hopper") != std::string::npos) return true;
        if (blockName.find("dispenser") != std::string::npos) return true;
        if (blockName.find("dropper") != std::string::npos) return true;
        if (blockName.find("brewing_stand") != std::string::npos) return true;
        return false;
    }

private:
    ContainerPreviewEngine() = default;
    std::mutex mMutex;
    int mTargetedX{0};
    int mTargetedY{0};
    int mTargetedZ{0};
    std::string mTargetedBlock;
    std::vector<ContainerPreviewCallback> mCallbacks;
};

} // namespace fabric::container
