#!/usr/bin/env bash
#
# local-build.sh — unitree_sdk2 本地构建: 编译 -> 安装到 ./output/<arch>/
#
#   ./local-build.sh                # Release 构建 (含 example), 产物 output/<arch>/
#   ./local-build.sh -d -j 8        # Debug 构建, 8 路并行
#   ./local-build.sh -c             # 先删 build/<arch>/ 再构建
#   ./local-build.sh -t arm64       # 交叉编译 aarch64 (等价于 -t aarch64)
#   ./local-build.sh --no-examples  # 只构建 SDK 本体
#   ./local-build.sh --all          # 构建全部 example (默认只构建 h2x 示例)
#
set -o errexit
set -o nounset
set -o pipefail

# ---------------------------------- 变量配置

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
HOST_ARCH="$(uname -m)"
ARCH="$HOST_ARCH"
BUILD_TYPE="Release"
BUILD_EXAMPLES="ON"
BUILD_ALL=0
JOBS="$(nproc 2>/dev/null || echo 4)"
CLEAN=0

# ---------------------------------- 参数解析

usage() {
    cat <<EOF
用法: ./local-build.sh [选项]

  -h, --help           显示帮助
  -d, --debug          Debug 构建 (默认 Release)
  -j, --jobs N         并行任务数 (默认: $(nproc 2>/dev/null || echo 4))
  -t, --target ARCH    目标架构: x86_64|amd64 | aarch64|arm64 (默认: 本机架构)
                       构建目录 ./build/ARCH, 产物目录 ./output/ARCH
  -c, --clean          构建前删除构建目录
  -E, --no-examples    不编译 example
  -a, --all            构建全部 example (默认只构建 example/h2x)
EOF
}

# 需要取值的选项: 校验后面确实还有参数
need() { [ $# -ge 2 ] || { echo "参数 $1 缺少取值 (使用 -h 查看帮助)" >&2; exit 2; }; }

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)        usage; exit 0 ;;
        -d|--debug)       BUILD_TYPE="Debug" ;;
        -j|--jobs)        need "$@"; JOBS="$2"; shift ;;
        -t|--target)      need "$@"; ARCH="$2"; shift ;;
        -c|--clean)       CLEAN=1 ;;
        -E|--no-examples) BUILD_EXAMPLES="OFF" ;;
        -a|--all)         BUILD_ALL=1 ;;
        *) echo "未知参数: $1 (使用 -h 查看帮助)" >&2; exit 2 ;;
    esac
    shift
done

# 架构别名归一化到仓库里的目录名: arm64|aarch64、amd64|x86_64
case "$ARCH" in
    arm64|aarch64) 
        ARCH="aarch64" 
        source "$HOME/.local/unitree/unitree-oss-sdp-1.0.1-linux-aarch64/unitreesdp-oss-env.sh"
        ;;
    amd64|x86_64)  
        ARCH="x86_64"
        source "$HOME/.local/unitree/unitree-oss-sdp-1.0.1-linux-x86_64/unitreesdp-oss-env.sh"
        ;;
esac

# 依赖 (yaml-cpp 等) 由 unitree 开源依赖产物提供, 按目标架构选择
# source "$HOME/.local/unitree/unitree-oss-sdp-1.0.1-linux-x86_64"

# 目录按目标架构分层, 因此要在解析完 -t 之后再确定
BUILD_DIR="$SCRIPT_DIR/build/$ARCH"
OUTPUT_DIR="$SCRIPT_DIR/output/$ARCH"

# ---------------------------------- main
## 产物输出到 ./output/<arch>/

# 前置检查: cmake 与目标架构的预编译库
command -v cmake >/dev/null 2>&1 || { echo "未找到 cmake, 请先安装" >&2; exit 1; }
SDK_LIB="$SCRIPT_DIR/lib/$ARCH/libunitree_sdk2.a"
if [ ! -f "$SDK_LIB" ]; then
    echo "未找到当前架构($ARCH)的预编译库: $SDK_LIB" >&2
    echo "可用架构: $(ls -m -- "$SCRIPT_DIR/lib")" >&2
    exit 1
