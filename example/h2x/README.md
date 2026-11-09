# H2X 示例使用说明

> **注意：H2X 的接口尚未确认。** 本目录的代码按 H2 的接口编写（H2X 与 H2 的关系未知），
> 每条命令、每个默认值都记录在 [H2X_MODEL_PARAMS.md](H2X_MODEL_PARAMS.md) 里。
> 真机上先跑第 3 节的只读步骤，确认接口一致后再往下走。

## 1. 前提

- 编译安装：`./local-build.sh`（本机）或 `./local-build.sh -t aarch64`（交叉编译）
- 产物：`output/<arch>/bin/h2x_*`；头文件：`output/<arch>/include/unitree/robot/h2x/`
- 运行前先 `source output/<arch>/env.sh`，或者用完整路径运行
- 第一个参数是**网卡名**（机器人所在网卡，用 `ip link` 查），填错时程序打印
  `dds init failed: ...` 并以退出码 1 结束，不会崩溃

## 2. 程序一览

| 程序 | 作用 | 是否驱动电机 | 前置状态 |
| --- | --- | --- | --- |
| `h2x_loco_client` | 运动服务 RPC（状态查询/切换） | 否（除 `--move` 等运动命令） | 运动服务正常运行 |
| `h2x_arm_action_example` | 上肢预设动作 RPC | 是（动作由机器人侧执行） | FSM 500/501/801（待验证） |
| `h2x_arm_sdk_dds_example` | 经 `rt/arm_sdk` 控制双臂和腰部 | 是 | 运动服务正常运行 |
| `h2x_userctrl_dds_example` | 经 `rt/user_lowcmd` + `SwitchToUserCtrl` 抬放双臂 | 是 | FSM 为 1，吊装/有保护 |
| `h2x_ankle_swing_example` | 低层踝关节正弦摆动 | 是 | 吊装/有保护 |
| `h2x_dual_arm_example` | 回放 `motion.seq` 双臂轨迹 | 是 | 吊装/有保护 |
| `h2x_termination` | 只读订阅 `rt/lowstate`，检查终止条件 | 否 | 无 |

## 3. 测试步骤

### 3.1 编译验证（不需要机器人）

```bash
./local-build.sh
./local-build.sh -t aarch64
file output/aarch64/bin/h2x_loco_client        # 应为 ARM aarch64
ls output/aarch64/bin/h2x_* | wc -l            # 应为 7 个
```

`h2x_dual_arm_example` 构建时用 yaml-cpp，链接静态库，产物中不应出现 `libyaml-cpp.so`；
若找不到 yaml-cpp，该目标不构建，其余示例不受影响：

```bash
aarch64-linux-gnu-objdump -p output/aarch64/bin/h2x_dual_arm_example | grep NEEDED
```

### 3.2 离线验证（不需要机器人，验参数处理）

```bash
h2x_arm_action_example            # 打印用法，退出码 2
h2x_ankle_swing_example           # 打印用法，退出码 0
h2x_dual_arm_example              # 打印用法，退出码 0
h2x_userctrl_dds_example          # 打印用法，退出码 1
h2x_loco_client                   # 无命令时只初始化 DDS，退出码 0
```

### 3.3 只读验证（接机器人，第一步）

只查询、不改状态，用来判断 H2X 的服务是否与 G1 一致。命令列表用 `-h` 查看：

```bash
h2x_loco_client -h
```

```bash
# 状态与能力查询
h2x_loco_client --network_interface=eth0 --get_fsm_id --get_fsm_mode \
    --get_balance_mode --get_stand_height --get_swing_height --get_phase

# 上肢动作表（只查询，不执行）
h2x_arm_action_example -n eth0 --list

# 订阅 rt/lowstate，只打印告警，不写任何指令
h2x_termination -n eth0
```

`h2x_loco_client` 每条命令都会打印 `command: <name>` 和返回值；RPC 失败时打印
`<name> failed, error code: <n>` 并以退出码 1 结束，进程不崩。

- 网卡参数是 `--network_interface=<iface>`（也支持 `-n`），不写时默认 `lo`（连不到机器人）
- 未知命令（含拼写错误）会打印 `unknown command: xxx` 并以退出码 1 结束

`--list` 默认打印 `id` / `name` 表格，表格之后固定打印 `Raw:` 原始 JSON；
只想要原始数据用 `--raw`。响应布局不认识时只打印原始 JSON，不丢信息。
字段结构取自 G1，H2X 未验证。

### 3.4 服务级动作（低风险）

```bash
h2x_loco_client --network_interface=eth0 --get_fsm_id    # 先确认当前状态
h2x_loco_client --network_interface=eth0 --damp
h2x_loco_client --network_interface=eth0 --stand_up
h2x_loco_client --network_interface=eth0 --set_velocity="0.2 0 0 1.0"
```

```bash
h2x_arm_action_example -n eth0 -i 15      # 执行一个预设动作
h2x_arm_action_example -n eth0 -i 99      # 释放手臂
```

### 3.5 上肢 arm_sdk

```bash
h2x_arm_sdk_dds_example eth0
```

参数既可以写成 `-n eth0`，也可以写成 `--network eth0`。
示例不检查 FSM，直接把 14 个臂关节和 3 个腰关节写到 `rt/arm_sdk`，
最后一个槽位（下标 29）是混合权重：

程序会提示按 ENTER，然后依次执行：零位 2 s → 抬臂 5 s → 放下 5 s → 权重降到 0 退出。

### 3.6 低层控制（必须吊装或有保护）

逐个运行，一次只跑一个。四个低层程序都支持 `-h` / `--help` 查看用法和分阶段说明：

