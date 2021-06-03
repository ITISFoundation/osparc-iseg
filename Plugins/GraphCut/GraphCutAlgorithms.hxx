/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

namespace itk {

template<int NDim>
struct NeighborInfo
{
	static unsigned int numberOfNeighbors(eGcConnectivity type)
	{
		static_assert(NDim == 3, "default implementation assumes DNim==3");
		return (type == eGcConnectivity::kFaceNeighbors) ? 6 : 26;
	}

	template<class OffsetType>
	static void buildNeighbors(std::vector<OffsetType>& neighbors, eGcConnectivity connectivity)
	{
		auto con = numberOfNeighbors(connectivity);

		Gc::Energy::Neighbourhood<NDim, Gc::Int32> nb;
		nb.Common(con, false);
		Gc::System::Algo::Sort::Heap(nb.Begin(), nb.End());

		// Traverses the image adding the following bidirectional edges:
		// 1. currentPixel <-> pixel below it
		// 2. currentPixel <-> pixel to the right of it
		// 3. currentPixel <-> pixel in front of it
		// This prevents duplicate edges (i.e. we cannot add an edge to all 6-connected neighbors of every pixel or
		// almost every edge would be duplicated.
		for (unsigned int i = 0; i < con; ++i)
		{
			neighbors.push_back(OffsetType{nb.m_data[i][0], nb.m_data[i][1], nb.m_data[i][2]});
		}
	}
};

template<>
struct NeighborInfo<2>
{
	static unsigned int numberOfNeighbors(eGcConnectivity type)
	{
		return (type == eGcConnectivity::kFaceNeighbors) ? 4 : 8;
	}

	template<class OffsetType>
	static void buildNeighbors(std::vector<OffsetType>& neighbors, eGcConnectivity connectivity)
	{
		auto con = numberOfNeighbors(connectivity);

		Gc::Energy::Neighbourhood<2, Gc::Int32> nb;
		nb.Common(con, false);
		Gc::System::Algo::Sort::Heap(nb.Begin(), nb.End());

		// Traverses the image adding the following bidirectional edges:
		// 1. currentPixel <-> pixel below it
		// 2. currentPixel <-> pixel to the right of it
		// 3. currentPixel <-> pixel in front of it
		// This prevents duplicate edges (i.e. we cannot add an edge to all 6-connected neighbors of every pixel or
		// almost every edge would be duplicated.
		for (unsigned int i = 0; i < con; ++i)
		{
			neighbors.push_back(OffsetType{nb.m_data[i][0], nb.m_data[i][1]});
		}
	}
};

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
	auto output = this->GetOutput();
	auto output_region = output->GetRequestedRegion();

	// init ITK progress reporter
	// InitializeGraph() traverses the input image once, but only output requested region
	// CutGraph() traverses the output image once
	ProgressReporter progress(this, 0, output_region.GetNumberOfPixels() + output_region.GetNumberOfPixels());

	// allocate output
	output->SetBufferedRegion(output_region); // \todo Is this correct?
	output->Allocate();

	// get the total image size
	auto size = input->GetLargestPossibleRegion().GetSize();
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
	Gc::Energy::Neighbourhood<NDimension, Gc::Int32> nb;
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
	auto output = this->GetOutput();
	auto output_region = output->GetRequestedRegion();
	OffsetType center;
	center.Fill(0);
	itk::Size<NDimension> radius;
	radius.Fill(1);

	// Setup neighborhood (graph connectivity)
	std::vector<OffsetType> neighbors;
	NeighborInfo<NDimension>::buildNeighbors(neighbors, m_Connectivity);
	auto const num_neighbors = neighbors.size();

	MaskIteratorType iterator(radius, input, output_region);
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
		magnitude_filter->GetOutput()->SetRequestedRegion(output_region);
		magnitude_filter->Update();
		auto gradient_magnitude = magnitude_filter->GetOutput();

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

		GradientMagnitudeIteratorType iterator2(radius, gradient_magnitude, output_region);
		for (iterator.GoToBegin(), iterator2.GoToBegin(); !iterator.IsAtEnd() && !abort; ++iterator, ++iterator2)
		{
			progress.CompletedPixel();

			bool pixelIsValidcenter; // BL TODO, slow
			auto centerPixel = iterator.GetPixel(center, pixelIsValidcenter);
			if (!pixelIsValidcenter)
			{
				continue;
			}

			unsigned int nodeIndex1 = input->ComputeOffset(iterator.GetIndex(center));

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

			unsigned int nodeIndex1 = input->ComputeOffset(iterator.GetIndex(center));

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
			// \warning If we update less than LargestPossibleRegion, this may be incorrect
			unsigned int voxelIndex = input->ComputeOffset(input_iterator.GetIndex());
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

} // namespace itk
