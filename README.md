# RouteLoop
Tired of manually checking for router entries to find if you or your customers accidentally created a routing loop? Want to help ISPs or conduct scientific studies on the WAN?
Look no further, because this program can help you!

What RouteLoop can do:
- Look for routing loops by pinging networks and seeing if a "ttl exceeded" gets replied
- Work fast or slow
- Pick IP-Addresses at random to avoid cluttering up a network

What RouteLoop cannot do:
- Actually fix these routing loops
- Look into configurations of routers

# Caution
Please use with caution, and with your own risk. There are mechanisms in place to prevent an accidental DOS attack, but you should still be careful.

# How to use
1. Install requirements with `pip install requirements.txt`
2. Launch the program:

python3 main.py [Network] [Delay] [Optional: --v]

with Network as your IPv4-Network in CIDR-Notation and Delay in milliseconds (A delay of 0 means no delay). `--v` prints out the progress of the application, but potentially reduces speed. If not specified, the program will only print out
the suspected Loops (IPv4-Addresses for which the TTL exceeded in transit)

# Math
Really, math? Yes, this is to estimate the duration of the scan, or rather, the net waiting time. A /24 network for example has 2 to the power of 32-24 hosts, namely 256. Divide that by the milliseconds you specified and it should provide you with a reasonable estimate, provided, your network speed is fast enough. If you don't want delay, the network delay and bandwidth can limit the speed, but a few milliseconds already are slower than that and cancel out the effects. The program shows the speed estimate when verbose is specified however. 

# Roadmap
- Add support for exporting results
- Add support for IPv6
- Port the program to C
