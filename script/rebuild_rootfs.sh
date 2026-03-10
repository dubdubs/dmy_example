#!/bin/bash
# 脚本名称: rebuild_rootfs.sh
# 功能: 解压/解包 rootfs.cpio.gz，等待用户修改，然后重新打包并压缩

set -e  # 遇到错误立即退出

# 检查参数
if [ $# -ne 1 ]; then
    echo "用法: $0 <rootfs.cpio.gz 路径>"
    exit 1
fi

SRC_FILE="$1"
BASENAME=$(basename "$SRC_FILE" .gz)          # 去掉 .gz 后缀，得到 rootfs.cpio
WORK_DIR="rootfs_tmp"                          # 解包临时目录

# 1. 检查源文件是否存在
if [ ! -f "$SRC_FILE" ]; then
    echo "错误: 文件 $SRC_FILE 不存在"
    exit 1
fi

echo "=== 步骤1: 解压 $SRC_FILE ==="
gunzip -k "$SRC_FILE" || true   # 如果已存在 .cpio 文件，-k 保留原 .gz；若失败（如已解压过）则忽略

# 2. 创建临时目录并解包
echo "=== 步骤2: 创建临时目录 $WORK_DIR 并解包 ==="
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"
cpio -idmv < "../$BASENAME"    # 注意 BASENAME 可能包含路径，这里假设与 SRC_FILE 同目录

echo "=== 步骤3: 文件已解压到 $WORK_DIR ==="
echo "请在此目录下添加/修改文件（例如拷贝需要的文件）。"
echo "完成后按 Enter 键继续..."
read -r  # 等待用户回车

# 4. 重新打包
echo "=== 步骤4: 重新打包 ==="
find . | cpio -o -H newc > "../rootfs_new.cpio"

cd ..

# 5. 压缩
echo "=== 步骤5: 压缩 rootfs_new.cpio ==="
gzip rootfs_new.cpio

echo "=== 完成: 生成 rootfs_new.cpio.gz ==="