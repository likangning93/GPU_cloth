set(SOURCE_FILES
    "glslUtility.hpp"
    "glslUtility.cpp"
    "utilityCore.hpp"
    "utilityCore.cpp"
    "nbody.hpp"
    "nbody.cpp"
    "cloth.hpp"
    "cloth.cpp"
    "rbody.hpp"
    "rbody.cpp"
    "mesh.hpp"
    "mesh.cpp"
    )

add_library(src
    ${SOURCE_FILES}
    )
