# Event Horizon Observatory

Native C++ OpenGL black hole simulation inspired by gravitational lensing, curved spacetime, and accretion-disk visualizations.

## Requirements

- C++17 compiler
- CMake 3.16+
- GLFW3 and OpenGL development packages

On Debian or Ubuntu:

```bash
sudo apt install build-essential cmake libglfw3-dev libgl1-mesa-dev
```

## Build and run

```bash
cmake -B build -S .
cmake --build build
./build/event_horizon
```

## Controls

- `Space`: pause or resume the field
- `G`: toggle the spacetime grid
- `R`: reseed the particle field
- `Up` / `Down`: increase or decrease mass
- `Left` / `Right`: change spin
- Hold left mouse button and drag: orbit the camera around the black hole
- `Esc`: exit

The fullscreen scene includes a curved spacetime grid, a rotating accretion disk, a moving light object, and lensing rays that bend around the event horizon.