#find_package(Eigen3 REQUIRED)
set(DatasetExporter_src
    main.cpp
    export.h
    export.cpp
    main.h
    enums.h
    nativeCaller.h
    natives.h
    types.h
    scenario.h
    scenario.cpp
    keyboard.h
    keyboard.cpp
)

add_library(DatasetExporter SHARED ${DatasetExporter_src})
target_link_libraries(DatasetExporter PUBLIC ${PROJECT_SOURCE_DIR}/deps/ScriptHookV.lib)
#TARGET_INCLUDE_DIRECTORIES(DatasetExporter PRIVATE $ENV{EIGEN3_INCLUDE_DIR})
set_target_properties(DatasetExporter PROPERTIES SUFFIX ".asi")

