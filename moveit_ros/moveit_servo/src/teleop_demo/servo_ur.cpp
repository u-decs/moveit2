

#include <chrono>
#include <control_msgs/msg/joint_jog.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include "isaac_ros_apriltag_interfaces/msg/april_tag_detection_array.hpp"
#include "isaac_ros_apriltag_interfaces/msg/april_tag_detection.hpp"
#include <rclcpp/rclcpp.hpp>
#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

// Some constants used in the Servo Teleop demo
namespace
{
const std::string TWIST_TOPIC = "/servo_node/delta_twist_cmds";
const size_t ROS_QUEUE_SIZE = 10;
const std::string PLANNING_FRAME_ID = "base_link";
const std::string EE_FRAME_ID = "camera_link";
}  // namespace


int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  auto nh_ = rclcpp::Node::make_shared("servo_ur_input");
  auto twist_pub_ = nh_->create_publisher<geometry_msgs::msg::TwistStamped>(TWIST_TOPIC, ROS_QUEUE_SIZE);


  //rclcpp::Node::SharedPtr nh_;
  //nh_ = rclcpp::Node::make_shared("servo_ur_input");
  //rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr twist_pub_;

  auto subscriber = nh_->create_subscription<isaac_ros_apriltag_interfaces::msg::AprilTagDetectionArray>(
    "tag_detections", 10, [&](const isaac_ros_apriltag_interfaces::msg::AprilTagDetectionArray::SharedPtr msg) {
        for (const auto& detection : msg->detections)
        {
          // Extract the pose from each detection

          geometry_msgs::msg::TwistStamped twist_msg;
          //posed.pose = detection.pose.pose;
          //posed.header.frame_id = "camera_link";
          //posed.header.stamp = nh_->now();

         twist_msg.twist.linear.y = 0.6;//*detection.pose.pose.pose.position.y;//equals y component
         twist_msg.twist.linear.x = 0.0;// 0.5*detection.pose.pose.pose.position.x;//equals x component
         twist_msg.twist.linear.z = 0.0;
         twist_msg.twist.angular.x = 0.0;
         twist_msg.twist.angular.y = 0.0;
         twist_msg.twist.angular.z = 0.0;


            // publish the message no
          twist_msg.header.stamp = nh_->now();
          twist_msg.header.frame_id = "base_link";
          twist_pub_->publish(std::move(twist_msg));
  


          RCLCPP_INFO(nh_->get_logger(), "Message not in use");
        }
    });
   // // Create the messages we might publish

  rclcpp::Rate loop_rate(10);

  while (rclcpp::ok()) {

    rclcpp::spin_some(nh_);
    loop_rate.sleep();
  }

  rclcpp::shutdown();
  return 0;

};


