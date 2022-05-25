# NERV_FILESYS

> This is a little demo build by fuse to create a chat room

<center><img src="./pic/nerv.svg" alt="nerv_icon" style="zoom: 10%;" /></center>

## 搭建方法

* 先在本地建立`fuse3`环境，在文件夹下运行

  ```bash
  make
  ```

* 启动程序

  ```bash
  make start
  ```

* 关闭程序

  ```bash
  make close
  make fclean #强制解除挂载
  ```

* 清除安装内容

  ```bash
  make clean
  ```

* 查看运行状态

  ```bash
  make status
  ```

## 使用方法

* 进入挂载文件系统的目录（默认为`nerv`）

* 在`register_bot`目录下新建文件则为加入聊天室，同理删除对应文件为离开聊天室

  * 在该目录下不能创建文件夹

* 查看每个人对应的文件可以查看每个人的发言记录

* 查看文件`chatRoom`可以看到公屏聊天内容

* 使用echo发送消息

  ```bash
  echo "text" >register_bot/<bot_name>
  ```

* 使用`./nerv_bin -f nerv`来维持程序前台运行查看log

* 支持一些其他与聊天室无关的linux文件基本操作