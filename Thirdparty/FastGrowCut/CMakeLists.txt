#project(FastGrowCut)

USE_VTK()

set(FASTGROWCUT_SRCS
	FibHeap.cxx
	FibHeap.h
	vtkImageGrowCutSegment.cxx
	vtkImageGrowCutSegment.h
)

add_library(FastGrowCut ${FASTGROWCUT_SRCS})
target_link_libraries(FastGrowCut ${MY_EXTERNAL_LINK_LIBRARIES})
VS_SET_PROPERTY(FastGrowCut Thirdparty)