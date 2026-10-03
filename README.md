# Wayfinding Engine

An interactive map viewer and route-planning application for OpenStreetMap
data. It renders cities as a pannable, zoomable map and computes
shortest-time driving routes between intersections, with turn-by-turn
directions.
<img width="1035" height="693" alt="image" src="https://github.com/user-attachments/assets/616a2cc5-b76b-419c-a003-f962d0e14138" />

## Features

- Renders streets, features (water, parks, buildings, beaches, glaciers),
  points of interest, and transit detail from OpenStreetMap data.
- Pan and zoom with the mouse or on-screen controls, with level-of-detail
  that adjusts road width and label density to the current zoom.
- Shortest-travel-time routing between two intersections using A* with a
  turn penalty, rendered as a highlighted path with written directions.
- Street search with prefix auto-completion, and intersection search by two
  street names.
- Point-of-interest layers (food, leisure, driving, school, emergency,
  subway stations) toggled individually.
- Light and dark themes.
- Switchable maps for a set of cities via a labelled drop-down.

## Build

The project builds with GNU Make and a C++17 compiler. It depends on GTK 3,
Cairo, X11, libcurl, and the course-provided StreetsDatabase and OSMDatabase
libraries.

    make            # builds the mapper executable
    make clean      # removes build artifacts
    make test       # builds and runs the unit tests

Parallel builds are supported, for example `make -j4`.

## Run

    ./mapper                 # loads the default map
    ./mapper <path.streets.bin>   # loads a specific map

The application prints the render time and frame rate to the terminal on
every refresh.

## Source layout

The library source lives under `libstreetmap/src`, organized by domain
rather than by development milestone:

    loading/map_loader.cpp     Map loading and all derived data structures.
    query/map_queries.cpp      Distance, geometry, and lookup queries.
    render/map_renderer.cpp     Canvas drawing and the frame loop.
    render/spatial_grid.h       Uniform grid used to cull off-screen geometry.
    render/ui_state.h           Shared render and UI state declarations.
    ui/map_ui.cpp               Controls, callbacks, directions, and styling.
    routing/path_finder.cpp     A* routing and path reconstruction.
    courier/courier.cpp         Multi-stop delivery (traveling-courier) solver.
    global.h                    Shared data structures and declarations.

The application entry point is in `main/src/main.cpp`. The vendored EZGL
graphics layer is in `libstreetmap/src/ezgl`.

## Performance notes

Map data is loaded in parallel across several stages, with each stage writing
to disjoint, pre-sized structures to avoid data races. Per-segment attributes
(road class, projected endpoints, bounding box) are computed once at load so
the draw loop performs no database queries or string comparisons. A uniform
spatial grid restricts each frame to the geometry overlapping the visible
area instead of scanning the full data set.
