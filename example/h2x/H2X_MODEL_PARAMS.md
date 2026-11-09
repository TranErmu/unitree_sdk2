# H2X 机型相关量清单

> **状态：低层接口参数已确认与 G1 完全一致；高层示例已按 G1 改写，H2X 上仍待验证。**
> `include/unitree/robot/h2x/` 下的头文件是 `include/unitree/robot/g1/` 的同构拷贝（只有命名空间、
> 包含路径和类名不同），高层示例同样取自 `example/g1/`。
> 低层项代码中已无 `verify` 标记；高层仍有 13 处待验证标记，见 README 第 4 节。
>
> 用途：等 H2X 接口定稿后，逐项核对并填最后一列的"H2X 结论"。
>
> 用法与测试步骤见 [README.md](README.md)。
>
> 本文只做记录，不含任何代码改动。

## 0. 优先级

| 优先级 | 项目 | 若取值错误 |
| --- | --- | --- |
| P0 | 电机总数 29（已确认，同 G1） | 数组长度、`LowCmd` 校验、CRC 全部失效，可能越界 |
| P0 | 踝关节序号 pitch=4 / roll=5（已确认，同 G1） | 踝关节正反颠倒，直接顶限位 |
| P0 | 腰部 12/13/14 | 腰部目标值需 A/B 换算，直接发会得到错误姿态 |
| P0 | Kp / Kd 表（已确认，取 G1 数值） | 过柔撑不住或过刚振荡 |
| P0 | FSM 编号（start 500、arm action 500/501/801） | 服务拒绝或切到错误状态 |
| P1 | loco / arm 服务名、API 版本、api id | RPC 直接失败（对照 `include/unitree/robot/g1` 的取值） |
| P1 | 话题名 | 收不到状态或指令无人接收 |
| P2 | 动作幅度、阶段时长、回放速度 | 示例能跑，但不符合 H2X 的实际尺寸/臂长 |
| P2 | 安全阈值 | 误报警或漏报警 |

## 1. 关节与整机结构

| 项目 | 当前值 | G1 对照 | 代码位置 |
| --- | --- | --- | --- |
| 电机总数 | 29（同上，已确认） | 29 | `h2x_ankle_swing_example.cpp`（`H2X_NUM_MOTOR`）、`h2x_dual_arm_example.cpp`、`h2x_userctrl_dds_example.cpp` |
| 下肢 0–11 | hip pitch/roll/yaw、knee、ankle、左右镜像 | 相同 | `h2x_ankle_swing_example.cpp:92` 起 |
| 踝关节序号 | pitch = 4，roll = 5（同上，已确认） | pitch = 4，roll = 5 | `h2x_ankle_swing_example.cpp`（`H2XJointIndex`） |
| 腰部 12/13/14 | yaw / roll(A) / pitch(B)（同上，同 G1） | yaw / roll(A) / pitch(B) | `h2x_ankle_swing_example.cpp`（`H2XJointIndex`） |
| 上肢 15–28 | 每臂 7 DOF：shoulder p/r/y、elbow、wrist r/p/y | 同左 | `h2x_arm_sdk_dds_example.cpp`、`h2x_userctrl_dds_example.cpp` |
| 头部 29/30 | 不存在（29 电机） | 无头部关节 | — |
| arm_sdk 混合权重关节 | 下标 29（`kNotUsedJoint`）的 `q` | 相同 | `h2x_arm_sdk_dds_example.cpp`（`kNotUsedJoint`） |
| arm_sdk 控制的关节 | 14 个臂关节 15–28 + 3 个腰关节 12–14，共 17 个 | 相同 | `h2x_arm_sdk_dds_example.cpp`（`arm_joints`） |

## 2. 控制参数

| 项目 | 当前值 | 代码位置 |
| --- | --- | --- |
| 控制周期 | 2 ms（500 Hz）；arm_sdk 示例为 20 ms（50 Hz） | `h2x_ankle_swing_example.cpp`、`h2x_arm_sdk_dds_example.cpp`（`control_dt`） |
| 模式编码 | `mode_pr`：0 = PR 串联，1 = AB 并联；电机 `mode = 1` | `h2x_ankle_swing_example.cpp:86-89` |
| Kp / Kd | ankle_swing、userctrl 用平表：60,60,60,100,40,40 / 60,60,60,100,40,40 / 60,40,40 / 40 ×14；dual_arm 按电机类型：臂 40/1，腿 M 40/1、L 100/1、S 40/1；arm_sdk 用 60/1.5 | `h2x_ankle_swing_example.cpp`（`Kp`）、`h2x_dual_arm_example.cpp`（`GetMotorKp`）、`h2x_arm_sdk_dds_example.cpp`（`kp`） |
| 关节速度限幅 | 0.5 rad/s | `h2x_userctrl_dds_example.cpp:144` |
| 阶段时长 | 零位过渡 3 s；userctrl 过渡 2 s + 抬臂 5 s + 放下 5 s；arm_sdk 初始化 2 s + 抬臂 5 s + 放下 5 s + 权重下降 2 s；阻尼退出 0.5 s | `h2x_dual_arm_example.cpp`、`h2x_userctrl_dds_example.cpp`（`init_time` / `period`） |

