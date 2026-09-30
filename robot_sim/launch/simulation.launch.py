from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    share = FindPackageShare('robot_sim')
    description = PathJoinSubstitution([share, 'description', 'robot.urdf.xacro'])
    world = PathJoinSubstitution([share, 'worlds', 'empty.sdf'])

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare('ros_gz_sim'), '/launch/gz_sim.launch.py'
        ]),
        launch_arguments={'gz_args': ['-r ', world]}.items(),
    )
    state = Node(
        package='robot_state_publisher', executable='robot_state_publisher',
        parameters=[{'robot_description': Command(['xacro ', description])}],
    )
    marker = Node(
        package='robot_sim', executable='hello_marker_node', output='screen',
    )
    spawn = Node(
        package='ros_gz_sim', executable='create',
        arguments=['-topic', 'robot_description', '-name', 'robot', '-z', '0.2'],
        output='screen',
    )
    bridge = Node(
        package='ros_gz_bridge', executable='parameter_bridge',
        arguments=['/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
                   '/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
                   '/joint_states@sensor_msgs/msg/JointState[gz.msgs.Model'],
    )
    rviz = Node(
        package='rviz2', executable='rviz2',
        arguments=['-d', PathJoinSubstitution([share, 'rviz', 'robot.rviz'])],
    )
    return LaunchDescription([gazebo, state, spawn, bridge, marker, rviz])