# Generated from skipcleanup.pro.

#####################################################################
## skipcleanup Binary:
#####################################################################

add_qt_executable(skipcleanup
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_skipcleanup.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(skipcleanup)
# special case end
