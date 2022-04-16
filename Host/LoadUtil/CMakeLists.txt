####################################################################################################
# Programmable load utility
#
# Helper to deal with programmable loads
####################################################################################################

add_executable(loadutil
    Sources/Main.cpp
)

target_link_libraries(loadutil PRIVATE libload)

# external libraries
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
