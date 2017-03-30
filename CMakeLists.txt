set(CMAKE_CXXFLAGS ${CMAKE_CXXFLAGS} --std=c++11)
include_directories(extern)
add_executable(example examplemon.cpp jsonbin.cpp extern/jsoncpp.cpp extern/mongoose.c base64.cpp)
