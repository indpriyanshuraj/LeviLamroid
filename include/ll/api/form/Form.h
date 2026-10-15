#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "nlohmann/json.hpp"

namespace ll::form {

class Form {
protected:
    std::string mTitle;

public:
    explicit Form(std::string title = "") : mTitle(std::move(title)) {}
    virtual ~Form() = default;

    const std::string& getTitle() const { return mTitle; }
    void setTitle(std::string title) { mTitle = std::move(title); }

    virtual std::string serialize() const = 0;
};

class SimpleForm : public Form {
public:
    struct Button {
        std::string text;
        std::string imageType;
        std::string imagePath;
        std::function<void()> onClick;
    };

private:
    std::string mContent;
    std::vector<Button> mButtons;

public:
    explicit SimpleForm(std::string title = "", std::string content = "")
        : Form(std::move(title)), mContent(std::move(content)) {}

    SimpleForm& setContent(std::string content) {
        mContent = std::move(content);
        return *this;
    }

    SimpleForm& addButton(std::string text, std::function<void()> cb = nullptr) {
        mButtons.push_back({std::move(text), "", "", std::move(cb)});
        return *this;
    }

    SimpleForm& addButtonWithIcon(std::string text, std::string imgType, std::string imgPath, std::function<void()> cb = nullptr) {
        mButtons.push_back({std::move(text), std::move(imgType), std::move(imgPath), std::move(cb)});
        return *this;
    }

    std::string serialize() const override {
        nlohmann::json root;
        root["type"] = "form";
        root["title"] = mTitle;
        root["content"] = mContent;
        nlohmann::json btns = nlohmann::json::array();
        for (const auto& b : mButtons) {
            nlohmann::json btnObj;
            btnObj["text"] = b.text;
            if (!b.imagePath.empty()) {
                btnObj["image"] = {{"type", b.imageType}, {"data", b.imagePath}};
            }
            btns.push_back(btnObj);
        }
        root["buttons"] = btns;
        return root.dump();
    }

    void handleResponse(int selectedIndex) const {
        if (selectedIndex >= 0 && selectedIndex < (int)mButtons.size()) {
            if (mButtons[selectedIndex].onClick) {
                mButtons[selectedIndex].onClick();
            }
        }
    }
};

class ModalForm : public Form {
private:
    std::string mContent;
    std::string mButton1Text = "Yes";
    std::string mButton2Text = "No";
    std::function<void(bool)> mCallback;

public:
    explicit ModalForm(std::string title = "", std::string content = "")
        : Form(std::move(title)), mContent(std::move(content)) {}

    ModalForm& setContent(std::string content) {
        mContent = std::move(content);
        return *this;
    }

    ModalForm& setUpperButton(std::string text) {
        mButton1Text = std::move(text);
        return *this;
    }

    ModalForm& setLowerButton(std::string text) {
        mButton2Text = std::move(text);
        return *this;
    }

    ModalForm& onResponse(std::function<void(bool)> cb) {
        mCallback = std::move(cb);
        return *this;
    }

    std::string serialize() const override {
        nlohmann::json root;
        root["type"] = "modal";
        root["title"] = mTitle;
        root["content"] = mContent;
        root["button1"] = mButton1Text;
        root["button2"] = mButton2Text;
        return root.dump();
    }

    void handleResponse(bool choice) const {
        if (mCallback) {
            mCallback(choice);
        }
    }
};

} // namespace ll::form
