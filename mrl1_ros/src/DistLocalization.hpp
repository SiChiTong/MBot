#ifndef DIST_LOCALIZATION__HPP_
#define DIST_LOCALIZATION__HPP_

#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Float64.h"
#include "sensor_msgs/LaserScan.h"
#include "ProcessLaserScan.hpp"
#include "ProcessNeighborLaserScan.hpp"
#include "PositionGT.hpp"
#include "ExpMath.cpp"
#include "geometry_msgs/Point.h"
#include "geometry_msgs/Pose2D.h"
#include "geometry_msgs/Pose.h"
#include "NeighborInfoSub.hpp"
#include "NeighborInfoSubEKF.hpp"
#include "JointEncoderSub.hpp"
#include "mrl1_ros/ExpL.h"
#include "tf/transform_datatypes.h"
#include "geometry_msgs/Quaternion.h"
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/LU>
#include <eigen3/unsupported/Eigen/MatrixFunctions>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>


using namespace std;
using namespace GroupMathSE;

// Inherits from class "ProcessLaserScan"
class DistLocalization : public ProcessLaserScan, public ProcessNeighborLaserScan, public NeighborInfoSub, public NeighborInfoSubEKF,public JointEncoderSub{

    public:
 //private:
        //used in function "SDEPrediction"
        ros::Time previousTime;
        ros::Time previousTimeEKF;
        double freq;

        // used in function "velocityPID"
        float previous_error;
        float integral_error;
//----------- ExpLocalization Member Variables ----------
        //initial position used in "expLocalization"
        Eigen::Matrix3d a_i;
        Eigen::Matrix3d a_iDR;
        Eigen::Vector3d currPose;
        Eigen::Matrix3d mu_i;
        Eigen::Matrix3d cov_i;
        Eigen::Matrix3d cov_iDR;
        Eigen::Vector3d poseDR;


        Eigen::Matrix3d a_j;
        //Eigen::Matrix3d mu_j;
       // Eigen::Matrix3d cov_j;

        Eigen::Matrix3d mu_m;
        Eigen::Matrix3d cov_m;

        Eigen::Matrix3d mu_i_bar;
        Eigen::Matrix3d Sigma_i_bar;

        Eigen::Matrix3d neighborIP_;
        Eigen::Matrix3d initialPose_;
        float w1;
        float w2;
        
        // For distEKF-----
        float Vim,Wim;

        Eigen::Matrix3d stateCovCurr;
        
        Eigen::Matrix2d processNoiseCov;
        //------

        Eigen::Vector3d neighborGT;
        Eigen::Vector3d selfGT;

        geometry_msgs::Pose2D estError;

        Eigen::Vector3d poseCurr;

   // public:
        std_msgs::Float64 leftEffort;
        std_msgs::Float64 rightEffort;
        mrl1_ros::ExpL posEsti;
        mrl1_ros::ExpL posEstiEKF;
        geometry_msgs::Point rqtEst;

        ros::Publisher leftWheelPub_;
        ros::Publisher rightWheelPub_;
       
        ros::Publisher poseEstimate_;
        ros::Publisher poseEstimateEKF_;
        ros::Publisher rqtEst_;
        ros::NodeHandle nh_;
   
        ros::Subscriber neighborGT_;
        ros::Subscriber selfGT_;
        ros::Publisher estError_;

        std::string robot;


        double dcov_m;
        // Constructor
        DistLocalization(ros::NodeHandle& nh, Eigen::Matrix3d initialPose,Eigen::Matrix3d neighborIP,std::string laserTopic,std::string neighborLaserTopic,std::string leftwheelcontrollertopic, std::string rightwheelcontrollertopic, std::string poseEstPubTopic, std::string poseEstPubTopicEKF,std::string poseEstSubTopic,std::string poseEstSubTopicEKF,std::string jointStateTopic,std::string selfGTTopic,std::string neighborGTTopic,std::string errorTopic):ProcessLaserScan(nh,initialPose,neighborIP,laserTopic),ProcessNeighborLaserScan(nh,initialPose,neighborIP,neighborLaserTopic),NeighborInfoSub(nh,neighborIP,poseEstSubTopic),NeighborInfoSubEKF(nh,neighborIP,poseEstSubTopicEKF),JointEncoderSub(nh,jointStateTopic)
        {
            nh_ = nh;

            freq = 15;

             leftWheelPub_ = nh_.advertise<std_msgs::Float64>(leftwheelcontrollertopic, 1000);
            rightWheelPub_ = nh_.advertise<std_msgs::Float64>(rightwheelcontrollertopic, 1000);  
            
            poseEstimate_ = nh_.advertise<mrl1_ros::ExpL>(poseEstPubTopic,1000);

            poseEstimateEKF_ = nh_.advertise<mrl1_ros::ExpL>(poseEstPubTopicEKF,1000);



            estError_ = nh_.advertise<geometry_msgs::Pose2D>(errorTopic,1000);

 neighborGT_ = nh_.subscribe<geometry_msgs::Pose>(neighborGTTopic, 1000, &DistLocalization::neighborGTCallback,this); 

 selfGT_ = nh_.subscribe<geometry_msgs::Pose>(selfGTTopic, 1000, &DistLocalization::selfGTCallback,this); 


            
            neighborGT = ExpMath::SE2ToXYTheta(neighborIP);
             neighborIP_ = neighborIP;
             initialPose_ = initialPose;
            selfGT = ExpMath::SE2ToXYTheta(initialPose);
            poseDR = selfGT;
            
            robot = laserTopic;

            previousTime = ros::Time::now();

            previous_error = 0;
            integral_error = 0;

            a_i = initialPose;
            a_iDR = initialPose;

            poseCurr = ExpMath::SE2ToXYTheta(initialPose);
            currPose = poseCurr;    
            // state initial covariance for expLocalization
            // larger value expL respond to updates slower, too small value expL insensitive to measurement updates
            cov_i << 4,0,0,
                     0,1,0,
                     0,0,4;

            cov_iDR << 4,0,0,
                     0,4,0,
                     0,0,4;

            // state covariance initialize for distEKF
            stateCovCurr = cov_i;

            // larger value EKF deviates faster
            processNoiseCov << 0.01,0,
                               0,0.01;

                               

            a_j << 1,0,0,
                   0,1,0,
                   0,0,1;

            dcov_m = 1;
           
            // measurement noise covariance    
            // larger value follows closer at the beginning but diverges faster
            cov_m << 1, 0, 0,
                     0, 1, 0,
                     0, 0, 1;


            w1 = 0;
            w2 = 0;

            mu_i_bar = Eigen::MatrixXd::Identity(3,3);
            Sigma_i_bar = mu_i_bar;
        }


