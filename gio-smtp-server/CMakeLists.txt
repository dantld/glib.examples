cmake_minimum_required(VERSION 3.12)
project(gio-smtp-server)

set(BINARY gio-stmp-server)

add_executable(${BINARY}
    d_timeout.cpp
    d_smtp_state.cpp
    d_smtp_command.cpp
    d_smtp_connection.cpp
    d_smtp_server_app.cpp
    d_smtp_server.cpp
    d_smtp_server_main.cpp
    )

target_link_libraries(${BINARY}
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${GIOUNIX_LIBRARIES}
    )
