#ifndef DIST_LOCALIZATION__HPP_
#define DIST_LOCALIZATION__HPP_

#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Float64.h"
#include "sensor_msgs/LaserScan.h"
#include "ProcessLaserScan.hpp"
#include "PositionGT.hpp"
#include "ExpMath.cpp"
#include "geometry_msgs/Point.h"
#include "geometry_msgs/Pose2D.h"
#include "NeighborInfoSub.hpp"
#include "JointEncoderSub.hpp"

#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/LU>
#include <eigen3/unsupported/Eigen/MatrixFunctions>
#include <sstream>
#include <vector>
#include <iostream>
#include <math.h>


using namespace std;

// Inherits from class "ProcessLaserScan"
class DistLocalization : public ProcessLaserScan, public NeighborInfoSub, public JointEncoderSub{

    public:
        std_msgs::Float64 leftEffort;
        std_msgs::Float64 rightEffort;
        geometry_msgs::Pose2D posEst;

        ros::Publisher leftWheelPub_;
        ros::Publisher rightWheelPub_;
       
        ros::Publisher poseEstimate_;
        ros::NodeHandle nh_;

        //used in function "SDEPrediction"
        ros::Time previousTime;

        // used in function "velocityPID"
        float previous_error;
        float integral_error;


        std::string robot;
        // Constructor
        DistLocalization(ros::NodeHandle& nh,std::string laserTopic, std::string leftwheelcontrollertopic, std::string rightwheelcontrollertopic, std::string poseEstPubTopic, std::string poseEstSubTopic,std::string jointStateTopic):ProcessLaserScan(nh,laserTopic),NeighborInfoSub(nh,poseEstSubTopic),JointEncoderSub(nh,jointStateTopic)
        {
            nh_ = nh;

             leftWheelPub_ = nh_.advertise<std_msgs::Float64>(leftwheelcontrollertopic, 1000);
            rightWheelPub_ = nh_.advertise<std_msgs::Float64>(rightwheelcontrollertopic, 1000);  
            
            poseEstimate_ = nh_.advertise<geometry_msgs::Pose2D>(poseEstPubTopic,1000);


            robot = laserTopic;

            previousTime = ros::Time::now();

            previous_error = 0;
            integral_error = 0;
        }


        // Destructor
        ~DistLocalization(void)
        {
        }

        
        // PID control velocity via effort
        std_msgs::Float64 velocityPID(float velDesire, float currVel,float kp, float ki, float kd);

        // main method to control robot
        void moveToLongestScan();


        void moveSinasoidal();

        
        // publish motor commands
        void Explore(void)
        {
              
           // if(rangeReadings.size() < scanNum)
            if(encoderLeftVel > 10)
            { 
                ros::Duration(0.5).sleep(); 
            }
            else
            {

               //current exploration method! 
              
              //this->moveToLongestScan();    
              //this->moveSinasoidal();



                leftEffort = this->velocityPID(1,encoderLeftVel,0.5,0.4,0.2);
                rightEffort = this->velocityPID(1,encoderRightVel,0.5,0.4,0.2);
               leftWheelPub_.publish(leftEffort);
               rightWheelPub_.publish(rightEffort);
 
                cout<< leftEffort.data<<","<<rightEffort.data<<endl;
                //cout<< encoderLeftVel << endl;
            }

        }


//----------  Distributed Localization Methods  ---------
        

//////////////////  Distributed EKF //////////////////


        // Main method to call
        void distEKF();
       
       

        
        
///////////////// Exponential Localization ///////////
        
        // Main method to call
        void expLocalization();

        // given command of the two wheels, returns the predicted mean and covariance of the kinematic cart
        // inputs: w1 and w2 are left and right angular velocities
        // outputs: predicted mean and covariance in SE2
        void SDEPrediction(float w1, float w2, Eigen::Matrix3d& mu, Eigen::Matrix3d& cov);
 
  
        // returns xi and Si
         void generate_Si(Eigen::Matrix3d Sigma_i, Eigen::Vector3d& xi, Eigen::Matrix3d& Si);       
       
        // returns xm and Sm
         void generate_Sm(Eigen::Matrix3d m_im,Eigen::Matrix3d mu_m,Eigen::Matrix3d a_m,Eigen::Matrix3d a_i,Eigen::Matrix3d mu_i,Eigen::Matrix3d Sigma_m,Eigen::Vector3d& xm,Eigen::Matrix3d& Sm);

         // % mu_i and cov_i are 3x3 matrices representing the estimated mean
         // % and covariance of the robot to be updated
         //
         // % a_i is a 3x3 matrix denoting the initial position of robot i
         //
         // % MU_m, COV_m are 3x3xn matrices representing the means and covariances of the n neighboring robots for robot i to take measurements from
         //
         // % A_i is a 3x3 matrix denoting the initial positions of all other m robots
         // % Mim 3x3 is the exact measurement of robot i relative to m [xi,Si] = Generate_Si(cov_i)
         void Fusion(Eigen::Matrix3d a_i,Eigen::Matrix3d mu_i,Eigen::Matrix3d cov_i,Eigen::Matrix3d A_m,Eigen::Matrix3d MU_m,Eigen::Matrix3d COV_m,Eigen::Matrix3d Mim, Eigen::Matrix3d& mu_i_bar, Eigen::Matrix3d& Sigma_i_bar);

        // returns the final estimate update for mean (mu_i_bar) and covariance (Sigma_i_bar)
         void fusion_with_sensor_noise(Eigen::Matrix3d a_i,Eigen::Matrix3d mu_i,Eigen::Matrix3d cov_i,Eigen::Matrix3d a_j,Eigen::Matrix3d mu_j,Eigen::Matrix3d cov_j,Eigen::Matrix3d mu_m,Eigen::Matrix3d cov_m,Eigen::Matrix3d& mu_i_bar,Eigen::Matrix3d& Sigma_i_bar);


};

#endif



























