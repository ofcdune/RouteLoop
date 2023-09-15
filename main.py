# Automatically detects any routing loop in the given networks

from ipaddress import IPv4Network, IPv4Address
from sys import argv
from scapy.layers.inet import IP, ICMP
from scapy.sendrecv import sr1
from scapy.config import conf
from threading import Thread
from time import sleep, perf_counter_ns
from random import shuffle, seed


conf.verb = False


def loop_ping(ip_addr: IPv4Address):
    scan_packet = IP(dst=str(ip_addr), ttl=255) / ICMP(type=8, code=0)
    nxt = sr1(scan_packet, timeout=.1)

    if nxt is None:
        return

    if nxt.sprintf("%ICMP.type%") == "time-exceeded":
        print(f"\033[31;1;4m{ip_addr}: TTL exceeded in transit, Loop suspected!\033[0m")
        return
    else:
        return


def scan_no_delay(hosts: list, verbose_: bool):
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

