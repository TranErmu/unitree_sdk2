#include <iostream>
#include <string>
#include <vector>

#include <unitree/robot/channel/channel_factory.hpp>

#include "unitree/robot/h2x/arm/h2x_arm_action_api.hpp"
#include "unitree/robot/h2x/arm/h2x_arm_action_client.hpp"
#include "unitree/robot/h2x/arm/h2x_arm_action_error.hpp"

using namespace unitree::robot;
using namespace unitree::robot::h2x;

namespace {

void PrintUsage() {
  std::cout << "Usage: h2x_arm_action_example [options]\n"
            << "  -n, --network <iface>  dds interface, default empty\n"
            << "  -l, --list             list all preset actions\n"
            << "  -i, --id <int>         execute the preset action with this id, 99 releases\n"
            << "      --name <string>    execute a taught action by name\n"
            << "      --stop             stop the running taught action\n"
            << "      --raw              print the action list without parsing it\n"
            << "  -h, --help             show this help\n"
            << "\n"
            << "Preset actions run on the robot side through rt/arm_sdk.\n"
            << "The robot usually has to be in fsm 500/501/801, see the verify markers.\n";
}

void PrintError(int32_t ret) {
  switch (ret) {
    case UT_ROBOT_ARM_ACTION_ERR_ARMSDK:
      std::cerr << UT_ROBOT_ARM_ACTION_ERR_ARMSDK_DESC << std::endl;
      break;
    case UT_ROBOT_ARM_ACTION_ERR_HOLDING:
      std::cerr << UT_ROBOT_ARM_ACTION_ERR_HOLDING_DESC << std::endl;
      break;
    case UT_ROBOT_ARM_ACTION_ERR_INVALID_ACTION_ID:
      std::cerr << UT_ROBOT_ARM_ACTION_ERR_INVALID_ACTION_ID_DESC << std::endl;
      break;
    case UT_ROBOT_ARM_ACTION_ERR_INVALID_FSM_ID:
      std::cerr << "actions are only supported in fsm 500/501/801, fsm mode 0/3 in 801"
                << std::endl;
      break;
    default:
      std::cerr << "rpc failed, error code: " << ret << std::endl;
      break;
  }
}

std::string FindField(const std::string& block, const std::string& key) {
  const std::string quoted_key = "\"" + key + "\"";
  size_t pos = block.find(quoted_key);
  if (pos == std::string::npos) return "";
  pos = block.find(':', pos + quoted_key.size());
  if (pos == std::string::npos) return "";
  pos += 1;
  while (pos < block.size() && (block[pos] == ' ' || block[pos] == '\t')) ++pos;
  if (pos < block.size() && block[pos] == '"') {
    size_t end = block.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return block.substr(pos + 1, end - pos - 1);
  }
  size_t end = block.find_first_of(",}", pos);
  if (end == std::string::npos) end = block.size();
  return block.substr(pos, end - pos);
}

void PrintActionList(const std::string& data) {
  std::cout << "Raw: " << data << std::endl;

  // verify: H2X action list keeps the id and name fields of the G1 format
  const std::string fields[] = {"id", "name"};
  std::vector<std::vector<std::string>> rows;
  size_t pos = 0;
  while ((pos = data.find('{', pos)) != std::string::npos) {
    size_t end = data.find('}', pos);
    if (end == std::string::npos) break;
    const std::string block = data.substr(pos, end - pos + 1);
    std::vector<std::string> row;
    bool empty = true;
    for (const std::string& field : fields) {
      std::string value = FindField(block, field);
      empty = empty && value.empty();
      row.push_back(value);
    }
    if (!empty) rows.push_back(row);
    pos = end + 1;
  }

  if (rows.empty()) return;

  std::cout << "\n| id | name |\n| --- | --- |\n";
  for (const auto& row : rows) {
    std::cout << "| " << row[0] << " | " << row[1] << " |\n";
  }
}

}  // namespace

int main(int argc, char const* argv[]) {
  std::string network_interface;
  bool list_actions = false;
  bool raw = false;
  bool stop = false;
  bool has_id = false;
  int32_t action_id = 0;
  std::string action_name;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      PrintUsage();
      return 0;
    }
    if (arg == "-l" || arg == "--list") {
      list_actions = true;
      continue;
    }
    if (arg == "--raw") {
      raw = true;
      continue;
    }
    if (arg == "--stop") {
      stop = true;
      continue;
    }
    if (arg == "-n" || arg == "--network") {
      if (i + 1 >= argc) {
        std::cerr << "missing value for " << arg << std::endl;
        return 2;
      }
      network_interface = argv[++i];
      continue;
    }
    if (arg.compare(0, 10, "--network=") == 0) {
      network_interface = arg.substr(10);
      continue;
    }
    if (arg == "-i" || arg == "--id") {
      if (i + 1 >= argc) {
        std::cerr << "missing value for " << arg << std::endl;
        return 2;
      }
      arg = argv[++i];
      try {
        size_t pos = 0;
        action_id = std::stoi(arg, &pos);
        if (pos != arg.size()) throw std::invalid_argument("trailing characters");
      } catch (const std::exception&) {
        std::cerr << "invalid value for --id: '" << arg << "'" << std::endl;
        return 2;
      }
      has_id = true;
      continue;
    }
    if (arg.compare(0, 5, "--id=") == 0) {
      std::string text = arg.substr(5);
      try {
        size_t pos = 0;
        action_id = std::stoi(text, &pos);
        if (pos != text.size()) throw std::invalid_argument("trailing characters");
      } catch (const std::exception&) {
        std::cerr << "invalid value for --id: '" << text << "'" << std::endl;
        return 2;
      }
      has_id = true;
      continue;
    }
    if (arg == "--name") {
      if (i + 1 >= argc) {
        std::cerr << "missing value for --name" << std::endl;
        return 2;
      }
      action_name = argv[++i];
      continue;
    }
    if (arg.compare(0, 7, "--name=") == 0) {
      action_name = arg.substr(7);
      continue;
    }
    std::cerr << "unknown option: " << arg << std::endl;
    return 2;
  }

  if (!list_actions && !has_id && action_name.empty() && !stop) {
    PrintUsage();
    return 2;
  }

  try {
    unitree::robot::ChannelFactory::Instance()->Init(0, network_interface);
  } catch (const std::exception& e) {
    std::cerr << "dds init failed: " << e.what() << std::endl;
    return 1;
  }

  H2XArmActionClient client;
  client.Init();
  client.SetTimeout(10.0f);

  if (list_actions) {
    std::string data;
    int32_t ret = client.GetActionList(data);
    if (ret != 0) {
      PrintError(ret);
      return 1;
    }
    if (raw) {
      std::cout << "Raw: " << data << std::endl;
    } else {
      PrintActionList(data);
    }
    return 0;
  }

  if (has_id) {
    int32_t ret = client.ExecuteAction(action_id);
    if (ret != 0) {
      PrintError(ret);
      return 1;
    }
    std::cout << "action " << action_id << " started" << std::endl;
    return 0;
  }

  if (!action_name.empty()) {
    int32_t ret = client.ExecuteAction(action_name);
    if (ret != 0) {
      std::cerr << "execute custom action failed, error code: " << ret << std::endl;
      return 1;
    }
    std::cout << "custom action " << action_name << " started" << std::endl;
    return 0;
  }

  int32_t ret = client.StopCustomAction();
  if (ret != 0) {
    PrintError(ret);
    return 1;
  }
  std::cout << "custom action stopped" << std::endl;
  return 0;
}
