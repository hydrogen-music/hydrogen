# make clean does not delete *.ts and *.qm files
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

file(GLOB TS_FILES ${CMAKE_CURRENT_LIST_DIR}/*.ts)

qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(translations ALL DEPENDS ${QM_FILES})

IF(MINGW)
	install(FILES ${QM_FILES} DESTINATION "./data/i18n")
ELSE()
	install(FILES ${QM_FILES} DESTINATION "${H2_SYS_PATH}/data/i18n")
ENDIF()
