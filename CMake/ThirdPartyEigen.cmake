#
# Define how to use third party, i.e. 
# - include directory
# - library
# - preprocessor definitions needed by package
#

MACRO(USE_EIGEN)
	FIND_PACKAGE (Eigen3 3.3 REQUIRED QUIET NO_MODULE)
	IF (Eigen3_FOUND)
		LIST( APPEND MY_EXTERNAL_LINK_LIBRARIES Eigen3::Eigen )
	ELSE()
		FIND_PATH(EIGEN_INCLUDE_DIR NAMES Eigen/Dense Eigen/Dense PATHS /usr/local/include/eigen3)
		INCLUDE_DIRECTORIES( ${EIGEN_INCLUDE_DIR} )
	ENDIF()
ENDMACRO()

# tell Eigen to use MKL BLAS, LAPACK and Intel VML (for vector operations)
# note: you should link to ${MKL_LIBRARIES}
MACRO(USE_EIGEN_MKL)
	USE_EIGEN()
	
	ADD_DEFINITIONS(-DEIGEN_USE_MKL_ALL)
	IF(MSVC)
		ADD_DEFINITIONS("/wd4244") # conversions int <-> int64
	ENDIF()
	USE_MKL()
ENDMACRO()
