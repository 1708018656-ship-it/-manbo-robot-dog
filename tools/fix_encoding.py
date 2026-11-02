#!/usr/bin/env python3
"""
Convert firmware sources to UTF-8 without changing a single compiled byte.

The upstream sources mix GBK, UTF-8 and even both inside one file. Keil
shows half of them as mojibake depending on the editor setting, and git
diffs are unreadable.

Rules:
  * Comments are converted to UTF-8 text.
  * String / char literals containing non-ASCII text keep their *exact*
    original bytes (GBK for SYN6288 speech and the OLED Chinese font keys),
    rewritten as \\xHH escapes followed by a /* readable */ comment.
  * Line endings are preserved.

Usage: tools/fix_encoding.py [--check] FILE...
  --check  only report files that still contain non-ASCII string literals
           or non-UTF-8 bytes (exit 1 if any).
"""
import re
import sys

HEX = set("0123456789abcdefABCDEF")


def decode_line(raw):
    try:
        return raw.decode("utf-8"), "utf-8"
    except UnicodeDecodeError:
        return raw.decode("gbk"), "gbk"   # raise if neither: we want to know


def escape_literal(body, enc):
    """body: literal contents without quotes (decoded text)."""
    out = []
    prev_was_hex_escape = False
    i = 0
    while i < len(body):
        ch = body[i]
        if ch == "\\":                       # keep existing escapes verbatim
            m = re.match(r'\\(x[0-9a-fA-F]+|[0-7]{1,3}|.)', body[i:])
            esc = m.group(0)
            if prev_was_hex_escape and esc[1] in HEX:
                out.append('" "')
            out.append(esc)
            prev_was_hex_escape = esc[1] == "x"
            i += len(esc)
            continue
        if ord(ch) < 0x80:
            if prev_was_hex_escape and ch in HEX:
                out.append('" "')          # stop \x from swallowing the digit
            out.append(ch)
            prev_was_hex_escape = False
        else:
            for b in ch.encode(enc):
                out.append("\\x%02X" % b)
            prev_was_hex_escape = True
        i += 1
    return "".join(out)


def convert(data):
    lines = data.split(b"\n")
    out_lines = []
    state = "code"          # code | block
    issues = []
    for lineno, raw in enumerate(lines, 1):
        text, enc = decode_line(raw)
        res = []
        i = 0
        n = len(text)
        while i < n:
            c = text[i]
            if state == "block":
                j = text.find("*/", i)
                if j < 0:
                    res.append(text[i:]); i = n
                else:
                    res.append(text[i:j + 2]); i = j + 2; state = "code"
                continue
            if text.startswith("//", i):
                res.append(text[i:]); i = n
                continue
            if text.startswith("/*", i):
                state = "block"; res.append("/*"); i += 2
                continue
            if c in "\"'":
                q = c
                j = i + 1
                while j < n and text[j] != q:
                    j += 2 if text[j] == "\\" else 1
                body = text[i + 1:j]
                if any(ord(ch) >= 0x80 for ch in body):
                    if enc != "gbk":
                        issues.append((lineno, "UTF-8 literal kept as UTF-8 bytes: " + body))
                    res.append(q + escape_literal(body, enc) + q)
                    res.append(" /* " + body.replace("*/", "* /") + " */")
                else:
                    res.append(text[i:j + 1])
                i = j + 1
                continue
            res.append(c); i += 1
        out_lines.append("".join(res).encode("utf-8"))
    return b"\n".join(out_lines), issues


def main(argv):
    check = "--check" in argv
    files = [a for a in argv if a != "--check"]
    bad = 0
    for f in files:
        data = open(f, "rb").read()
        new, issues = convert(data)
        for ln, msg in issues:
            print("%s:%d: %s" % (f, ln, msg))
        if check:
            try:
                data.decode("utf-8")
                ok = new == data
            except UnicodeDecodeError:
                ok = False
            if not ok:
                print("needs conversion:", f); bad += 1
        elif new != data:
            open(f, "wb").write(new)
            print("converted:", f)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
