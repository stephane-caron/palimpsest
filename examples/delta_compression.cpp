// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Inria

#include <palimpsest/Dictionary.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using palimpsest::Dictionary;

// Number of steps of the evaluation loop
constexpr int kNumSteps = 10000;

// Make a big dictionary representative of a robotics use case
Dictionary make_big_robot_dictionary();

int main() {
  Dictionary init_robot = make_big_robot_dictionary();
  Dictionary robot = make_big_robot_dictionary();

  // Setup random number generation
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> pos_dis(-3.14, 3.14);
  std::uniform_real_distribution<double> vel_dis(-10.0, 10.0);
  std::uniform_real_distribution<double> torque_dis(-5.0, 5.0);

  const std::vector<std::string> servo_names = {"left_wheel", "left_knee",
                                                "left_hip",   "right_wheel",
                                                "right_knee", "right_hip"};

  // Temporary files for comparison
  const std::string full_dict_file = "/tmp/full_dictionary.mpack";
  const std::string delta_dict_file = "/tmp/delta_dictionary.mpack";

  // Open streams to output temporary files
  std::ofstream full_output(full_dict_file, std::ofstream::binary);
  std::ofstream delta_output(delta_dict_file, std::ofstream::binary);

  // Timing variables
  auto full_time_total = std::chrono::microseconds::zero();
  auto delta_time_total = std::chrono::microseconds::zero();
  double init_time = init_robot.get<double>("time");
  double dt = 0.001;  // seconds

  std::cout << "Starting delta compression comparison over " << kNumSteps
            << " steps...\n";

  for (int step = 0; step < kNumSteps; ++step) {
    // Update current dictionary with arbitrary values
    robot("time") = init_time + step * dt;
    robot("observation")("time") = init_robot.get<double>("time") + step * dt;
    for (const auto& servo_name : servo_names) {
      Dictionary& servo_action = robot("action")("servo")(servo_name);
      Dictionary& servo_observation = robot("observation")("servo")(servo_name);
      servo_action("feedforward_torque") = torque_dis(gen);
      servo_action("position") = pos_dis(gen);
      servo_action("velocity") = vel_dis(gen);
      servo_observation("position") = pos_dis(gen);
      servo_observation("torque") = torque_dis(gen);
      servo_observation("velocity") = vel_dis(gen);
    }

    // Serialize and write full dictionary
    auto start_full = std::chrono::high_resolution_clock::now();
    std::vector<char> full_buffer;
    size_t full_size = robot.serialize(full_buffer);
    full_output.write(full_buffer.data(), static_cast<int>(full_size));
    auto end_full = std::chrono::high_resolution_clock::now();
    full_time_total += std::chrono::duration_cast<std::chrono::microseconds>(
        end_full - start_full);

    // Compute difference and serialize to delta stream
    auto start_delta = std::chrono::high_resolution_clock::now();
    Dictionary delta = robot.difference(init_robot);
    std::vector<char> delta_buffer;
    size_t delta_size = delta.serialize(delta_buffer);
    delta_output.write(delta_buffer.data(), static_cast<int>(delta_size));
    auto end_delta = std::chrono::high_resolution_clock::now();
    delta_time_total += std::chrono::duration_cast<std::chrono::microseconds>(
        end_delta - start_delta);

    if ((step + 1) % (kNumSteps / 10) == 0) {
      std::cout << "Completed " << (step + 1) << " steps...\n";
    }
  }

  // Close output streams
  full_output.close();
  delta_output.close();

  // Compare memory usage
  std::uintmax_t full_file_size = std::filesystem::file_size(full_dict_file);
  std::uintmax_t delta_file_size = std::filesystem::file_size(delta_dict_file);

  std::cout << "\n=== Memory performance ===\n";
  std::cout << "Full dictionary file size: " << full_file_size << " bytes\n";
  std::cout << "Delta dictionary file size: " << delta_file_size << " bytes\n";
  std::cout << "Compression ratio: "
            << ((1.0 * full_file_size) / delta_file_size) << "\n";
  std::cout << "Space saved: " << (full_file_size - delta_file_size)
            << " bytes ("
            << (100.0 * (full_file_size - delta_file_size) / full_file_size)
            << "%)\n";

  // Compare times
  std::cout << "\n=== Time performance  ===\n";
  std::cout << "Full dictionary avg time per step: "
            << (full_time_total.count() / kNumSteps) << " μs\n";
  std::cout << "Delta dictionary avg time per step: "
            << (delta_time_total.count() / kNumSteps) << " μs\n";

  // Clean up temporary files
  std::filesystem::remove(full_dict_file);
  std::filesystem::remove(delta_dict_file);

  return EXIT_SUCCESS;
}

