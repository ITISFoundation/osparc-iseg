# Macro to provide add a target to a filter in VS. Has no effect for other compilers:
#
#  VS_SET_PROPERTY (TargetName FolderName)
#
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

MACRO(VS_SET_PROPERTY targetname foldername)
	IF(MSVC)
		SET_PROPERTY(TARGET ${targetname} PROPERTY FOLDER ${foldername})
	ENDIF(MSVC)
ENDMACRO(VS_SET_PROPERTY)


