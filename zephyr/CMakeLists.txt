if(CONFIG_OPENBMC_SDBUSPLUS)
    zephyr_library()
    zephyr_library_sources(
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/async/context.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/async/match.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/match.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/message/native_types.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/server/interface.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/server/transaction.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/event.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/exception.cpp
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/sdbus.cpp
    )
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)

endif()