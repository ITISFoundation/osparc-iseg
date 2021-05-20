//
//  kalman_filter.cpp
//  iSegData
//
//  Created by Benjamin Fuhrer on 26.11.18.
//

#include "KalmanFilter.h"

#include <cmath>
#include <random>

using Eigen::MatrixXd;
using Eigen::VectorXd;

KalmanFilter::KalmanFilter() : m_Z(VectorXd::Constant(N, 0)), m_ZHat(VectorXd::Constant(N, 0)), m_X(VectorXd::Random(N)), m_V(VectorXd::Random(N)), m_N(VectorXd::Random(N)), m_M(VectorXd::Constant(N, m_X.mean())), m_F(MatrixXd::Identity(N, N)), m_H(MatrixXd::Identity(N, N)), m_P(MatrixXd::Identity(N, N)), m_Q(MatrixXd::Identity(N, N)), m_R(MatrixXd::Identity(N, N)), m_W(MatrixXd::Random(N, N)), m_S(MatrixXd::Random(N, N))
{
	m_P = 10 * m_P;
	std::random_device rd;	//Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<> dis(0, 0.5);
	std::normal_distribution<> norm(0, dis(gen));
	m_Q *= norm(gen);
	m_R *= norm(gen);
	for (unsigned int i(0); i < N; i++)
	{
		m_V(i) = norm(gen);
		m_N(i) = 0;
	}
}

KalmanFilter::KalmanFilter(const KalmanFilter& k)
{
	m_Z = k.m_Z;
	m_ZHat = k.m_ZHat;
	m_X = k.m_X;
	m_V = k.m_V;
	m_N = k.m_N;
	m_M = k.m_M;
	m_F = k.m_F;
	m_H = k.m_H;
	m_P = k.m_P;
	m_W = k.m_W;
	m_S = k.m_S;
	m_Q = k.m_Q;
	m_R = k.m_R;
	m_Slice = k.m_Slice;
	m_Iteration = k.m_Iteration;
	m_Label = k.m_Label;
	m_LastSlice = k.m_LastSlice;
}

void KalmanFilter::SetLastSlice(int i)
{
	m_LastSlice = i;
}
int KalmanFilter::GetLastSlice() const
{
	return m_LastSlice;
}
void KalmanFilter::StatePrediction()
{
	m_X = m_F * m_X + m_V;
}
void KalmanFilter::MeasurementPrediction()
{

	m_ZHat = m_H * m_X;
}

void KalmanFilter::MeasurementResidual()
{
	m_V = m_Z - m_ZHat;
}

void KalmanFilter::SetMeasurement(std::vector<double>& params)
{

	if (params.size() == m_Z.size())
		for (unsigned int i(0); i < params.size(); i++)
			m_Z(i) = params[i];
}

void KalmanFilter::StatePredictionCovariance()
{
	m_P = m_F * m_P * m_F.transpose() + m_Q;
}

void KalmanFilter::MeasurementPredictionCovariance()
{
	m_S = m_H * m_P * m_H.transpose() + m_R;
}
bool KalmanFilter::EnsureProbability(double probability)
{
	if (probability >= 0 && probability <= 1)
		return true;
	else
		return false;
}

void KalmanFilter::Gain()
{
	m_W = m_P * m_H.transpose() * m_S.inverse();
}

void KalmanFilter::UpdatedStateEstimate()
{
	m_X = m_X + m_W * m_V;
}

void KalmanFilter::UpdatedStateCovariance()
{
	m_P = m_P - m_W * m_S * m_W.transpose();
}

void KalmanFilter::Work()
{

	StatePrediction();
	MeasurementPrediction();
	MeasurementResidual();
	StatePredictionCovariance();
	MeasurementPredictionCovariance();
	Gain();
	UpdatedStateEstimate();
	UpdatedStateCovariance();
	m_Iteration += 1;
}

double KalmanFilter::StandardDeviation(const VectorXd& vec) const
{
	double std(0);
	VectorXd abs_vec(N);
	for (unsigned int i(0); i < vec.size(); i++)
		abs_vec(i) = abs(vec(i));
	for (unsigned int i(0); i < vec.size(); i++)
		std += pow(abs_vec(i) - abs_vec.mean(), 2);
	std = std / (N - 1);
	return sqrt(std);
}

std::vector<double> KalmanFilter::GetMeasurement()
{
	std::vector<double> measurement(N);
	for (unsigned int i(0); i < N; i++)
	{
		measurement[i] = m_Z(i);
	}
	return measurement;
}

int KalmanFilter::GetIteration() const
{
	return m_Iteration;
}

void KalmanFilter::SetIteration(int i)
{
	m_Iteration = i;
}

int KalmanFilter::GetSlice() const
{
	return m_Slice;
}

void KalmanFilter::SetSlice(int i)
{
	m_Slice = i;
}

void KalmanFilter::SetLabel(std::string str)
{
	m_Label = str;
}

std::string KalmanFilter::GetLabel() const
{
	return m_Label;
}
double KalmanFilter::DiffBtwPredicatedObject(std::vector<double> object_params)
{
	VectorXd v(N);
	for (unsigned int i(0); i < N; i++)
	{
		v(i) = abs(m_ZHat(i) - object_params[i]);
	}
	return v.sum();
}

std::vector<double> KalmanFilter::GetPrediction() const
{
	std::vector<double> measurement(N);
	for (unsigned int i(0); i < N; i++)
	{
		measurement[i] = m_ZHat(i);
	}
	return measurement;
}
