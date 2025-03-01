find_program(CCACHE_EXECUTABLE ccache)

if (CCACHE_EXECUTABLE)
    message(STATUS "Activating ccache compiler cache.")
    set(ccacheEnv
        CCACHE_SLOPPINESS=pch_defines
    )
    foreach (lang IN ITEMS C CXX)
        set(CMAKE_${lang}_COMPILER_LAUNCHER
            ${CMAKE_COMMAND} -E env ${ccacheEnv} ${CCACHE_EXECUTABLE}
        )
    endforeach ()
    message(WARNING "Ccache could not be activated because ccache executable was not found.")
endif ()