Dictionary make_big_robot_dictionary() {
  Dictionary robot;

  robot("action")("servo")("right_knee")("kd_scale") = 1.0;
  robot("action")("servo")("right_knee")("velocity") = 0.0;
  robot("action")("servo")("right_knee")("kp_scale") = 1.0;
  robot("action")("servo")("right_knee")("maximum_torque") = 1.0;
  robot("action")("servo")("right_knee")("feedforward_torque") = 0.0;

  robot("action")("servo")("right_wheel")("kd_scale") = 1.0;
  robot("action")("servo")("right_wheel")("velocity") = 0.0;
  robot("action")("servo")("right_wheel")("kp_scale") = 1.0;
  robot("action")("servo")("right_wheel")("maximum_torque") = 1.0;
  robot("action")("servo")("right_wheel")("feedforward_torque") = 0.0;

  robot("action")("servo")("right_hip")("kd_scale") = 1.0;
  robot("action")("servo")("right_hip")("velocity") = 0.0;
  robot("action")("servo")("right_hip")("kp_scale") = 1.0;
  robot("action")("servo")("right_hip")("maximum_torque") = 1.0;
  robot("action")("servo")("right_hip")("feedforward_torque") = 0.0;

  robot("action")("servo")("left_wheel")("kd_scale") = 1.0;
  robot("action")("servo")("left_wheel")("velocity") = 0.0;
  robot("action")("servo")("left_wheel")("kp_scale") = 1.0;
  robot("action")("servo")("left_wheel")("maximum_torque") = 1.0;
  robot("action")("servo")("left_wheel")("feedforward_torque") = 0.0;

  robot("action")("servo")("left_knee")("kd_scale") = 1.0;
  robot("action")("servo")("left_knee")("velocity") = 0.0;
  robot("action")("servo")("left_knee")("kp_scale") = 1.0;
  robot("action")("servo")("left_knee")("maximum_torque") = 1.0;
  robot("action")("servo")("left_knee")("feedforward_torque") = 0.0;

  robot("action")("servo")("left_hip")("kd_scale") = 1.0;
  robot("action")("servo")("left_hip")("velocity") = 0.0;
  robot("action")("servo")("left_hip")("kp_scale") = 1.0;
  robot("action")("servo")("left_hip")("maximum_torque") = 1.0;
  robot("action")("servo")("left_hip")("feedforward_torque") = 0.0;

  robot("config")("wheel_odometry")("signed_radius")("right_wheel") = 0.0725;
  robot("config")("wheel_odometry")("signed_radius")("left_wheel") = -0.0725;
  robot("config")("floor_contact")("upper_leg_torque_threshold") = 10.0;

  Eigen::Matrix3d rotation_base_to_imu;
  rotation_base_to_imu << -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0;
  robot("config")("base_orientation")("rotation_base_to_imu") =
      rotation_base_to_imu;

  robot("config")("wheel_contact")("touchdown_inertia") = 0.004;
  robot("config")("wheel_contact")("min_touchdown_torque") = 0.015;
  robot("config")("wheel_contact")("min_touchdown_acceleration") = 2.0;
  robot("config")("wheel_contact")("liftoff_inertia") = 0.001;
  robot("config")("wheel_contact")("cutoff_period") = 0.2;

  robot("config")("bullet")("torque_control")("kd") = 1.0;
  robot("config")("bullet")("torque_control")("kp") = 20.0;

  Eigen::VectorXd joint_config(6);
  joint_config << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
  robot("config")("bullet")("reset")("joint_configuration") = joint_config;

  Eigen::Vector3d angular_vel_base(0.0, 0.0, 0.0);
  robot("config")("bullet")("reset")("angular_velocity_base_in_base") =
      angular_vel_base;

  Eigen::Vector3d linear_vel_base(0.0, 0.0, 0.0);
  robot("config")("bullet")("reset")("linear_velocity_base_to_world_in_world") =
      linear_vel_base;

  Eigen::Vector3d position_base(0.0, 0.0, 0.6);
  robot("config")("bullet")("reset")("position_base_in_world") = position_base;

  Eigen::Quaterniond orientation_base(1.0, 0.0, 0.0, 0.0);
  robot("config")("bullet")("reset")("orientation_base_in_world") =
      orientation_base;

  robot("config")("bullet")("gui") = true;
  robot("config")("bullet")("follower_camera") = false;

  robot("spine")("state_cycle_beginning") = 1;
  robot("spine")("rx_count") = 6;
  robot("spine")("logger_last_size") = 0;
  robot("spine")("state_cycle_end") = 2;
  robot("spine")("clock")("slack") = 0.000964;
  robot("spine")("clock")("skip_count") = 0;
  robot("spine")("clock")("measured_period") = 0.00099;

  robot("time") = 1731936372.679786;

  robot("observation")("wheel_odometry")("velocity") = 0.0;
  robot("observation")("wheel_odometry")("position") = 0.0;
  robot("observation")("floor_contact")("left_wheel")("inertia") = 0.0;
  robot("observation")("floor_contact")("left_wheel")("abs_torque") = 0.0;
  robot("observation")("floor_contact")("left_wheel")("contact") = false;
  robot("observation")("floor_contact")("left_wheel")("abs_acceleration") =
      0.0033693581209750527;
  robot("observation")("floor_contact")("right_wheel")("inertia") = 0.0;
  robot("observation")("floor_contact")("right_wheel")("abs_torque") = 0.0;
  robot("observation")("floor_contact")("right_wheel")("contact") = false;
  robot("observation")("floor_contact")("right_wheel")("abs_acceleration") =
      0.0011231193736583511;
  robot("observation")("floor_contact")("upper_leg_torque") = 0.0;
  robot("observation")("floor_contact")("contact") = false;

  Eigen::Vector3d angular_velocity(0.05545446658531275, -0.029179723431780196,
                                   -0.11170878031451718);
  robot("observation")("base_orientation")("angular_velocity") =
      angular_velocity;
  robot("observation")("base_orientation")("pitch") = 0.03826629806372296;

  robot("observation")("joystick")("square_button") = false;
  robot("observation")("joystick")("right_trigger") = -1.0;
  robot("observation")("joystick")("right_button") = false;
  robot("observation")("joystick")("left_trigger") = -1.0;

  Eigen::Vector2d pad_axis(0.0, 0.0);
  robot("observation")("joystick")("pad_axis") = pad_axis;

  Eigen::Vector2d right_axis(0.0, 0.0);
  robot("observation")("joystick")("right_axis") = right_axis;

  robot("observation")("joystick")("left_button") = false;
  robot("observation")("joystick")("triangle_button") = false;

  Eigen::Vector2d left_axis(0.0, 0.0);
  robot("observation")("joystick")("left_axis") = left_axis;

  robot("observation")("joystick")("cross_button") = false;
  robot("observation")("cpu_temperature") = 34.563;

  robot("observation")("servo")("left_wheel")("voltage") = 25.0;
  robot("observation")("servo")("left_wheel")("velocity") = 0.13477432483900212;
  robot("observation")("servo")("left_wheel")("torque") = 0.0;
  robot("observation")("servo")("left_wheel")("mode") = 0;
  robot("observation")("servo")("left_wheel")("temperature") = 22.0;
  robot("observation")("servo")("left_wheel")("position") = -0.4057052752845859;
  robot("observation")("servo")("left_wheel")("fault") = 0;

  robot("observation")("servo")("left_knee")("voltage") = 25.0;
  robot("observation")("servo")("left_knee")("velocity") = 0.007476990515543708;
  robot("observation")("servo")("left_knee")("torque") = 0.0;
  robot("observation")("servo")("left_knee")("mode") = 0;
  robot("observation")("servo")("left_knee")("temperature") = 20.0;
  robot("observation")("servo")("left_knee")("position") = -0.11328583108844795;
  robot("observation")("servo")("left_knee")("fault") = 0;

  robot("observation")("servo")("right_knee")("voltage") = 25.0;
  robot("observation")("servo")("right_knee")("velocity") = 0.0;
  robot("observation")("servo")("right_knee")("torque") = 0.0;
  robot("observation")("servo")("right_knee")("mode") = 0;
  robot("observation")("servo")("right_knee")("temperature") = 20.0;
  robot("observation")("servo")("right_knee")("position") = -0.0851371609122834;
  robot("observation")("servo")("right_knee")("fault") = 0;

  robot("observation")("servo")("right_wheel")("voltage") = 25.0;
  robot("observation")("servo")("right_wheel")("velocity") =
      -0.044924774946334046;
  robot("observation")("servo")("right_wheel")("torque") = 0.0;
  robot("observation")("servo")("right_wheel")("mode") = 0;
  robot("observation")("servo")("right_wheel")("temperature") = 21.0;
  robot("observation")("servo")("right_wheel")("position") =
      -1.3452928061202214;
  robot("observation")("servo")("right_wheel")("fault") = 0;

  robot("observation")("servo")("right_hip")("voltage") = 25.0;
  robot("observation")("servo")("right_hip")("velocity") =
      -0.007476990515543708;
  robot("observation")("servo")("right_hip")("torque") = 0.0;
  robot("observation")("servo")("right_hip")("mode") = 0;
  robot("observation")("servo")("right_hip")("temperature") = 20.0;
  robot("observation")("servo")("right_hip")("position") = -0.10398671683382217;
  robot("observation")("servo")("right_hip")("fault") = 0;

  robot("observation")("servo")("left_hip")("voltage") = 25.0;
  robot("observation")("servo")("left_hip")("velocity") = 0.02990796206217483;
  robot("observation")("servo")("left_hip")("torque") = 0.0;
  robot("observation")("servo")("left_hip")("mode") = 0;
  robot("observation")("servo")("left_hip")("temperature") = 20.0;
  robot("observation")("servo")("left_hip")("position") = 0.14237697906068944;
  robot("observation")("servo")("left_hip")("fault") = 0;

  Eigen::Vector3d raw_angular_velocity(
      -0.05545446658531275, -0.11170878031451718, -0.029179723431780196);
  robot("observation")("imu")("raw_angular_velocity") = raw_angular_velocity;

  Eigen::Vector3d linear_acceleration(0.582870364189148, -0.16759386658668518,
                                      0.43342703580856323);
  robot("observation")("imu")("linear_acceleration") = linear_acceleration;

  Eigen::Vector3d imu_angular_velocity(
      -0.05545446658531275, -0.11170878031451718, -0.029179723431780196);
  robot("observation")("imu")("angular_velocity") = imu_angular_velocity;

  Eigen::Vector3d raw_linear_acceleration(0.9581711397918002, 9.63417167222633,
                                          0.2897550135856335);
  robot("observation")("imu")("raw_linear_acceleration") =
      raw_linear_acceleration;

  Eigen::Quaterniond orientation(0.2956166044467877, -0.31553863745961175,
                                 -0.6269869498093612, 0.6480228053195634);
  robot("observation")("imu")("orientation") = orientation;

  robot("observation")("time") = 1731936372.679786;
  return robot;
}
