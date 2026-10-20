# LeviLamroid Mod Template

A production-ready C++20 starter template for building native client mods for **Minecraft Bedrock Edition (Android ARM64)** using the **LeviLamroid** (`v1.0.0-beta`) mod loader runtime on **LeviLauncher**.

> [!NOTE]
> **Beta Testing Status**: This template targets **LeviLamroid v1.0.0-beta**. As the loader runtime is under active development, be sure to keep your LeviLamroid installation updated to receive the latest hook stability fixes and RVA mappings.

---

## Features Included in This Template

1. **LeviLamina & LeviLauncher Universal Lifecycle**: Uses `ll::mod::NativeMod` and the `LL_REGISTER_MOD` macro to export both LeviLauncher PreLoader (`PLGetModRegistration`) and LeviLamina (`ll_mod_load`) C ABIs.
2. **Fabric RenderDragon 3D ESP & 2D HUD**: Subscribes to `ll::event::RenderLevelEvent` (3D in-world geometry, entity ESP, tracers, waypoint markers) and `ll::event::UIRenderEvent` (2D in-game HUDs, armor status, coords overlay).
3. **On-Screen Floating HUD Button**: Demonstrates `pl::modmenu::registerButton` to create customizable on-screen touch buttons for mobile users.
4. **LeviLauncher Drawer Module**: Registers a toggle switch in the launcher top-bar menu.
5. **Hardware Input Interception**: Captures physical keyboard shortcuts (e.g. key `O` to toggle features) and touch events without interfering with the player camera.
6. **Event Bus Integration**: Subscribes to `ll::event::StartMenuOpenEvent`, `RenderLevelEvent`, `UIRenderEvent`, and `RenderFramePrepareEvent`.
7. **Persistent JSON Configuration**: Automatically reads and writes `/sdcard/Android/media/org.levimc.launcher/minecraft/<profile>/mods/<ModName>/config.json` using `nlohmann::json`.
8. **One-Click Packaging & Deployment**: `package.sh` builds the `.so`, generates `.levipack` and `.zip` archives, and auto-deploys to active LeviLauncher profiles.

---

## Project Structure

```
template/
├── CMakeLists.txt          # CMake build script configured for Android ARM64
├── manifest.json           # LeviLauncher mod descriptor
├── package.sh              # One-click build, pack, and deployment script
├── README.md               # This guide
└── src/
    ├── ExampleMod.h        # Mod class definition inheriting ll::mod::NativeMod
    └── ExampleMod.cpp      # Implementation with event listeners and HUD buttons
```

---

## Quick Start: Building the Template

### Option 1: On-Device Building with Termux
Run directly in your Termux or Android environment:
```bash
cd template
bash package.sh
```

### Option 2: Cross-Compiling on Linux x86_64
Using Android NDK:
```bash
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-28 \
      ..
make -j4
```

---

## How to Customize for Your Mod

### 1. Rename the Mod
- In `manifest.json`: Change `"name"`, `"description"`, `"uuid"`, and `"entry": "lib<your_mod>.so"`.
- In `CMakeLists.txt`: Update `project(...)` and `OUTPUT_NAME "<your_mod>"`.
- In `src/ExampleMod.h` and `src/ExampleMod.cpp`: Change the class name and `NativeMod("<YourModName>")`.
- In `package.sh`: Update the output file names.

### 2. Add an On-Screen HUD Button
In `src/ExampleMod.cpp`:
```cpp
pl::modmenu::ButtonInfo btn;
btn.buttonId = "my_button";
btn.moduleId = "MyMod";
btn.displayName = "My Feature";
btn.label = "Run";
btn.behavior = pl::modmenu::ButtonBehavior::Click;
btn.normalBgColor = 0xAA1E293B; // Dark Cyan ARGB
btn.activeBgColor = 0xDD00E5FF; // Glow Cyan
btn.textColor = 0xFFFFFFFF;
btn.onEvent = [](std::string_view, pl::modmenu::ButtonEvent ev, float) {
    if (ev == pl::modmenu::ButtonEvent::Click) {
        // Your action here!
    }
};
pl::modmenu::registerButton(btn);
```

### 3. Intercept Keyboard Shortcuts
In `src/ExampleMod.cpp`:
```cpp
pl::input::registerKeyCallback([](const pl::input::KeyEvent& ev) -> bool {
    // Android KeyCodes: 41='M', 43='O', 111=ESC, 4=BACK
    if (ev.isKeyDown && ev.keyCode == 41) {
        // Toggle your mod menu
        return true; // Consume event so player doesn't move
    }
    return false;
});
```

### 4. Hook Minecraft Functions by RVA
To detour any function in `libminecraftpe.so`:
```cpp
#include <levilamroid/api/mcpe/Offsets.h>
#include <ll/api/memory/Hook.h>

void* sOriginalLocalPlayerMove = nullptr;
void hooked_LocalPlayer_move(void* player, float x, float y, float z) {
    // Custom movement logic or speed multiplier...
    auto orig = reinterpret_cast<void(*)(void*, float, float, float)>(sOriginalLocalPlayerMove);
    orig(player, x, y, z);
}

// In enable():
uintptr_t rva = levilamroid::mcpe::getOffsets().localPlayerMove;
ll::memory::hookRva(rva, (void*)&hooked_LocalPlayer_move, &sOriginalLocalPlayerMove);
```

### 5. Access Built-In Libraries
LeviLamroid automatically provides:
- `#include <nlohmann/json.hpp>` for JSON parsing.
- `#include <glm/glm.hpp>` for vector and matrix mathematics.
- `#include <imgui.h>` and `#include <imgui_impl_opengl3.h>` for Dear ImGui.
- `#include <mc/client/ClientInstance.h>` for Minecraft internal types.

---

## Installing on LeviLauncher

1. **Automatic**: Running `bash package.sh` automatically installs the mod into:
   ```
   /sdcard/Android/media/org.levimc.launcher/minecraft/<profile>/mods/<YourMod>/
   ```
   and enables it in `mods_config.json`.
2. **Via `.levipack`**:
   - Transfer `dist/<your_mod>.levipack` to your device.
   - Open **LeviLauncher** -> **Mods** -> **Import** -> Select the `.levipack` file.
