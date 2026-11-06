#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

using geometry_msgs::msg::Twist;
using Clock = std::chrono::steady_clock;

const double MAX_SPEED = 0.5;  // same in driver.cpp
const double WHEELBASE = 0.14;
const double REF_V = 0.1;

int readKey() {                           // waits up to 50 ms
  fd_set f; FD_ZERO(&f); FD_SET(0, &f);
  timeval tv{0, 50000};
  char c;
  if (select(1, &f, 0, 0, &tv) > 0 && read(0, &c, 1) == 1) return tolower(c);
  return -1;
}

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("teleop");
  auto pub = node->create_publisher<Twist>("cmd_vel", 10);

  termios old, raw;                       // read keys without Enter
  tcgetattr(0, &old); raw = old;
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &raw);

  printf("HOLD W/S drive | TAP A/D steer | C center | 1-9,0 speed | SPACE stop | Q quit\n");

  double speed = 1.0, steer = 0;          // steer: degrees, + = left
  char held = 0;
  auto last = Clock::now();
  bool repeating = false;

  while (rclcpp::ok()) {
    int k = readKey();
    auto t = Clock::now();

    if (k == 'q') break;
    if (k == 'w' || k == 's') { repeating = (k == held); held = k; last = t; }
    if (k == 'a') steer = std::min(35.0, steer + 10);
    if (k == 'd') steer = std::max(-35.0, steer - 10);
    if (k == 'c') steer = 0;
    if (k == ' ') held = 0;
    if (k >= '1' && k <= '9') speed = (k - '0') / 10.0;
    if (k == '0') speed = 1.0;

    // released? keys repeat ~25x/s while held; first repeat takes ~0.5 s
    double idle = std::chrono::duration<double>(t - last).count();
    if (idle > (repeating ? 0.15 : 0.7)) held = 0;

    double v = (held == 'w' ? 1 : held == 's' ? -1 : 0) * speed * MAX_SPEED;
    double vs = std::abs(v) > 0.01 ? v : REF_V;

    Twist m;
    m.linear.x = v;
    m.angular.z = vs * std::tan(steer * M_PI / 180) / WHEELBASE;
    pub->publish(m);

    printf("\r drive=%c steer=%+3.0f speed=%3.0f%%  ", held ? held : '-', steer, speed * 100);
    fflush(stdout);
  }

  pub->publish(Twist());                  // stop
  tcsetattr(0, TCSANOW, &old);
  rclcpp::shutdown();
}