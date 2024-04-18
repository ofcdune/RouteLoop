#!/usr/bin/python3

# Automatically detects any routing loop in the given networks
# Author: dune-official

from ipaddress import IPv4Network
from sys import argv
from scapy.sendrecv import AsyncSniffer, send
from scapy.layers.inet import IP, ICMP
from scapy.config import conf
from time import sleep, perf_counter_ns
from random import shuffle, seed

# disables verbose output from scapy (still displays warnings)
conf.verb = False


def scan_no_delay(hosts: list, verbose_: bool):
    """
    Scans without a delay option.

    :param hosts:
    :param verbose_:
    :return:
    """

    asyncsn = AsyncSniffer(store=0, filter="icmp", prn=ttl_exceeded)
    asyncsn.start()

    for rnd_host in hosts:

        ping(rnd_host)
        if verbose_:
            print(f"Pinged {rnd_host}")

    sleep(0.1)
    asyncsn.stop()


def scan_delay(hosts: list, verbose_: bool, dl: int):
    """
    Scans without a delay option.

    :param hosts:
    :param verbose_:
    :return:
    """

    asyncsn = AsyncSniffer(store=0, filter="icmp", prn=ttl_exceeded)
    asyncsn.start()

    for rnd_host in hosts:

        ping(rnd_host)
        if verbose_:
            print(f"Pinged {rnd_host}")

        sleep(dl / 1000)

    sleep(0.1)
    asyncsn.stop()


def ping(addr):
    # maximum TTL to really be sure
    send(IP(dst=str(addr), ttl=255) / ICMP(type=8, code=0))


def ttl_exceeded(packet):
    # use sprintf to search packet fields
    ip = packet.sprintf("%ICMP.dst%")
    if packet.sprintf("%ICMP.type%") == "time-exceeded":
        print(f"\033[31;1;4m{ip}: TTL exceeded in transit, Loop suspected!\033[0m")
        return
    else:
        return


if __name__ == '__main__':

    if len(argv) > 2:

        # use: routeloop.py <NETWORK> <DELAY=0> <--v optionally>
        network = IPv4Network(argv[1])
        delay = int(argv[2])

        all_hosts = [hst for hst in network.hosts()]
        seed(perf_counter_ns())
        shuffle(all_hosts)

        if len(argv) == 4 and argv[3] == "--v":
            verbose = True
            if delay:
                print(f"Duration is estimated for {(network.num_addresses * delay) / 1000} s")
        else:
            verbose = False

        if delay == 0:
            scan_no_delay(all_hosts, verbose)
        else:
            scan_delay(all_hosts, verbose, delay)

    else:
        print(f"Usage: {argv[0]} [Network] [Delay] [Optional: --v]")
        exit(0)
