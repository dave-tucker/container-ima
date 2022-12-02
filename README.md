# Container IMA using eBPF

## Overview
The goal of this project is to extend the Linux Integrity Measurement Architecture to Linux containers without kernel changes using eBPF.

It uses eBPF to probe the mmap system call to measure executable files mapped in a container. vTPMs are employed per container to aid in scalability and separation of measurements between host and containers

To do: 
    - init securityfs (per container or all containers or resuse host IMA's securityfs)
    - ml signing 
    - policy configurations 
    - clean up upon exit, figure out how to check if containers still exist?