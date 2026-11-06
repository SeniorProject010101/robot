#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <lgpio.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

using geometry_msgs::msg::Twist;
using namespace std::chrono_literals;

// ---- car settings (keep in sync with teleop.cpp) ----
const double CENTER    = 60.0;   // servo angle for straight
const double MAX_TURN  = 35.0;   // degrees each way
const double WHEELBASE = 0.14;   // metres
const double MAX_SPEED = 0.5;    // m/s = full power
const double REF_V     = 0.1;    // pretend speed for steering while stopped

// ---- Pi GPIO numbers (not physical pins) ----
const int IN1   = 23;  // phys 16 (shield 7)
const int IN2   = 24;  // phys 18 (shield 8)
const int ENA   = 25;  // phys 22 (shield 5)
const int STEER = 12;  // phys 32 (shield 9)

// ---- hardware PWM (GPIO12 = pwmchip0 channel 0) ----
const std::string PWM = "/sys/class/pwm/pwmchip0";

bool pwmWrite(const std::string& file, long value) {
  std::ofstream f(PWM + file);
  if (!f) return false;
  f << value;
  return static_cast<bool>(f);
}

class Driver : public rclcpp::Node {
public:
  Driver() : Node("driver") {
    // steering: try hardware PWM first
    if (std::filesystem::exists(PWM)) {
      if (!std::filesystem::exists(PWM + "/pwm0")) {
        pwmWrite("/export", 0);
        std::this_thread::sleep_for(200ms);
      }
      hw_pwm_ = pwmWrite("/pwm0/period", 20000000)   // 20 ms = 50 Hz
             && pwmWrite("/pwm0/enable", 1);
    }

    // motor (and steering fallback) via lgpio
    h_ = lgGpiochipOpen(0);
    if (h_ >= 0) {
      for (int pin : {IN1, IN2, ENA}) lgGpioClaimOutput(h_, 0, pin, 0);
      if (!hw_pwm_) lgGpioClaimOutput(h_, 0, STEER, 0);
      RCLCPP_INFO(get_logger(), "GPIO ready (real hardware)");
    } else {
      RCLCPP_WARN(get_logger(), "No GPIO chip -> print-only mode");
    }
    RCLCPP_INFO(get_logger(), hw_pwm_ ? "steering: hardware PWM"
                                      : "steering: software PWM (may jitter)");

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
    std::this_thread::sleep_for(300ms);          // let the servo reach center
    if (hw_pwm_) pwmWrite("/pwm0/enable", 0);
    if (h_ >= 0) {
      if (!hw_pwm_) lgTxServo(h_, STEER, 0, 50, 0, 0);
      lgGpiochipClose(h_);
    }
  }

private:
  // speed -1.0 .. 1.0; only touches pins when it changes
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

  // servo angle; only updates when it changes
  void setSteer(double deg) {
    if (std::abs(deg - last_steer_) < 0.5) return;
    last_steer_ = deg;
    RCLCPP_INFO(get_logger(), "steer=%.1f", deg);
    int us = static_cast<int>(544 + deg / 180.0 * (2400 - 544));
    if (hw_pwm_)      pwmWrite("/pwm0/duty_cycle", us * 1000L);   // us -> ns
    else if (h_ >= 0) lgTxServo(h_, STEER, us, 50, 0, 0);
  }

  void onCmd(const Twist& msg) {
    last_cmd_ = now();
    stopped_ = false;
    double v = msg.linear.x;
    double w = msg.angular.z;

    double drive = std::clamp(v / MAX_SPEED, -1.0, 1.0);

    // stopped: use the agreed reference speed so the wheels can turn in place
    double v_for_steer = (std::abs(v) > 0.01) ? v : REF_V;
    double deg = std::atan(WHEELBASE * w / v_for_steer) * 180.0 / M_PI;
    deg = std::clamp(deg, -MAX_TURN, MAX_TURN);

    setDrive(drive);
    setSteer(CENTER - deg);   // left = smaller angle on this car
  }

  void watchdog() {
    if (!stopped_ && (now() - last_cmd_).seconds() > 0.5) {
      stopped_ = true;
      setDrive(0.0);
      setSteer(CENTER);
      RCLCPP_INFO(get_logger(), "no command -> STOP + center");
    }
  }

  int h_ = -1;
  bool hw_pwm_ = false;
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