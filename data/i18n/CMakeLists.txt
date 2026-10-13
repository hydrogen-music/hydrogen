# make clean does not delete *.ts and *.qm files
SET_DIRECTORY_PROPERTIES(PROPERTIES CLEAN_NO_CUSTOM 1)

FILE(GLOB TS_FILES ${CMAKE_CURRENT_LIST_DIR}/*.ts)

# Both qt5_add_translation and qt6_add_translation default to emitting
# the .qm files into CMAKE_CURRENT_BINARY_DIR. But the runtime layout —
# and the unit test suite, whose system data dir is redirected to this
# source data/ folder — expect them next to their .ts sources: the same
# place build.sh's translations step writes them, and the place
# .gitignore anticipates. Without this a fresh checkout + build ships
# no catalogs in data/i18n and translator-dependent tests fail (the
# AppVeyor Ubuntu 22.04 lesson: no environment-dependent greens).
set_source_files_properties(${TS_FILES} PROPERTIES
	OUTPUT_LOCATION "${CMAKE_CURRENT_LIST_DIR}")

if(H2CORE_HAVE_QT6)
	QT6_ADD_TRANSLATION(QM_FILES ${TS_FILES})
else()
	QT5_ADD_TRANSLATION(QM_FILES ${TS_FILES})
endif()
ADD_CUSTOM_TARGET(translations ALL DEPENDS ${QM_FILES})

IF(MINGW)
	INSTALL(FILES ${QM_FILES} DESTINATION "./data/i18n")
ELSE()
	# Relative (H2_DATA_PATH = share/hydrogen) so the install relocates with
	# CMAKE_INSTALL_PREFIX / DESTDIR / CPack staging — resolves to the runtime
	# system data path (H2_SYS_PATH) at the default prefix. Was H2_SYS_PATH (an
	# absolute path), which broke staged/packaged installs (T6.2).
	INSTALL(FILES ${QM_FILES} DESTINATION "${H2_DATA_PATH}/data/i18n")
ENDIF()
