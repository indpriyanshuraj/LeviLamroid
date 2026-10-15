#pragma once
#include "mc/deps/nbt/Tag.h"
#include "mc/deps/nbt/StringTag.h"
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <type_traits>

class CompoundTagVariant {
public:
    std::shared_ptr<Tag> mTag;

    CompoundTagVariant() = default;
    CompoundTagVariant(std::shared_ptr<Tag> tag) : mTag(std::move(tag)) {}
    CompoundTagVariant(Tag const& tag) : mTag(tag.copy()) {}

    CompoundTagVariant(std::string str) : mTag(std::make_shared<StringTag>(std::move(str))) {}
    CompoundTagVariant(const char* str) : mTag(std::make_shared<StringTag>(str)) {}
    CompoundTagVariant(std::string_view str) : mTag(std::make_shared<StringTag>(str)) {}

    template <typename T, typename = std::enable_if_t<std::is_base_of_v<Tag, std::decay_t<T>>>>
    CompoundTagVariant(T const& tag) : mTag(tag.copy()) {}

    template <typename T, typename = std::enable_if_t<std::is_base_of_v<Tag, std::decay_t<T>> && !std::is_same_v<std::decay_t<T>, Tag>>>
    CompoundTagVariant(T&& tag) : mTag(std::make_shared<std::decay_t<T>>(std::forward<T>(tag))) {}

    CompoundTagVariant& operator=(std::string str) {
        mTag = std::make_shared<StringTag>(std::move(str));
        return *this;
    }
    CompoundTagVariant& operator=(const char* str) {
        mTag = std::make_shared<StringTag>(str);
        return *this;
    }
    CompoundTagVariant& operator=(std::string_view str) {
        mTag = std::make_shared<StringTag>(str);
        return *this;
    }
    template <typename T, typename = std::enable_if_t<std::is_base_of_v<Tag, std::decay_t<T>>>>
    CompoundTagVariant& operator=(T const& tag) {
        mTag = tag.copy();
        return *this;
    }
    template <typename T, typename = std::enable_if_t<std::is_base_of_v<Tag, std::decay_t<T>> && !std::is_same_v<std::decay_t<T>, Tag>>>
    CompoundTagVariant& operator=(T&& tag) {
        mTag = std::make_shared<std::decay_t<T>>(std::forward<T>(tag));
        return *this;
    }

    template <typename T>
    [[nodiscard]] bool hold() const {
        return dynamic_cast<T const*>(mTag.get()) != nullptr;
    }

    template <typename T>
    [[nodiscard]] T const& get() const {
        return *dynamic_cast<T const*>(mTag.get());
    }

    template <typename T>
    [[nodiscard]] T& get() {
        return *dynamic_cast<T*>(mTag.get());
    }

    [[nodiscard]] Tag* getTag() { return mTag.get(); }
    [[nodiscard]] Tag const* getTag() const { return mTag.get(); }
    [[nodiscard]] Tag::Type getId() const { return mTag ? mTag->getId() : Tag::Type::End; }

    bool operator==(CompoundTagVariant const& other) const {
        if (!mTag && !other.mTag) return true;
        if (!mTag || !other.mTag) return false;
        return mTag->equals(*other.mTag);
    }
    bool operator!=(CompoundTagVariant const& other) const {
        return !(*this == other);
    }
};

class ListTag : public Tag {
public:
    std::vector<CompoundTagVariant> mList;

    ListTag() = default;

    [[nodiscard]] size_t size() const { return mList.size(); }
    [[nodiscard]] bool empty() const { return mList.empty(); }
    [[nodiscard]] CompoundTagVariant const& operator[](size_t index) const { return mList[index]; }
    [[nodiscard]] CompoundTagVariant& operator[](size_t index) { return mList[index]; }

    void add(std::unique_ptr<Tag> tag) {
        if (tag) {
            mList.emplace_back(std::shared_ptr<Tag>(std::move(tag)));
        }
    }

    [[nodiscard]] auto begin() const { return mList.begin(); }
    [[nodiscard]] auto end() const { return mList.end(); }
    [[nodiscard]] auto begin() { return mList.begin(); }
    [[nodiscard]] auto end() { return mList.end(); }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::List; }
    [[nodiscard]] std::string toString() const override { return "TAG_List[" + std::to_string(mList.size()) + "]"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::List;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        auto res = std::make_unique<ListTag>();
        for (auto const& item : mList) {
            if (item.mTag) {
                res->add(item.mTag->copy());
            }
        }
        return res;
    }
};
