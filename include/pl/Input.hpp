#pragma once

/**
 * @file Input.hpp
 * @brief Input callback API for native mods.
 */

#include <functional>

#include "pl/Export.hpp"

namespace pl::input {

/**
 * @brief Touch event delivered from the Android view.
 */
struct TouchEvent {
  int action{};
  int pointerId{};
  float x{};
  float y{};
};

/**
 * @brief Keyboard event delivered from the Android view.
 */
struct KeyEvent {
  int keyCode{};
  unsigned int unicodeChar{};
  bool isKeyDown{};
};

/**
 * @brief Mouse button event delivered from the Android view.
 */
struct MouseEvent {
  int button{};
  bool isDown{};
};

using TouchCallback = std::function<bool(const TouchEvent &)>;
using KeyCallback = std::function<bool(const KeyEvent &)>;
using MouseCallback = std::function<bool(const MouseEvent &)>;

/**
 * @brief Registers a process-wide touch callback.
 */
PL_EXPORT void registerTouchCallback(TouchCallback callback);

/**
 * @brief Registers a process-wide key callback.
 */
PL_EXPORT void registerKeyCallback(KeyCallback callback);

/**
 * @brief Registers a process-wide mouse callback.
 */
PL_EXPORT void registerMouseCallback(MouseCallback callback);

/**
 * @brief Dispatches a touch event to all registered touch callbacks.
 * @return True if any callback swallowed/handled the event.
 */
PL_EXPORT bool dispatchTouchEvent(const TouchEvent &event);

/**
 * @brief Dispatches a key event to all registered key callbacks.
 * @return True if any callback swallowed/handled the event.
 */
PL_EXPORT bool dispatchKeyEvent(const KeyEvent &event);

/**
 * @brief Dispatches a mouse event to all registered mouse callbacks.
 * @return True if any callback swallowed/handled the event.
 */
PL_EXPORT bool dispatchMouseEvent(const MouseEvent &event);

/**
 * @brief Requests the Android soft keyboard.
 */
PL_EXPORT void showKeyboard();

/**
 * @brief Hides the Android soft keyboard.
 */
PL_EXPORT void hideKeyboard();

} // namespace pl::input
