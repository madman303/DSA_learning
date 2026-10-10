#!/usr/bin/env python3
"""Minimal pure-Python PDF text extractor (fallback when PyMuPDF/pdftotext unavailable).

Handles: FlateDecode streams, Identity-H CID fonts with ToUnicode CMaps,
simple-font /Differences encodings, and the usual BT/ET text operators.
Usage: python3 pdftext.py input.pdf [--pages 1-5] > out.txt
"""
from __future__ import annotations

import re
import sys
import zlib

OBJ_RE = re.compile(rb"(\d+)\s+(\d+)\s+obj\b(.*?)\bendobj", re.DOTALL)
STREAM_RE = re.compile(rb"stream\r?\n(.*?)\r?\nendstream", re.DOTALL)

# Adobe standard glyph names we care about for common Latin text.
GLYPH_TO_UNI = {
    "space": " ", "exclam": "!", "quotedbl": '"', "numbersign": "#", "dollar": "$",
    "percent": "%", "ampersand": "&", "quoteright": "'", "quotesingle": "'",
    "parenleft": "(", "parenright": ")", "asterisk": "*", "plus": "+", "comma": ",",
    "hyphen": "-", "period": ".", "slash": "/", "zero": "0", "one": "1", "two": "2",
    "three": "3", "four": "4", "five": "5", "six": "6", "seven": "7", "eight": "8",
    "nine": "9", "colon": ":", "semicolon": ";", "less": "<", "equal": "=",
    "greater": ">", "question": "?", "at": "@", "bracketleft": "[", "backslash": "\\",
    "bracketright": "]", "asciicircum": "^", "underscore": "_", "grave": "`",
    "braceleft": "{", "bar": "|", "braceright": "}", "asciitilde": "~",
    "quotedblleft": '"', "quotedblright": '"', "quoteleft": "'",
    "endash": "-", "emdash": "--", "bullet": "*", "fi": "fi", "fl": "fl",
    "ff": "ff", "ffi": "ffi", "ffl": "ffl", "degree": "deg", "plusminus": "+/-",
    "multiply": "x", "divide": "/", "lessequal": "<=", "greaterequal": ">=",
    "notequal": "!=", "infinity": "inf", "element": "in", "arrowright": "->",
    "arrowleft": "<-", "arrowboth": "<->", "summation": "SUM", "product": "PROD",
    "integral": "INT", "radical": "sqrt", "partialdiff": "d", "Delta": "Delta",
    "alpha": "alpha", "beta": "beta", "gamma": "gamma", "delta": "delta",
    "epsilon": "epsilon", "theta": "theta", "lambda": "lambda", "mu": "mu",
    "pi": "pi", "rho": "rho", "sigma": "sigma", "tau": "tau", "phi": "phi",
    "omega": "omega", "Omega": "Omega", "Sigma": "Sigma", "Pi": "Pi",
}


def decode_stream(raw: bytes, dct: bytes) -> bytes:
    if b"FlateDecode" in dct:
        try:
            raw = zlib.decompress(raw)
        except zlib.error:
            try:
                raw = zlib.decompressobj().decompress(raw)
            except zlib.error:
                return b""
    elif b"ASCIIHexDecode" in dct:
        hexs = re.sub(rb"[^0-9A-Fa-f]", b"", raw.split(b">")[0])
        if len(hexs) % 2:
            hexs += b"0"
        raw = bytes.fromhex(hexs.decode("ascii"))
    return raw


