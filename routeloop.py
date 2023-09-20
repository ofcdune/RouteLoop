#!/usr/bin/python3

# Automatically detects any routing loop in the given networks
# Author: dune-official

from ipaddress import IPv4Network, IPv4Address
from sys import argv
from scapy.layers.inet import IP, ICMP
from scapy.sendrecv import sr1
from scapy.config import conf
from threading import Thread
from time import sleep, perf_counter_ns
from random import shuffle, seed


# disables verbose output from scapy (still displays warnings)
conf.verb = False


def loop_ping(ip_addr: IPv4Address):
    """
    The 'meat' of this script. Pings one IPv4 address and waits for a reply. If the reply is a TTL timeout, the function prints it into stdout.

    :param ip_addr: The IPv4 address
    :return:
    """

    # maximum TTL to really be sure
    scan_packet = IP(dst=str(ip_addr), ttl=255) / ICMP(type=8, code=0)

    # please modify the timeout to your liking, however keep in mind that the worse the Delay, the longer the timeout should be
    # in seconds
    nxt = sr1(scan_packet, timeout=.1)

    if nxt is None:
        return

    # use sprintf to search packet fields
    if nxt.sprintf("%ICMP.type%") == "time-exceeded":
        print(f"\033[31;1;4m{ip_addr}: TTL exceeded in transit, Loop suspected!\033[0m")
        return
    else:
        return


def scan_no_delay(hosts: list, verbose_: bool):
    """
    Scans without a delay option.

    :param hosts:
    :param verbose_:
    :return:
    """
    threads = []
    for rnd_host in hosts:
        thread = Thread(target=loop_ping, args=[rnd_host])
        threads.append(thread)
        thread.start()

        if verbose_:
            print(f"Pinged {rnd_host}")

    for i in range(len(threads)):
        threads[i].join()


def scan_delay(hosts: list, verbose_: bool, dl: int):
    """
    Scans with a delay option.

    :param hosts:
    :param verbose_:
    :param dl:
    :return:
    """
    
    threads = []
    for rnd_host in hosts:
        thread = Thread(target=loop_ping, args=[rnd_host])
        threads.append(thread)
        thread.start()

        if verbose_:
            print(f"Pinged {rnd_host}")

        sleep(dl / 1000)

    for i in range(len(threads)):
        threads[i].join()


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

