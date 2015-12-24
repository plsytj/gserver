aux_source_directory(. SRCS)

include_directories(
        ${INC_DIR}
)

link_directories(
		${LINK_DIR}
)

add_executable(gtw
	${SRCS}
)

target_link_libraries(
    engine
)