fi

[ "$CLEAN" -eq 1 ] && rm -rf -- "$BUILD_DIR" "$OUTPUT_DIR"
mkdir -p -- "$BUILD_DIR" "$OUTPUT_DIR"

# 目标架构与本机不一致时使用 ${ARCH}-linux-gnu 交叉工具链
if [ "$ARCH" != "$HOST_ARCH" ]; then
    CROSS="${ARCH}-linux-gnu"
    command -v "$CROSS-g++" >/dev/null 2>&1 \
        || { echo "未找到交叉编译器 $CROSS-g++, 请安装 g++-${ARCH}-linux-gnu" >&2; exit 1; }
    cat > "$BUILD_DIR/toolchain.cmake" <<TOOLEOF
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR $ARCH)
set(CMAKE_C_COMPILER $CROSS-gcc)
set(CMAKE_CXX_COMPILER $CROSS-g++)
TOOLEOF
    TOOLCHAIN=(-DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/toolchain.cmake")
else
    TOOLCHAIN=()
fi

echo "==> 配置 (${ARCH}, ${BUILD_TYPE}, examples=${BUILD_EXAMPLES})"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$OUTPUT_DIR" \
    -DBUILD_EXAMPLES="$BUILD_EXAMPLES" \
    ${TOOLCHAIN[@]+"${TOOLCHAIN[@]}"} \
    -DCMAKE_INSTALL_RPATH='$ORIGIN/../lib' \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
    -DCMAKE_EXE_LINKER_FLAGS=-Wl,--disable-new-dtags

echo "==> 编译 (-j ${JOBS})"
if [ "$BUILD_ALL" -eq 1 ] || [ "$BUILD_EXAMPLES" = "OFF" ]; then
    cmake --build "$BUILD_DIR" -j "$JOBS"
else
    cmake --build "$BUILD_DIR" -j "$JOBS" --target h2x_examples
fi

echo "==> 安装到 $OUTPUT_DIR"
cmake --install "$BUILD_DIR" --prefix "$OUTPUT_DIR" >/dev/null

## example 没有 install 规则, 这里统一暂存到 output/<arch>/bin
if [ "$BUILD_EXAMPLES" = "ON" ] && [ -d "$BUILD_DIR/bin" ]; then
    mkdir -p -- "$OUTPUT_DIR/bin"
    cp -f -- "$BUILD_DIR"/bin/* "$OUTPUT_DIR/bin/"
elif [ "$BUILD_EXAMPLES" = "OFF" ]; then
    rm -rf -- "$OUTPUT_DIR/bin"
fi

## 环境脚本: 自定位, 整个 output/ 目录可整体拷贝到别处使用
cat > "$OUTPUT_DIR/env.sh" <<ENVEOF
# 由 local-build.sh 生成, 用法: source $OUTPUT_DIR/env.sh
if [ -n "\${BASH_SOURCE:-}" ]; then
    UNITREE_SDK2_DIR="\$(cd -- "\$(dirname -- "\${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)"
else
    UNITREE_SDK2_DIR="$OUTPUT_DIR"
fi
export UNITREE_SDK2_DIR
export LD_LIBRARY_PATH="\$UNITREE_SDK2_DIR/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
export PATH="\$UNITREE_SDK2_DIR/bin\${PATH:+:\$PATH}"
export CMAKE_PREFIX_PATH="\$UNITREE_SDK2_DIR\${CMAKE_PREFIX_PATH:+:\$CMAKE_PREFIX_PATH}"
ENVEOF

echo "==> 完成"
echo "产物目录: $OUTPUT_DIR"
if [ -d "$OUTPUT_DIR/bin" ]; then
    echo "可执行程序:"
    ls -1 -- "$OUTPUT_DIR/bin" | sed 's/^/  /'
    echo
    echo "用法: source $OUTPUT_DIR/env.sh && $OUTPUT_DIR/bin/<程序> <网卡名>"
fi
