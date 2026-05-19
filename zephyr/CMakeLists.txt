if(CONFIG_OPENBMC_SDBUSPLUS)
    zephyr_library_named(sdbusplus)
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
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)

    zephyr_export_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)

    # get_filename_component(BASU_DIR
    #     "${ZEPHYR_CURRENT_MODULE_DIR}/../../modules/lib/basu" ABSOLUTE)
    # message(BASU_DIR: ${BASU_DIR})

    # zephyr_library_include_directories(
    #    ${BASU_DIR}/include
    #    ${BASU_DIR}/zephyr/include
    #    ${BASU_DIR}/include/systemd
    #    )

    # zephyr_library_sources(${BASU_DIR}/src/libsystemd/sd-event/sd-event.c)
    zephyr_library_link_libraries(basu)
    zephyr_library_link_libraries(nlohmann_json)
    # add_subdirectory("${BASU_DIR}/zephyr" basu)
    # # target_include_directories(basu PRIVATE ${BASU_DIR}/include/systemd)

    # target_link_libraries(sdbusplus PRIVATE basu)

endif()
