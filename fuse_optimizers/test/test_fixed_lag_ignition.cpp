/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2020, Locus Robotics
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#include <fuse_models/SetPose.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <nav_msgs/Odometry.h>
#include <ros/ros.h>

#include <gtest/gtest.h>


TEST(FixedLagIgnition, SetInitialState)
{
  auto node_handle = ros::NodeHandle();
  auto pose_publisher = node_handle.advertise<geometry_msgs::PoseWithCovarianceStamped>("/pose", 1);

  // Wait for the optimizer to be ready
  ASSERT_TRUE(ros::service::waitForService("/fixed_lag/reset", ros::Duration(1.0)));
  ASSERT_TRUE(ros::service::waitForService("/fixed_lag/set_pose", ros::Duration(1.0)));

  // Set the initial pose to something near zero
  fuse_models::SetPose::Request req;
  req.pose.header.frame_id = "map";
  req.pose.header.stamp = ros::Time(1, 0);
  req.pose.pose.pose.position.x = 100.1;
  req.pose.pose.pose.position.y = 100.2;
  req.pose.pose.pose.position.z = 0.0;
  req.pose.pose.pose.orientation.x = 0.0;
  req.pose.pose.pose.orientation.y = 0.0;
  req.pose.pose.pose.orientation.z = 0.0;
  req.pose.pose.pose.orientation.w = 1.0;
  req.pose.pose.covariance[0] = 1.0;
  req.pose.pose.covariance[7] = 1.0;
  req.pose.pose.covariance[35] = 1.0;
  fuse_models::SetPose::Response res;
  ros::service::call("/fixed_lag/set_pose", req, res);

  // I need to wait for the subscriber to be ready. I don't know of a better way.
  // There is no ros::topic::waitForSubscriber() method.
  ros::Duration(0.5).sleep();

  // Publish a relative pose
  auto pose_msg1 = geometry_msgs::PoseWithCovarianceStamped();
  pose_msg1.header.stamp = ros::Time(2, 0);
  pose_msg1.header.frame_id = "base_link";
  pose_msg1.pose.pose.position.x = 0.0;
  pose_msg1.pose.pose.position.y = 0.0;
  pose_msg1.pose.pose.position.z = 0.0;
  pose_msg1.pose.pose.orientation.x = 0.0;
  pose_msg1.pose.pose.orientation.y = 0.0;
  pose_msg1.pose.pose.orientation.z = 0.0;
  pose_msg1.pose.pose.orientation.w = 0.0;
  pose_msg1.pose.covariance[0] = 1.0;
  pose_msg1.pose.covariance[7] = 1.0;
  pose_msg1.pose.covariance[35] = 1.0;
  pose_publisher.publish(pose_msg1);

  auto pose_msg2 = geometry_msgs::PoseWithCovarianceStamped();
  pose_msg2.header.stamp = ros::Time(3, 0);
  pose_msg2.header.frame_id = "base_link";
  pose_msg2.pose.pose.position.x = 10.0;
  pose_msg2.pose.pose.position.y = 20.0;
  pose_msg2.pose.pose.position.z = 0.0;
  pose_msg2.pose.pose.orientation.x = 0.0;
  pose_msg2.pose.pose.orientation.y = 0.0;
  pose_msg2.pose.pose.orientation.z = 0.5000;
  pose_msg2.pose.pose.orientation.w = 0.8660;
  pose_msg2.pose.covariance[0] = 1.0;
  pose_msg2.pose.covariance[7] = 1.0;
  pose_msg2.pose.covariance[35] = 1.0;
  pose_publisher.publish(pose_msg2);

  // Wait for the optimizer to publish the first pose
  auto odom_msg = ros::topic::waitForMessage<nav_msgs::Odometry>("/odom", ros::Duration(1.0));
  ASSERT_TRUE(odom_msg);

  // The optimizer is configured for 0 iterations, so it should return the initial variable values
  // If we did our job correctly, the initial variable values should be the same as the service call state, give or
  // take the motion model forward prediction.
  EXPECT_NEAR(100.1, odom_msg->pose.pose.position.x, 0.10);
  EXPECT_NEAR(100.2, odom_msg->pose.pose.position.y, 0.10);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "fixed_lag_ignition_test");
  auto spinner = ros::AsyncSpinner(1);
  spinner.start();
  int ret = RUN_ALL_TESTS();
  spinner.stop();
  ros::shutdown();
  return ret;
}
