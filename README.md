# SVD 3D Geometric Simulation (C++ / raylib)

An interactive 3D geometric visualization of **Singular Value Decomposition ($A = U \Sigma V^T$)** built in C++ using [raylib](https://www.raylib.com/). 

This simulation demonstrates how any linear transformation $A$ acting on a 3D unit sphere and its orthogonal basis vectors can be decomposed into three distinct geometric operations:
1. **$V^T$ (Initial Rotation):** Aligns the input basis with the principal axes.
2. **$\Sigma$ (Scaling / Deformation):** Stretches and compresses the sphere along the principal axes ($\sigma_1 = 2.5, \sigma_2 = 1.2, \sigma_3 = 0.4$) into an ellipsoid.
3. **$U$ (Final Rotation):** Orients the deformed ellipsoid into its final spatial position.

---

## Features

- **Hierarchical Transformation Stack:** Real-time matrix composition ($M = U \cdot \Sigma \cdot V^T$) with inverse-transpose normal matrix correction for accurate lighting during non-uniform scaling.
- **Smooth Interpolation:** Frame-rate independent Quaternion SLERP (for $U$ and $V^T$) and Vector LERP (for $\Sigma$).
- **Custom Lighting & Exponential Fog:** Dual directional Blinn-Phong shading, translucent two-pass sphere rendering, wireframe overlay, and exponential distance fog (`FogExp2`).
- **Damped Orbit Controls:** Smooth spherical camera rotation and scroll-wheel zoom.
- **Built-in UI Overlay:** Interactive step buttons and a real-time explanation panel.

---

## Prerequisites

You need a **C++17 compiler** (`g++` or `clang++`) and the **raylib** library installed on your system.

### 1. Install raylib

- **Ubuntu / Debian:**
  ```bash
  sudo apt update
  sudo apt install build-essential git libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
