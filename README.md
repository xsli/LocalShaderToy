# 🎨 Local Shadertoy

<p align="center">
  <img src="docs/screenshot.png" alt="Local Shadertoy Screenshot" width="800">
</p>

A **native OpenGL implementation** of [Shadertoy](https://www.shadertoy.com/) that runs locally on your machine. Write, edit, and run Shadertoy-compatible GLSL shaders without needing a browser or internet connection.

## ✨ Features

### Core Rendering
- 🖥️ **OpenGL 4.3+** hardware-accelerated rendering
- 🔄 **Real-time shader compilation** with instant feedback
- 📺 **Full Shadertoy uniform support**: `iTime`, `iResolution`, `iMouse`, `iFrame`, `iTimeDelta`, `iDate`, `iChannelResolution`
- 🎯 **Automatic GLSL transpilation** (WebGL → Desktop OpenGL)

### Built-in Textures
- 🌫️ **Procedural Noise**: Gray Noise, RGBA Noise (multiple sizes)
- 🌊 **Perlin Noise**: 256x256 and 512x512 variants
- 💠 **Blue Noise**: For high-quality dithering
- 🎨 **Organic Textures**: Tileable procedural patterns
- ⬛ **Utility**: Checkerboard, UV gradient, solid colors

### User Interface
- 📝 **Integrated code editor** with GLSL syntax highlighting
- 🎛️ **Texture panel**: Bind any built-in texture to `iChannel0-3`
- ⏯️ **Playback controls**: Play, pause, reset time
- 📊 **Performance stats**: FPS counter, resolution display
- 💾 **Project management**: Save/load shader projects as JSON

### Developer Experience
- 🔥 **Hot reload**: Press F5 to recompile instantly
- 📋 **Paste & run**: Directly paste Shadertoy code from clipboard
- ❌ **Error highlighting**: Clear compilation error messages
- 🖱️ **Mouse interaction**: Full `iMouse` support for interactive shaders

## � Quick Start

### Prerequisites
- **Windows 10/11** with Visual Studio 2019 or later
- **CMake 3.20+**
- **Git**
- GPU with **OpenGL 4.3** support

### Build from Source

```bash
# Clone the repository
git clone https://github.com/xsli/LocalShaderToy.git
cd LocalShaderToy

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake -G "Visual Studio 17 2022" -A x64 ..

# Build
cmake --build . --config Release

# Run
./bin/Release/LocalShadertoy.exe
```

### Pre-built Binaries
Download the latest release from the [Releases](https://github.com/xsli/LocalShaderToy/releases) page.

## � Usage

### Running a Shader
1. Launch the application
2. Write or paste your Shadertoy shader code in the editor
3. Press **F5** or click "Compile" to see the result

### Using Textures
1. Open the **Textures (iChannel)** panel from View menu
2. Select a built-in texture for each channel (iChannel0-3)
3. Access them in your shader:
```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    vec4 noise = texture(iChannel0, uv);
    fragColor = noise;
}
```

### Keyboard Shortcuts
| Key | Action |
|-----|--------|
| `F5` | Compile shader |
| `Space` | Play/Pause |
| `R` | Reset time |
| `Ctrl+S` | Save project |
| `Ctrl+O` | Open project |
| `Ctrl+N` | New project |
| `Esc` | Exit |

## 🏗️ Project Structure

```
LocalShaderToy/
├── src/
│   ├── core/           # Application, ShaderEngine, UniformManager
│   ├── renderer/       # Renderer, Framebuffer, TextureManager
│   ├── transpiler/     # GLSL WebGL→OpenGL transpiler
│   ├── ui/             # ImGui-based UI components
│   ├── input/          # Resource loading
│   └── utils/          # Utilities (FileDialog, Timer, etc.)
├── third_party/
│   ├── glad/           # OpenGL loader
│   └── stb/            # Image loading (stb_image)
├── shaders/            # Example shaders
├── resources/          # Application resources
└── CMakeLists.txt      # Build configuration
```

## 🔧 Dependencies

All dependencies are automatically downloaded via CMake FetchContent:

| Library | Version | Purpose |
|---------|---------|---------|
| [GLFW](https://www.glfw.org/) | 3.3.8 | Window & input management |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Mathematics library |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.6-docking | User interface |
| [ImGuiColorTextEdit](https://github.com/BalazsJako/ImGuiColorTextEdit) | master | Code editor |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | JSON serialization |
| [GLAD](https://glad.dav1d.de/) | - | OpenGL loader (included) |
| [stb_image](https://github.com/nothings/stb) | - | Image loading (included) |

## 🎯 Shadertoy Compatibility

### Supported Uniforms
| Uniform | Type | Description |
|---------|------|-------------|
| `iResolution` | `vec3` | Viewport resolution (width, height, 1.0) |
| `iTime` | `float` | Playback time in seconds |
| `iTimeDelta` | `float` | Time since last frame |
| `iFrame` | `int` | Current frame number |
| `iMouse` | `vec4` | Mouse coordinates and click state |
| `iDate` | `vec4` | Year, month, day, time in seconds |
| `iChannel0-3` | `sampler2D` | Texture inputs |
| `iChannelResolution[4]` | `vec3` | Resolution of each channel |

### Automatic Transpilation
The transpiler handles:
- ✅ Adding `#version 430` declaration
- ✅ Wrapping `mainImage()` into `main()`
- ✅ Removing `precision` statements (not needed in desktop GL)
- ✅ Declaring standard Shadertoy uniforms

## 📸 Screenshots

<p align="center">
  <i>Screenshots coming soon...</i>
</p>

## 🗺️ Roadmap

- [ ] **Multi-pass rendering**: Buffer A/B/C/D support
- [ ] **Audio input**: FFT analysis with `iChannel` audio textures
- [ ] **Video/GIF export**: Record shader animations
- [ ] **Cubemap support**: Environment mapping
- [ ] **Linux/macOS**: Cross-platform builds

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## � License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [Shadertoy](https://www.shadertoy.com/) - The amazing online shader community
- [Inigo Quilez](https://iquilezles.org/) - For incredible shader tutorials
- All the shader artists whose work inspires this project

---

<p align="center">
  Made with ❤️ for the shader art community
</p>