from __future__ import annotations

import json
import os
from pathlib import Path

import win32com.client


ROOT = Path(r"E:/PROJ/ArduinoNode/DEMO")
SOURCE = ROOT / "ArduinoNode_APESS2025_backup_20260929.pptx"
OUTPUT = ROOT / ".ppt-build/output/ArduinoNode_NTU_source_faithful_v3.pptx"
PLOT = ROOT / ".ppt-build/assets/PLOTS-6-1.png"
AUDIT = ROOT / ".ppt-build/source_faithful_audit.json"


def rgb(hex_color: str) -> int:
    h = hex_color.lstrip("#")
    return int(h[0:2], 16) + int(h[2:4], 16) * 256 + int(h[4:6], 16) * 65536


DEEP = rgb("#265D7B")
INK = rgb("#244B63")
MUTED = rgb("#6D8290")
ACCENT = rgb("#3B9299")
PALE = rgb("#8FD7DC")
PAPER = rgb("#F9FAFB")
WHITE = rgb("#FFFFFF")


def set_background(slide, color: int) -> None:
    slide.Background.Fill.Solid()
    slide.Background.Fill.ForeColor.RGB = color
    if color == DEEP:
        backing = slide.Shapes.AddShape(1, 0, 0, 720, 450)
        backing.Fill.Solid()
        backing.Fill.ForeColor.RGB = DEEP
        backing.Line.Visible = 0
        backing.ZOrder(1)


def add_text(slide, value: str, left: float, top: float, width: float,
             height: float, size: float, color: int, bold: bool = False,
             align: int = 1):
    shape = slide.Shapes.AddTextbox(1, left, top, width, height)
    shape.TextFrame.MarginLeft = 0
    shape.TextFrame.MarginRight = 0
    shape.TextFrame.MarginTop = 0
    shape.TextFrame.MarginBottom = 0
    run = shape.TextFrame.TextRange
    run.Text = value
    run.Font.Name = "Arial"
    run.Font.Size = size
    run.Font.Bold = -1 if bold else 0
    run.Font.Color.RGB = color
    run.ParagraphFormat.Alignment = align
    return shape


def text_of(shape) -> str:
    try:
        return shape.TextFrame.TextRange.Text
    except Exception:
        return ""


def prefix_for(original_number: int) -> str | None:
    if 6 <= original_number <= 18:
        return f"I.{original_number - 5}"
    if 21 <= original_number <= 41:
        return f"II.{original_number - 20}"
    if 42 <= original_number <= 43:
        return f"III.{original_number - 41}"
    return None


