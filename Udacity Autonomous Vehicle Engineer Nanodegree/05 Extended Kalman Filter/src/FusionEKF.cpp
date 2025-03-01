// Code based on Udacity course

#include "FusionEKF.h"
#include <iostream>
#include "Eigen/Dense"
#include "tools.h"

using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::cout;
using std::endl;
using std::vector;

/**
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
              0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
              0, 0.0009, 0,
              0, 0, 0.09;

  /**
   * TODO: Finish initializing the FusionEKF.
   * TODO: Set the process and measurement noises
   */
	H_laser_ << 1, 0, 0, 0,
  			0, 1, 0, 0;

}

/**
 * Destructor.
 */
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {
  /**
   * Initialization
   */
  if (!is_initialized_) {
    /**
     * TODO: Initialize the state ekf_.x_ with the first measurement.
     * TODO: Create the covariance matrix.
     * You'll need to convert radar from polar to cartesian coordinates.
     */
	  float first_measurement_x = measurement_pack.raw_measurements_[0];
    float first_measurement_y = measurement_pack.raw_measurements_[1];
    
    // first measurement
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    ekf_.x_ << 1, 1, 1, 1;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      // TODO: Convert radar from polar to cartesian coordinates 
      //         and initialize state.
		ekf_.x_[0] = first_measurement_x*cos(first_measurement_y);
		ekf_.x_[1] = first_measurement_x*sin(first_measurement_y);
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      // TODO: Initialize state.
		ekf_.x_[0] = first_measurement_x;
		ekf_.x_[1] = first_measurement_y;
    }
    ekf_.P_ = MatrixXd(4, 4);
    ekf_.P_ << 10, 0, 0, 0,
        0, 10, 0, 0,
        0, 0, 1000, 0,
        0, 0, 0, 1000;
    
    previous_timestamp_ = measurement_pack.timestamp_;
    
    ekf_.F_ = MatrixXd(4, 4);
    ekf_.F_ << 1, 0, 1, 0,
        0, 1, 0, 1,
        0, 0, 1, 0,
        0, 0, 0, 1;
    
    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /**
   * Prediction
   */

  /**
   * TODO: Update the state transition matrix F according to the new elapsed time.
   * Time is measured in seconds.
   * TODO: Update the process noise covariance matrix.
   * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */
  /*Taking into account the timestamp*/
  float time = measurement_pack.timestamp_ - previous_timestamp_;
  //Converting time to seconds.
  time = time/pow(10.0, 6);

  //Setting previous timestamp to current timestamp
  previous_timestamp_ = measurement_pack.timestamp_;

  /*Initializing Process covariance matrix*/
  float noise_ax = 9;
  float noise_ay = 9;
  float time_r2 = pow(time, 2);
  float time_r3 = pow(time, 3);
  float time_r4 = pow(time, 4);

  //Update F matrix to take into account time for latest measurement received.
  ekf_.F_.row(0)[2] = time;
  ekf_.F_.row(1)[3] = time;

  //Declare and fill state covariance matrix representing stocastic part of motion.
  ekf_.Q_ = MatrixXd(4, 4);
  ekf_.Q_ << time_r4*noise_ax/4, 0, time_r3*noise_ax/2, 0,
      0, time_r4*noise_ay/4, 0, time_r3*noise_ay/2,
      time_r3*noise_ax/2, 0, time_r2*noise_ax, 0,
      0, time_r3*noise_ay/2, 0, time_r2*noise_ay;
  
  ekf_.Predict();

  /**
   * Update
   */

  /**
   * TODO:
   * - Use the sensor type to perform the update step.
   * - Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // TODO: Radar updates
    ekf_.R_ = R_radar_;
    Hj_ = tools.CalculateJacobian(ekf_.x_);
    ekf_.H_ = Hj_;
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // TODO: Laser updates
    	ekf_.R_ = R_laser_;
    	ekf_.H_ = H_laser_;
    	ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}

