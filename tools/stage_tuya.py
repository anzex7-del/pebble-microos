#!/usr/bin/env python3
"""Stage app and guarded display fixes against the pinned TuyaOpen SDK."""
from pathlib import Path
import shutil
import sys

PATCHES = (
    ("src/peripherals/display/tdd_display/src/qspi/tdd_display_qspi.c",
     "__disp_qspi_set_window(&disp_qspi_dev->cfg, p_fb->x_start, p_fb->y_start, p_fb->width-1, p_fb->height-1);",
     "__disp_qspi_set_window(&disp_qspi_dev->cfg, p_fb->x_start, p_fb->y_start, p_fb->x_start + p_fb->width - 1, p_fb->y_start + p_fb->height - 1);"),
    ("boards/T5AI/WAVESHARE_T5AI_TOUCH_AMOLED_1_75/Kconfig",
     "select ENABLE_LVGL_DUAL_DISP_BUFF if (ENABLE_LIBLVGL)",
     "select ENABLE_LVGL_DUAL_DISP_BUFF if (ENABLE_LIBLVGL && !ENABLE_LVGL_PARTIAL_FLUSH)"),
)


def patch_display(sdk):
    # Validate every patch before writing; unknown SDK revisions fail closed.
    edits = []
    for name, before, after in PATCHES:
        path = sdk / name
        text = path.read_text()
        if text.count(after) == 1:
            continue
        if text.count(before) != 1:
            raise RuntimeError(f"Pinned display patch no longer matches: {name}")
        edits.append((path, text.replace(before, after)))
    for path, text in edits:
        path.write_text(text)


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python3 tools/stage_tuya.py /absolute/path/to/TuyaOpen")
    sdk = Path(sys.argv[1]).expanduser().resolve()
    board = sdk / "boards/T5AI/WAVESHARE_T5AI_TOUCH_AMOLED_1_75"
    if not board.is_dir() or not (sdk / "export.sh").is_file():
        raise SystemExit("Expected TuyaOpen SDK with the WAVESHARE_T5AI_TOUCH_AMOLED_1_75 board.")
    target = sdk / "examples/pebble_microos"
    if target.exists():
        raise SystemExit(f"Refusing to overwrite existing application: {target}")
    patch_display(sdk)
    shutil.copytree(Path(__file__).resolve().parents[1] / "firmware", target)
    print(f"Staged {target} with cropped-display fixes. Nothing has been flashed.")


if __name__ == "__main__":
    main()
