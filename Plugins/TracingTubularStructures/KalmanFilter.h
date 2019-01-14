//
//  kalman_filter.h
//  iSegData
//
//  Created by Benjamin Fuhrer on 26.11.18.
//

#pragma once

#include <vector>
#include <iostream>
#include <typeinfo>
#include <array>
#include<math.h>
#include <Eigen/Dense>


class KalmanFilter
{
    
    public:
    int N = 2;
    KalmanFilter();
    ~KalmanFilter(){}
    KalmanFilter(const KalmanFilter &k);
    
    bool ensure_probability(double probability);
    bool ensure_probabilities(std::vector<double> probabilities);
    
    void set_measurement(std::vector<double>& params);
    
    void state_prediction();
    void measurement_prediction();
    
    void measurement_residual();
    
    void state_prediction_covariance();
    void measurement_prediction_covariance();
    
    void gain();
    
    void updated_state_estimate();
    void updated_state_covariance();
    
    void work();
    
    int get_iteration() const;
    
    void set_iteration(int i);
    
    std::vector<double> get_measurement();
    
    double diff_btw_predicated_object(std::vector<double> object_params);
    
    double standard_deviation(const Eigen::VectorXd &v);
    
    std::string get_label() const;
    
    int get_slice() const;
    void set_slice (int i);
    
    void set_label(std::string str);
    
    void set_last_slice(int i);
    int get_last_slice() const;
    
    friend std::ostream& operator<<(std::ostream& os, const KalmanFilter& k_filter)
    {
        os << "***************" << std::endl;
        os <<  "Kalman Filter: " << k_filter.label << std::endl;
        //os << "State x: " << std::endl;
        //os << k_filter.x << std::endl;
        os << "Measurement Prediction z_hat: " << std::endl;
        os << k_filter.z_hat << std::endl;
        os << "Measurement z: " << std::endl;
        os << k_filter.z << std::endl;
       // os << "Covaraince P: " << std::endl;
        //os << k_filter.P << std::endl;
        //os << "Filter Gain W: " << std::endl;
        //os << k_filter.W << std::endl;
        os << "Iteration: " << k_filter.iteration << std::endl;
        os << "First appeared in slice: " << k_filter.slice << std::endl;
        os << "Last slice updated in: " << k_filter.last_slice << std::endl;
        os << "***************" << std::endl;
        return os;
    }
    
    Eigen::VectorXd get_z() const {return z;}
    void set_z(Eigen::VectorXd _z) { z = _z;}
    Eigen::VectorXd get_z_hat() const {return z_hat;}
    void set_z_hat(Eigen::VectorXd _z_hat) { z_hat = _z_hat;}
    Eigen::VectorXd get_x() const {return x;}
    void set_x(Eigen::VectorXd _x) { x = _x;}
    Eigen::VectorXd get_v() const {return v;}
    void set_v(Eigen::VectorXd _v) { v = _v;}
    Eigen::VectorXd get_n() const {return n;}
    void set_n(Eigen::VectorXd _n) { n = _n;}
    Eigen::VectorXd get_m() const {return m;}
    void set_m(Eigen::VectorXd _m) { m = _m;}
    Eigen::MatrixXd get_F() const {return F;}
    void set_F(Eigen::MatrixXd _F) { F = _F;}
    Eigen::MatrixXd get_H() const {return H;}
    void set_H(Eigen::MatrixXd _H) { H = _H;}
    Eigen::MatrixXd get_P() const {return P;}
    void set_P(Eigen::MatrixXd _P) { P = _P;}
    Eigen::MatrixXd get_Q() const {return Q;}
    void set_Q(Eigen::MatrixXd _Q) { Q = _Q;}
    Eigen::MatrixXd get_R() const {return R;}
    void set_R(Eigen::MatrixXd _R) { R = _R;}
    Eigen::MatrixXd get_W() const {return W;}
    void set_W(Eigen::MatrixXd _W) { W = _W;}
    Eigen::MatrixXd get_S() const {return S;}
    void set_S(Eigen::MatrixXd _S) { S = _S;}
    
private:
    Eigen::VectorXd z;
    Eigen::VectorXd z_hat;
    Eigen::VectorXd x;
    Eigen::VectorXd v;
    Eigen::VectorXd n;
    Eigen::VectorXd m;
    Eigen::MatrixXd F;
    Eigen::MatrixXd H;
    Eigen::MatrixXd P;
    Eigen::MatrixXd Q;
    Eigen::MatrixXd R;
    Eigen::MatrixXd W;
    Eigen::MatrixXd S;
    std::string label = "";
    int slice;
    int iteration;
    int last_slice;
    
    
};


