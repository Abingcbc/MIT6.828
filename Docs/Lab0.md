# Lab0

这一部分大多数是跟着官方教程来的。但在我安装 `Qemu`的时候，有一个地方官方教程没有提到。

当我使用普通用户编译 Qemu时， `make && make install` ，报错如下：

```shell
install -d -m 0755 "/usr/local/share/qemu"
cannot change permissions of ‘/usr/local/share/qemu’: No such file or directory
```

很明显，原因是普通用户没有权限，使用 `root`运行，解决问题。

还有一点，我花了一点时间。因为对Linux命令不是很熟，官方文档中需要运行下面这条命令：

```shell
./configure --disable-kvm --disable-werror [--prefix=PFX] [--target-list="i386-softmmu x86_64-softmmu"]
```

然后报错没有命令 [—prefix=PFX].只要去掉`[`和  `]`, 就能成功运行。

