/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#ifdef DEBUG_GC
#	include <itkRescaleIntensityImageFilter.h>
#	include <itkImageFileWriter.h>
#endif

namespace itk {

template<int NDim>
struct NeighborInfo
{
	static unsigned int numberOfNeighbors(eGcConnectivity type)
	{
		return (type == eGcConnectivity::kFaceNeighbors) ? 6 : 26;
	}

	template<class OffsetType>
	static void buildNeighbors(std::vector<OffsetType>& neighbors, eGcConnectivity connectivity)
	{
		Gc::Energy::Neighbourhood<3, Gc::Int32> nb;
		nb.Common(numberOfNeighbors(connectivity), false);
		Gc::System::Algo::Sort::Heap(nb.Begin(), nb.End());

		// Traverses the image adding the following bidirectional edges:
		// 1. currentPixel <-> pixel below it
		// 2. currentPixel <-> pixel to the right of it
		// 3. currentPixel <-> pixel in front of it
		// This prevents duplicate edges (i.e. we cannot add an edge to all 6-connected neighbors of every pixel or
		// almost every edge would be duplicated.
		OffsetType frontbottomrightr = {{nb.m_data[0][0], nb.m_data[0][1], nb.m_data[0][2]}};
		neighbors.push_back(frontbottomrightr);
		OffsetType graph4r = {{nb.m_data[1][0], nb.m_data[1][1], nb.m_data[1][2]}};
		neighbors.push_back(graph4r);
		OffsetType bottom = {{nb.m_data[2][0], nb.m_data[2][1], nb.m_data[2][2]}};
		neighbors.push_back(bottom);
		OffsetType right = {{nb.m_data[3][0], nb.m_data[3][1], nb.m_data[3][2]}};
		neighbors.push_back(right);
		OffsetType front = {{nb.m_data[4][0], nb.m_data[4][1], nb.m_data[4][2]}};
		neighbors.push_back(front);
		OffsetType frontright = {{nb.m_data[5][0], nb.m_data[5][1], nb.m_data[5][2]}};
		neighbors.push_back(frontright);
		if (connectivity == eGcConnectivity::kNodeNeighbors)
		{
			OffsetType frontbottom = {{nb.m_data[6][0], nb.m_data[6][1], nb.m_data[6][2]}};
			neighbors.push_back(frontbottom);
			OffsetType bottomright = {{nb.m_data[7][0], nb.m_data[7][1], nb.m_data[7][2]}};
			neighbors.push_back(bottomright);
			OffsetType frontbottomright = {{nb.m_data[8][0], nb.m_data[8][1], nb.m_data[8][2]}};
			neighbors.push_back(frontbottomright);
			OffsetType graph1 = {{nb.m_data[9][0], nb.m_data[9][1], nb.m_data[9][2]}};
			neighbors.push_back(graph1);
			OffsetType graph2 = {{nb.m_data[10][0], nb.m_data[10][1], nb.m_data[10][2]}};
			neighbors.push_back(graph2);
			OffsetType graph3 = {{nb.m_data[11][0], nb.m_data[11][1], nb.m_data[11][2]}};
			neighbors.push_back(graph3);
			OffsetType graph4 = {{nb.m_data[12][0], nb.m_data[0][1], nb.m_data[12][2]}};
			neighbors.push_back(graph4);
			OffsetType graph5 = {{nb.m_data[13][0], nb.m_data[13][1], nb.m_data[13][2]}};
			neighbors.push_back(graph5);
			OffsetType graph6 = {{nb.m_data[14][0], nb.m_data[14][1], nb.m_data[14][2]}};
			neighbors.push_back(graph6);
			OffsetType bottomr = {{nb.m_data[15][0], nb.m_data[15][1], nb.m_data[15][2]}};
			neighbors.push_back(bottomr);
			OffsetType rightr = {{nb.m_data[16][0], nb.m_data[16][1], nb.m_data[16][2]}};
			neighbors.push_back(rightr);
			OffsetType frontr = {{nb.m_data[17][0], nb.m_data[17][1], nb.m_data[17][2]}};
			neighbors.push_back(frontr);
			OffsetType frontrightr = {{nb.m_data[18][0], nb.m_data[18][1], nb.m_data[18][2]}};
			neighbors.push_back(frontrightr);
			OffsetType frontbottomr = {{nb.m_data[19][0], nb.m_data[19][1], nb.m_data[19][2]}};
			neighbors.push_back(frontbottomr);
			OffsetType bottomrightr = {{nb.m_data[20][0], nb.m_data[20][1], nb.m_data[20][2]}};
			neighbors.push_back(bottomrightr);
			OffsetType graph1r = {{nb.m_data[21][0], nb.m_data[21][1], nb.m_data[21][2]}};
			neighbors.push_back(graph1r);
			OffsetType graph2r = {{nb.m_data[22][0], nb.m_data[22][1], nb.m_data[22][2]}};
			neighbors.push_back(graph2r);
			OffsetType graph3r = {{nb.m_data[23][0], nb.m_data[23][1], nb.m_data[23][2]}};
			neighbors.push_back(graph3r);
			OffsetType graph5r = {{nb.m_data[24][0], nb.m_data[24][1], nb.m_data[24][2]}};
			neighbors.push_back(graph5r);
			OffsetType graph6r = {{nb.m_data[25][0], nb.m_data[25][1], nb.m_data[25][2]}};
			neighbors.push_back(graph6r);
		}
	}
};

template<>
struct NeighborInfo<2>
{
	static unsigned int numberOfNeighbors(eGcConnectivity type)
	{
		return (type == eGcConnectivity::kFaceNeighbors) ? 6 : 26;
	}
};

namespace {

template<typename TImage>
void dump_image(TImage* img, const std::string& file_path)
{
#ifdef DEBUG_GC
	auto writer = itk::ImageFileWriter<TImage>::New();
	writer->SetInput(img);
	writer->SetFileName(file_path);
	writer->Update();
#endif
}

} // namespace

template<typename TInput, typename TOutput, typename TInputIntensityImage>
GraphCutLabelSeparator<TInput, TOutput, TInputIntensityImage>::GraphCutLabelSeparator()
{
	// set number of required inputs ?
}

template<typename TInput, typename TOutput, typename TInputIntensityImage>
void GraphCutLabelSeparator<TInput, TOutput, TInputIntensityImage>::GenerateData()
{
	itk::TimeProbesCollectorBase timer;

	timer.Start("ITK init");
	auto input = GetMaskInput();
	auto inputRegion = input->GetLargestPossibleRegion();
	auto output = this->GetOutput();
	auto outputRegion = output->GetRequestedRegion();

	// init ITK progress reporter
	// InitializeGraph() traverses the input image once
	// CutGraph() traverses the output image once
	ProgressReporter progress(this, 0, inputRegion.GetNumberOfPixels() + outputRegion.GetNumberOfPixels());

	// allocate output
	output->SetBufferedRegion(outputRegion);
	output->Allocate();

	// get the total image size
	auto size = inputRegion.GetSize();
	timer.Stop("ITK init");

	// create graph
	timer.Start("Graph creation");
	GraphType* graph = nullptr;
	if (m_MaxFlowAlgorithm == kKohli)
	{
		graph = new Gc::Flow::Grid::Kohli<NDimension, Gc::Float32, Gc::Float32, Gc::Float32, false>;
	}
	else if (m_MaxFlowAlgorithm == kPushLabelFifo)
	{
		graph = new Gc::Flow::Grid::PushRelabel::Fifo<NDimension, Gc::Float32, Gc::Float32, false>;
	}
	else if (m_MaxFlowAlgorithm == kPushLabelHighestLevel)
	{
		graph = new Gc::Flow::Grid::PushRelabel::HighestLevel<NDimension, Gc::Float32, Gc::Float32, false>;
	}

	Gc::Math::Algebra::Vector<NDimension, Gc::Size> sizing;
	for (unsigned int i = 0; i < NDimension; ++i)
	{
		sizing[i] = size[i];
	}
	Gc::Energy::Neighbourhood<3, Gc::Int32> nb;
	nb.Common(NeighborInfo<NDimension>::numberOfNeighbors(m_Connectivity), false);
	Gc::System::Algo::Sort::Heap(nb.Begin(), nb.End());
	graph->Init(sizing, nb);
	timer.Stop("Graph creation");

	timer.Start("Graph init");
	InitializeGraph(graph, progress);
	timer.Stop("Graph init");

	if (this->GetAbortGenerateData())
	{
		return;
	}

	// cut graph
	timer.Start("Graph cut");
	graph->FindMaxFlow();
	timer.Stop("Graph cut");

	timer.Start("Query results");
	CutGraph(graph, progress); //&
	timer.Stop("Query results");

	if (m_PrintTimer)
	{
		timer.Report(std::cout);
	}
}

template<typename TInput, typename TOutput, typename TInputIntensityImage>
void GraphCutLabelSeparator<TInput, TOutput, TInputIntensityImage>::InitializeGraph(GraphType* graph, ProgressReporter& progress)
{
	using RealImageType = itk::Image<float, NDimension>;
	using MaskIteratorType = itk::ShapedNeighborhoodIterator<InputImageType>;
	using GradientMagnitudeIteratorType = itk::ShapedNeighborhoodIterator<RealImageType>;
	using OffsetType = typename MaskIteratorType::OffsetType;
	using GradientMagnitudeFilter = itk::GradientMagnitudeRecursiveGaussianImageFilter<IntensityImageType, RealImageType>;

	const bool& abort = this->GetAbortGenerateData(); // use reference (alias) to original flag!
	auto input = GetMaskInput();
	OffsetType center;
	center.Fill(0);
	itk::Size<NDimension> radius;
	radius.Fill(1);

	// Setup neighborhood (graph connectivity)
	std::vector<OffsetType> neighbors;
	NeighborInfo<NDimension>::buildNeighbors(neighbors, m_Connectivity);
	auto const num_neighbors = neighbors.size();

	auto input_region = input->GetLargestPossibleRegion();
	MaskIteratorType iterator(radius, input, input_region);
	iterator.ClearActiveList();
	for (size_t i = 0; i < neighbors.size(); ++i)
	{
		iterator.ActivateOffset(neighbors[i]);
	}
	iterator.ActivateOffset(center);

	if (m_UseGradientMagnitude)
	{
		if (!GetIntensityInput())
		{
			throw std::runtime_error("Expecting input with image intensities");
		}
		//Gaussian Filter for Edge enhancement
		auto magnitude_filter = GradientMagnitudeFilter::New();
		magnitude_filter->SetInput(GetIntensityInput());
		magnitude_filter->SetSigma(m_Sigma);
		magnitude_filter->UpdateLargestPossibleRegion();
		auto gradient_magnitude = magnitude_filter->GetOutput();

#ifdef DEBUG_GC
		{
			auto rescale = itk::RescaleIntensityImageFilter<RealImageType>::New();
			rescale->SetInput(magnitude_filter->GetOutput());
			rescale->SetOutputMinimum(0.0);
			rescale->SetOutputMaximum(1.0);
			rescale->UpdateLargestPossibleRegion();
			dump_image(rescale->GetOutput(), "E:/temp/_gc_grad_mag.nii.gz");
		}
#endif

		auto calculator = itk::MinimumMaximumImageCalculator<RealImageType>::New();
		calculator->SetImage(gradient_magnitude);
		calculator->Compute();

		auto min_gm = calculator->GetMinimum();
		auto max_gm = calculator->GetMaximum();

		// rescale to range [0,1]
		float scale = 0.f, shift = 0.f;
		if (min_gm != max_gm)
		{
			scale = 1.f / (max_gm - min_gm);
		}
		else if (max_gm != 0.f)
		{
			scale = 1.f / max_gm;
		}
		shift = -min_gm * scale;

		auto rescale = [scale, shift](float v) {
			return v * scale + shift;
		};

		if (abort)
		{
			return;
		}

		GradientMagnitudeIteratorType iterator2(radius, gradient_magnitude, input_region);
		for (iterator.GoToBegin(), iterator2.GoToBegin(); !iterator.IsAtEnd() && !abort; ++iterator, ++iterator2)
		{
			progress.CompletedPixel();

			bool pixelIsValidcenter; // BL TODO, slow
			auto centerPixel = iterator.GetPixel(center, pixelIsValidcenter);
			if (!pixelIsValidcenter)
			{
				continue;
			}

			unsigned int nodeIndex1 = ConvertIndexToVertexDescriptor(iterator.GetIndex(center), input_region);

			// Set weights on edges
			for (size_t i = 0; i < num_neighbors; i++)
			{
				bool pixelIsValid; // BL TODO, slow
				auto neighborPixel = iterator.GetPixel(neighbors[i], pixelIsValid);
				if (!pixelIsValid)
				{
					continue;
				}

				if (centerPixel != m_BackgroundValue && neighborPixel != m_BackgroundValue)
				{
					// Compute the edge weight GradientMag-Vector also available
					bool is_valid; // assume is always valid
					auto gm1 = rescale(iterator.GetPixel(center, is_valid));
					auto gm2 = rescale(iterator.GetPixel(neighbors[i], is_valid));

					auto invgm = 1.0 / (0.01 + gm1 + gm2); // [0.5, 100.0]
					graph->SetArcCap(nodeIndex1, i, invgm);
				}
			}

			// Set source/sink nodes in graph
			if (centerPixel == m_Object1Value)
				graph->SetTerminalArcCap(nodeIndex1, 100000, 0); // source
			else if (centerPixel == m_Object2Value)
				graph->SetTerminalArcCap(nodeIndex1, 0, 100000); // sink
		}
	}
	else
	{
		for (iterator.GoToBegin(); !iterator.IsAtEnd() && !abort; ++iterator)
		{
			progress.CompletedPixel();

			bool pixelIsValidcenter; // BL TODO, slow
			auto centerPixel = iterator.GetPixel(center, pixelIsValidcenter);
			if (!pixelIsValidcenter)
			{
				continue;
			}

			unsigned int nodeIndex1 = ConvertIndexToVertexDescriptor(iterator.GetIndex(center), input_region);

			// Set weights on edges
			for (size_t i = 0; i < num_neighbors; i++)
			{
				bool pixelIsValid; // BL TODO, slow
				auto neighborPixel = iterator.GetPixel(neighbors[i], pixelIsValid);
				if (!pixelIsValid)
				{
					continue;
				}

				if (centerPixel != m_BackgroundValue && neighborPixel != m_BackgroundValue)
				{
					graph->SetArcCap(nodeIndex1, i, 1.0);
				}
			}

			// Set source/sink nodes in graph
			if (centerPixel == m_Object1Value)
				graph->SetTerminalArcCap(nodeIndex1, 100000, 0); // source
			else if (centerPixel == m_Object2Value)
				graph->SetTerminalArcCap(nodeIndex1, 0, 100000); // sink
		}
	}
}

template<typename TInput, typename TOutput, typename TInputIntensityImage>
void GraphCutLabelSeparator<TInput, TOutput, TInputIntensityImage>::CutGraph(GraphType* graph, ProgressReporter& progress)
{
	// Iterate over the output image, querying the graph for the association of each pixel
	auto input = GetMaskInput();
	auto output = this->GetOutput();
	auto outputRegion = output->GetRequestedRegion();
	itk::ImageRegionIterator<InputImageType> input_iterator(input, outputRegion);
	itk::ImageRegionIterator<OutputImageType> output_iterator(output, outputRegion);

	// \todo BL iterate over flat indices to avoid converting ijk to voxelIndex
	const bool& abort = this->GetAbortGenerateData();
	output_iterator.GoToBegin();
	input_iterator.GoToBegin();
	for (; !output_iterator.IsAtEnd() && !abort; ++input_iterator, ++output_iterator)
	{
		if (input_iterator.Get() == m_BackgroundValue)
		{
			output_iterator.Set(m_BackgroundValue);
		}
		else
		{
			unsigned int voxelIndex = ConvertIndexToVertexDescriptor(output_iterator.GetIndex(), outputRegion); // \bug If we update less than LargestPossibleRegion, this is incorrect (I think ?)
			if (graph->NodeOrigin(voxelIndex) == Gc::Flow::Source)
			{
				output_iterator.Set(m_Object1Value);
			}
			else
			{
				output_iterator.Set(m_Object2Value);
			}
		}
		progress.CompletedPixel();
	}
}

template<typename TInput, typename TOutput, typename TInputIntensityImage>
unsigned int GraphCutLabelSeparator<TInput, TOutput, TInputIntensityImage>::ConvertIndexToVertexDescriptor(const itk::Index<3> index, typename TInput::RegionType region)
{
	auto size = region.GetSize();
	return index[0] + index[1] * size[0] + index[2] * size[0] * size[1];
}
} // namespace itk
