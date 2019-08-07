# Tools

Most of this part is following the official tutorial. There is one thing not mentioned in the tutorial but I meet when I install `Qemu`.

When I compile Qemu by using `make && make install` in a normal user, it reports a error as follows:

```shell
install -d -m 0755 "/usr/local/share/qemu"
cannot change permissions of ‘/usr/local/share/qemu’: No such file or directory
```

Apparently, the reason is that I have no permission. Then just run it again in `root` mode and it works.

One more things may confuse someone who don't know the form of Linux command is that in the previous step, official tutorial shows to run following command:

```shell
./configure --disable-kvm --disable-werror [--prefix=PFX] [--target-list="i386-softmmu x86_64-softmmu"]
```

It will report a error like no command `[—prefix=PFX]`. Just remove the `[` and `]`, then it will be OK.