        // Destructor
        ~DistLocalization(void)
        {
        }



        void neighborGTCallback(const geometry_msgs::Pose::ConstPtr& neighborGT_msg)
        {
            double theta;

            float x = neighborGT_msg->orientation.x;
            float y = neighborGT_msg->orientation.y;
            float z = neighborGT_msg->orientation.z;
            float w = neighborGT_msg->orientation.w;

            float mag = sqrt(pow(x,2)+pow(y,2)+pow(z,2)+pow(w,2));

            geometry_msgs::Quaternion temp;
            temp = neighborGT_msg->orientation;
 
             temp.x = temp.x/mag;
             temp.y = temp.y/mag;
             temp.z = temp.z/mag;
             temp.w = temp.w/mag;


            theta = tf::getYaw(temp);

            if(isnan(theta))
            {
                theta = 0.0000000001;
            }

//cout<<"theta:"<<theta<<endl;
//cout<<"x:"<<neighborGT_msg->position.x<<endl;
//cout<<"y:"<<neighborGT_msg->position.y<<endl;


            neighborGT << neighborGT_msg->position.x,neighborGT_msg->position.y,theta;
        }

 void selfGTCallback(const geometry_msgs::Pose::ConstPtr& selfGT_msg)
        {
            double theta;

            float x = selfGT_msg->orientation.x;
            float y = selfGT_msg->orientation.y;
            float z = selfGT_msg->orientation.z;
            float w = selfGT_msg->orientation.w;


          float mag = sqrt(pow(x,2)+pow(y,2)+pow(z,2)+pow(w,2));

            geometry_msgs::Quaternion temp;
            temp = selfGT_msg->orientation;
            
             temp.x = temp.x/mag;
             temp.y = temp.y/mag;
             temp.z = temp.z/mag;
             temp.w = temp.w/mag;




            theta = tf::getYaw(temp);
            if(isnan(theta))
            {
                theta = 0.000000001;
            }
//cout<<"theta:"<<theta<<endl;
//cout<<"x:"<<selfGT_msg->position.x<<endl;
//cout<<"y:"<<selfGT_msg->position.y<<endl;

            selfGT << selfGT_msg->position.x,selfGT_msg->position.y,theta;
        }
	
        // PID control velocity via effort
        std_msgs::Float64 velocityPID(float velDesire, float currVel,float kp, float ki, float kd);

        // main method to control robot
        void moveToLongestScan();


        void moveSinasoidal();

        void moveStraight()
        {
            leftEffort = this->velocityPID(0.3,encoderLeftVel,0.5,0.4,0.2);
                rightEffort = this->velocityPID(0.3,encoderRightVel,0.5,0.4,0.2);

        }

        
        void moveRandom()
        {
            double leftVelDes;
            double rightVelDes;

            double t = currTime.toSec();
           // double tempLeft, tempRight;

           // tempLeft = 0.6*(abs(sin(currTime.toSec()))+0.05);

           // tempRight = 0.6*(abs(cos(currTime.toSec()))+0.05);
            

            if(t < 15)

            {
                leftVelDes = 0.4;
                rightVelDes = 0.4;
            }
            else if(t >= 15)
            {
                leftVelDes = 0.1;
                rightVelDes = 1;

            }
           /* else if(t >=50)
            {
                leftVelDes = 1;
                rightVelDes = 0.1;
            }*/
            //leftVelDes = pow(tempLeft,2);
            //rightVelDes = tempRight;

            
            leftEffort = this->velocityPID(leftVelDes,encoderLeftVel,0.5,0.4,0.2);
                
            
            rightEffort = this->velocityPID(rightVelDes,encoderRightVel,0.5,0.4,0.2);


        }       
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
             // this->moveSinasoidal();

                this->moveRandom();
               
                leftWheelPub_.publish(leftEffort);
               rightWheelPub_.publish(rightEffort);
 
               // cout<< leftEffort.data<<","<<rightEffort.data<<endl;
                //cout<< encoderLeftVel << endl;
            }

        }


//----------  Distributed Localization Methods  ---------
        

//////////////////  Distributed EKF //////////////////


        // Main method to call
        void distEKF();
       
       // prediction 
       void distEKFPred(Eigen::Vector3d& poseCurr,Eigen::Matrix3d& stateCovCurr, double Vim, double Wim);

       //update
       void distEKFUpdate(Eigen::Vector3d& poseCurr,Eigen::Matrix3d& stateCovCurr,Eigen::Vector3d z);

        
        
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

