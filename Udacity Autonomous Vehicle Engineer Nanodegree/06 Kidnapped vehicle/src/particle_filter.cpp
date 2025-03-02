/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;
//adding std library namespace
using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1.
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method
   *   (and others in this file).
   */
  //start random seed
  std:: default_random_engine gen;
  // set particle filter size to 100
  num_particles = 100;  
  //create gaussian distributions
  normal_distribution<double> dist_x(x, std[0]);
  normal_distribution<double> dist_y(y, std[1]);
  normal_distribution<double> dist_theta(theta, std[2]);
  //iterate over particles size
  for(int i = 0 ; i < num_particles ; i++){
    //initiate values for each particle
    Particle particle;
    particle.id = i;
    particle.x = dist_x(gen);
    particle.y = dist_y(gen);
    particle.theta = dist_theta(gen);
    particle.weight =   1.0;
    particles.push_back(particle);
  }
  //change initalization flag
  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[],
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
  //start random seed
  std::default_random_engine gen;
  //create gaussian distributions
  normal_distribution<double> dist_x(0,std_pos[0]);
  normal_distribution<double> dist_y(0,std_pos[1]);
  normal_distribution<double> dist_theta(0,std_pos[2]);

  //avoid 0 division
  if( fabs( yaw_rate ) > 0.00001){
    // update each particle state using the bicycle model
    for(int i = 0; i < num_particles; i++){
      particles[i].x += (velocity/yaw_rate)*(sin(particles[i].theta + yaw_rate * delta_t) - sin(particles[i].theta)) + dist_x(gen);

      particles[i].y += (velocity/yaw_rate)*(cos(particles[i].theta)-cos(particles[i].theta + yaw_rate * delta_t)) + dist_y(gen);

      particles[i].theta += yaw_rate * delta_t  + dist_theta(gen);

    }
  }
  else{
    // update each particle state using the bicycle model
    for(int i = 0; i < num_particles; i++){
      particles[i].x += velocity * delta_t * cos(particles[i].theta) + dist_x(gen);

      particles[i].y += velocity * delta_t * sin(particles[i].theta) + dist_x(gen);

      particles[i].theta += dist_theta(gen);

    }

  }
}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted,
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each
   *   observed measurement and assign the observed measurement to this
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will
   *   probably find it useful to implement this method and use it as a helper
   *   during the updateWeights phase.
   */
  // start random seed
  std::default_random_engine gen;
  //iterate over all observations
  for( int i = 0; i < observations.size(); i++){
    //set initial values for matching observation and prediction
    double min_dist = numeric_limits<double>::max();
    int map_index = -1;
    //iterate over all predictions
    for( int j = 0; j < predicted.size(); j++){
      // check the distance between observation and predictions
      double distance = dist(observations[i].x,observations[i].y,predicted[j].x,predicted[j].y);
      //pop up the nearest map landmark for each observation
      if(distance < min_dist){
        map_index = predicted[j].id;
        min_dist = distance;

      }
    }
    //update the observation id with the landmark value
    observations[i].id = map_index;
  }
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[],
                                   const vector<LandmarkObs> &observations,
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian
   *   distribution. You can read more about this distribution here:
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system.
   *   Your particles are located according to the MAP'S coordinate system.
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
  //start random seed
  std::default_random_engine gen;
  // for each particle
  for(int i = 0; i < num_particles; i++){

    // initalize vector to store transformed observation
    vector<LandmarkObs> obs_trans;
    //update each observation coordinate from vechicle to map coordinate system
    for(int j = 0; j < observations.size(); j++){

      LandmarkObs observed;
      observed.id = observations[j].id;
      observed.x = particles[i].x + (cos(particles[i].theta)*observations[j].x) - (sin(particles[i].theta) * observations[j].y);

      observed.y = particles[i].y + (sin(particles[i].theta)*observations[j].x) + (cos(particles[i].theta)* observations[j].y);

      obs_trans.push_back(observed);

    }

    //initialize potential map landmark list
    vector<LandmarkObs> predicted_landmarks;
    //get a list of landmarks within sensor range
    for(int j = 0; j < map_landmarks.landmark_list.size(); j++){
      double distance = dist(map_landmarks.landmark_list[j].x_f, map_landmarks.landmark_list[j].y_f, particles[i].x, particles[i].y);

      if(distance <= sensor_range){
        predicted_landmarks.push_back(LandmarkObs {map_landmarks.landmark_list[j].id_i, map_landmarks.landmark_list[j].x_f, map_landmarks.landmark_list[j].y_f});
      }
    }

    //link potential landmarks with obervation on map coordinate
    dataAssociation (predicted_landmarks, obs_trans );
    //clear particle weight
    particles[i].weight = 1.0;

    //initalize helper values
    double sigma_x_2 = 2*pow(std_landmark[0], 2);
    double sigma_y_2 = 2*pow(std_landmark[1], 2);
    double normalizer = (1.0/(2.0 * M_PI * std_landmark[0] * std_landmark[1]));

    //update each particle weights based on the distance between each landmark and observation
    for(int j = 0; j < obs_trans.size(); j++){
      bool found = false;
      int k =0;
      while(!found && k < predicted_landmarks.size()){
        if(predicted_landmarks[k].id == obs_trans[j].id){
          double weight = normalizer * exp( -( pow(obs_trans[j].x-predicted_landmarks[k].x,2)/(sigma_x_2) + (pow(obs_trans[j].y-predicted_landmarks[k].y,2)/(sigma_y_2))));
          if(weight == 0){
            particles[i].weight *= 0.0001;
          } else{
            particles[i].weight *= weight;
          }
          found = true;//break;
        }
        k++;
      }
    }
    //norm_weights += particles[i].weight;
    //add weight to list
    weights.push_back(particles[i].weight);
  }

  //for(int i =0; i < num_particles; i++){
    //particles[i].weight /= norm_weights;
    //weights.push_back(particles[i].weight);
  //}
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional
   *   to their weight.
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  //std::default_random_engine gen;
  // initalize vector to store the new particles
  vector<Particle> resampled;
  //double mw = *std::max_element(weights.begin(),weights.end());
  //std::uniform_int_distribution<int> un_int_dist(0, num_particles-1);
  //std::uniform_real_distribution<double> un_real_dist(0.0, mw);
  //int index =  un_int_dist(gen)%num_particles;
  //double beta =0.0;
  /*
  for(int i = 0; i < num_particles; i++){
    beta += 2.0 * un_real_dist(gen);
    while(beta > weights[index]){
      beta -= weights[index];
      index = (index + 1) % num_particles;
    }
  */
  //start random seed
  random_device rd;
  default_random_engine gen(rd());
  //select particles for next iteration based on the weight
  for(int i = 0; i < num_particles ; i++){
    discrete_distribution<int> index(weights.begin(), weights.end());
    resampled.push_back(particles[index(gen)%num_particles]);
  }
  
  //resampled.push_back(particles[index]);
  //}
  //replace particles
  particles = resampled;

  //clear helper variables from memory
  weights.clear();
  resampled.clear();
}

void ParticleFilter::SetAssociations(Particle& particle,
                                     const vector<int>& associations,
                                     const vector<double>& sense_x,
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association,
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}
