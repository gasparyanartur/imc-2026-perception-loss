#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

VIEW_NAMES = ["+X", "-X", "+Y", "-Y", "+Z", "-Z"]


def ensure_renderer(root: Path) -> Path:
    binary = root / "official_like_renderer"
    source = root / "official_like_renderer.cpp"
    if not binary.exists() or binary.stat().st_mtime < source.stat().st_mtime:
        subprocess.run(["g++", "-std=c++17", "-O2", "-pipe", str(source), "-o", str(binary)], check=True)
    return binary


def labeled(image: Image.Image, text: str, width: int) -> Image.Image:
    image = image.convert("RGB")
    image.thumbnail((width, width), Image.Resampling.LANCZOS)
    canvas = Image.new("RGB", (width, image.height + 28), "white")
    x = (width - image.width) // 2
    canvas.paste(image, (x, 28))
    draw = ImageDraw.Draw(canvas)
    draw.text((8, 6), text, fill="black")
    return canvas


def create_sheet(render_dir: Path, case: str, tile: int = 360) -> Path:
    rows: list[Image.Image] = []
    for view, name in enumerate(VIEW_NAMES):
        files = [
            (render_dir / f"original_view{view}_normal.ppm", f"{name} original normal"),
            (render_dir / f"candidate_view{view}_normal.ppm", f"{name} candidate normal"),
            (render_dir / f"original_view{view}_depth.pgm", f"{name} original depth"),
            (render_dir / f"candidate_view{view}_depth.pgm", f"{name} candidate depth"),
        ]
        cells = [labeled(Image.open(path), label, tile) for path, label in files]
        height = max(cell.height for cell in cells)
        row = Image.new("RGB", (tile * 2, height * 2), "white")
        row.paste(cells[0], (0, 0))
        row.paste(cells[1], (tile, 0))
        row.paste(cells[2], (0, height))
        row.paste(cells[3], (tile, height))
        rows.append(row)
    sheet = Image.new("RGB", (tile * 2, sum(row.height for row in rows)), "white")
    y = 0
    for row in rows:
        sheet.paste(row, (0, y))
        y += row.height
    path = render_dir / f"{case}_2x2x6_contact_sheet.png"
    sheet.save(path)
    return path


def main() -> int:
    parser = argparse.ArgumentParser(description="Render and compare two IMC2026 meshes, then create a six-view contact sheet.")
    parser.add_argument("original", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--dest", type=Path, required=True)
    parser.add_argument("--case", default=None)
    parser.add_argument("--resolution", type=int, default=1024)
    parser.add_argument("--tile", type=int, default=360)
    args = parser.parse_args()

    root = Path(__file__).resolve().parent
    renderer = ensure_renderer(root)
    args.dest.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [str(renderer), str(args.original), str(args.candidate), str(args.dest), str(args.resolution)],
        check=True,
    )
    case = args.case or args.original.stem
    sheet = create_sheet(args.dest, case, args.tile)
    metrics = json.loads((args.dest / "render_metrics.json").read_text())
    metrics["contact_sheet"] = str(sheet)
    (args.dest / "render_metrics.json").write_text(json.dumps(metrics, indent=2))
    print(json.dumps(metrics, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
