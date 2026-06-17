cmake_minimum_required(VERSION 3.20)

project(DDSGenerator)

set(CMAKE_CXX_STANDARD 17)

file(GLOB PSU_DATA_SOURCES_CXX "${CMAKE_SOURCE_DIR}/PsuData/*.cxx")

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/xml
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release
    FILES_MATCHING PATTERN "*"
)

add_executable(DDSPublisher publisher.cpp ${PSU_DATA_SOURCES_CXX})

target_include_directories(DDSPublisher PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/PsuData
)

target_link_libraries(DDSPublisher PRIVATE
    "${CMAKE_SOURCE_DIR}/lib/fastdds-3.6.lib"
    "${CMAKE_SOURCE_DIR}/lib/fastcdr-2.3.lib"
    "${CMAKE_SOURCE_DIR}/lib/tinyxml2.lib"
)

target_compile_definitions(DDSPublisher PRIVATE
    FASTDDS_DYN_LINK
    FASTCDR_DYN_LINK
)

add_executable(DDSSubscriber subscriber.cpp ${PSU_DATA_SOURCES_CXX})

target_include_directories(DDSSubscriber PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/PsuData
)

target_link_libraries(DDSSubscriber PRIVATE
    "${CMAKE_SOURCE_DIR}/lib/fastdds-3.6.lib"
    "${CMAKE_SOURCE_DIR}/lib/fastcdr-2.3.lib"
    "${CMAKE_SOURCE_DIR}/lib/tinyxml2.lib"
)

target_compile_definitions(DDSSubscriber PRIVATE
    FASTDDS_DYN_LINK
    FASTCDR_DYN_LINK
)

add_custom_command(TARGET DDSPublisher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin
            $<TARGET_FILE_DIR:DDSPublisher>
)