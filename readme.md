# 一个简陋的VPN程序

一个简单的点对点通信程序，实现加密功能，具体说明[请看这里](docs/implement.md)

## 如何运行

环境要求：

+ \*nix操作系统
+ openssl库，使用如下命令安装

    ```bash
    sudo apt install openssl
    sudo apt install libssl-dev
    ```

在运行之前，查看一下<code>src/makefile</code>文件，里面的第5行是对<code>main.c</code>程序的编译，编译命令中默认是添加了<code>RECV</code>和<code>SEND</code>的宏定义的，表明主程序既接收也发送数据，如果你不想让程序发送或者接收（或者都不想），请对应删除编译命令中的<code>-DRECV</code>和<code>-DSEND</code>。（看了<code>main.c</code>第122-138行就知道啦）

执行下面的命令：

```bash
make
```

就可以完成编译的工作了，二进制文件被存放在<code>bin/</code>目录下

在两个终端分别运行（启动设备、设置路由等操作是需要管理员权限的）

```bash
sudo ./bin/vpn
```

```bash
# 注意这里的IP是内网IP哦
./main <IP>
```

## 下一步
