#
# Define how to use third party, i.e. 
# - include directory
# - library
# - preprocessor definitions needed by package
#

#SET(EigenRootDir "" CACHE PATH "Directory containing Eigen/Cholesky")
#SET(EIGEN_INCLUDE_DIR ${EigenRootDir})
 
MACRO(USE_EIGEN)
	find_package (Eigen3 3.3 REQUIRED NO_MODULE)

	#INCLUDE_DIRECTORIES( ${EIGEN_INCLUDE_DIR} )
	LIST( APPEND MY_EXTERNAL_LINK_LIBRARIES Eigen3::Eigen )
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
