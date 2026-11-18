# Singular Value Decomposition ($A = U \Sigma V^T$) 3D Geometric Simulation

This repository features a 3D geometric visualization of **Singular Value Decomposition**, built in C++ using [raylib](https://www.raylib.com/).

This program demonstrates how any linear transformation matrix $A \in \mathbb{R}^{3 \times 3}$ acting on a 3D unit sphere and its orthogonal basis vectors ($X, Y, Z$), can be decomposed into three sequential geometric operations:

$$A = U \Sigma V^T$$

---

## Geometric Transformation Steps Featured in the Program

1. **Initial State ($I$):** 
   Starts with a translucent unit sphere and three orthogonal basis vectors ($X$: Red, $Y$: Green, $Z$: Blue) representing untransformed 3D space.
2. **Step 1 -> Apply $V^T$ (Initial Rotation):** 
   Applies the orthogonal matrix $V^T$ using spherical linear interpolation (SLERP) with Euler angles $(\frac{\pi}{4}, \frac{\pi}{3}, 0)$. Because $V^T$ is an orthogonal transformation, the sphere remains unstretched while its basis vectors rotate to align the principal axes with the global coordinate axes.
3. **Step 2 -> Apply $\Sigma$ (Principal Scaling / Deformation):** 
   Applies the diagonal singular value matrix $\Sigma = \text{diag}(\sigma_1, \sigma_2, \sigma_3)$ using linear interpolation (LERP). The unit sphere deforms into a 3D ellipsoid:
   - Stretches strongly along the $X$-axis ($\sigma_1 = 2.5$)
   - Stretches slightly along the $Y$-axis ($\sigma_2 = 1.2$)
   - Compresses along the $Z$-axis ($\sigma_3 = 0.4$)
4. **Step 3 -> Apply $U$ (Final Rotation):** 
   Applies the second orthogonal matrix $U$ via SLERP with Euler angles $(-\frac{\pi}{6}, -\frac{\pi}{4}, \frac{\pi}{6})$, orienting the deformed ellipsoid into its final spatial configuration $A = U \Sigma V^T$.

---

## Complete Step-by-Step Setup & Execution Guide

Please follow the instructions below for your specific operating system to install the compiler, install `raylib`, compile the code and finally run the simulation!

---

### Linux

#### Step 1: Update system packages and install the C++ compiler, Git, and CMake
Open a terminal and run:
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential git cmake pkg-config
```

#### Step 2: Install required OpenGL and X11 windowing dependencies
```bash
sudo apt install -y libasound2-dev libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev \
    libwayland-dev libxkbcommon-dev
```

#### Step 3: Download, build, and install `raylib` from source
```bash
git clone --depth 1 https://github.com/raysan5/raylib.git /tmp/raylib
mkdir -p /tmp/raylib/build
cd /tmp/raylib/build
cmake -DBUILD_SHARED_LIBS=ON ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ~
rm -rf /tmp/raylib
```

#### Step 4: Clone this repository and navigate into the project folder
```bash
git clone https://github.com/akamariadis/svd-simulation.git
cd svd-simulation
```

#### Step 5: Compile the program
```bash
g++ svd_simulation.cpp -o svd_simulation -O2 -std=c++17 -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```

#### Step 6: Run the executable
```bash
./svd_simulation
```

---

### Option B: Linux (Arch Linux / Fedora)

#### Step 1: Install the compiler and `raylib`
- **On Arch Linux / Manjaro:**
  ```bash
  sudo pacman -Syu --needed base-devel git gcc raylib
  ```
- **On Fedora:**
  ```bash
  sudo dnf groupinstall "Development Tools" "C Development Tools and Libraries"
  sudo dnf install git gcc-c++ raylib-devel
  ```

#### Step 2: Clone the repository and enter the directory
```bash
git clone https://github.com/your-username/svd-simulation.git
cd svd-simulation
```

#### Step 3: Compile the program
```bash
g++ svd_simulation.cpp -o svd_simulation -O2 -std=c++17 -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```

#### Step 4: Run the executable
```bash
./svd_simulation
```

---

### Option C: macOS (Apple Silicon & Intel)

#### Step 1: Install Xcode Command Line Tools
Open **Terminal** and run:
```bash
xcode-select --install
```
*(Follow the on-screen prompt if a dialog window appears).*

#### Step 2: Install Homebrew (skip if already installed)
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### Step 3: Install `raylib` and `pkg-config`
```bash
brew update
brew install raylib pkg-config
```

#### Step 4: Clone the repository and enter the directory
```bash
git clone https://github.com/your-username/svd-simulation.git
cd svd-simulation
```

#### Step 5: Compile the program
```bash
clang++ svd_simulation.cpp -o svd_simulation -O2 -std=c++17 $(pkg-config --libs --cflags raylib) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
```

#### Step 6: Run the executable
```bash
./svd_simulation
```

---

### Option D: Windows (via MSYS2 / MinGW-w64)

#### Step 1: Download and install MSYS2
1. Download the installer from [https://www.msys2.org/](https://www.msys2.org/).
2. Run the installer (`msys2-x86_64-latest.exe`) and follow the default installation prompts (`C:\msys64`).

#### Step 2: Open the MSYS2 MinGW 64-bit terminal
Launch **MSYS2 MINGW64** from the Windows Start Menu.

#### Step 3: Update packages and install the C++ compiler and `raylib`
Run the following commands inside the MSYS2 MINGW64 terminal:
```bash
pacman -Syu
pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-raylib git
```
Press `Enter` to select all default packages and type `Y` when prompted.

#### Step 4: Clone the repository and navigate into the folder
```bash
git clone https://github.com/your-username/svd-simulation.git
cd svd-simulation
```

#### Step 5: Compile the program
```bash
g++ svd_simulation.cpp -o svd_simulation.exe -O2 -std=c++17 -lraylib -lopengl32 -lgdi32 -lwinmm
```

#### Step 6: Run the executable
```bash
./svd_simulation.exe
```

---

## Alternative: Cross-Platform CMake Build (Auto-Downloads `raylib`)

If you have **CMake** ($3.14+$) and a C++ compiler installed, you can build the project without manually installing `raylib`.

#### Step 1: Create a `CMakeLists.txt` file in the root folder
Create a file named `CMakeLists.txt` alongside `svd_simulation.cpp` with the following contents:
```cmake
cmake_minimum_required(VERSION 3.14)
project(svd_simulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(svd_simulation svd_simulation.cpp)
target_link_libraries(svd_simulation PRIVATE raylib)
```

#### Step 2: Configure and compile with CMake
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

#### Step 3: Run the generated binary
- **Linux / macOS:**
  ```bash
  ./svd_simulation
  ```
- **Windows:**
  ```bash
  .\Release\svd_simulation.exe
  ```

---

## Step-by-Step Guide to Interacting with the Program

Once the window opens, follow these steps to explore the decomposition:

1. **Inspect the Initial Sphere:**
   - Click and drag anywhere in the 3D viewport with the **Left Mouse Button** to orbit the camera around the origin.
   - Scroll the **Mouse Wheel** up or down to zoom in and out.
2. **Apply Step 1 ($V^T$):**
   - Click the **"Step 1: Apply V^T (Rotation)"** button in the top-left panel (or press `1` on your keyboard).
   - Watch the unit sphere and its internal red, green, and blue orthogonal basis vectors smoothly rotate into alignment.
3. **Apply Step 2 ($\Sigma$):**
   - Click **"Step 2: Apply Sigma (Deformation)"** (or press `2` on your keyboard).
   - Observe the sphere stretch along the $X$ and $Y$ principal axes and compress along the $Z$ axis into an ellipsoid.
4. **Apply Step 3 ($U$):**
   - Click **"Step 3: Apply U (Final Rotation)"** (or press `3` on your keyboard).
   - Watch the deformed ellipsoid rotate into its final spatial orientation.
5. **Reset at Any Time:**
   - Click **"Reset Sphere"** (or press `0` or `R` on your keyboard) to smoothly animate back to the identity state.
   - Note that you can also jump directly from the reset state to Step 2 or Step 3 to see combined transformations animate simultaneously.

### Summary of Keyboard & Mouse Controls

| Input | Action |
| :--- | :--- |
| **Left Mouse Click + Drag** | Orbit 3D camera around the origin |
| **Mouse Scroll Wheel** | Zoom camera in / out |
| **Left Click UI Buttons** | Trigger corresponding SVD transformation step |
| **`0` or `R` Key** | **Reset:** Return sphere to initial identity state ($I$) |
| **`1` Key** | **Step 1:** Apply $V^T$ (Initial Rotation) |
| **`2` Key** | **Step 2:** Apply $\Sigma V^T$ (Rotation + Principal Scaling) |
| **`3` Key** | **Step 3:** Apply $U \Sigma V^T$ (Full SVD Transformation) |
| **`ESC` Key** | Close window and exit program |

---

## Troubleshooting

- **`error while loading shared libraries: libraylib.so` (Linux):**
  Run `sudo ldconfig` in your terminal so the dynamic linker refreshes its cache of `/usr/local/lib`.
- **`raylib.h: No such file or directory`:**
  Ensure `raylib` is installed for your compiler's target architecture, or use the **CMake Build** method above, which automatically downloads and links `raylib` for you.
- **Window appears too small on High-DPI / Retina displays:**
  The window is resizable—simply drag any corner of the window or maximize it; the 3D perspective projection matrix automatically updates on resize.

---

## Project Structure

```text
svd-simulation/
├── svd_simulation.cpp   # Complete C++17 source code (3D math, lighting, fog, and UI)
├── CMakeLists.txt       # (Optional) Cross-platform CMake build configuration
└── README.md            # Complete setup, compilation, and usage documentation
```
