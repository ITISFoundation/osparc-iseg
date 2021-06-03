//
//  kalman_filter.h
//  iSegData
//
//  Created by Benjamin Fuhrer on 26.11.18.
//

#pragma once

#include <Eigen/Dense>

#include <iostream>
#include <vector>

class KalmanFilter
{

public:
	int N = 2;
	KalmanFilter();
	~KalmanFilter() = default;
	KalmanFilter(const KalmanFilter& k);

	bool EnsureProbability(double probability);

	void SetMeasurement(std::vector<double>& params);

	void StatePrediction();
	void MeasurementPrediction();

	void MeasurementResidual();

	void StatePredictionCovariance();
	void MeasurementPredictionCovariance();

	void Gain();

	void UpdatedStateEstimate();
	void UpdatedStateCovariance();

	void Work();

	int GetIteration() const;

	void SetIteration(int i);

	std::vector<double> GetMeasurement();
	std::vector<double> GetPrediction() const;

	double DiffBtwPredicatedObject(std::vector<double> object_params);

	double StandardDeviation(const Eigen::VectorXd& v) const;

	std::string GetLabel() const;

	int GetSlice() const;
	void SetSlice(int i);

	void SetLabel(std::string str);

	void SetLastSlice(int i);
	int GetLastSlice() const;

	friend std::ostream& operator<<(std::ostream& os, const KalmanFilter& k_filter)
	{
		os << "***************" << std::endl;
		os << "Kalman Filter: " << k_filter.m_Label << std::endl;
		//os << "State x: " << std::endl;
		//os << k_filter.x << std::endl;
		os << "Measurement Prediction z_hat: " << std::endl;
		os << k_filter.m_ZHat << std::endl;
		os << "Measurement z: " << std::endl;
		os << k_filter.m_Z << std::endl;
		// os << "Covaraince P: " << std::endl;
		//os << k_filter.P << std::endl;
		//os << "Filter Gain W: " << std::endl;
		//os << k_filter.W << std::endl;
		os << "Iteration: " << k_filter.m_Iteration << std::endl;
		os << "First appeared in slice: " << k_filter.m_Slice << std::endl;
		os << "Last slice updated in: " << k_filter.m_LastSlice << std::endl;
		os << "***************" << std::endl;
		return os;
	}

	Eigen::VectorXd GetZ() const { return m_Z; }
	void SetZ(Eigen::VectorXd _z) { m_Z = _z; }
	Eigen::VectorXd GetZHat() const { return m_ZHat; }
	void SetZHat(Eigen::VectorXd _z_hat) { m_ZHat = _z_hat; }
	Eigen::VectorXd GetX() const { return m_X; }
	void SetX(Eigen::VectorXd _x) { m_X = _x; }
	Eigen::VectorXd GetV() const { return m_V; }
	void SetV(Eigen::VectorXd _v) { m_V = _v; }
	Eigen::VectorXd GetN() const { return m_N; }
	void SetN(Eigen::VectorXd _n) { m_N = _n; }
	Eigen::VectorXd GetM() const { return m_M; }
	void SetM(Eigen::VectorXd _m) { m_M = _m; }
	Eigen::MatrixXd GetF() const { return m_F; }
	void SetF(Eigen::MatrixXd _F) { m_F = _F; }
	Eigen::MatrixXd GetH() const { return m_H; }
	void SetH(Eigen::MatrixXd _H) { m_H = _H; }
	Eigen::MatrixXd GetP() const { return m_P; }
	void SetP(Eigen::MatrixXd _P) { m_P = _P; }
	Eigen::MatrixXd GetQ() const { return m_Q; }
	void SetQ(Eigen::MatrixXd _Q) { m_Q = _Q; }
	Eigen::MatrixXd GetR() const { return m_R; }
	void SetR(Eigen::MatrixXd _R) { m_R = _R; }
	Eigen::MatrixXd GetW() const { return m_W; }
	void SetW(Eigen::MatrixXd _W) { m_W = _W; }
	Eigen::MatrixXd GetS() const { return m_S; }
	void SetS(Eigen::MatrixXd _S) { m_S = _S; }

private:
	Eigen::VectorXd m_Z;
	Eigen::VectorXd m_ZHat;
	Eigen::VectorXd m_X;
	Eigen::VectorXd m_V;
	Eigen::VectorXd m_N;
	Eigen::VectorXd m_M;
	Eigen::MatrixXd m_F;
	Eigen::MatrixXd m_H;
	Eigen::MatrixXd m_P;
	Eigen::MatrixXd m_Q;
	Eigen::MatrixXd m_R;
	Eigen::MatrixXd m_W;
	Eigen::MatrixXd m_S;
	std::string m_Label = "";
	int m_Slice = 0;
	int m_Iteration = 0;
	int m_LastSlice = -1;
};