app = win32com.client.DispatchEx("PowerPoint.Application")
app.Visible = 1
pres = None
try:
    pres = app.Presentations.Open(str(SOURCE), WithWindow=False, ReadOnly=False)
    assert pres.Slides.Count == 44
    assert abs(pres.PageSetup.SlideWidth - 720) < 0.1
    assert abs(pres.PageSetup.SlideHeight - 450) < 0.1

    original_text = {}
    for n in range(1, 45):
        slide = pres.Slides(n)
        original_text[n] = [text_of(shape) for shape in slide.Shapes if text_of(shape).strip()]

        dark = n in (1, 5, 19)
        set_background(slide, DEEP if dark else PAPER)
        title = next(shape for shape in slide.Shapes if text_of(shape).strip())
        title_run = title.TextFrame.TextRange
        title_before = title_run.Text
        prefix = prefix_for(n)

        if n == 1:
            title.Left = 64
            title.Top = 132
            title.Width = 592
            title.Height = 110
            title_run.Font.Name = "Arial"
            title_run.Font.Size = 30
            title_run.Font.Bold = -1
            title_run.Font.Color.RGB = WHITE
            title_run.ParagraphFormat.Alignment = 2

            # Replace the original outlined WordArt with plain, editable text.
            label = next(s for s in slide.Shapes if text_of(s).strip() == "Lab Experiment Course")
            label_text = text_of(label)
            label.Delete()
            add_text(slide, label_text, 40, 28, 640, 36, 22, PALE, False, 2)
            author = next(s for s in slide.Shapes if "Yuguang Fu" in text_of(s))
            author.TextFrame.TextRange.Font.Color.RGB = WHITE
        elif n in (5, 19):
            title.Left = 80
            title.Top = 174
            title.Width = 560
            title.Height = 58
            title_run.Font.Name = "Arial"
            title_run.Font.Size = 34
            title_run.Font.Bold = -1
            title_run.Font.Color.RGB = WHITE
            title_run.ParagraphFormat.Alignment = 2
        elif n == 44:
            title_run.Font.Name = "Arial"
            title_run.Font.Color.RGB = INK
            label = next(s for s in slide.Shapes if text_of(s).strip() == "Lab Experiment Course")
            label_text = text_of(label)
            label.Delete()
            add_text(slide, label_text, 40, 24, 640, 34, 21, ACCENT, False, 2)
        elif prefix:
            cleaned = title_before.lstrip("🪒🔨🛠️ ").strip()
            title_run.Text = f"{prefix}  {cleaned}"
            title.Left = 11
            title.Top = 5
            title.Width = 696
            title.Height = 47
            title_run.Font.Name = "Arial"
            title_run.Font.Size = 29
            title_run.Font.Bold = -1
            title_run.Font.Color.RGB = INK
        elif n in (2, 3, 4):
            title_run.Font.Name = "Arial"
            title_run.Font.Size = 30
            title_run.Font.Bold = -1
            title_run.Font.Color.RGB = INK
        elif n == 20:
            title_run.Font.Color.RGB = INK

        for shape in slide.Shapes:
            if not text_of(shape).strip():
                continue
            if shape.Top > 410 and text_of(shape).strip().isdigit():
                shape.TextFrame.TextRange.Font.Color.RGB = MUTED

    # The original 44 slides remain intact. This divider makes the provenance
    # of the following indoor and outdoor examples clear to NTU students.
    section = pres.Slides.Add(42, 12)
    set_background(section, DEEP)
    add_text(section, "Section III — Vibration Test", 56, 142, 615, 64, 34, WHITE, True)
    add_text(section, "The following indoor and outdoor layouts come from the earlier APESS2025 course.\n"
             "The instructor will confirm which apparatus and procedure are available for this NTU session.",
             58, 226, 605, 82, 17, PALE)

    # One historical result page supplements the original teaching sequence.
    result = pres.Slides.Add(45, 12)
    set_background(result, PAPER)
    add_text(result, "Previous APESS2025 validation", 24, 13, 655, 43, 29, INK, True)
    add_text(result, "A reference case for reading vibration spectra", 25, 57, 655, 28, 16, MUTED)
    result.Shapes.AddPicture(str(PLOT), False, True, 29, 92, 344, 326)
    add_text(result, "Reading exercise", 406, 105, 265, 32, 20, INK, True)
    add_text(result, "Find peaks that recur across sensors.\n\n"
             "Compare the wireless and reference traces.\n\n"
             "Ask whether noise, calibration or mounting could explain any differences.",
             407, 148, 258, 207, 16, INK)
    add_text(result, "Source: PLOTS.pptx · earlier APESS2025 validation; NTU results may differ.",
             25, 421, 620, 17, 10, MUTED)

    # Re-number page markers after the two inserted slides.
    for index in range(1, pres.Slides.Count + 1):
        slide = pres.Slides(index)
        markers = [s for s in slide.Shapes
                   if s.Top > 410 and text_of(s).strip().isdigit()]
        if markers:
            for marker in markers:
                marker.TextFrame.TextRange.Text = str(index)
                marker.TextFrame.TextRange.Font.Color.RGB = MUTED if index not in (1, 5, 19, 42) else PALE
        else:
            add_text(slide, str(index), 645, 420, 36, 18, 9,
                     PALE if index in (1, 5, 19, 42) else MUTED, False, 3)

    # Verify that every source text object except the page marker and first
    # title remains on the same original slide.
    map_index = lambda n: n if n <= 41 else n + 1 if n <= 43 else 46
    changed_body = []
    for n in range(1, 45):
        final_slide = pres.Slides(map_index(n))
        current = [text_of(s) for s in final_slide.Shapes if text_of(s).strip()]
        for value in original_text[n][1:]:
            if value.strip().isdigit():
                continue
            if value not in current:
                changed_body.append({"original_slide": n, "text": value[:120]})
    if changed_body:
        raise RuntimeError(f"Body text changed unexpectedly: {changed_body[:5]}")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    pres.SaveAs(str(OUTPUT), 24)
    AUDIT.write_text(json.dumps({
        "source_slide_count": 44,
        "output_slide_count": pres.Slides.Count,
        "changed_original_body_shapes": changed_body,
        "inserted_slides": [42, 45],
        "source": str(SOURCE),
        "output": str(OUTPUT)
    }, indent=2), encoding="utf-8")
    print(f"saved {OUTPUT}; {pres.Slides.Count} slides; original body text preserved")
finally:
    if pres is not None:
        pres.Close()
    app.Quit()