```bash
h2x_ankle_swing_example -h
h2x_dual_arm_example -h
h2x_userctrl_dds_example -h
h2x_termination -h
```

```bash
h2x_ankle_swing_example eth0    # 踝关节 PR/AB 两段摆动，各 3 s 循环
h2x_dual_arm_example eth0       # 回放 motion.seq 的 14 个上肢关节，循环播放
h2x_userctrl_dds_example eth0   # 经 rt/user_lowcmd 抬臂再放下，每步按 ENTER
```

`ankle_swing` / `dual_arm` 先释放运动服务（日志打印 `Releasing motion service: ...`），
再用 `rt/lowcmd` 直接驱动关节，`Ctrl+C` 后进入阻尼（`kp=0`）并退出。
`userctrl` 不释放运动服务，而是调用 `SWITCH_TO_USER_CTRL` 切到用户模式，
结束时用 `SWITCH_TO_INTERNAL_CTRL` 切回；这条通道是 G1 机制，H2X 上待验证（第 4 节）。

低层接口参数已确认与 G1 完全一致：29 个电机、踝 pitch=4 / roll=5、
没有头部关节。因此 `ankle_swing` / `dual_arm` / `userctrl` 的电机数与关节序号
与 G1 同名示例相同，`dual_arm` 回放的仍是 `motion.seq` 里 14 个上肢关节。

### 3.7 回填接口结论

把 3.3 的输出和真机表现记到 [H2X_MODEL_PARAMS.md](H2X_MODEL_PARAMS.md) 第 8 节的核对表里。
若某项不一致，对应示例需要按 H2X 的实际值修改，而不是改机器人。逐项验证方法见第 4 节。

## 4. 逐项验证 verify 标记

低层接口已确认与 G1 一致，剩下的待确认项在高层（loco / arm action / arm_sdk）
和 `userctrl` 用的 G1 user ctrl 通道，共 13 处 `// verify:`。
验证就是把这些标记逐条变成实测结论。

### 查找

```bash
grep -rn "// verify:" example/h2x          # 全部
grep -rn "// verify: fsm" example/h2x      # 按类型
```

### 分布

| 程序 | 标记数 | 待确认内容 |
| --- | --- | --- |
| `h2x_loco_client` | 7 | 服务名/版本、start 与 stand_up 的 FSM 编号、task id、speed mode、mimic motion 接口 |
| `h2x_arm_action_example` | 1 | 动作表字段名 |
| `h2x_arm_sdk_dds_example` | 1 | `rt/arm_sdk` 通道与权重槽位 29 |
| `h2x_userctrl_dds_example` | 4 | `rt/user_lowcmd` 通道、`SWITCH_TO_USER_CTRL` / `SWITCH_TO_INTERNAL_CTRL` 接口、FSM 1 |

### 验证顺序与判据

| 顺序 | 类别 | 怎么验 | 通过判据 |
| --- | --- | --- | --- |
| 1 | 服务是否存在 | `--get_fsm_id`、`h2x_arm_action_example --list` | 两条 RPC 都返回 0，不出现 `error:3104` |
| 2 | FSM 编号 | `--get_fsm_id`，`--start` 后再查一次 | 站起流程走 500，`damp` 为 1、`squat` 为 2、`sit` 为 3 |
| 3 | 动作表字段 | 看 `--list` 的表格列 | 能解析出 `id` 与 `name`，否则只打印 `Raw:` |
| 4 | arm_sdk 权重关节 | 跑 `h2x_arm_sdk_dds_example`，逐步观察手臂 | 权重槽位 29 生效，手臂随指令抬起放下，结束时回到 0 |
| 5 | user ctrl 通道 | 在 passive FSM 下跑 `h2x_userctrl_dds_example` | `SwitchToUserCtrl` 返回 0，手臂按 init/target 位姿抬起放下，退出后回到内部模式 |
| 6 | 动作层语义 | 逐个执行 loco 动作 | 机器人反应与命令名一致（如 `shake_hand` 确实握手） |

第 1–3 步只读；第 4 步起会驱动电机，必须在吊装或有人看护的条件下做。

### 记录与收尾

- 结论填进 [H2X_MODEL_PARAMS.md](H2X_MODEL_PARAMS.md) 第 8 节的核对表
- 验证通过的标记改成实测值，例如 `// verify: G1 start fsm is 500` → `// verified: start fsm is 500`
- 验证不通过的，按实测值改代码，并把该行标成 `// changed: ...` 说明依据
- 一条 `// verify:` 还在，就说明这一项还没确认过

## 5. 常见问题

| 现象 | 原因 |
| --- | --- |
| `selected interface "lo" is not multicast-capable` | 网卡不可组播（如 `lo`），换成机器人网卡 |
| `dds init failed: ...` | 网卡名不存在或 DDS 初始化失败，程序直接退出；用 `ip link` 查正确名字 |
| RPC 返回 `error:3104` | 服务端没有该 API，或接口不匹配 |
| arm action 返回 7404 | FSM 不在允许集合内 |
| arm action 返回 7400 | `rt/arm_sdk` 被占用，先释放 |
| arm action 返回 7401 | 动作保持中，先执行 99 |
| 低层程序运行后关节不动 | 运动服务没释放成功，或被其他进程重新占用 |
| `Lost connection!` | 网线/网卡异常，或 `rt/lowstate` 没有数据 |

## 6. 安全

- 3.6 的低层程序绕过运动服务，平衡与限幅保护失效，只能在吊装或有保护措施时运行。
- 低层参数已确认与 G1 一致，但高层（FSM 编号、动作 id、task id）仍来自 H2，
  第一次运行请从 3.3 开始，逐级推进。
