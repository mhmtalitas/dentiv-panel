#!/usr/bin/env python3
"""
index.html dosyasini ESP32 firmware'ine gomulecek bir C header'a (webpage.h)
donusturur. index.html'de degisiklik yaptiktan sonra bu script'i calistirip
firmware'i yeniden derle/yukle.

Kullanim:
    python3 tools/generate_firmware_html.py
"""
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "index.html"
DST = ROOT / "firmware" / "dentiv_esp32" / "webpage.h"
DELIM = "HTMLPAGE"

def main():
    html = SRC.read_text(encoding="utf-8")
    close_seq = f")${DELIM}\"".replace("$", "")
    if close_seq in html:
        raise SystemExit(
            f"index.html icinde raw-string kapanis dizisi ({close_seq}) bulundu; "
            "DELIM degerini degistirip tekrar deneyin."
        )

    header = (
        "// BU DOSYA OTOMATIK URETILMISTIR - elle duzenlemeyin.\n"
        "// Kaynak: index.html  ->  tools/generate_firmware_html.py\n"
        "#pragma once\n\n"
        f'static const char INDEX_HTML[] PROGMEM = R"{DELIM}(\n'
        f"{html}\n"
        f'){DELIM}";\n'
    )
    DST.write_text(header, encoding="utf-8")
    print(f"Yazildi: {DST}  ({len(html)} bayt HTML gomuldu)")

if __name__ == "__main__":
    main()
