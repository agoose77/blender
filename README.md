blender
=======

Blender Cleanup Branch


This branch facilitates an engine cleanup for the BGE

Deliverables:

- Python API for updating engine components.
- Isolation of SCA_ classes and methods from KX_ code.
- Replace internal gameloop with a fixed timestep game loop.
- Overhaul of Data API, including support to create new meshes and cleaner LibNew.
- Python Collision API for Bullet (callbacks, contacts, modify existing logic bricks).
- Update of Python API to provide properties instead of get/set methods, rename bge.logic to bge.engine, rename of methods to more pythonic names.
- Complete separation of SCA systems from KX design, which would remove KX_GameObject.sensors, ..., 
- Investigate component-esque architecture for scene.physics, object.physics, object.logic.
