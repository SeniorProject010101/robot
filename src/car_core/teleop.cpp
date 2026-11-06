#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>

using geometry_msgs::msg::Twist;
using Clock = std::chrono::steady_clock;

const double MAX_SPEED   = 0.5;   // must match driver.cpp
const double WHEELBASE   = 0.14;  // must match driver.cpp
const double TURN_DEG    = 30.0;
const double FIRST_HOLD  = 0.7;   // wait for keyboard auto-repeat to kick in
const double REPEAT_HOLD = 0.15;  // gap between repeats while held

struct RawTerminal {                 // single keys, no Enter, no echo
  termios old{};
  RawTerminal() {
    tcgetattr(STDIN_FILENO, &old);
    termios raw = old;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }
  ~RawTerminal() { tcsetattr(STDIN_FILENO, TCSANOW, &old); }
};

int readKey(int timeout_ms) {
  fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
  timeval tv{0, timeout_ms * 1000};
  if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) return std::tolower(c);
  }
  return -1;
}

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("teleop");
  auto pub  = node->create_publisher<Twist>("cmd_vel", 10);

  std::printf("\nHOLD  W forward   S back   A fwd-left   D fwd-right   Z back-left   C back-right\n"
              "1-9 = 10-90%% speed   0 = 100%%   SPACE stop   X quit\n\n");

  RawTerminal term;
  double speed = 0.6;
  char held = 0;
  int repeats = 0;
  auto last_key = Clock::now();

  while (rclcpp::ok()) {
    int k = readKey(50);                      // ~20 loops per second
    auto t = Clock::now();

    if (k == 'x') break;
    if (k >= '1' && k <= '9') speed = (k - '0') / 10.0;
    else if (k == '0') speed = 1.0;
    else if (k == ' ') held = 0;
    else if (k != -1 && std::string("wsadzc").find(k) != std::string::npos) {
      repeats = (k == held) ? repeats + 1 : 0;
      held = k;
      last_key = t;
    }

    // key released? (no repeats arriving)
    double idle = std::chrono::duration<double>(t - last_key).count();
    if (held && idle > (repeats ? REPEAT_HOLD : FIRST_HOLD)) held = 0;

    double v = 0, steer = 0;                  // steer in degrees, + = left
    switch (held) {
      case 'w': v =  1; break;
      case 's': v = -1; break;
      case 'a': v =  1; steer =  TURN_DEG; break;
      case 'd': v =  1; steer = -TURN_DEG; break;
      case 'z': v = -1; steer =  TURN_DEG; break;
      case 'c': v = -1; steer = -TURN_DEG; break;
    }
    v *= speed * MAX_SPEED;

    Twist msg;
    msg.linear.x  = v;
    msg.angular.z = v * std::tan(steer * M_PI / 180.0) / WHEELBASE;  // inverse of driver's math
    pub->publish(msg);

    std::printf("\r key=%c  speed=%3d%%   ", held ? held : '-', int(speed * 100));
    std::fflush(stdout);
  }

  try { pub->publish(Twist()); } catch (...) {}   // final stop
  rclcpp::shutdown();
  std::printf("\nstopped\n");
  return 0;
}