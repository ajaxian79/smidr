# Smidr

A modern CSG-based CAD program. Built with C++20, Dear ImGui, and the
[Skald](https://github.com/ajaxian79/skald) design system.

**Smidr** (Old Norse *smiðr* — "craftsman, smith") provides real-time
constructive solid geometry modeling with a clean, professional interface.

## Features

- **Parametric primitives** — sphere, box, cylinder, cone, torus
- **CSG boolean operations** — union, intersection, difference
- **Hierarchical scene graph** — group, nest, and transform objects
- **Real-time 3D viewport** — orbit, pan, zoom with Phong-lit solid/wireframe
- **Document format** — human-readable JSON (.smidr)
- **Professional UI** — dark theme, dockable panels, workspace tabs

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/smidr
```

### Dependencies

- C++20 compiler (GCC 12+, Clang 15+, MSVC 2022+)
- CMake 3.20+
- OpenGL 3.2+
- GLFW 3.x (fetched automatically if not found)
- Dear ImGui (fetched automatically)
- [Skald](https://github.com/ajaxian79/skald) (fetched automatically)

## License

MIT — see [LICENSE](LICENSE).
