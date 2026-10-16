# AGENTS.md — LeviLamroid Architecture & Maintenance Guide

> **Audience**: AI Agents (Antigravity, Claude Code, Codex, Cursor) and Native C++ Developers maintaining **LeviLamroid** on Android ARM64.  
> **Maintainer**: **SpyGamingOG** (`spygamingog`).

---

## 1. Architectural Philosophy

`LeviLamroid` is designed as a standalone, non-intrusive native loader shim for Minecraft Bedrock on Android ARM64.
Its responsibility is strictly isolated:
1. **Host-Level Hook Management**: Provide safe, thread-safe, conflict-free hooking into `libminecraftpe.so`.
2. **Centralized Engine Offsets**: Maintain golden RVAs across supported Bedrock version families with dynamic runtime version detection.
3. **Event Dispatching**: Broadcast engine callbacks (UI, Render, Input, World) through `ll::event::EventBus`.
4. **Mod Loading & PreLoader Bridge**: Dynamically load secondary client mods from `mods/` and interface with LeviLauncher's PreLoader ABI.

---

## 2. Code Map

| Directory / File | Description |
|---|---|
| `include/levilamroid/LeviLamroid.h` | Root master header and version definitions |
| `include/ll/api/` | **Canonical** LeviLamina drop-in C++ API layer (`memory/`, `event/`, `mod/`, `Logger.h`) |
| `include/levilamroid/api/` | Namespaced forwarding aliases (`levilamroid::...`) delegating to `include/ll/api/` |
| `include/pl/` | LeviLauncher PreLoader C/C++ ABI headers (`Input.hpp`, `ModMenu.hpp`, `memory/`) |
| `include/mc/` | Audited Minecraft Bedrock class and type definitions |
| `data/` | Multi-version golden RVA offset JSON tables (`offsets_1.26.52.3_arm64.json`, `offsets_1.26.45.1_arm64.json`) |
| `include/imgui/` | **Canonical** Dear ImGui v1.91.x library with OpenGL ES 3.0 Android backend |
| `src/LeviLamroid.cpp` | Root initialization, LeviLauncher `PLGetModRegistration` export, fallback C ABIs |
| `src/memory/Hook.cpp` | Base discovery (`/proc/self/maps`), AArch64 inline trampoline, GOT patcher |
| `src/memory/PatternScan.cpp` | Wildcard byte scanner across memory pages |
| `src/mcpe/Offsets.cpp` | Multi-version GNU Build ID detector (`dl_iterate_phdr`) and golden RVA tables |
| `src/hooks/` | Core, UI, and Render hook management |
| `src/plugin/PluginManager.cpp` | Dynamic mod loader (`dlopen`, `PLGetModRegistration`, `ll_mod_load`) |
| `template/` | **Default Mod Template**: Complete C++20 project starter with CMake, manifest, HUD button, and packaging |
| `FUNCTIONS.md` | **Complete API Master Reference**: Full catalog of every function, class, struct, and macro |

---

## 3. Hook Verification & Relocation Details

### 3.1 Base Discovery & Version Identification
LeviLamroid discovers `libminecraftpe.so` memory base and exact version via:
1. `dl_iterate_phdr` to parse the loaded ELF headers directly from memory.
2. Locates the `PT_NOTE` program segment and extracts the 20-byte SHA-1 **GNU Build ID**:
   - `56de9eed077631e03a31f4f58eb2f0e00071d335` $\rightarrow$ Bedrock `1.26.52.3`
   - `868e275cb295e9a275bb29d2258edc2f7dc48761` $\rightarrow$ Bedrock `1.26.45.1`
3. Fallback: Parses `/proc/self/maps` and `/proc/self/cmdline` to extract the package/profile path.

### 3.2 AArch64 Trampoline Detour
When external hook libraries are absent, `inlineHookAArch64` writes a 16-byte atomic jump:
```arm64
LDR X16, #8
BR  X16
<64-bit absolute detour address>
```
The replaced 16 bytes are saved in an executable trampoline pool followed by a jump back to `target + 16`. Instruction cache is flushed using `__builtin___clear_cache` to prevent CPU pipeline stale instruction fetches.

### 3.3 GOT (Global Offset Table) Relocation
For shared library functions (such as `glClear` from `libGLESv2.so`), LeviLamroid locates the GOT slot in `libminecraftpe.so`'s `.got` section:
1. `1.26.52.3`: GOT RVA `0x132893D8`
2. `1.26.45.1`: GOT RVA `0x124C9688`
LeviLamroid unprotects the page with `mprotect(PROT_READ | PROT_WRITE)`, atomically overwrites the function pointer with the detour, and restores protection.

