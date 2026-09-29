import os
import traceback

import adsk.core
import adsk.fusion


OUTPUT_PATH = r"E:\PROJ\ArduinoNode\Enclosure\export\ArduinoNode_shell_cover.stl"


def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        design = adsk.fusion.Design.cast(app.activeProduct)
        if not design or app.activeDocument.name != "ArduinoNode Enclosure Assembly":
            raise RuntimeError("Open ArduinoNode Enclosure Assembly before exporting")

        bodies = design.rootComponent.bRepBodies
        matches = [bodies.item(i) for i in range(bodies.count)
                   if bodies.item(i).name == "shell cover"]
        if len(matches) != 1 or not matches[0].isSolid:
            raise RuntimeError("Expected exactly one solid shell cover body")

        options = design.exportManager.createSTLExportOptions(matches[0], OUTPUT_PATH)
        options.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementHigh
        options.sendToPrintUtility = False
        if not design.exportManager.execute(options) or os.path.getsize(OUTPUT_PATH) == 0:
            raise RuntimeError("STL export failed")
        ui.messageBox("Updated shell STL:\n{}\n{} bytes".format(
            OUTPUT_PATH, os.path.getsize(OUTPUT_PATH)))
    except Exception:
        if ui:
            ui.messageBox("Shell STL export failed:\n{}".format(traceback.format_exc()))
