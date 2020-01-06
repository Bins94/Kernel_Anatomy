## QEMU
### host

```  
ip link add br0 type bridge
ip tuntap add mode tap tap0
ip link set tap0 master br0
ip link set br0 up
ip link set tap0 up
```  

### guest
```  
qemu-system-x86_64 -m 2048 -net tap -device e1000,netdev=mynet -netdev tap,id=mynet,ifname=tap0,script=no,downscript=no ... 	-append " ... ip=dhcp ... " ...
```  

