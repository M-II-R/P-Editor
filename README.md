# P-Editor
A scene editor for Panda3D game engine that can work with both C++ and Python projects.

# Building:
On GNU/Linux: just run build.sh script. You must have installed Panda3D and its dependencies. It will produce an executable 'editor' and will move it to the root of the project.

On other systems: I didn't try to build this app for other systems.

# Features:
  1. Basic scene editing.
  2. Import of models in formats supported by Panda3D.
  3. Export to Python and C++ code.
  4. Saving in this editor's own format.
# Issues:
  1. When I started to write the code for this app, I didn't know anything about good architecture, so now its architecture is awful. I don't know if I will try to change it.
  2. It segfaults when you close the window and I have no suppositions about why it is happening (GDB says it's something inside Panda3d).

# Screenshots:
<img width="854" height="451" alt="Screenshot1" src="https://raw.githubusercontent.com/M-II-R/P-Editor/refs/heads/main/editor-screenshot-1.png" />

<img width="854" height="451" alt="Screenshot2" src="https://raw.githubusercontent.com/M-II-R/P-Editor/refs/heads/main/editor-screenshot-2.png" />

<img width="854" height="451" alt="Screenshot3" src="https://raw.githubusercontent.com/M-II-R/P-Editor/refs/heads/main/editor-screenshot-3.png" />

# Used libraries:
[Panda3D](https://github.com/panda3d/panda3d). License: [modified BSD](https://github.com/panda3d/panda3d/blob/master/LICENSE).

[fast_float](https://github.com/fastfloat/fast_float). License: [Apache-2.0](https://github.com/fastfloat/fast_float/blob/main/LICENSE-APACHE) or two others.

[Native File Dialog](https://github.com/mlabbe/nativefiledialog). License: [Zlib](https://github.com/mlabbe/nativefiledialog/blob/master/LICENSE).

[GTK3](https://www.gtk.org/) - for Native File Dialog. License: [LGPL](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html).

[Dear ImGui](https://github.com/ocornut/imgui). License: [MIT](https://github.com/ocornut/imgui/blob/master/LICENSE.txt).

[Panda3D ImGui Helper](https://github.com/bluekyu/panda3d_imgui) - not a library, but made not by me. License: [MIT](https://github.com/bluekyu/panda3d_imgui/blob/master/LICENSE.md).
