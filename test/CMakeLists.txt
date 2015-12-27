aux_source_directory(. SRCS)

include_directories(
        ${INC_DIR}
)

link_directories(
		${LINK_DIR}
)

add_executable(test_v6
	${SRCS}
)

target_link_libraries(test_v6
    pthread
    engine
)
