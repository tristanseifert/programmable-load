####################################################################################################
# Programmable load utility
#
# Helper to deal with programmable loads
####################################################################################################

add_executable(loadutil
    Sources/Main.cpp
    Sources/GetInfo.cpp
)

target_link_libraries(loadutil PRIVATE libload)

# external libraries
target_link_libraries(loadutil PRIVATE fmt::fmt-header-only)

FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    GIT_TAG        v2.2.0
)
FetchContent_MakeAvailable(cli11)
target_link_libraries(loadutil PRIVATE CLI11::CLI11)

FetchContent_Declare(tabulate
    GIT_REPOSITORY https://github.com/p-ranav/tabulate
    GIT_TAG        v1.4
)
FetchContent_MakeAvailable(tabulate)
target_link_libraries(loadutil PRIVATE tabulate::tabulate)

FetchContent_Declare(rang
    GIT_REPOSITORY https://github.com/agauniyal/rang.git
    GIT_TAG        v3.2
)
FetchContent_MakeAvailable(rang)
target_link_libraries(loadutil PRIVATE rang)