class PDF:
    def __init__(self, data: bytes):
        self.objs: dict[int, bytes] = {}
        for m in OBJ_RE.finditer(data):
            self.objs[int(m.group(1))] = m.group(3)
        self.cache: dict[int, bytes] = {}

    # ---------- low level ----------
    def resolve(self, token: bytes) -> tuple[int | None, bytes]:
        m = re.match(rb"\s*(\d+)\s+\d+\s+R\b", token)
        if m:
            n = int(m.group(1))
            return n, self.objs.get(n, b"")
        return None, token

    def stream_of(self, num: int) -> bytes:
        if num in self.cache:
            return self.cache[num]
        body = self.objs.get(num, b"")
        m = STREAM_RE.search(body)
        out = b""
        if m:
            start = body.find(b"stream")
            header = body[:start]
            raw = m.group(1)
            # honour a direct /Length when it disagrees with the regex boundary
            lm = re.search(rb"/Length\s+(\d+)(?!\s+\d+\s+R)", header)
            if lm:
                ln = int(lm.group(1))
                s = body.find(b"stream")
                s = body.find(b"\n", s) + 1
                raw = body[s:s + ln]
            out = decode_stream(raw, header)
        self.cache[num] = out
        return out

    def dict_of(self, num: int) -> bytes:
        body = self.objs.get(num, b"")
        return body[:body.find(b"stream")] if b"stream" in body else body

    # ---------- fonts ----------
    def build_tounicode(self, font_num: int) -> dict[int, str]:
        dct = self.dict_of(font_num)
        m = re.search(rb"/ToUnicode\s+(\d+)\s+\d+\s+R", dct)
        if not m:
            return {}
        cmap = self.stream_of(int(m.group(1)))
        table: dict[int, str] = {}
        for blk in re.findall(rb"beginbfchar(.*?)endbfchar", cmap, re.DOTALL):
            for src, dst in re.findall(rb"<([0-9A-Fa-f]+)>\s*<([0-9A-Fa-f]+)>", blk):
                table[int(src, 16)] = bytes.fromhex(dst.decode()).decode("utf-16-be", "replace")
        for blk in re.findall(rb"beginbfrange(.*?)endbfrange", cmap, re.DOTALL):
            for lo, hi, dst in re.findall(
                rb"<([0-9A-Fa-f]+)>\s*<([0-9A-Fa-f]+)>\s*<([0-9A-Fa-f]+)>", blk
            ):
                a, b = int(lo, 16), int(hi, 16)
                base = int(dst, 16)
                for i in range(b - a + 1):
                    table[a + i] = chr(base + i) if base + i < 0x110000 else "?"
            for lo, hi, arr in re.findall(
                rb"<([0-9A-Fa-f]+)>\s*<([0-9A-Fa-f]+)>\s*\[(.*?)\]", blk, re.DOTALL
            ):
                items = re.findall(rb"<([0-9A-Fa-f]+)>", arr)
                for i, it in enumerate(items):
                    table[int(lo, 16) + i] = bytes.fromhex(it.decode()).decode("utf-16-be", "replace")
        return table

    def font_info(self, font_num: int) -> dict:
        dct = self.dict_of(font_num)
        info = {"tounicode": self.build_tounicode(font_num), "twobyte": False, "diff": {}}
        if b"Identity" in dct or re.search(rb"/Subtype\s*/Type0", dct):
            info["twobyte"] = True
        m = re.search(rb"/Differences\s*\[(.*?)\]", dct, re.DOTALL)
        if m:
            code = 0
            for tok in re.findall(rb"(\d+)|/([A-Za-z0-9._]+)", m.group(1)):
                if tok[0]:
                    code = int(tok[0])
                else:
                    info["diff"][code] = GLYPH_TO_UNI.get(
                        tok[1].decode("latin-1"), tok[1].decode("latin-1")
                    )
                    code += 1
        return info

    # ---------- pages ----------
    def pages(self) -> list[int]:
        out = []
        for num, body in self.objs.items():
            head = body[:400]
            if re.search(rb"/Type\s*/Page\b", head) and not re.search(rb"/Type\s*/Pages\b", head):
                out.append(num)
        return sorted(out)

    def page_fonts(self, page_num: int) -> dict[str, int]:
        dct = self.dict_of(page_num)
        fonts: dict[str, int] = {}
        fm = re.search(rb"/Font\s*<<(.*?)>>", dct, re.DOTALL)
        if fm:
            for name, ref in re.findall(rb"/([A-Za-z0-9._]+)\s+(\d+)\s+\d+\s+R", fm.group(1)):
                fonts[name.decode()] = int(ref)
        if not fonts:
            rm = re.search(rb"/Resources\s+(\d+)\s+\d+\s+R", dct)
            if rm:
                rdct = self.dict_of(int(rm.group(1)))
                fm = re.search(rb"/Font\s*<<(.*?)>>", rdct, re.DOTALL)
                if fm:
                    for name, ref in re.findall(
                        rb"/([A-Za-z0-9._]+)\s+(\d+)\s+\d+\s+R", fm.group(1)
                    ):
                        fonts[name.decode()] = int(ref)
        return fonts

    def page_content(self, page_num: int) -> bytes:
        dct = self.dict_of(page_num)
        nums: list[int] = []
        m = re.search(rb"/Contents\s+(\d+)\s+\d+\s+R", dct)
        if m:
            nums = [int(m.group(1))]
        else:
            m = re.search(rb"/Contents\s*\[(.*?)\]", dct, re.DOTALL)
            if m:
                nums = [int(x) for x in re.findall(rb"(\d+)\s+\d+\s+R", m.group(1))]
        return b"\n".join(self.stream_of(n) for n in nums)

    # ---------- text ----------
    @staticmethod
    def _unescape(s: bytes) -> bytes:
        out = bytearray()
        i = 0
        while i < len(s):
            c = s[i]
            if c == 0x5C and i + 1 < len(s):
                nxt = s[i + 1]
                simple = {0x6E: 10, 0x72: 13, 0x74: 9, 0x62: 8, 0x66: 12}
                if nxt in simple:
                    out.append(simple[nxt]); i += 2; continue
                if 0x30 <= nxt <= 0x37:
                    j = i + 1
                    oct_digits = b""
                    while j < len(s) and len(oct_digits) < 3 and 0x30 <= s[j] <= 0x37:
                        oct_digits += bytes([s[j]]); j += 1
                    out.append(int(oct_digits, 8) & 0xFF); i = j; continue
                out.append(nxt); i += 2; continue
            out.append(c); i += 1
        return bytes(out)

    def decode_with(self, raw: bytes, fi: dict) -> str:
        tu = fi.get("tounicode") or {}
        diff = fi.get("diff") or {}
        if fi.get("twobyte"):
            out = []
            for i in range(0, len(raw) - 1, 2):
                code = (raw[i] << 8) | raw[i + 1]
                out.append(tu.get(code, ""))
            return "".join(out)
        out = []
        for b in raw:
            if b in tu:
                out.append(tu[b])
            elif b in diff:
                out.append(diff[b])
            elif 32 <= b < 127:
                out.append(chr(b))
            elif b >= 0x80:
                out.append(tu.get(b, ""))
        return "".join(out)

    # token kinds: b'num' | b'str' | b'hex' | b'name' | b'arr' | b'op'
    TOK_RE = re.compile(
        rb"(?P<str>\((?:\\.|[^\\()]|\((?:[^()\\]|\\.)*\))*\))|"
        rb"(?P<hex><[0-9A-Fa-f\s]*>)|"
        rb"(?P<name>/[^\s/\[\]()<>{}]+)|"
        rb"(?P<num>[-+]?(?:\d+\.\d*|\.\d+|\d+))|"
        rb"(?P<arr>[\[\]])|"
        rb"(?P<op>[A-Za-z'*\"][A-Za-z0-9*'\"]*)",
        re.DOTALL,
    )

    def _tokens(self, content: bytes):
        for m in self.TOK_RE.finditer(content):
            kind = m.lastgroup
            raw = m.group(0)
            if kind == "str":
                yield ("str", raw[1:-1])
            elif kind == "hex":
                yield ("hex", re.sub(rb"[^0-9A-Fa-f]", b"", raw).decode())
            elif kind == "name":
                yield ("name", raw[1:].decode("latin-1"))
            elif kind == "num":
                yield ("num", float(raw))
            elif kind == "arr":
                yield ("arr", raw)
            else:
                yield ("op", raw.decode("latin-1"))

    def _decode_operand(self, item, cur: dict) -> str:
        kind, val = item
        if kind == "str":
            return self.decode_with(self._unescape(val), cur)
        if kind == "hex":
            hx = val if len(val) % 2 == 0 else val + "0"
            try:
                return self.decode_with(bytes.fromhex(hx), cur)
            except ValueError:
                return ""
        return ""

    def extract_page(self, page_num: int) -> str:
        fonts = {k: self.font_info(v) for k, v in self.page_fonts(page_num).items()}
        content = self.page_content(page_num)

        text: list[str] = []
        cur: dict = {}
        cur_size = 12.0
        last_y: float | None = None
        cur_ty = 0.0          # translation from the current CTM (Typst places text with cm)
        started = False
        stack: list = []
        arr_depth = 0

        def emit(s: str) -> None:
            nonlocal last_y, started
            if not s:
                return
            if not started or last_y is None or abs(cur_ty - last_y) > 0.6:
                text.append("\n")
                last_y = cur_ty
                started = True
            text.append(s)

        for kind, val in self._tokens(content):
            if kind == "num":
                stack.append((kind, val)); continue
            if kind in ("str", "hex", "name"):
                stack.append((kind, val)); continue
            if kind == "arr":
                if val == b"[":
                    stack.append(("marker", None)); arr_depth += 1
                else:
                    if arr_depth:
                        items = []
                        while stack and stack[-1][0] != "marker":
                            items.append(stack.pop())
                        if stack:
                            stack.pop()
                        items.reverse()
                        stack.append(("array", items))
                        arr_depth -= 1
                continue

            op = val
            if op == "cm":
                nums = [it[1] for it in stack if it[0] == "num"]
                if len(nums) >= 6:
                    cur_ty = nums[-1]
                stack = []
                continue
            if op == "Tf":
                for it in reversed(stack):
                    if it[0] == "name":
                        cur = fonts.get(it[1], {})
                        break
                for it in reversed(stack):
                    if it[0] == "num":
                        cur_size = it[1] or cur_size
                        break
                stack = []
                continue
            if op in ("Tj", "'", '"'):
                for it in stack:
                    if it[0] in ("str", "hex"):
                        emit(self._decode_operand(it, cur))
                if op in ("'", '"'):
                    cur_ty += cur_size
                    last_y = None
                stack = []
                continue
            if op == "TJ":
                for it in stack:
                    if it[0] == "array":
                        for el in it[1]:
                            if el[0] in ("str", "hex"):
                                emit(self._decode_operand(el, cur))
                            elif el[0] == "num" and el[1] < -180:
                                emit(" ")
                stack = []
                continue
            if op in ("Td", "TD", "T*", "Tm"):
                nums = [it[1] for it in stack if it[0] == "num"]
                if op == "Tm" and len(nums) >= 2:
                    cur_ty += nums[-1]
                elif op in ("Td", "TD") and len(nums) >= 2:
                    cur_ty += nums[-1]
                elif op == "T*":
                    cur_ty += cur_size
                stack = []
                continue
            stack = []

        out = "".join(text)
        out = re.sub(r"[ \t]+", " ", out)
        out = re.sub(r" ?\n ?", "\n", out)
        out = re.sub(r"\n{3,}", "\n\n", out)
        return out.strip()


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        print(__doc__)
        sys.exit(1)
    data = open(args[0], "rb").read()
    pdf = PDF(data)
    pages = pdf.pages()
    first, last = 1, len(pages)
    for a in sys.argv[1:]:
        if a.startswith("--pages"):
            spec = a.split("=", 1)[1] if "=" in a else "1-9999"
            lo, _, hi = spec.partition("-")
            first, last = int(lo), int(hi or lo)
    for idx, p in enumerate(pages, 1):
        if not (first <= idx <= last):
            continue
        print(f"\n===== PAGE {idx} =====")
        print(pdf.extract_page(p))


if __name__ == "__main__":
    main()
