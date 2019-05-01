# Event Planner

Renders 2D shapes and patterns to simulate an event planner application, where a dynamic number of various item symbols symbolizes different items like chairs, tables, or lamps.

**Preview**

<img src="https://github.com/eutopi/event-planner/blob/master/screenshot.png" alt="drawing" width="400"/>

## Features 

1. **Plant**: symbolized by a many-pointed star, resembling a palm or fern from above. 
2. **Coat rack**: symbolized by the quadrifolia. The perimeter vertices are obtained by evaluating a rose-curve formula. 
3. **Striped**: stripe orientation is currently fixed at 45 degrees, but the colors and widths are parametrizable.
4. **Heartbeat**: passed time as an uniform to the shaders to generate smoothly changing, pulsating colors.
5. **Mouse pick**: clicking near the center of an object selects it and deselects other objects.
6. **Mouse drag**: object translation corresponds with mouse offset.
7. **Key rotate**: change orientations of the selected objects when the `A` or `D` keys are held down.
8. **Delete**: selected objects should be removed if `DEL` is pressed.
9. **Zoom**: pressing `Z` should zoom in, pressing `X` should zoom out.
10. **Move camera**: `I`, `J`, `K`, `L` keys to move camera

## Frameworks
- OpenGL
- GLUT
