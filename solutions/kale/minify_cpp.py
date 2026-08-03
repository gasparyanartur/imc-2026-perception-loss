#!/usr/bin/env python3
"""Conservatively remove C++ whitespace/comments without expanding headers."""

from __future__ import annotations

import argparse
from pathlib import Path


ALIASES = {
    "int": "zA", "const": "zB", "return": "zC", "for": "zD",
    "continue": "zE", "auto": "zF", "bool": "zG", "false": "zH",
    "true": "zI", "break": "zJ", "else": "zK", "struct": "zL",
    "void": "zM", "long": "zN", "float": "zO", "push_back": "zP",
    "size": "zQ", "reserve": "zR", "begin": "zS", "end": "zT",
    "assign": "zU", "clear": "zV", "swap": "zW", "max": "zX",
    "min": "zY", "floor": "zZ",
}


def needs_space(left: str, right: str) -> bool:
    word = lambda c: c.isalnum() or c in "_$"
    if word(left) and word(right):
        return True
    if left == right and left in "+-<>&|:.#":
        return True
    if left == "/" and right in "/*":
        return True
    if (left.isdigit() and right == ".") or (left == "." and right.isdigit()):
        return True
    if left in "\"'" and word(right):
        return True
    return False


def minify(source: str) -> str:
    out: list[str] = []
    i = 0
    pending = False
    line_start = True
    aliases_added = False
    n = len(source)
    while i < n:
        if line_start:
            j = i
            while j < n and source[j] in " \t\r":
                j += 1
            if j < n and source[j] == "#":
                if out and out[-1] != "\n":
                    out.append("\n")
                k = source.find("\n", j)
                if k < 0:
                    k = n
                out.append(source[j:k].rstrip())
                out.append("\n")
                i = min(n, k + 1)
                line_start = True
                pending = False
                continue
            if not aliases_added:
                out.append("".join(f"#define {short} {token}\n"
                                   for token, short in ALIASES.items()))
                aliases_added = True
        c = source[i]
        if c.isspace():
            pending = True
            line_start = c == "\n"
            i += 1
            continue
        line_start = False
        if c == "/" and i + 1 < n and source[i + 1] == "/":
            pending = True
            k = source.find("\n", i + 2)
            i = n if k < 0 else k + 1
            line_start = True
            continue
        if c == "/" and i + 1 < n and source[i + 1] == "*":
            pending = True
            k = source.find("*/", i + 2)
            if k < 0:
                raise ValueError("unterminated block comment")
            line_start = "\n" in source[i:k + 2]
            i = k + 2
            continue
        if pending and out and out[-1] != "\n" and needs_space(out[-1][-1], c):
            out.append(" ")
        pending = False
        if c.isalpha() or c in "_$":
            j = i + 1
            while j < n and (source[j].isalnum() or source[j] in "_$"):
                j += 1
            out.append(ALIASES.get(source[i:j], source[i:j]))
            i = j
            continue
        if c in "\"'":
            quote = c
            out.append(c)
            i += 1
            while i < n:
                c = source[i]
                out.append(c)
                i += 1
                if c == "\\" and i < n:
                    out.append(source[i])
                    i += 1
                elif c == quote:
                    break
            else:
                raise ValueError("unterminated quoted literal")
            continue
        out.append(c)
        i += 1
    if not out or out[-1] != "\n":
        out.append("\n")
    return "".join(out)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(minify(args.source.read_text()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
