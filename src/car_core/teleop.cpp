#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>

using geometry_msgs::msg::Twist;
using Clock = std::chrono::steady_clock;

// ---- keep in sync with driver.cpp ----
const double MAX_SPEED   = 0.8;
const double WHEELBASE   = 0.14;
const double REF_V       = 0.1;

const double MAX_TURN    = 30.0;  // degrees
const double STEP        = 10.0;  // degrees per A/D tap
const double FIRST_HOLD  = 0.7;   // wait for keyboard auto-repeat to start
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

  std::printf("\nHOLD W forward / S back    TAP A/D steer (stays)    R center\n"
              "1-9 = 10-90%% speed   0 = 100%%   SPACE stop   X quit\n\n");

  RawTerminal term;
  double speed = 0.6;
  double steer = 0;        // degrees, + = left
  char held = 0;
  int repeats = 0;
  auto last_key = Clock::now();

  while (rclcpp::ok()) {
    int k = readKey(50);   // ~20 loops per second
    auto t = Clock::now();

    if (k == 'x') break;
    if (k >= '1' && k <= '9') speed = (k - '0') / 10.0;
    else if (k == '0') speed = 1.0;
    else if (k == ' ') held = 0;
    else if (k == 'a') steer = std::min(MAX_TURN,  steer + STEP);
    else if (k == 'd') steer = std::max(-MAX_TURN, steer - STEP);
    else if (k == 'r') steer = 0;
    else if (k == 'w' || k == 's') {
      repeats = (k == held) ? repeats + 1 : 0;
      held = k;
      last_key = t;
    }

    // W/S released? (no repeats arriving)
    double idle = std::chrono::duration<double>(t - last_key).count();
    if (held && idle > (repeats ? REPEAT_HOLD : FIRST_HOLD)) held = 0;

    double v = (held == 'w') ? 1.0 : (held == 's') ? -1.0 : 0.0;
    v *= speed * MAX_SPEED;

    // inverse of the driver's math; REF_V when stopped so steering still works
    double v_for_steer = (std::abs(v) > 0.01) ? v : REF_V;
    Twist msg;
    msg.linear.x  = v;
    msg.angular.z = v_for_steer * std::tan(steer * M_PI / 180.0) / WHEELBASE;
    pub->publish(msg);

    std::printf("\r drive=%c  steer=%+3.0f  speed=%3d%%   ",
                held ? held : '-', steer, int(speed * 100));
    std::fflush(stdout);
  }

  try { pub->publish(Twist()); } catch (...) {}   // final stop + center
  rclcpp::shutdown();
  std::printf("\nstopped\n");
  return 0;
}