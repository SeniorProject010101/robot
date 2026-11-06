#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <algorithm>
#include <cmath>

using geometry_msgs::msg::Twist;
using namespace std::chrono_literals;

const double CENTER = 60.0;
const double MAX_TURN = 35.0;
const double WHEELBASE = 0.14;
const double MAX_SPEED = 0.5;

class Driver : public rclcpp::Node {
public:
  Driver() : Node("driver") {
    sub_ = create_subscription<Twist>("cmd_vel", 10,
      [this](Twist::SharedPtr msg) { onCmd(*msg); });
    timer_ = create_wall_timer(100ms, [this] { watchdog(); });
    last_cmd_ = now();
    RCLCPP_INFO(get_logger(), "driver ready");
  }

private:
  void onCmd(const Twist& msg) {
    last_cmd_ = now();
    stopped_ = false;

    double v = msg.linear.x;
    double w = msg.angular.z;

    double drive = std::clamp(v / MAX_SPEED, -1.0, 1.0);

    double deg = 0.0;
    if (std::abs(v) > 0.01) {
      deg = std::atan(WHEELBASE * w / v) * 180.0 / M_PI;
    }
    deg = std::clamp(deg, -MAX_TURN, MAX_TURN);
    double servo = CENTER - deg;

    RCLCPP_INFO(get_logger(), "drive=%+.2f  servo=%.1f", drive, servo);
  }

  void watchdog() {
    if (!stopped_ && (now() - last_cmd_).seconds() > 0.5) {
      stopped_ = true;
      RCLCPP_INFO(get_logger(), "no command -> STOP");
    }
  }

  rclcpp::Subscription<Twist>::SharedPtr sub_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Time last_cmd_;
  bool stopped_ = true;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Driver>());
  rclcpp::shutdown();
  return 0;
}