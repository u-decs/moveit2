#include <rclcpp/rclcpp.hpp>

// MoveIt and Servo
#include <moveit_servo/servo_parameters.h>
#include <moveit_servo/servo.h>
#include <moveit/planning_scene_monitor/planning_scene_monitor.h>

// AprilTag detection message
#include <isaac_ros_apriltag_interfaces/msg/april_tag_detection_array.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>

using namespace std::chrono_literals;

static const rclcpp::Logger LOGGER = rclcpp::get_logger("moveit2_tutorials.servo_demo_node.cpp");

rclcpp::Node::SharedPtr node_;
rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr twist_cmd_pub_;

// Proportional control constants for linear and angular velocity
const double k_p_linear = 1.0;
const double k_p_angular = 1.0;

void apriltag_callback(const isaac_ros_apriltag_interfaces::msg::AprilTagDetectionArray::SharedPtr msg)
{
    // Check if any AprilTags are detected
    if (msg->detections.empty())
    {
        RCLCPP_WARN(LOGGER, "No AprilTags detected.");
        return;
    }

    // Assume we're following the first detected AprilTag
    auto tag = msg->detections[0];

    // Extract pose information (position and orientation of the tag)
    auto position = tag.pose.pose.pose.position;
    auto orientation = tag.pose.pose.pose.orientation;

    // Create a TwistStamped message to control the robot's end-effector
    auto twist_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
    twist_msg->header.stamp = node_->now();
    twist_msg->header.frame_id = "panda_link0";  // Use the appropriate reference frame

    // Proportional control: Generate linear and angular velocities
    twist_msg->twist.linear.x = k_p_linear * position.x;  // Move towards the tag in the X axis
    twist_msg->twist.linear.y = k_p_linear * position.y;  // Move left/right
    twist_msg->twist.linear.z = k_p_linear * position.z;  // Move up/down

    twist_msg->twist.angular.x = k_p_angular * orientation.x;
    twist_msg->twist.angular.y = k_p_angular * orientation.y;
    twist_msg->twist.angular.z = k_p_angular * orientation.z;

    // Publish the TwistStamped message
    twist_cmd_pub_->publish(std::move(twist_msg));

    RCLCPP_INFO(LOGGER, "Following AprilTag ID: %d", tag.id);
}

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::NodeOptions node_options;
    node_options.use_intra_process_comms(false);
    node_ = std::make_shared<rclcpp::Node>("servo_demo_node", node_options);

    // Pause for RViz to come up
    rclcpp::sleep_for(std::chrono::seconds(4));

    // Setup planning_scene_monitor
    auto tf_buffer = std::make_shared<tf2_ros::Buffer>(node_->get_clock());
    auto planning_scene_monitor = std::make_shared<planning_scene_monitor::PlanningSceneMonitor>(
        node_, "robot_description", tf_buffer, "planning_scene_monitor");

    if (planning_scene_monitor->getPlanningScene())
    {
        planning_scene_monitor->startStateMonitor("/joint_states");
        planning_scene_monitor->setPlanningScenePublishingFrequency(25);
        planning_scene_monitor->startPublishingPlanningScene(planning_scene_monitor::PlanningSceneMonitor::UPDATE_SCENE,
                                                             "/moveit_servo/publish_planning_scene");
        planning_scene_monitor->startSceneMonitor();
        planning_scene_monitor->providePlanningSceneService();
    }
    else
    {
        RCLCPP_ERROR(LOGGER, "Planning scene not configured");
        return EXIT_FAILURE;
    }

    // Publisher for Twist messages to control the end-effector
    twist_cmd_pub_ = node_->create_publisher<geometry_msgs::msg::TwistStamped>("servo_demo_node/delta_twist_cmds", 10);

    // Subscribe to the AprilTag detection topic
    auto tag_sub = node_->create_subscription<isaac_ros_apriltag_interfaces::msg::AprilTagDetectionArray>(
        "/apriltag_detections", 10, apriltag_callback);

    // Initialize MoveIt Servo
    auto servo_parameters = moveit_servo::ServoParameters::makeServoParameters(node_);
    if (!servo_parameters)
    {
        RCLCPP_FATAL(LOGGER, "Failed to load the servo parameters");
        return EXIT_FAILURE;
    }

    auto servo = std::make_unique<moveit_servo::Servo>(node_, servo_parameters, planning_scene_monitor);
    servo->start();

    // Use a multithreaded executor for handling concurrent processes
    auto executor = std::make_unique<rclcpp::executors::MultiThreadedExecutor>();
    executor->add_node(node_);
    executor->spin();

    rclcpp::shutdown();
    return 0;
}