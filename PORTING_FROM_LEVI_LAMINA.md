# Porting a LeviLamina Mod to Android via LeviLamroid

> **Guide Target**: Minecraft Bedrock Edition (Android ARM64) on LeviLauncher  
> **Source Platform**: LeviLamina on Windows (Bedrock Dedicated Server / BDS)  
> **Target Platform**: LeviLamroid on Android ARM64 (`liblevilamroid.so`)  
> **Maintainer**: **SpyGamingOG** (`spygamingog`)  
> **Standard Version**: `v1.0.0-beta` (Initial Beta Testing Release)

> [!WARNING]
> ### ⚠️ Under Active Development — Beta Testing Notice
> LeviLamroid is currently in its initial **Beta Testing Release** (`v1.0.0-beta`).
> - While the core `ll::` API provides drop-in source parity with Windows LeviLamina, Android Bedrock is a full closed-source graphical client (unlike headless BDS).
> - Certain game hooks, event handlers, and engine RVAs are under active beta testing and may vary or fail across different minor Minecraft releases.
> - Developers porting mods should test thoroughly and report any broken hooks or missing symbols on [GitHub Issues](https://github.com/spygamingog/LeviLamroid/issues).

---

## 1. Architectural Philosophy: Canonical Parity (`namespace ll`)

LeviLamroid is architected with one fundamental rule: **100% LeviLamina Windows source-level parity where architecturally possible**.

Modders code directly against:
- `ll::Logger`
- `ll::event::EventBus`
- `ll::mod::NativeMod`
- `ll::command::CommandRegistrar`
- `ll::chrono::GameTickScheduler`
- `ll::form::SimpleForm`
- `ll::service::Bedrock`

This means that clean, well-structured LeviLamina mods can be ported to Android with **minimal code rewrites**.

---

## 2. Complete Parity Matrix

| Feature / Subsystem | LeviLamina (Windows BDS) | LeviLamroid (Android ARM64) | Compatibility Status |
| :--- | :--- | :--- | :--- |
| **Namespace** | `namespace ll` | `namespace ll` | **100% Identical** |
| **Mod Base Class** | `ll::mod::NativeMod` | `ll::mod::NativeMod` | **100% Identical** |
| **Mod Registration** | `LL_REGISTER_MOD(...)` | `LL_REGISTER_MOD(...)` | **100% Identical** |
| **Event Bus** | `ll::event::EventBus` | `ll::event::EventBus` | **100% Identical** |
| **Logging** | `ll::Logger` | `ll::Logger` | **100% Identical** (Android Logcat + Disk) |
| **Service Locator** | `ll::service::Bedrock` | `ll::service::Bedrock` | **100% Identical** |
| **Command System** | `ll::command::CommandRegistrar` | `ll::command::CommandRegistrar` | **100% Identical** |
| **Task Scheduling** | `ll::chrono` game tick clock | `ll::chrono::GameTickScheduler` | **100% Identical** |
| **Form Dialogs** | `ll::form::SimpleForm` | `ll::form::SimpleForm` / `ModalForm` | **100% Identical** |
| **Function Hooking** | `ll::memory::hook` | `ll::memory::hook` / `hookRva` | **Compatible** (AArch64 Trampoline) |
| **ImGui UI Overlay** | Direct3D 11/12 Backend | OpenGL ES 3.0 Backend | **Adapted** (Android EGL Recovery) |
| **Mobile Input** | N/A (Keyboard/Mouse only) | `pl::input` (Multi-Touch + Keys) | **LeviLamroid Exclusive** |
| **HUD Touch Buttons** | N/A | `pl::modmenu` Floating Widgets | **LeviLamroid Exclusive** |
| **In-Memory Audio** | N/A | `levilamroid::sound::FmodAudioEngine` | **LeviLamroid Exclusive** |
| **Virtual Resource Pack**| N/A | `levilamroid::resource::VirtualPackHub` | **LeviLamroid Exclusive** |

---

## 3. What We CANNOT Have on Android (and Why)

Because Android is an entirely different operating system, hardware architecture, and execution environment than Windows, certain desktop features cannot exist on mobile:

### 1. Windows Structured Exception Handling (SEH)
- **Windows**: `__try { ... } __except(...) { ... }` allows recovering from null-pointer dereferences and access violations.
- **Android Linux**: SEH does not exist. Hardware memory faults generate POSIX signals (`SIGSEGV`, `SIGBUS`, `SIGILL`). You must write crash-safe code or use POSIX `sigaction` with an alternate signal stack (`sigaltstack`).

### 2. Runtime PDB Symbol Parsing
- **Windows**: LeviLamina can parse `.pdb` symbol databases at runtime using `DbgHelp.dll` to find unexported function names.
- **Android ELF**: Android `.so` libraries have no PDB files. Release binaries are stripped of symbol tables (`.symtab`). LeviLamroid resolves offsets at runtime by inspecting the ELF `PT_NOTE` segment for the **GNU Build ID** and binding golden RVA tables.

### 3. MSVC Name Mangling & Calling Conventions
- **Windows**: MSVC decorates symbols with `?` (e.g., `?normalTick@Actor@@UEAAXXZ`), uses Microsoft x64 calling conventions (`RCX`, `RDX`, `R8`, `R9`), and compiler-specific register conventions (`__vectorcall`, `__thiscall`).
- **Android**: Clang on Android uses the **Itanium C++ ABI** (`_ZN5Actor10normalTickEv`) and **AAPCS64** calling conventions (registers `X0-X7` for arguments/return, `X16/X17` for intra-procedure scratch jumps).

### 4. Direct Win32 API & Dynamic DLL Loading
- **Windows**: Calling `LoadLibraryExW`, `GetModuleHandle`, Win32 console windows, or registry keys.
- **Android**: Android uses Bionic libc (`dlopen`, `dlsym`, `/proc/self/maps`). File access must respect Android Storage Access Framework (SAF).

### 5. Headless BDS Server Assumptions
- **Windows**: LeviLamina is primarily built for BDS (Bedrock Dedicated Server), which is a headless daemon without rendering, UI, or client input loops.
- **Android**: `libminecraftpe.so` is the **Full Client Game**. It runs RenderDragon skybox passes, Cohtml OreUI HTML5 views, JSON UI screens, and mobile touch loops. Blocking the main thread causes an Android Application Not Responding (ANR) crash.

---

## 4. What We Don't Have Yet (Future Roadmap)

1. **Full 150+ Server Event Catalog**: Specialized BDS server events (RCON remote command execution, server player whitelisting permissions, dedicated server backup hooks).
2. **Compile-time Reflection Deserializer (`ll::reflection`)**: Automatic struct-to-JSON serialization without writing manual json encoders.
3. **FastUI Web Component Engine**: Embedding standalone React/Vue web applications as in-game panels.

---

## 5. Step-by-Step Migration Guide: Porting a Mod

Follow these 5 steps to convert an existing Windows LeviLamina mod to Android LeviLamroid:

### Step 1: Update the Build System (`CMakeLists.txt`)
Replace `xmake.lua` or Visual Studio `.vcxproj` with an Android ARM64 CMake configuration:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_ported_mod CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure AArch64 / Android ARM64 Target
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
endif()

# Point to LeviLamroid include directory
set(LEVILAMROID_ROOT "/home/devroom/LeviLamroid" CACHE PATH "Path to LeviLamroid")
include_directories(
    ${LEVILAMROID_ROOT}/include
    ${LEVILAMROID_ROOT}/include/levilamroid
    ${LEVILAMROID_ROOT}/include/ll
    ${LEVILAMROID_ROOT}/include/pl
    ${LEVILAMROID_ROOT}/include/mc
    ${LEVILAMROID_ROOT}/include/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(my_ported_mod SHARED
    src/MyMod.cpp
)

target_link_libraries(my_ported_mod PRIVATE
    GLESv3
    log
)

target_compile_options(my_ported_mod PRIVATE
    -fPIC -O3 -Wall -Wextra -Wno-unused-parameter
)
```

### Step 2: Update Header Includes
Replace ad-hoc header includes with LeviLamroid's master header:

```cpp
// Windows LeviLamina:
#include "ll/api/mod/NativeMod.h"
#include "ll/api/Logger.h"
#include "ll/api/event/EventBus.h"

// LeviLamroid (Drop-in):
#include <levilamroid/LeviLamroid.h>
```

### Step 3: Replace MSVC Hooking Macros with RVA / Detours
On Windows, mods often use MSVC mangled signatures with `LL_TYPE_INSTANCE_HOOK`. On Android, use `ll::memory::hookRva`:

```cpp
// Windows (MSVC):
// LL_TYPE_INSTANCE_HOOK(MyPlayerHook, HookPriority::Normal, Player, "?normalTick@Actor@@UEAAXXZ", void)

// Android LeviLamroid:
static void* sOrigPlayerTick = nullptr;

static void hooked_playerTick(void* self) {
    // Custom logic here
    if (sOrigPlayerTick) {
        reinterpret_cast<void(*)(void*)>(sOrigPlayerTick)(self);
    }
}

// In onEnable():
const auto& offsets = ll::mcpe::getOffsets();
ll::memory::hookRva(offsets.playerNormalTick, (void*)&hooked_playerTick, &sOrigPlayerTick);
```

### Step 4: Add Mobile Touch & HUD Controls
Enhance your mod with mobile capabilities using `pl::modmenu`:

```cpp
#include <pl/ModMenu.hpp>

// Add an on-screen touch button to toggle your mod's menu
pl::modmenu::ButtonInfo btn;
btn.buttonId = "mymod_btn";
btn.moduleId = "MyMod";
btn.displayName = "My Mod Menu";
btn.label = "Menu";
btn.behavior = pl::modmenu::ButtonBehavior::Click;
btn.normalBgColor = 0xAA1E293B; // Dark Slate
btn.activeBgColor = 0xDD00E5FF; // Glow Cyan
btn.onEvent = [](std::string_view, pl::modmenu::ButtonEvent ev, float) {
    if (ev == pl::modmenu::ButtonEvent::Click) {
        gMenuOpen = !gMenuOpen;
    }
};
pl::modmenu::registerButton(btn);
```

### Step 5: Package as a `.levipack`
Create `manifest.json` initialized to `v1.0.0`:

```json
{
  "format_version": 2,
  "header": {
    "name": "MyPortedMod",
    "description": "Ported LeviLamina mod for Android ARM64",
    "version": [1, 0, 0],
    "uuid": "e3a89e00-1111-4444-8888-abcdef012345",
    "type": "native-mod",
    "entry": "libmy_ported_mod.so"
  }
}
```

Package the compiled `.so` and `manifest.json` into a `.zip` archive renamed to `.levipack`, and place it in:
`/sdcard/Android/media/org.levimc.launcher/minecraft/<profile>/mods/MyPortedMod/`
