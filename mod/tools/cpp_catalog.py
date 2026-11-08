"""生成有界临时内存的C++目录正文；不执行游戏或验收。"""


def string_expression(text, delimiter):
    """一次预留UTF-8字节，再逐段追加；禁止长串operator+保留全部前缀临时值。"""
    if ")" + delimiter + '"' in text:
        raise ValueError("catalog_raw_string_delimiter")
    parts = [text[i:i + 8000] for i in range(0, len(text), 8000)]
    return (
        "[](){\n// 目录只建立一个文本缓冲；各append是独立完整表达式。\n"
        "std::string catalog_text;\n"
        f"catalog_text.reserve({len(text.encode('utf-8'))});\n"
        + "".join(
            f'catalog_text.append(R"{delimiter}({part}){delimiter}",'
            f"{len(part.encode('utf-8'))});\n" for part in parts
        )
        + "return catalog_text;\n}()"
    )
