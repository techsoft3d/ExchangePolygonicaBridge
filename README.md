# ExchangePolygonicaBridge
## Installation
* Set the POLYGONICA_DIR environment variable to the base directory of your Polygonica installation
* Set the HEXCHANGE_INSTALL_DIR environment variable to the base directory of your HOOPS Exchange installation
* Open TranslateToPGSolids project file, don't upgrade but keep it as VS2015 (VC14)
* Build and run
## Todo
* Support linux/macosx
* Support edges - do not need at this time
* Add app data to PG entities that point to PRC data
* Support textures
* Benchmark performance for arboleda
* Check status returns
* Document functions
* Add gaurd on name object
* Add C# sample project running the unmanaged bridge.
* Suppport transparency (water bottle)
* Support materials with specular and ambient colors
* Wrap bridge as class? - TBD
* Properly set triangle handedness of elbows in arboleda-plumb
## Doing
* Clean up IndicesPerFaceAsTriangles (Jonathan)
* Supported textured faces - ignore textures but keep triangles (Jonathan)
## Done
* Some leaf nodes in arobleda-plumb names are not accessible (jonathan)
* When selecting a region based on the app_data, other parts get highlighted (jonathan)
* Port to a standalone project (Peter)
* Handle Normals
* Get Arboleda Plumb dataset working (Jonathan)
* Handle colors defined as materials (currently only using the defuse channel)
* Handle cascading colors
* Use indexed styles instead of creating a new style for every part
* Support faces - do so with regions (PTEntityGroups), important for CAE
* Fixed bug in getName()
* Remove use of global m_sTessData
* Add names & paths to parts in app data "Model/SubAssem/bearing"
* Instancing
* Create destructor for path to free memory in the paths
## Roadmap
* May 2022 - Development
* June 2022 - Document, Code Review, Cleanup, & Testing
* July 2022 - Release