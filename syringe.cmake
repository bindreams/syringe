cmake_minimum_required(VERSION 3.15.0)

function(inject_files)
	cmake_parse_arguments(INJECT "" "VARIABLE;PREFIX;RELATIVE;OUTPUT" "FILES" ${ARGN})

	find_program(SYRINGE_EXECUTABLE syringe REQUIRED)

	# Parse files ------------------------------------------------------------------------------------------------------
	if(NOT INJECT_FILES)
		message(SEND_ERROR "inject_files: required FILES argument is empty or missing")
	endif()

	foreach(INJECT_FILE IN LISTS INJECT_FILES)
		file(REAL_PATH "${INJECT_FILE}" INJECT_FILE_ BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" EXPAND_TILDE)
		list(APPEND INJECT_FILES_ "${INJECT_FILE_}")
	endforeach()
	set(INJECT_FILES ${INJECT_FILES_})

	# Parse output file ------------------------------------------------------------------------------------------------
	if(NOT INJECT_OUTPUT)
		message(SEND_ERROR "inject_files: required OUTPUT argument is empty or missing")
	endif()

	file(REAL_PATH
		"${INJECT_OUTPUT}" INJECT_OUTPUT
		BASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/syringe_include/_"
		EXPAND_TILDE
	)
	get_filename_component(INJECT_OUTPUT_DIR "${INJECT_OUTPUT}" DIRECTORY)

	# Parse keyword args -----------------------------------------------------------------------------------------------
	if(INJECT_RELATIVE)
		set(INJECT_RELATIVE_ARGS --relative "${INJECT_RELATIVE}")
	endif()

	if(INJECT_PREFIX)
		set(INJECT_PREFIX_ARGS --prefix "${INJECT_PREFIX}")
	endif()

	if(INJECT_VARIABLE)
		set(INJECT_VARIABLE_ARGS --variable "${INJECT_VARIABLE}")
	endif()

	# Create command ---------------------------------------------------------------------------------------------------
	add_custom_command(
		OUTPUT "${INJECT_OUTPUT}"
		DEPENDS ${INJECT_FILES}
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${INJECT_OUTPUT_DIR}"
		COMMAND "${SYRINGE_EXECUTABLE}"
			${INJECT_FILES}
			${INJECT_RELATIVE_ARGS}
			${INJECT_PREFIX_ARGS}
			${INJECT_VARIABLE_ARGS}
			> "${INJECT_OUTPUT}"
		COMMENT "Injecting files into ${INJECT_OUTPUT}"
		VERBATIM
	)
endfunction()

function(target_inject_files TARGET)
	cmake_parse_arguments(INJECT "" "VARIABLE;PREFIX;RELATIVE;OUTPUT" "FILES" ${ARGN})

	set(BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/syringe_include/${TARGET}")
	if(IS_ABSOLUTE ${INJECT_OUTPUT})
		message(SEND_ERROR "target_inject_files: OUTPUT cannot be an absolute path - header path is appended to ${BASE_DIR}")
	endif()

	inject_files(
		FILES ${INJECT_FILES}
		OUTPUT "${BASE_DIR}/${INJECT_OUTPUT}"
		VARIABLE "${INJECT_VARIABLE}"
		RELATIVE "${INJECT_RELATIVE}"
		PREFIX "${INJECT_PREFIX}"
	)

	target_include_directories(${TARGET} PRIVATE "${BASE_DIR}")
	target_sources(${TARGET} PRIVATE "${BASE_DIR}/${INJECT_OUTPUT}")
endfunction()
