# 🎨 Local Shadertoy

A **native OpenGL implementation** of [Shadertoy](https://www.shadertoy.com/) that runs locally on your machine. Write, edit, and run Shadertoy-compatible GLSL shaders without needing a browser or internet connection.

**Now with Windows Screensaver support!** 🖥️✨

## ✨ Features

- 🖥️ **OpenGL 4.3+** hardware-accelerated rendering
- 🔄 **Real-time shader compilation** with instant feedback
- 📺 **Full Shadertoy uniform support**: `iTime`, `iResolution`, `iMouse`, `iFrame`, `iTimeDelta`, `iDate`, `iChannel0-3`
- 🎯 **Automatic GLSL transpilation** (WebGL → Desktop OpenGL)
- 📝 **Integrated code editor** with GLSL syntax highlighting
- 🎛️ **Built-in textures**: Noise, Perlin, Blue Noise, Checkerboard, etc.
- 🖼️ **Windows Screensaver**: Use your shaders as system screensavers!
- 🔀 **Multi-pass rendering**: Full support for Buffer A/B/C/D and Common code
- 🎲 **Random shuffle**: Automatically cycle through your shader collection

## 🚀 Quick Start

### Prerequisites
- **Windows 10/11** 
- **CMake 3.20+** and **Visual Studio 2019+**
- GPU with **OpenGL 4.3** support

### Build from Source

```bash
git clone https://github.com/xsli/LocalShaderToy.git
cd LocalShaderToy
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

### Run

```bash
# Editor mode
./bin/Release/LocalShadertoy.exe

# Screensaver mode (fullscreen)
./bin/Release/LocalShadertoy.scr /s
```

## ⌨️ Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `F5` | Compile shader |
| `Space` | Play/Pause |
| `R` | Reset time |
| `Ctrl+S` | Save project |
| `Ctrl+O` | Open project |
| `Esc` | Exit |

## 🖼️ Windows Screensaver Setup

### Install as System Screensaver

1. **Copy the `.scr` file to system directory** (requires Admin):
   ```cmd
   copy build\bin\Release\LocalShadertoy.scr C:\Windows\System32\
   ```

2. **Open Screensaver Settings**:
   - Press `Win + R`, type `control desk.cpl,,@screensaver`
   - Or: Settings → Personalization → Lock screen → Screen saver settings

3. **Select "LocalShadertoy"** from the dropdown list

4. **Set wait time** and click OK

### Save Shader as Screensaver Profile

1. Open `LocalShadertoy.exe`
2. Write or paste your shader code
3. Go to **File → Screensaver → Save as Screensaver Profile...**
4. Enter a profile name and click Save
5. The shader will now be used when the screensaver activates

### Manage Multiple Profiles

- **File → Screensaver → Manage Profiles...** - Rename, delete, or load profiles
- **File → Screensaver → [Profile List]** - Click any profile to set it as active

### Random Shuffle Mode 🎲

Enable automatic cycling through multiple shader profiles:

1. Open **File → Screensaver → Manage Profiles...**
2. **Check the box** next to profiles you want to include in random rotation
3. Enable **"Random shuffle"** checkbox
4. Set the **interval** (seconds between shader switches)
5. Click **Save Settings**

When screensaver activates, it will randomly cycle through checked profiles.

## 🔀 Multi-pass Rendering

Local Shadertoy supports full multi-pass rendering just like Shadertoy.com:

### Available Passes

| Pass | Description |
|------|-------------|
| **Common** | Shared code included in all other passes |
| **Buffer A-D** | Intermediate render targets (can read from each other) |
| **Image** | Final output pass |

### Using Buffers

1. Click the **Buffer A/B/C/D** tabs in the editor
2. Write your buffer shader code
3. Set **iChannel bindings** to read from other buffers
4. The **Image** pass reads from buffers and outputs to screen

### Buffer Self-Reference

Buffers can read their own previous frame for feedback effects:
- In Buffer A, set `iChannel0 = Buffer A` to read last frame's content
- Uses ping-pong double buffering internally

### Configuration File Location

Screensaver profiles are stored at:
```
%APPDATA%\LocalShadertoy\config.json
```

## 🎯 Shadertoy Compatibility

### Supported Uniforms

| Uniform | Type | Description |
|---------|------|-------------|
| `iResolution` | `vec3` | Viewport resolution |
| `iTime` | `float` | Playback time in seconds |
| `iTimeDelta` | `float` | Time since last frame |
| `iFrame` | `int` | Current frame number |
| `iMouse` | `vec4` | Mouse coordinates and click state |
| `iDate` | `vec4` | Year, month, day, time |
| `iChannel0-3` | `sampler2D` | Texture inputs |

### Example Shader

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
    fragColor = vec4(col, 1.0);
}
```

## 🔧 Dependencies

All dependencies are automatically downloaded via CMake FetchContent:

- [GLFW](https://www.glfw.org/) - Window management
- [GLM](https://github.com/g-truc/glm) - Mathematics
- [Dear ImGui](https://github.com/ocornut/imgui) - User interface
- [ImGuiColorTextEdit](https://github.com/BalazsJako/ImGuiColorTextEdit) - Code editor
- [nlohmann/json](https://github.com/nlohmann/json) - JSON serialization

## 🗺️ Roadmap

- [x] Multi-pass rendering (Buffer A/B/C/D) ✅
- [x] Random shuffle screensaver mode ✅
- [ ] Audio FFT input
- [ ] Video/GIF export
- [ ] Cubemap support
- [ ] Linux/macOS support

## 📄 License

MIT License - see [LICENSE](LICENSE) for details.

---

<p align="center">
  Made with ❤️ for the shader art community
</p>