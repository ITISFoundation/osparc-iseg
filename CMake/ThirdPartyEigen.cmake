#
# Define how to use third party, i.e. 
# - include directory
# - library
# - preprocessor definitions needed by package
#

SET(EIGEN_INCLUDE_DIR ${EigenRootDir})
 
MACRO(USE_EIGEN)
	INCLUDE_DIRECTORIES( ${EIGEN_INCLUDE_DIR} )
ENDMACRO()

# tell Eigen to use MKL BLAS, LAPACK and Intel VML (for vector operations)
# note: you should link to ${MKL_LIBRARIES}
MACRO(USE_EIGEN_MKL)
	INCLUDE_DIRECTORIES( ${EIGEN_INCLUDE_DIR} )
	
	ADD_DEFINITIONS(-DEIGEN_USE_MKL_ALL)
	IF(MSVC)
		ADD_DEFINITIONS("/wd4244") # conversions int <-> int64
	ENDIF()
	USE_MKL()
ENDMACRO()
