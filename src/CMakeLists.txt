add_executable(magnetwatcher
    "transmission.cpp"
    "main.cpp"
)

find_package(Qt5Gui)
find_package(Qt5Core)
find_package(Qt5Network)

include_directories()

target_link_libraries(magnetwatcher Qt5::Gui Qt5::Network)
set_target_properties(magnetwatcher PROPERTIES OUTPUT_NAME "magnet-watcher")
