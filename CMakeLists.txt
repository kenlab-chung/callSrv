cmake_minimum_required(VERSION 3.10)

project(cs VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/inc
                    /opt/boost_1_85_0
                    /opt/rapidjson/include
                    ./libpqxx/include
                    )
link_directories(${PROJECT_SOURCE_DIR}/lib
                    /opt/boost_1_85_0/stage/lib
                    ./libpqxx/lib                  
)

add_executable(${PROJECT_NAME} src/main.cpp)
target_sources(${PROJECT_NAME}  PRIVATE src/CallService.cpp  
                                        src/CRestfulServer.cpp  
                                        src/WebsocketSession.cpp 
                                        src/WebsocketServer.cpp                                    
                                        )

target_link_libraries(${PROJECT_NAME}  libesl.a
                                    libcurl.a
                                    libpqxx.a
                                    libpq.a
                                    libecpg.a
                                    libecpg_compat.a
                                    libpgcommon.a
                                    libpgcommon_shlib.a
                                    libpgfeutils.a
                                    libpgport.a
                                    libpgport_shlib.a
                                    libpgtypes.a
                                  )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
