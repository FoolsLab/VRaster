
find_package(VRaster REQUIRED)

add_executable(simple-box simple-box/main.cpp)
target_link_libraries(simple-box VRaster)

# add_executable(simple-stg simple-stg/main.cpp)
# target_link_libraries(simple-stg VRaster)