## 3. 服务与 FSM

| 项目 | 当前值 | G1 对照 | 代码位置 |
| --- | --- | --- | --- |
| loco 服务名 / 版本 | `sport` / `1.0.0.0` | 相同 | `h2x/loco/h2x_loco_api.hpp` |
| loco api id | 7001–7006、7100–7107、7110、7111 | 相同 | `h2x/loco/h2x_loco_api.hpp` |
| arm action 服务名 / 版本 | `arm` / `1.0.0.14` | 相同 | `h2x/arm/h2x_arm_action_api.hpp` |
| arm action api id | 7106 / 7107 / 7108 / 7113 | 相同 | `h2x/arm/h2x_arm_action_api.hpp` |
| FSM 编号 | zero torque 0、damp 1、squat 2、sit 3、stand_up 4、**start 500** | 相同 | `h2x_loco_client_example.cpp`（`RunCommand`） |
| arm_sdk 允许的 FSM | 示例不检查 FSM，只写 `rt/arm_sdk` | 相同 | `h2x_arm_sdk_dds_example.cpp` |
| arm action 允许的 FSM | 500 / 501 / 801，801 下 fsm mode 限 0 / 3 | 相同 | `h2x/arm/h2x_arm_action_error.hpp` |

## 4. 话题

| 话题 | 用途 | 使用位置 |
| --- | --- | --- |
| `rt/lowcmd` | 低层整机指令 | ankle_swing、dual_arm |
| `rt/user_lowcmd` | 用户模式整机指令 | userctrl（G1 机制，待验证） |
| `rt/lowstate` | 整机状态 | 全部低层示例、terminations |
| `rt/secondary_imu` | 躯干姿态 | ankle_swing |
| `rt/arm_sdk` | 上肢外部控制 | arm_sdk 示例（配合 `rt/lowstate` 读状态） |

G1-D 的 `rt/waistpitchcmd` / `rt/waistpitchstate` 未使用：腰部走 `rt/lowcmd` 的 12~14（与 G1 低层一致）。

## 5. 动作数据与幅度

| 项目 | 当前值 | 代码位置 |
| --- | --- | --- |
| motion.seq 元数据 | 500 Hz、3817 帧、`num_parts: 14`、1.0 MB | `behavior_lib/motion.seq:3834-3838`（JointDisplacement 分量） |
| 轨迹对应关节 | 15–28（左臂 15–21、右臂 22–28） | `h2x_dual_arm_example.cpp:90-104`（`H2XJointValidIndex`） |
| 回放速度 | 1 倍（与 G1 的 dual_arm 相同） | `h2x_dual_arm_example.cpp:345` |
| ankle_swing 摆幅 | pitch ±30°、roll ±10°，1 Hz 正弦，PR 模式 | `h2x_ankle_swing_example.cpp:352-353` |
| 抬臂目标（arm_sdk） | shoulder roll ±90°、elbow 90°、waist 0，其余 0 | `h2x_arm_sdk_dds_example.cpp`（`target_pos`） |
| 抬臂目标（userctrl） | init：髋 -0.1、膝 0.3、踝 -0.2、肩 0.2、肘 0.9；target：髋 -1.0、膝 1.5、踝 0.2、肩水平 0.2、肘 0 | `h2x_userctrl_dds_example.cpp:150-159` |
| userctrl 初始位姿 | 全零（先发 kp=0 的零位，再切用户模式） | `h2x_userctrl_dds_example.cpp` |

## 6. 安全阈值

| 项目 | 示例中的取值 | 头文件默认值 | 代码位置 |
| --- | --- | --- | --- |
| 姿态偏离 | 1.0 rad | 1.0 rad | `terminations.cpp:39` |
| 通信超时 | 1000 ms | 1000 ms | `terminations.cpp:42` |
| 关节速度 | 未启用 | 10 rad/s | `h2x/common/terminations.hpp` |
| 机身角速度 | 未启用 | 6 rad/s | 同上 |
| 绕组温度 | 未启用 | 120 ℃ | 同上 |
| 外壳温度 | 未启用 | 85 ℃ | 同上 |
| 电池 SOC | 未启用 | 20% | 同上 |

## 7. 其它

| 项目 | 当前值 | 代码位置 |
| --- | --- | --- |
| 遥控器报文 | `wireless_remote` 40 字节（当前用 24 字节） | `low_level/gamepad.hpp:34,51` |

## 8. 核对记录（待填）

低层接口参数：已确认与 G1 完全一致（官方消息），代码已按 G1 改完，无需再核对。

| 项目 | H2X 实际值 | 结论 | 核对人 / 日期 |
| --- | --- | --- | --- |
| 电机总数 | | | |
| 踝关节序号 | | | |
| 腰部关节模式 | | | |
| 上肢关节范围 | | | |
| arm_sdk 权重关节 | | | |
| Kp / Kd 表 | | | |
| 控制频率 | | | |
| loco 服务版本 | | | |
| loco api id | | | |
| arm 服务版本 / id | | | |
| FSM 编号 | | | |
| arm action 允许的 FSM | | | |
| 话题名 | | | |
| 动作幅度 | | | |
| 安全阈值 | | | |