---

## 4. LeviLauncher PreLoader ABI Compatibility

LeviLauncher's `PreLoader` expects mods to export `PLGetModRegistration()` returning a pointer to a `pl::mod::ModRegistration` structure:
```cpp
struct ModRegistration {
    pl::mod::LifecycleFunc load;     // void (*)(pl::mod::ModContext&)
    pl::mod::LifecycleFunc enable;   // void (*)(pl::mod::ModContext&)
    pl::mod::LifecycleFunc disable;  // void (*)(pl::mod::ModContext&)
    pl::mod::LifecycleFunc unload;   // void (*)(pl::mod::ModContext&)
};
```
> [!CRITICAL]
> Never return a string or raw pointer to non-callable data from `PLGetModRegistration`. Calling unaligned addresses or string literals as function pointers causes an immediate `SIGBUS (BUS_ADRALN)` crash!

LeviLamroid also exports fallback symbols:
- `PLMod_Load`
- `LeviMod_Load`
- `ll_mod_load`

---

## 5. Adding New Minecraft Bedrock Versions

To add support for a new Bedrock version (e.g. `1.26.60`):
1. Extract `libminecraftpe.so` from the APK or device.
2. Read the GNU Build ID:
   ```bash
   readelf -n libminecraftpe.so | grep "Build ID"
   ```
3. Locate golden RVAs in IDA / Ghidra using string xrefs or pattern scanning:
   - `getTechStackForScreen`: String xref `"ScreenTechStackSelector"`
   - `startMenuOnOpen`: String xref `"StartMenuScreenModel::onOpen"`
   - `glClearGotRva`: GOT relocation entry for `glClear`
4. Add the entry to `sVersionTable` in [`src/mcpe/Offsets.cpp`](file:///home/devroom/LeviLamroid/src/mcpe/Offsets.cpp).
5. The memory overhead is merely 200 bytes per version with zero duplicate code!

---

## 6. Resourcepack-Free Audio & Texture Systems

### 6.1 Audio: Method A vs. Method B
- **Method A (`levilamroid::sound::FmodAudioEngine`)**:
  - Resolves `FMOD_System_CreateSound` and `FMOD_System_PlaySound` from `libfmod.so`.
  - Directly plays raw in-memory WAV/OGG/MP3 buffers using `FMOD_OPENMEMORY`.
  - Use when the mod needs custom UI clicks, sound effects, or background music without touching `sounds.json`.
- **Method B (`levilamroid::mcpe::SoundEngine` & `SoundPlayEvent`)**:
  - Intercepts Minecraft's internal 3D audio dispatch: `SoundEngine::play(name, pos, vol, pitch)`, `playUI`, `playAt`.
  - Emits `ll::event::SoundPlayEvent` across `ll::event::EventBus`.
  - Use for **Sound Physics**: The mod listens to `SoundPlayEvent`, raycasts against the block collision geometry between `position` and the local player's eye coordinates, calculates distance attenuation / muffling through blocks, adjusts volume/pitch, and modifies or dampens the sound accordingly.

### 6.2 Texture: Option A vs. Option B
- **Option A (`mce::TextureGroup::uploadTexture`)**:
  - Uploads raw RGBA pixel arrays directly to RenderDragon VRAM under a custom `ResourceLocation`.
  - Ideal for custom HUD elements, 3D overlays, and procedural icons.
- **Option B (`levilamroid::resource::VirtualPackHub`)**:
  - Mounts an `InMemoryPackAccessStrategy` on top of Bedrock's `ResourcePackStack`.
  - Intercepts virtual asset queries (`textures/blocks/stone.png`, `textures/items/diamond_sword.png`, `sounds/sound_definitions.json`).
  - Allows mods to override existing vanilla block and item textures dynamically in RAM without shipping physical `.mcpack` files.

---

## 7. Universal Reverse Engineering Studio (`re_studio`) Integration

`universal-modder/tools/re_studio/main.py` provides an automated cross-verification engine for Bedrock ARM64:
- Direct Function Search:
  ```bash
  python3 tools/re_studio/main.py --search-function "SoundEngine::play"
  ```
- Direct Class Inspection & VTable Harvesting:
  ```bash
  python3 tools/re_studio/main.py --search-class "SoundEngine"
  ```
- Automatically persists verified RVAs and AAPCS64 prologues into `inputs/<version>/offsets_<version>_arm64.json` and updates `LeviLamroid/data/`.


