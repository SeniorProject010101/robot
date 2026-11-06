#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <lgpio.h>
#include <algorithm>
#include <cmath>

using geometry_msgs::msg::Twist;
using namespace std::chrono_literals;

const double CENTER = 60.0;   // straight
const double MAX_TURN = 35.0;
const double WHEELBASE = 0.14;
const double MAX_SPEED = 0.5;  // same in teleop.cpp
const double REF_V = 0.1;      // same in teleop.cpp

const int IN1 = 23, IN2 = 24, ENA = 25, STEER = 12;

class Driver : public rclcpp::Node {
public:
  Driver() : Node("driver") {
    h_ = lgGpiochipOpen(0);
    for (int pin : {IN1, IN2, ENA, STEER}) lgGpioClaimOutput(h_, 0, pin, 0);
    setSteer(CENTER);
    sub_ = create_subscription<Twist>("cmd_vel", 10,
      [this](Twist::SharedPtr m) { onCmd(*m); });
    timer_ = create_wall_timer(100ms, [this] { watchdog(); });
    RCLCPP_INFO(get_logger(), "driver ready");
  }

  ~Driver() override { setDrive(0); lgGpiochipClose(h_); }

private:
  void setDrive(double s) {               // -1 .. 1
    if (s == drive_) return;
    drive_ = s;
    lgGpioWrite(h_, IN1, s < 0);
    lgGpioWrite(h_, IN2, s > 0);
    lgTxPwm(h_, ENA, 100, std::abs(s) * 100, 0, 0);
  }

  void setSteer(double deg) {             // servo angle
    if (deg == steer_) return;
    steer_ = deg;
    lgTxServo(h_, STEER, int(544 + deg / 180 * 1856), 50, 0, 0);
  }

  void onCmd(const Twist& m) {
    last_ = now();
    double v = m.linear.x;
    double vs = std::abs(v) > 0.01 ? v : REF_V;
    double deg = std::atan(WHEELBASE * m.angular.z / vs) * 180 / M_PI;
    setDrive(std::clamp(v / MAX_SPEED, -1.0, 1.0));
    setSteer(CENTER - std::clamp(deg, -MAX_TURN, MAX_TURN));
  }

  void watchdog() {                       // no commands for 0.5 s -> stop
    if ((now() - last_).seconds() > 0.5) setDrive(0);
  }

  int h_;
  double drive_ = -9, steer_ = -9;
  rclcpp::Time last_{0, 0, RCL_ROS_TIME};
  rclcpp::Subscription<Twist>::SharedPtr sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Driver>());
  rclcpp::shutdown();
}