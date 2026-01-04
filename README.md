# Hostname 管理 Web 服务

基于 Mongoose 的简单 HTTP 服务器，提供通过 Web 界面管理系统 hostname 的功能。

## 编译

支持交叉编译，通过 `ARCH` 环境变量指定架构：

**x86 架构（默认）：**
```bash
./build.sh
# 或
ARCH=x86 ./build.sh
```

**ARM64 架构：**
```bash
ARCH=arm64 ./build.sh
```

**清理构建文件：**
```bash
./build.sh clean
```

**说明：**
- CMake 生成的所有文件都放在 `./build` 目录下
- 编译后的可执行文件会自动安装到 `./bin/`
- 每次构建前会自动清理 build 目录，确保使用正确的编译器

## 运行

**注意**：修改 hostname 需要 root 权限，因此需要使用 `sudo` 运行，`index.html` 需要跟可执行文件在相同路径下。

```bash
# 使用 build.sh 编译后（可执行文件在 bin/ 目录）
sudo ./bin/server
```

## 使用

1. 启动服务器后，在浏览器中访问：`http://localhost:8000`
2. 点击"读取"按钮获取当前 hostname
3. 在输入框中修改 hostname
4. 点击"保存"按钮应用更改