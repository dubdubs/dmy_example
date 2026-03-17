#!/bin/bash
# 批量替换 thirdparty/glib 下所有 .pc 文件中的任意路径/wisteria 为当前工作目录

target_dir="thirdparty/glib/v2.64.6/lib64/pkgconfig"

# 检查目标目录是否存在
if [ ! -d "$target_dir" ]; then
    echo "错误：目录 $target_dir 不存在于当前路径下。"
    exit 1
fi

# 获取当前工作目录的绝对路径
current_pwd=$(pwd)

# 递归查找所有 .pc 文件并进行替换
find "$target_dir" -type f -name "*.pc" | while read -r pc_file; do
    echo "正在处理：$pc_file"
    # 使用 | 作为 sed 分隔符，避免与路径中的斜杠冲突
    # 匹配以 / 开头，后跟任意非空格字符，最后是 /wisteria 的字符串，全局替换
    sed -i "s|/[^ ]*/wisteria\b|$current_pwd|g" "$pc_file"
done

echo "替换完成。"