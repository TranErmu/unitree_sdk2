#include <chrono>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <unitree/robot/channel/channel_factory.hpp>
#include "unitree/robot/h2x/loco/h2x_loco_api.hpp"
#include "unitree/robot/h2x/loco/h2x_loco_client.hpp"

using unitree::robot::h2x::LocoClient;

namespace {

const char kDefaultNetworkInterface[] = "lo";

const std::set<std::string> kKnownCommands = {
    // queries
    "get_balance_mode", "get_fsm_id", "get_fsm_mode", "get_mimic_motion",
    "get_phase", "get_stand_height", "get_swing_height",
    // parameters
    "continous_gait", "move", "set_balance_mode", "set_fsm_id",
    "set_speed_mode", "set_stand_height", "set_swing_height", "set_task_id",
    "set_velocity", "switch_move_mode",
    // actions
    "balance_stand", "damp", "high_stand", "low_stand", "shake_hand", "sit",
    "squat", "stand_up", "start", "stop_move", "wave_hand",
    "wave_hand_with_turn", "zero_torque",
};

void PrintUsage() {
  std::cout
      << "Usage: h2x_loco_client [--network_interface=<iface>] <command>[=<param>] ...\n"
      << "  --network_interface=<iface>  dds interface, default " << kDefaultNetworkInterface << "\n"
      << "  -n <iface>                   same as --network_interface\n"
      << "\n"
      << "Commands run in the order they are given, each one is one rpc call.\n"
      << "\n"
      << "Queries:\n"
      << "  --get_fsm_id --get_fsm_mode --get_balance_mode\n"
      << "  --get_swing_height --get_stand_height --get_phase --get_mimic_motion\n"
      << "\n"
      << "Parameters:\n"
      << "  --set_fsm_id=<int> --set_balance_mode=<int>\n"
      << "  --set_swing_height=<float> --set_stand_height=<float>\n"
      << "  --set_velocity=<vx vy omega [duration]> --move=<vx vy omega>\n"
      << "  --set_task_id=<int> --set_speed_mode=<int>\n"
      << "  --continous_gait=<true|false> --switch_move_mode=<true|false>\n"
      << "\n"
      << "Actions:\n"
      << "  --damp --start --squat --sit --stand_up --zero_torque --stop_move\n"
      << "  --high_stand --low_stand --balance_stand\n"
      << "  --wave_hand --wave_hand_with_turn --shake_hand\n"
      << "\n"
      << "Options:\n"
      << "  -h, --help  show this help\n";
}

bool ParseInt(const std::string& text, int& value) {
  try {
    size_t pos = 0;
    value = std::stoi(text, &pos);
    return pos == text.size();
  } catch (const std::exception&) {
    return false;
  }
}

bool ParseFloat(const std::string& text, float& value) {
  try {
    size_t pos = 0;
    value = std::stof(text, &pos);
    return pos == text.size();
  } catch (const std::exception&) {
    return false;
  }
}

bool ParseFloats(const std::string& text, std::vector<float>& values) {
  std::istringstream stream(text);
  float value = 0.0f;
  while (stream >> value) {
    values.push_back(value);
  }
  return !values.empty() && stream.eof();
}

bool ParseBool(const std::string& text, bool& value) {
  if (text == "true") {
    value = true;
    return true;
  }
  if (text == "false") {
    value = false;
    return true;
  }
  return false;
}

bool CheckRet(const std::string& name, int32_t ret) {
  if (ret != 0) {
    std::cerr << name << " failed, error code: " << ret << std::endl;
    return false;
  }
  return true;
}

bool RunCommand(LocoClient& client, const std::string& name,
                const std::string& value) {
  if (name == "get_fsm_id") {
    int fsm_id = 0;
    if (!CheckRet(name, client.GetFsmId(fsm_id))) return false;
    std::cout << "fsm_id: " << fsm_id << std::endl;
    return true;
  }
  if (name == "get_fsm_mode") {
    int fsm_mode = 0;
    if (!CheckRet(name, client.GetFsmMode(fsm_mode))) return false;
    std::cout << "fsm_mode: " << fsm_mode << std::endl;
    return true;
  }
  if (name == "get_balance_mode") {
    int balance_mode = 0;
    if (!CheckRet(name, client.GetBalanceMode(balance_mode))) return false;
    std::cout << "balance_mode: " << balance_mode << std::endl;
    return true;
  }
  if (name == "get_swing_height") {
    float swing_height = 0.0f;
    if (!CheckRet(name, client.GetSwingHeight(swing_height))) return false;
    std::cout << "swing_height: " << swing_height << std::endl;
    return true;
  }
  if (name == "get_stand_height") {
    float stand_height = 0.0f;
    if (!CheckRet(name, client.GetStandHeight(stand_height))) return false;
    std::cout << "stand_height: " << stand_height << std::endl;
    return true;
  }
  if (name == "get_phase") {
    std::vector<float> phase;
    if (!CheckRet(name, client.GetPhase(phase))) return false;
    std::cout << "phase:";
    for (float value : phase) std::cout << " " << value;
    std::cout << std::endl;
    return true;
  }
  if (name == "get_mimic_motion") {
    // verify: H2X exposes the mimic motion api
    std::string data;
    if (!CheckRet(name, client.GetMimicMotion(data))) return false;
    std::cout << "mimic_motion: " << data << std::endl;
    return true;
  }
  if (name == "set_fsm_id") {
    int fsm_id = 0;
    if (!ParseInt(value, fsm_id)) {
      std::cerr << "invalid value for set_fsm_id: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.SetFsmId(fsm_id))) return false;
    std::cout << "fsm_id set to " << fsm_id << std::endl;
    return true;
  }
  if (name == "set_balance_mode") {
    int balance_mode = 0;
    if (!ParseInt(value, balance_mode)) {
      std::cerr << "invalid value for set_balance_mode: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.SetBalanceMode(balance_mode))) return false;
    std::cout << "balance_mode set to " << balance_mode << std::endl;
    return true;
  }
  if (name == "set_swing_height") {
    float swing_height = 0.0f;
    if (!ParseFloat(value, swing_height)) {
      std::cerr << "invalid value for set_swing_height: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.SetSwingHeight(swing_height))) return false;
    std::cout << "swing_height set to " << swing_height << std::endl;
    return true;
  }
  if (name == "set_stand_height") {
    float stand_height = 0.0f;
    if (!ParseFloat(value, stand_height)) {
      std::cerr << "invalid value for set_stand_height: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.SetStandHeight(stand_height))) return false;
    std::cout << "stand_height set to " << stand_height << std::endl;
    return true;
  }
  if (name == "set_velocity") {
    std::vector<float> param;
    if (!ParseFloats(value, param) || (param.size() != 3 && param.size() != 4)) {
      std::cerr << "invalid value for set_velocity: '" << value << "'" << std::endl;
      return false;
    }
    float duration = param.size() == 4 ? param[3] : 1.0f;
    if (!CheckRet(name, client.SetVelocity(param[0], param[1], param[2], duration))) {
      return false;
    }
    std::cout << "velocity set to " << param[0] << " " << param[1] << " "
              << param[2] << " for " << duration << " s" << std::endl;
    return true;
  }
  if (name == "move") {
    std::vector<float> param;
    if (!ParseFloats(value, param) || param.size() != 3) {
      std::cerr << "invalid value for move: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.Move(param[0], param[1], param[2]))) return false;
    std::cout << "move " << param[0] << " " << param[1] << " " << param[2]
              << std::endl;
    return true;
  }
  if (name == "set_task_id") {
    int task_id = 0;
    if (!ParseInt(value, task_id)) {
      std::cerr << "invalid value for set_task_id: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.SetTaskId(task_id))) return false;
    std::cout << "task_id set to " << task_id << std::endl;
    return true;
  }
  if (name == "set_speed_mode") {
    // verify: H2X speed mode ids follow G1
    int speed_mode = 0;
    if (!ParseInt(value, speed_mode)) {
      std::cerr << "invalid value for set_speed_mode: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.SetSpeedMode(speed_mode))) return false;
    std::cout << "speed_mode set to " << speed_mode << std::endl;
    return true;
  }
  if (name == "continous_gait") {
    bool flag = false;
    if (!ParseBool(value, flag)) {
      std::cerr << "invalid value for continous_gait: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.ContinuousGait(flag))) return false;
    std::cout << "continous_gait set to " << (flag ? "true" : "false") << std::endl;
    return true;
  }
  if (name == "switch_move_mode") {
    bool flag = false;
    if (!ParseBool(value, flag)) {
      std::cerr << "invalid value for switch_move_mode: '" << value << "'" << std::endl;
      return false;
    }
    if (!CheckRet(name, client.SwitchMoveMode(flag))) return false;
    std::cout << "move mode switched, locked: " << (flag ? "true" : "false")
              << std::endl;
    return true;
  }
  if (name == "damp") {
    return CheckRet(name, client.Damp());
  }
  if (name == "start") {
    // verify: G1 start fsm is 500, H2X unconfirmed
    return CheckRet(name, client.Start());
  }
  if (name == "squat") {
    return CheckRet(name, client.Squat());
  }
  if (name == "sit") {
    return CheckRet(name, client.Sit());
  }
  if (name == "stand_up") {
    // verify: G1 stand up fsm is 4, H2X unconfirmed
    return CheckRet(name, client.StandUp());
  }
  if (name == "zero_torque") {
    return CheckRet(name, client.ZeroTorque());
  }
  if (name == "stop_move") {
    return CheckRet(name, client.StopMove());
  }
  if (name == "high_stand") {
    return CheckRet(name, client.HighStand());
  }
  if (name == "low_stand") {
    return CheckRet(name, client.LowStand());
  }
  if (name == "balance_stand") {
    return CheckRet(name, client.BalanceStand());
  }
  if (name == "wave_hand") {
    // verify: G1 task ids, wave is 0 and 1
    return CheckRet(name, client.WaveHand());
  }
  if (name == "wave_hand_with_turn") {
    return CheckRet(name, client.WaveHand(true));
  }
  if (name == "shake_hand") {
    // verify: G1 task ids, shake hand is 2 and 3
    if (!CheckRet(name, client.ShakeHand(0))) return false;
    std::cout << "shake hand starts, waiting 10 s for the end" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return CheckRet(name, client.ShakeHand(1));
  }
  return false;
}

}  // namespace

int main(int argc, char const* argv[]) {
  if (argc < 2) {
    PrintUsage();
    return 0;
  }

  std::string network_interface = kDefaultNetworkInterface;
  std::vector<std::pair<std::string, std::string>> commands;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      PrintUsage();
      return 0;
    }
    if (arg == "-n") {
      if (i + 1 >= argc) {
        std::cerr << "missing value for -n" << std::endl;
        return 1;
      }
      network_interface = argv[++i];
      continue;
    }
    if (arg.compare(0, 20, "--network_interface=") == 0) {
      network_interface = arg.substr(20);
      continue;
    }
    if (arg.compare(0, 10, "--network=") == 0) {
      network_interface = arg.substr(10);
      continue;
    }
    if (arg.compare(0, 2, "--") != 0) {
      std::cerr << "unexpected argument: " << arg << std::endl;
      return 1;
    }

    std::string name = arg.substr(2);
    std::string value;
    size_t pos = name.find('=');
    if (pos != std::string::npos) {
      value = name.substr(pos + 1);
      name = name.substr(0, pos);
    }
    if (kKnownCommands.find(name) == kKnownCommands.end()) {
      std::cerr << "unknown command: " << name << std::endl;
      return 1;
    }
    commands.emplace_back(name, value);
  }

  try {
    unitree::robot::ChannelFactory::Instance()->Init(0, network_interface);
  } catch (const std::exception& e) {
    std::cerr << "dds init failed: " << e.what() << std::endl;
    return 1;
  }

  // verify: H2X exposes the sport service with version 1.0.0.0
  LocoClient client;
  client.Init();
  client.SetTimeout(10.0f);

  for (const auto& command : commands) {
    std::cout << "command: " << command.first << std::endl;
    if (!RunCommand(client, command.first, command.second)) {
      return 1;
    }
  }

  return 0;
}
