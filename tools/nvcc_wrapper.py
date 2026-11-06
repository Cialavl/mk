#!/usr/bin/env python3
"""Unwrap nvcc dependency flags that CMake puts behind -Xcompiler.

CMake 4.3 on Windows emits dependency flags (-MD/-MF/-MT) as host options.
It also emits the MSVC runtime flag as -Xcompiler=-MT when /MT is selected.
Only the dependency flags must move back to nvcc; the runtime flag must stay
behind -Xcompiler, otherwise nvcc sees two -MT options and fails with
"single input file is required".

CMAKE_CUDA_COMPILER_LAUNCHER calls this script as:
    python nvcc_wrapper.py <nvcc.exe> <compiler args...>
so argv[0] is the real compiler, not a flag.
"""
import os
import subprocess
import sys

DEP_FLAGS = ("-MD", "-MMD", "-MF", "-MT")
HOST_RUNTIME = {"-MD": "/MD", "-MT": "/MT"}


def rewrite(argv):
    out = []
    i = 0
    while i < len(argv):
        arg = argv[i]

        # Space form: "-Xcompiler -MD -MT obj -MF dep" -> dep flags to nvcc.
        if arg == "-Xcompiler":
            if i + 1 >= len(argv):
                i += 1
                continue
            nxt = argv[i + 1]
            if nxt in DEP_FLAGS:
                out.append(nxt)
            else:
                out.extend((arg, nxt))
            i += 2
            continue

        # Equals form: "-Xcompiler=-MT" is the MSVC runtime, keep it host side.
        if arg.startswith("-Xcompiler="):
            value = arg.split("=", 1)[1]
            if value in HOST_RUNTIME:
                out.append("-Xcompiler=" + HOST_RUNTIME[value])
            elif value == "-MF":
                out.append(value)
            else:
                out.append(arg)
            i += 1
            continue

        out.append(arg)
        i += 1
    return out


def main():
    argv = sys.argv[1:]
    if not argv:
        return 1

    # CMake launcher passes the real compiler as the first argument.
    if not argv[0].startswith("-"):
        compiler = argv[0]
        args = rewrite(argv[1:])
    else:
        compiler = os.environ.get("LLAMA_NVCC", "nvcc")
        args = rewrite(argv)

    debug = os.environ.get("NVCC_WRAPPER_DEBUG")
    if debug:
        with open(debug, "a", encoding="utf-8") as handle:
            handle.write("ARGS: " + repr(sys.argv[1:]) + "\n")
            handle.write("OUT:  " + repr(args) + "\n")
            handle.write("CMD:  " + subprocess.list2cmdline([compiler] + args) + "\n")

    return subprocess.call([compiler] + args)


if __name__ == "__main__":
    sys.exit(main())
