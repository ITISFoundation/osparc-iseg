/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef __ImageGraphCut3DFilter_hxx_
#define __ImageGraphCut3DFilter_hxx_

#include "itkTimeProbesCollectorBase.h"

#include <fstream>

namespace itk {

template<typename TImage, typename TForeground, typename TBackground, typename TOutput>
ImageGraphCutFilter<TImage, TForeground, TBackground, TOutput>::ImageGraphCutFilter()
	: m_Sigma(0.2), m_ForegroundPixelValue(255), m_BackgroundPixelValue(0), m_PrintTimer(true), m_6Connected(false), m_UseForegroundBackground(false), m_UseGradientMagnitude(false), m_MaxFlowAlgorithm(kKohli), m_UseIntensity(false), m_ForegroundValue(400), m_BackgroundValue(-50)
{
	this->SetNumberOfRequiredInputs(3);
}

template<typename TImage, typename TForeground, typename TBackground, typename TOutput>
ImageGraphCutFilter<TImage, TForeground, TBackground, TOutput>::~ImageGraphCutFilter()
{
}

template<typename TImage, typename TForeground, typename TBackground, typename TOutput>
void ImageGraphCutFilter<TImage, TForeground, TBackground, TOutput>::GenerateData()
{
	itk::TimeProbesCollectorBase timer;

	timer.Start("ITK init");
	// get all images
	ImageContainer images;
	images.input = GetInputImage();
	images.inputRegion = images.input->GetLargestPossibleRegion();
	images.foreground = this->GetInput(1);
	images.background = this->GetInput(2);
	images.output = this->GetOutput();
	images.outputRegion = images.output->GetRequestedRegion();

	// init ITK progress reporter
	// InitializeGraph() traverses the input image once
	int numberOfPixelDuringInit = images.inputRegion.GetNumberOfPixels();
	// CutGraph() traverses the output image once
	int numberOfPixelDuringOutput = images.outputRegion.GetNumberOfPixels();
	// since both report to the same ProgressReporter, we add the total amount of pixels
	ProgressReporter progress(this, 0, numberOfPixelDuringInit + numberOfPixelDuringOutput);

	// allocate output
	images.output->SetBufferedRegion(images.outputRegion);
	images.output->Allocate();

	// init samples and histogram
	typename SampleType::Pointer foregroundSample = SampleType::New();
	typename SampleType::Pointer backgroundSample = SampleType::New();
	typename SampleToHistogramFilterType::Pointer foregroundHistogramFilter = SampleToHistogramFilterType::New();
	typename SampleToHistogramFilterType::Pointer backgroundHistogramFilter = SampleToHistogramFilterType::New();

	// get the total image size
	typename InputImageType::SizeType size = images.inputRegion.GetSize();
	timer.Stop("ITK init");

	// create graph
	timer.Start("Graph creation");
	typename InputImageType::SizeType::SizeValueType d[] = {size[0], size[1], size[2]};
	Gc::Math::Algebra::Vector<3, Gc::Size> sizing;
	sizing[0] = d[0];
	sizing[1] = d[1];
	sizing[2] = d[2];
	Gc::Energy::Neighbourhood<3, Gc::Int32> nb(26);
	nb.Common(26, false);
	if (m_6Connected == true)
	{
		Gc::Energy::Neighbourhood<3, Gc::Int32> nb(6);
		nb.Common(6, false);
	}
	Gc::System::Algo::Sort::Heap(nb.Begin(), nb.End());
	GraphType* graph = nullptr;
	if (m_MaxFlowAlgorithm == kKohli)
	{
		graph = new Gc::Flow::Grid::Kohli<3, Gc::Float32, Gc::Float32, Gc::Float32, false>;
	}
	else if (m_MaxFlowAlgorithm == kPushLabelFifo)
	{
		graph = new Gc::Flow::Grid::PushRelabel::Fifo<3, Gc::Float32, Gc::Float32, false>;
	}
	else if (m_MaxFlowAlgorithm == kPushLabelHighestLevel)
	{
		graph = new Gc::Flow::Grid::PushRelabel::HighestLevel<3, Gc::Float32, Gc::Float32, false>;
	}

	graph->Init(sizing, nb);
	timer.Stop("Graph creation");

	timer.Start("Graph init");
	InitializeGraph(graph, images, progress);
	timer.Stop("Graph init");

	if (this->GetAbortGenerateData())
	{
		return;
	}

	// cut graph
	timer.Start("Graph cut");
	//graph.calculateMaxFlow();
	graph->FindMaxFlow();
	timer.Stop("Graph cut");

	timer.Start("Query results");
	CutGraph(graph, images, progress); //&
	timer.Stop("Query results");

	std::ofstream ofile("C:/Temp/gc_timer.log");
	if (ofile.is_open())
	{
		timer.Report(ofile);
	}

	if (m_PrintTimer)
	{
		timer.Report(std::cout);
	}
}

template<typename TImage, typename TForeground, typename TBackground, typename TOutput>
void ImageGraphCutFilter<TImage, TForeground, TBackground, TOutput>::InitializeGraph(GraphType* graph, ImageContainer images, ProgressReporter& progress)
{
	typename InputImageType::SizeType size = images.inputRegion.GetSize();

	std::vector<double> pixels(size[0] * size[1] * size[2]);

	//compute S field
	itk::Size<3> radius;
	radius.Fill(3);

	typedef itk::ShapedNeighborhoodIterator<InputImageType> IteratorType;
	typename IteratorType::OffsetType center = {{0, 0, 0}};

	//Gaussian Filter for Edge enhancement
	typedef itk::DiscreteGaussianImageFilter<InputImageType, InputImageType> filterType;
	auto gaussianFilter = filterType::New();
	gaussianFilter->SetInput(images.input);
	gaussianFilter->SetVariance(1);
	auto filtered = gaussianFilter->GetOutput();

	Gc::Energy::Neighbourhood<3, Gc::Int32> nb(26);
	nb.Common(26, false);
	if (m_6Connected == true)
	{
		Gc::Energy::Neighbourhood<3, Gc::Int32> nb(6);
		nb.Common(6, false);
	}
	Gc::System::Algo::Sort::Heap(nb.Begin(), nb.End());

	//typedef itk::ShapedNeighborhoodIterator<InputImageType> IteratorType;

	// Traverses the image adding the following bidirectional edges:
	// 1. currentPixel <-> pixel below it
	// 2. currentPixel <-> pixel to the right of it
	// 3. currentPixel <-> pixel in front of it
	// This prevents duplicate edges (i.e. we cannot add an edge to all 6-connected neighbors of every pixel or
	// almost every edge would be duplicated.
	std::vector<typename IteratorType::OffsetType> neighbors;
	typename IteratorType::OffsetType frontbottomrightr = {{nb.m_data[0][0], nb.m_data[0][1], nb.m_data[0][2]}};
	neighbors.push_back(frontbottomrightr);
	typename IteratorType::OffsetType graph4r = {{nb.m_data[1][0], nb.m_data[1][1], nb.m_data[1][2]}};
	neighbors.push_back(graph4r);
	typename IteratorType::OffsetType bottom = {{nb.m_data[2][0], nb.m_data[2][1], nb.m_data[2][2]}};
	neighbors.push_back(bottom);
	typename IteratorType::OffsetType right = {{nb.m_data[3][0], nb.m_data[3][1], nb.m_data[3][2]}};
	neighbors.push_back(right);
	typename IteratorType::OffsetType front = {{nb.m_data[4][0], nb.m_data[4][1], nb.m_data[4][2]}};
	neighbors.push_back(front);
	typename IteratorType::OffsetType frontright = {{nb.m_data[5][0], nb.m_data[5][1], nb.m_data[5][2]}};
	neighbors.push_back(frontright);

	IteratorType iterator(radius, images.input, images.input->GetLargestPossibleRegion());
	IteratorType iterator2(radius, filtered, filtered->GetLargestPossibleRegion());
	iterator.ClearActiveList();
	iterator.ActivateOffset(frontbottomrightr);
	iterator.ActivateOffset(graph4r);
	iterator.ActivateOffset(bottom);
	iterator.ActivateOffset(right);
	iterator.ActivateOffset(front);
	iterator.ActivateOffset(frontright);
	if (m_6Connected == false)
	{
		typename IteratorType::OffsetType frontbottom = {{nb.m_data[6][0], nb.m_data[6][1], nb.m_data[6][2]}};
		neighbors.push_back(frontbottom);
		typename IteratorType::OffsetType bottomright = {{nb.m_data[7][0], nb.m_data[7][1], nb.m_data[7][2]}};
		neighbors.push_back(bottomright);
		typename IteratorType::OffsetType frontbottomright = {{nb.m_data[8][0], nb.m_data[8][1], nb.m_data[8][2]}};
		neighbors.push_back(frontbottomright);
		typename IteratorType::OffsetType graph1 = {{nb.m_data[9][0], nb.m_data[9][1], nb.m_data[9][2]}};
		neighbors.push_back(graph1);
		typename IteratorType::OffsetType graph2 = {{nb.m_data[10][0], nb.m_data[10][1], nb.m_data[10][2]}};
		neighbors.push_back(graph2);
		typename IteratorType::OffsetType graph3 = {{nb.m_data[11][0], nb.m_data[11][1], nb.m_data[11][2]}};
		neighbors.push_back(graph3);
		typename IteratorType::OffsetType graph4 = {{nb.m_data[12][0], nb.m_data[0][1], nb.m_data[12][2]}};
		neighbors.push_back(graph4);
		typename IteratorType::OffsetType graph5 = {{nb.m_data[13][0], nb.m_data[13][1], nb.m_data[13][2]}};
		neighbors.push_back(graph5);
		typename IteratorType::OffsetType graph6 = {{nb.m_data[14][0], nb.m_data[14][1], nb.m_data[14][2]}};
		neighbors.push_back(graph6);
		typename IteratorType::OffsetType bottomr = {{nb.m_data[15][0], nb.m_data[15][1], nb.m_data[15][2]}};
		neighbors.push_back(bottomr);
		typename IteratorType::OffsetType rightr = {{nb.m_data[16][0], nb.m_data[16][1], nb.m_data[16][2]}};
		neighbors.push_back(rightr);
		typename IteratorType::OffsetType frontr = {{nb.m_data[17][0], nb.m_data[17][1], nb.m_data[17][2]}};
		neighbors.push_back(frontr);
		typename IteratorType::OffsetType frontrightr = {{nb.m_data[18][0], nb.m_data[18][1], nb.m_data[18][2]}};
		neighbors.push_back(frontrightr);
		typename IteratorType::OffsetType frontbottomr = {{nb.m_data[19][0], nb.m_data[19][1], nb.m_data[19][2]}};
		neighbors.push_back(frontbottomr);
		typename IteratorType::OffsetType bottomrightr = {{nb.m_data[20][0], nb.m_data[20][1], nb.m_data[20][2]}};
		neighbors.push_back(bottomrightr);
		typename IteratorType::OffsetType graph1r = {{nb.m_data[21][0], nb.m_data[21][1], nb.m_data[21][2]}};
		neighbors.push_back(graph1r);
		typename IteratorType::OffsetType graph2r = {{nb.m_data[22][0], nb.m_data[22][1], nb.m_data[22][2]}};
		neighbors.push_back(graph2r);
		typename IteratorType::OffsetType graph3r = {{nb.m_data[23][0], nb.m_data[23][1], nb.m_data[23][2]}};
		neighbors.push_back(graph3r);
		typename IteratorType::OffsetType graph5r = {{nb.m_data[24][0], nb.m_data[24][1], nb.m_data[24][2]}};
		neighbors.push_back(graph5r);
		typename IteratorType::OffsetType graph6r = {{nb.m_data[25][0], nb.m_data[25][1], nb.m_data[25][2]}};
		neighbors.push_back(graph6r);
		iterator.ActivateOffset(frontbottom);
		iterator.ActivateOffset(frontbottomright);
		iterator.ActivateOffset(bottomright);
		iterator.ActivateOffset(graph1);
		iterator.ActivateOffset(graph2);
		iterator.ActivateOffset(graph3);
		iterator.ActivateOffset(graph4);
		iterator.ActivateOffset(graph5);
		iterator.ActivateOffset(graph6);
		iterator.ActivateOffset(bottomr);
		iterator.ActivateOffset(rightr);
		iterator.ActivateOffset(frontr);
		iterator.ActivateOffset(frontrightr);
		iterator.ActivateOffset(frontbottomr);
		iterator.ActivateOffset(bottomrightr);
		iterator.ActivateOffset(graph1r);
		iterator.ActivateOffset(graph2r);
		iterator.ActivateOffset(graph3r);
		iterator.ActivateOffset(graph5r);
		iterator.ActivateOffset(graph6r);
	}
	iterator.ActivateOffset(center);
	//iterator.ActivateOffset(left);
	//iterator.ActivateOffset(up);
	//iterator.ActivateOffset(behind);
	iterator2.ClearActiveList();
	iterator2.ActivateOffset(center);

	// BL TODO, not needed for all "modes"
	size_t N = size[0] * size[1] * size[2];
	std::vector<double> lambda1(N);
	std::vector<double> lambda2(N);
	std::vector<double> lambda3(N);
	std::vector<double> S(N, 0.0);
	std::vector<double> GradientMag(N, 0.0);

	if (m_UseIntensity == false)
	{
		iterator2.GoToBegin();
		for (iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator)
		{
			bool pixelIsValid;
			auto centerPixel = iterator.GetPixel(center, pixelIsValid);
			if (!pixelIsValid)
			{
				continue;
			}
			auto centerPixelfilter = iterator2.GetPixel(center);
			iterator.SetCenterPixel(centerPixel + 10 * (centerPixel - centerPixelfilter));
			++iterator2;
		}

		std::cout << "gaussiancomplete";

		//Compute the components of the Hessian Ixx,Iyy,Izz,Ixz,Iyz,Ixy+GM
		double sigmas[3];
		sigmas[0] = 1.0;
		sigmas[1] = 0.75;
		sigmas[2] = 1.25; // BL TODO?

		for (int i = 0; i < 2; i++)
		{
			double sigma = sigmas[i];
			typedef itk::ImageDuplicator<InputImageType> DuplicatorType;
			typedef itk::RecursiveGaussianImageFilter<InputImageType, InputImageType> FilterType;

			auto duplicator = DuplicatorType::New();
			auto duplicator2 = DuplicatorType::New();
			auto duplicator3 = DuplicatorType::New();
			auto duplicator4 = DuplicatorType::New();
			auto duplicator5 = DuplicatorType::New();
			auto duplicator6 = DuplicatorType::New();

			auto ga = FilterType::New();
			auto gb = FilterType::New();
			auto gc = FilterType::New();
			ga->SetDirection(0);
			gb->SetDirection(1);
			gc->SetDirection(2);
			ga->SetSigma(sigma);
			gb->SetSigma(sigma);
			gc->SetSigma(sigma);

			ga->SetZeroOrder();
			gb->SetZeroOrder();
			gc->SetSecondOrder();
			ga->SetInput(images.input);
			gb->SetInput(ga->GetOutput());
			gc->SetInput(gb->GetOutput());
			duplicator->SetInputImage(gc->GetOutput());
			gc->Update();
			duplicator->Update();
			auto Izz = duplicator->GetOutput();

			gc->SetDirection(1);
			gb->SetDirection(2);
			gc->Update();
			duplicator2->SetInputImage(gc->GetOutput());
			duplicator2->Update();
			auto Iyy = duplicator2->GetOutput();

			gc->SetDirection(0);
			ga->SetDirection(1);
			gc->Update();
			duplicator3->SetInputImage(gc->GetOutput());
			duplicator3->Update();
			auto Ixx = duplicator3->GetOutput();

			ga->SetDirection(0);
			gb->SetDirection(1);
			gc->SetDirection(2);
			ga->SetZeroOrder();
			gb->SetFirstOrder();
			gc->SetFirstOrder();
			gc->Update();
			duplicator4->SetInputImage(gc->GetOutput());
			duplicator4->Update();
			auto Iyz = duplicator4->GetOutput();

			ga->SetDirection(1);
			gb->SetDirection(0);
			gc->SetDirection(2);
			ga->SetZeroOrder();
			gb->SetFirstOrder();
			gc->SetFirstOrder();
			gc->Update();
			duplicator5->SetInputImage(gc->GetOutput());
			duplicator5->Update();
			auto Ixz = duplicator5->GetOutput();

			ga->SetDirection(2);
			gb->SetDirection(0);
			gc->SetDirection(1);
			ga->SetZeroOrder();
			gb->SetFirstOrder();
			gc->SetFirstOrder();
			gc->Update();
			duplicator6->SetInputImage(gc->GetOutput());
			duplicator6->Update();
			auto Ixy = duplicator6->GetOutput();

			typedef itk::GradientMagnitudeImageFilter<InputImageType, InputImageType> GradFilterType;
			auto gradfilter = GradFilterType::New();
			gradfilter->SetInput(images.input);
			auto Igrad = gradfilter->GetOutput();

			std::cout << "Hessiancomplete";
			//Now we have all the components and need to compute the eigenvalues somehow. Store them in lambda1-3. Also compute GM-Vector
			{
				typedef itk::Matrix<double, 3, 3> MatrixType;
				typedef itk::Vector<double, 3> VectorType;
				typedef itk::SymmetricEigenAnalysis<MatrixType, VectorType, MatrixType> Eigenanalyse;

				IteratorType iterator3(radius, Ixx, Ixx->GetLargestPossibleRegion());
				IteratorType iterator4(radius, Iyy, Iyy->GetLargestPossibleRegion());
				IteratorType iterator5(radius, Izz, Izz->GetLargestPossibleRegion());
				IteratorType iterator6(radius, Ixy, Ixy->GetLargestPossibleRegion());
				IteratorType iterator7(radius, Ixz, Ixz->GetLargestPossibleRegion());
				IteratorType iterator8(radius, Iyz, Iyz->GetLargestPossibleRegion());
				IteratorType iterator9(radius, Igrad, Igrad->GetLargestPossibleRegion());
				iterator3.ClearActiveList();
				iterator3.ActivateOffset(center);
				iterator4.ClearActiveList();
				iterator4.ActivateOffset(center);
				iterator5.ClearActiveList();
				iterator5.ActivateOffset(center);
				iterator6.ClearActiveList();
				iterator6.ActivateOffset(center);
				iterator7.ClearActiveList();
				iterator7.ActivateOffset(center);
				iterator8.ClearActiveList();
				iterator8.ActivateOffset(center);
				iterator9.ClearActiveList();
				iterator9.ActivateOffset(center);

				iterator4.GoToBegin();
				iterator5.GoToBegin();
				iterator6.GoToBegin();
				iterator7.GoToBegin();
				iterator8.GoToBegin();
				iterator9.GoToBegin();
				for (iterator3.GoToBegin(); !iterator3.IsAtEnd(); ++iterator3)
				{
					bool pixelIsValid;
					auto Ixx_value = iterator3.GetPixel(center, pixelIsValid);
					if (!pixelIsValid)
					{
						continue;
					}
					auto Iyy_value = iterator4.GetPixel(center);
					auto Izz_value = iterator5.GetPixel(center);
					auto Ixy_value = iterator6.GetPixel(center);
					auto Ixz_value = iterator7.GetPixel(center);
					auto Iyz_value = iterator8.GetPixel(center);

					MatrixType Hessian;
					Hessian(0, 0) = Ixx_value;
					Hessian(1, 1) = Iyy_value;
					Hessian(2, 2) = Izz_value;
					Hessian(0, 1) = Ixy_value;
					Hessian(1, 0) = Ixy_value;
					Hessian(2, 0) = Ixz_value;
					Hessian(0, 2) = Ixz_value;
					Hessian(2, 1) = Iyz_value;
					Hessian(1, 2) = Iyz_value;

					VectorType eigenvalues;

					Eigenanalyse eig;
					eig.SetDimension(3);
					eig.SetOrderEigenMagnitudes(true);
					eig.ComputeEigenValues(Hessian, eigenvalues);

					auto nodeIndex = ConvertIndexToVertexDescriptor(iterator3.GetIndex(center), Ixx->GetLargestPossibleRegion());
					lambda1[nodeIndex] = eigenvalues[0];
					lambda2[nodeIndex] = eigenvalues[1];
					lambda3[nodeIndex] = eigenvalues[2];
					++iterator4;
					++iterator5;
					++iterator6;
					++iterator7;
					++iterator8;
					auto Igrad_value = static_cast<double>(iterator9.GetPixel(center));
					if (abs(Igrad_value) > abs((double)GradientMag[nodeIndex]))
					{
						GradientMag[nodeIndex] = Igrad_value;
					}
					++iterator9;
				}
			}

			std::cout << "eigenvaluecomplete";

			double T = 0;
			for (unsigned int i = 0; i < lambda1.size(); i++)
			{
				T += (lambda1[i] + lambda2[i] + lambda3[i]) / lambda1.size();
			}

			for (unsigned int i = 0; i < lambda1.size(); i++)
			{
				double Rtube = 0.0;
				double Rsheet = 0.0;
				double Rnoise = 0.0;
				double sign = 0.0;
				Rsheet = abs(lambda2[i]) / abs(lambda3[i]);
				Rtube = abs(lambda1[i]) / (abs(lambda3[i]) * abs(lambda2[i]));
				Rnoise = (abs(lambda1[i]) + abs(lambda2[i]) + abs(lambda3[i])) / T;
				sign = (lambda3[i]) / (abs(lambda3[i]));
				double tmp = -sign * exp(-(Rsheet * Rsheet) / 0.25) * exp(-(Rtube * Rtube) / 0.25) * (1 - exp(-(Rnoise * Rnoise) / 0.0625));
				if (abs(tmp) > S[i])
				{
					S[i] = tmp;
				}
			}
		}

		//Set input back to initial state:
		iterator2.GoToBegin();
		for (iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator)
		{
			bool pixelIsValid;
			typename InputImageType::PixelType centerPixel = iterator.GetPixel(center, pixelIsValid);
			if (!pixelIsValid)
			{
				continue;
			}
			typename InputImageType::PixelType centerPixelfilter = iterator2.GetPixel(center);
			iterator.SetCenterPixel((centerPixel + 10 * centerPixelfilter) / 11);
			++iterator2;
		}
	}
	std::cout << "readyforboundary";
	itk::Vector<double, 3> space;
	space = images.input->GetSpacing();

	unsigned int const con = m_6Connected ? 6 : 26;

	const bool& abort = this->GetAbortGenerateData(); // use reference (alias) to original flag!
	for (iterator.GoToBegin(); !iterator.IsAtEnd() && !abort; ++iterator)
	{
		progress.CompletedPixel();

		bool pixelIsValidcenter;
		typename InputImageType::PixelType centerPixel = iterator.GetPixel(center, pixelIsValidcenter);
		if (!pixelIsValidcenter)
		{
			continue;
		}
		unsigned int nodeIndex1 = ConvertIndexToVertexDescriptor(iterator.GetIndex(center), images.inputRegion);

		for (unsigned int i = 0; i < con; i++)
		{
			bool pixelIsValid;
			// BL TODO
			typename InputImageType::PixelType neighborPixel = iterator.GetPixel(neighbors[i], pixelIsValid);

			// If the current neighbor is outside the image, skip it
			if (!pixelIsValid)
			{
				continue;
			}

			// BL TODO
			unsigned int nodeIndex2 = ConvertIndexToVertexDescriptor(iterator.GetIndex(neighbors[i]), images.inputRegion);

			// Compute the edge weight GradientMag-Vector also available
			double weight = 0;
			if (m_UseGradientMagnitude == true) // Divide
			{
				// BL assumes background is ZERO
				if (centerPixel != 0 && neighborPixel != 0)
				{
					double invgm = 1.0 / (abs(GradientMag[nodeIndex1]) + abs(GradientMag[nodeIndex2]));
					graph->SetArcCap(nodeIndex1, i, 1 + 25 * invgm); //invgm,invgm
				}
			}
			else // gm == false
			{
				if (m_UseIntensity == true) //Intensity
				{
					typename IteratorType::OffsetType offset = iterator.GetOffset(i);
					double d = 0.0;
					d = offset[0] * offset[0] * space[0] + offset[1] * offset[1] * space[1] + offset[2] * offset[2] * space[2];
					weight = (1 / (std::sqrt(d))) * exp(-pow(centerPixel - neighborPixel, 2) / (2.0 * 50.0 * 50.0)); //50 =variance from paper
					if (centerPixel < neighborPixel)
						graph->SetArcCap(nodeIndex1, i, 1.0);
					else
						graph->SetArcCap(nodeIndex1, i, weight);
				}
				else //Sheetness
				{
					weight = exp(-abs(S[nodeIndex1] - S[nodeIndex2]) / (m_Sigma));
					if (S[nodeIndex1] >= S[nodeIndex2]) //>!!
					{
						graph->SetArcCap(nodeIndex1, i, 5 * weight); //
					}
					else
					{
						graph->SetArcCap(nodeIndex1, i, 5 * 1.0);
					}
				}
			}
		}

		if (m_UseIntensity == false && m_UseGradientMagnitude == false) //Terminal Edges for sheetness
		{
			if (S[nodeIndex1] > 0.0 && centerPixel > m_ForegroundValue)
				graph->SetTerminalArcCap(nodeIndex1, 1.0, 0);
			if (centerPixel < m_BackgroundValue)
				graph->SetTerminalArcCap(nodeIndex1, 0, 1.0);
		}
		else if (m_UseIntensity == true && m_UseGradientMagnitude == false) //Terminal Edges for intensity
		{
			if (centerPixel > m_ForegroundValue)
				graph->SetTerminalArcCap(nodeIndex1, 1000, 0);
			if (centerPixel < m_BackgroundValue)
				graph->SetTerminalArcCap(nodeIndex1, 0, 1000);
		}
	}

	if (abort)
	{
		return;
	}

	if (m_UseForegroundBackground == true) //Foreground&Background_used
	{
		itk::Size<3> rad;
		rad.Fill(1);
		typedef itk::ShapedNeighborhoodIterator<ForegroundImageType> IteratorTypefb;
		typename IteratorTypefb::OffsetType centerfb = {{0, 0, 0}};

		IteratorTypefb iteratorfb(rad, images.foreground, images.foreground->GetLargestPossibleRegion());
		IteratorTypefb iteratorbg(rad, images.background, images.background->GetLargestPossibleRegion());
		iteratorfb.ClearActiveList();
		iteratorfb.ActivateOffset(centerfb);
		iteratorbg.ClearActiveList();
		iteratorbg.ActivateOffset(centerfb);

		iteratorbg.GoToBegin();
		for (iteratorfb.GoToBegin(); !iteratorfb.IsAtEnd(); ++iteratorfb)
		{
			typename ForegroundImageType::PixelType centerPixel = iteratorfb.GetPixel(centerfb);
			typename ForegroundImageType::PixelType centerPixelbg = iteratorbg.GetPixel(centerfb);
			if ((int)centerPixelbg < -4000)
			{
				unsigned int sinkIndex = ConvertIndexToVertexDescriptor(iteratorbg.GetIndex(center), images.inputRegion);
				graph->SetTerminalArcCap(sinkIndex, 0, 100000);
			}
			if ((int)centerPixel > 4000)
			{
				unsigned int sourceIndex = ConvertIndexToVertexDescriptor(iteratorbg.GetIndex(center), images.inputRegion);
				graph->SetTerminalArcCap(sourceIndex, 100000, 0);
			}
			++iteratorbg;
		}
	}

	if (m_UseGradientMagnitude == true) //Divide
	{
		itk::Size<3> rad;
		rad.Fill(1);
		typedef itk::ShapedNeighborhoodIterator<ForegroundImageType> IteratorTypefb;
		typename IteratorTypefb::OffsetType centerfb = {{0, 0, 0}};

		IteratorTypefb iteratorfb(rad, images.foreground, images.foreground->GetLargestPossibleRegion());
		IteratorTypefb iteratorbg(rad, images.background, images.background->GetLargestPossibleRegion());
		iteratorfb.ClearActiveList();
		iteratorfb.ActivateOffset(centerfb);
		iteratorbg.ClearActiveList();
		iteratorbg.ActivateOffset(centerfb);

		iteratorbg.GoToBegin();
		for (iteratorfb.GoToBegin(); !iteratorfb.IsAtEnd(); ++iteratorfb)
		{
			auto centerPixel = iteratorfb.GetPixel(centerfb);
			auto centerPixelbg = iteratorbg.GetPixel(centerfb);
			if ((int)centerPixelbg < -4000)
			{
				unsigned int sinkIndex = ConvertIndexToVertexDescriptor(iteratorbg.GetIndex(center), images.inputRegion);
				graph->SetTerminalArcCap(sinkIndex, 0, 20.0);
			}
			if ((int)centerPixel > 4000)
			{
				unsigned int sourceIndex = ConvertIndexToVertexDescriptor(iteratorbg.GetIndex(center), images.inputRegion);
				graph->SetTerminalArcCap(sourceIndex, 20.0, 0);
			}
			++iteratorbg;
		}
	}
}

template<typename TImage, typename TForeground, typename TBackground, typename TOutput>
void ImageGraphCutFilter<TImage, TForeground, TBackground, TOutput>::CutGraph(GraphType* graph, ImageContainer images, ProgressReporter& progress)
{
	// Iterate over the output image, querying the graph for the association of each pixel
	itk::ImageRegionIterator<OutputImageType> outputImageIterator(images.output, images.outputRegion);
	outputImageIterator.GoToBegin();

	//int sourceGroup = graph->groupOfSource();
	const bool& abort = this->GetAbortGenerateData();
	while (!outputImageIterator.IsAtEnd() && !abort)
	{
		unsigned int voxelIndex = ConvertIndexToVertexDescriptor(outputImageIterator.GetIndex(), images.outputRegion);
		if (graph->NodeOrigin(voxelIndex) == Gc::Flow::Source) //->groupOf(voxelIndex) == sourceGroup
		{
			outputImageIterator.Set(m_ForegroundPixelValue);
		}
		// Libraries differ to some degree in how they define the terminal groups. however, the tested ones
		// (kolmogorvs MAXFLOW, boost graph, IBFS) use a fixed value for the source group and define other
		// values as background.
		else
		{
			outputImageIterator.Set(m_BackgroundPixelValue);
		}
		++outputImageIterator;
		progress.CompletedPixel();
	}
}

template<typename TImage, typename TForeground, typename TBackground, typename TOutput>
unsigned int ImageGraphCutFilter<TImage, TForeground, TBackground, TOutput>::ConvertIndexToVertexDescriptor(const itk::Index<3> index, typename TImage::RegionType region)
{
	typename TImage::SizeType size = region.GetSize();

	return index[0] + index[1] * size[0] + index[2] * size[0] * size[1];
}
} // namespace itk

#endif // __ImageGraphCut3DFilter_hxx_
