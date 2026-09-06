"""Restricted original map/archetype data parser; no third-party dependencies."""
import math
import re

class DataParser:
    """Restricted literal Lua tables. No functions, expressions or code execution."""
    token = re.compile(r'\s+|--[^\n]*|//[^\n]*|"(?:[^"\\]|\\.)*"|[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?|[A-Za-z_]\w*|[{},;=]', re.S)

    def __init__(self, text):
        self.tokens = []
        cursor = 0
        while cursor < len(text):
            match = self.token.match(text, cursor)
            if not match:
                raise ValueError(f"Unsupported map syntax: {text[cursor:cursor+60]!r}")
            value = match[0]
            if not value.isspace() and not value.startswith(("--", "//")):
                self.tokens.append(value)
            cursor = match.end()
        self.i = 0

    def pop(self):
        value = self.tokens[self.i]
        self.i += 1
        return value

    def value(self):
        token = self.pop()
        if token == "{":
            mapping, sequence = {}, []
            while self.tokens[self.i] != "}":
                if self.i + 1 < len(self.tokens) and self.tokens[self.i + 1] == "=":
                    key = self.pop()
                    self.pop()
                    if key in mapping:
                        raise ValueError(f"Duplicate map field {key}")
                    mapping[key] = self.value()
                else:
                    sequence.append(self.value())
                if self.tokens[self.i] in (",", ";"):
                    self.pop()
            self.pop()
            if mapping and sequence:
                raise ValueError("Mixed map/list table")
            return mapping if mapping else sequence
        if token.startswith('"'):
            return token[1:-1].replace('\\"', '"')
        if token in ("true", "false"):
            return token == "true"
        try:
            result = float(token)
        except ValueError as error:
            raise ValueError(f"Only literal map values supported: {token}") from error
        if not math.isfinite(result):
            raise ValueError("Nonfinite map number")
        return result

    def document(self):
        if self.pop() != "Map" or self.pop() != "=":
            raise ValueError("Expected Map literal")
        result = self.value()
        if self.i != len(self.tokens):
            raise ValueError("Trailing executable map content")
        return result

