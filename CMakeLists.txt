set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++11)
add_definitions(-DWITHMONGOOSE)
include_directories(extern)
add_executable(example examplemon.cpp jsonbag.cpp extern/jsoncpp.cpp extern/mongoose.c extern/base64.cpp)
