#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <lgpio.h>
#include <algorithm>
#include <cmath>

using geometry_msgs::msg::Twist;
using namespace std::chrono_literals;

const double CENTER    = 60.0;   // servo angle for straight
const double MAX_TURN  = 35.0;
const double WHEELBASE = 0.14;   // must match teleop.cpp
const double MAX_SPEED = 1.0;    // must match teleop.cpp

const int IN1   = 23;  // phys 16 (shield 7)
const int IN2   = 24;  // phys 18 (shield 8)
const int ENA   = 25;  // phys 22 (shield 5)
const int STEER = 12;  // phys 32 (shield 9)

class Driver : public rclcpp::Node {
public:
  Driver() : Node("driver") {
    h_ = lgGpiochipOpen(0);
    if (h_ >= 0) {
      for (int pin : {IN1, IN2, ENA, STEER}) lgGpioClaimOutput(h_, 0, pin, 0);
      RCLCPP_INFO(get_logger(), "GPIO ready (real hardware)");
    } else {
      RCLCPP_WARN(get_logger(), "No GPIO chip -> print-only mode");
    }
    setDrive(0.0);
    setSteer(CENTER);
    sub_ = create_subscription<Twist>("cmd_vel", 10,
      [this](Twist::SharedPtr msg) { onCmd(*msg); });
    timer_ = create_wall_timer(100ms, [this] { watchdog(); });
    last_cmd_ = now();
    RCLCPP_INFO(get_logger(), "driver ready");
  }

  ~Driver() override {
    setDrive(0.0);
    setSteer(CENTER);
    if (h_ >= 0) {
      lgTxServo(h_, STEER, 0, 50, 0, 0);   // stop servo pulses
      lgGpiochipClose(h_);
    }
  }

private:
  // speed -1.0 .. 1.0, only touches the pins when it changes
  void setDrive(double s) {
    if (std::abs(s - last_drive_) < 0.01) return;
    last_drive_ = s;
    RCLCPP_INFO(get_logger(), "drive=%+.2f", s);
    if (h_ < 0) return;
    if (s > 0)      { lgGpioWrite(h_, IN1, 0); lgGpioWrite(h_, IN2, 1); }
    else if (s < 0) { lgGpioWrite(h_, IN1, 1); lgGpioWrite(h_, IN2, 0); }
    else            { lgGpioWrite(h_, IN1, 0); lgGpioWrite(h_, IN2, 0); }
    lgTxPwm(h_, ENA, 1000, std::abs(s) * 100.0, 0, 0);
  }

  // servo angle, only restarts the pulse train when the angle changes
  void setSteer(double deg) {
    if (std::abs(deg - last_steer_) < 0.5) return;
    last_steer_ = deg;
    RCLCPP_INFO(get_logger(), "steer=%.1f", deg);
    if (h_ < 0) return;
    int us = static_cast<int>(544 + deg / 180.0 * (2400 - 544));
    lgTxServo(h_, STEER, us, 50, 0, 0);
  }

  void onCmd(const Twist& msg) {
    last_cmd_ = now();
    stopped_ = false;
    double v = msg.linear.x;
    double w = msg.angular.z;

    double drive = std::clamp(v / MAX_SPEED, -1.0, 1.0);

    double deg = 0.0;
    if (std::abs(v) > 0.01) deg = std::atan(WHEELBASE * w / v) * 180.0 / M_PI;
    deg = std::clamp(deg, -MAX_TURN, MAX_TURN);

    setDrive(drive);
    setSteer(CENTER - deg);   // left = smaller angle on this car
  }

  void watchdog() {
    if (!stopped_ && (now() - last_cmd_).seconds() > 0.5) {
      stopped_ = true;
      setDrive(0.0);
      setSteer(CENTER);
      RCLCPP_INFO(get_logger(), "no command -> STOP");
    }
  }

  int h_ = -1;
  rclcpp::Subscription<Twist>::SharedPtr sub_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Time last_cmd_;
  bool stopped_ = true;
  double last_drive_ = -99.0;   // impossible start values so the first call always applies
  double last_steer_ = -99.0;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Driver>());
  rclcpp::shutdown();
  return 0;
}