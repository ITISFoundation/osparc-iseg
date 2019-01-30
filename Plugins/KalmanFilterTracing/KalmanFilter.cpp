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

KalmanFilter::KalmanFilter() : z(VectorXd::Constant(N, 0)), z_hat(VectorXd::Constant(N, 0)), x(VectorXd::Random(N)), v(VectorXd::Random(N)), n(VectorXd::Random(N)), m(VectorXd::Constant(N, x.mean())), F(MatrixXd::Identity(N, N)), H(MatrixXd::Identity(N, N)), P(MatrixXd::Identity(N, N)), Q(MatrixXd::Identity(N, N)), R(MatrixXd::Identity(N, N)), W(MatrixXd::Random(N, N)), S(MatrixXd::Random(N, N)), slice(0), iteration(0), last_slice(-1)
{
	P = 10 * P;
	std::random_device rd;	//Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<> dis(0, 0.5);
	std::normal_distribution<> norm(0, dis(gen));
	Q *= norm(gen);
	R *= norm(gen);
	for (unsigned int i(0); i < N; i++)
	{
		v(i) = norm(gen);
		n(i) = 0;
	}
}

KalmanFilter::KalmanFilter(const KalmanFilter& k)
{
	z = k.z;
	z_hat = k.z_hat;
	x = k.x;
	v = k.v;
	n = k.n;
	m = k.m;
	F = k.F;
	H = k.H;
	P = k.P;
	W = k.W;
	S = k.S;
	Q = k.Q;
	R = k.R;
	slice = k.slice;
	iteration = k.iteration;
	label = k.label;
	last_slice = k.last_slice;
}

void KalmanFilter::set_last_slice(int i)
{
	last_slice = i;
}
int KalmanFilter::get_last_slice() const
{
	return last_slice;
}
void KalmanFilter::state_prediction()
{
	x = F * x + v;
}
void KalmanFilter::measurement_prediction()
{

	z_hat = H * x;
}

void KalmanFilter::measurement_residual()
{
	v = z - z_hat;
}

void KalmanFilter::set_measurement(std::vector<double>& params)
{

	if (params.size() == z.size())
		for (unsigned int i(0); i < params.size(); i++)
			z(i) = params[i];
}

void KalmanFilter::state_prediction_covariance()
{
	P = F * P * F.transpose() + Q;
}

void KalmanFilter::measurement_prediction_covariance()
{
	S = H * P * H.transpose() + R;
}
bool KalmanFilter::ensure_probability(double probability)
{
	if (probability >= 0 && probability <= 1)
		return true;
	else
		return false;
}

void KalmanFilter::gain()
{
	W = P * H.transpose() * S.inverse();
}

void KalmanFilter::updated_state_estimate()
{
	x = x + W * v;
}

void KalmanFilter::updated_state_covariance()
{
	P = P - W * S * W.transpose();
}

void KalmanFilter::work()
{

	state_prediction();
	measurement_prediction();
	measurement_residual();
	state_prediction_covariance();
	measurement_prediction_covariance();
	gain();
	updated_state_estimate();
	updated_state_covariance();
	iteration += 1;
}

double KalmanFilter::standard_deviation(const VectorXd& vec)
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

std::vector<double> KalmanFilter::get_measurement()
{
	std::vector<double> measurement(N);
	for (unsigned int i(0); i < N; i++)
	{
		measurement[i] = z(i);
	}
	return measurement;
}

int KalmanFilter::get_iteration() const
{
	return iteration;
}

void KalmanFilter::set_iteration(int i)
{
	iteration = i;
}

int KalmanFilter::get_slice() const
{
	return slice;
}

void KalmanFilter::set_slice(int i)
{
	slice = i;
}

void KalmanFilter::set_label(std::string str)
{
	label = str;
}

std::string KalmanFilter::get_label() const
{
	return label;
}
double KalmanFilter::diff_btw_predicated_object(std::vector<double> object_params)
{
	VectorXd _v(N);
	for (unsigned int i(0); i < N; i++)
	{
		_v(i) = abs(z_hat(i) - object_params[i]);
	}
	return _v.sum();
}

std::vector<double> KalmanFilter::get_prediction() const
{
	std::vector<double> measurement(N);
	for (unsigned int i(0); i < N; i++)
	{
		measurement[i] = z_hat(i);
	}
	return measurement;
}
