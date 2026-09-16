import adsk.core
import adsk.fusion
import math
import traceback


# Fusion 360 stores model distances in centimeters internally.
MM_TO_CM = 0.1


PARAMS_MM = {
    "plate_length": 110.0,
    "plate_width": 60.0,
    "plate_thickness": 3.0,
    "base_plate_thickness": 4.0,
    "base_leg_height": 30.0,
    "base_leg_size": 7.0,
    "shell_wall_thickness": 3.0,
    "shell_side_clearance": 0.6,
    "shell_top_clearance": 4.0,
    "shell_screw_clearance_diameter": 3.0,
    "shell_screw_height_below_plate": 18.0,
    "magnet_ear_width": 14.0,
    "magnet_ear_hole_offset_from_shell": 10.0,
    "magnet_hole_diameter": 4.6,
    "corner_hole_diameter": 3.0,
    "corner_hole_offset": 3.5,
    "standoff_height": 5.0,
    "standoff_diameter": 8.5,
    "arduino_standoff_top_diameter": 7.0,
    # Preliminary M2.5 heat-set insert pocket. Tune after choosing the exact insert.
    "insert_pocket_diameter": 3.8,
    "insert_pocket_depth": 3.5,
    "arduino_placeholder_height": 40.0,
    "bms_placeholder_height": 20.0,
    "board_mount_hole_diameter": 3.2,
    # Shift both boards toward one end of the mounting plate while preserving
    # each board's internal hole spacing from the reference drawings.
    "board_offset_x": 13.0,
    "board_offset_y": 0.0,
    "arduino_length": 68.58,
    "arduino_width": 53.34,
    "bms_length": 65.15,
    "bms_width": 56.21,
    "cable_notch_width": 12.0,
    "cable_notch_center_y": -5.0,
    "switch_slot_from_bms_edge_near": 5.0,
    "switch_slot_from_bms_edge_far": 25.0,
    "switch_slot_height": 10.0,
    "cable_exit_diameter": 10.0,
    "cable_exit_x": 14.0,
    "cable_exit_above_arduino_base": 30.0,
    "led_frame_center_x": -29.5,
    "led_frame_center_z": 28.0,
    "led_recess_length": 34.8,
    "led_recess_height": 26.8,
    "led_recess_depth": 0.8,
    "antenna_hole_diameter": 10.0,
    "antenna_above_arduino_base": 35.0,
    "antenna_offset_from_arduino_side": 22.0,
    # MPU6050 is 34 x 26 mm. Put the 34 mm side parallel to the base plate's
    # short side, so the locating pocket is 27 mm in X and 35 mm in Y.
    "mpu6050_inner_length": 27.0,
    "mpu6050_inner_width": 35.0,
    "mpu6050_frame_wall": 2.5,
    "mpu6050_frame_height": 3.0,
    "mpu6050_frame_center_x": -38.0,
    "mpu6050_frame_center_y": 0.0,
}


GENERATED_NAME_MARKERS = (
    "mounting plate",
    "Arduino placeholder",
    "BMS placeholder",
    "Arduino standoff",
    "BMS standoff",
    "base plate",
    "base support leg",
    "base magnet",
    "shell cover",
    "MPU6050 frame",
)


def mm(value):
    return value * MM_TO_CM


def offset_plane(component, base_plane, offset_mm, name):
    planes = component.constructionPlanes
    plane_input = planes.createInput()
    plane_input.setByOffset(base_plane, adsk.core.ValueInput.createByReal(mm(offset_mm)))
    plane = planes.add(plane_input)
    plane.name = name
    return plane


def add_rect_body(
    component,
    plane,
    length_mm,
    width_mm,
    thickness_mm,
    name,
    center_x_mm=0.0,
    center_y_mm=0.0,
    operation=adsk.fusion.FeatureOperations.NewBodyFeatureOperation,
):
    sketch = component.sketches.add(plane)
    sketch.name = f"{name} sketch"

    half_length = mm(length_mm / 2.0)
    half_width = mm(width_mm / 2.0)
    center_x = mm(center_x_mm)
    center_y = mm(center_y_mm)
    sketch.sketchCurves.sketchLines.addTwoPointRectangle(
        adsk.core.Point3D.create(center_x - half_length, center_y - half_width, 0),
        adsk.core.Point3D.create(center_x + half_length, center_y + half_width, 0),
    )

    extrudes = component.features.extrudeFeatures
    ext_input = extrudes.createInput(
        sketch.profiles.item(0),
        operation,
    )
    ext_input.setDistanceExtent(False, adsk.core.ValueInput.createByReal(mm(thickness_mm)))
    ext = extrudes.add(ext_input)
    if ext.bodies.count > 0:
        body = ext.bodies.item(0)
        body.name = name
        return body
    return None


def cut_rectangles(component, plane, rectangles_mm, depth_mm, name, participant_bodies=None):
    sketch = component.sketches.add(plane)
    sketch.name = f"{name} sketch"
    lines = sketch.sketchCurves.sketchLines

    for center_x, center_y, length, width in rectangles_mm:
        half_l = mm(length / 2.0)
        half_w = mm(width / 2.0)
        center_x_cm = mm(center_x)
        center_y_cm = mm(center_y)
        lines.addTwoPointRectangle(
            adsk.core.Point3D.create(center_x_cm - half_l, center_y_cm - half_w, 0),
            adsk.core.Point3D.create(center_x_cm + half_l, center_y_cm + half_w, 0),
        )

    profiles = adsk.core.ObjectCollection.create()
    for profile in sketch.profiles:
        profiles.add(profile)

    extrudes = component.features.extrudeFeatures
    ext_input = extrudes.createInput(
        profiles,
        adsk.fusion.FeatureOperations.CutFeatureOperation,
    )
    ext_input.setDistanceExtent(False, adsk.core.ValueInput.createByReal(mm(depth_mm)))
    if participant_bodies:
        ext_input.participantBodies = participant_bodies
    extrudes.add(ext_input)


def cut_rounded_rectangle(component, plane, center_x, center_y, length, width, radius, depth, name):
    cut_rectangles(
        component,
        plane,
        [(center_x, center_y, length, width - radius * 2.0)],
        depth,
        f"{name} center band",
    )
    cut_rectangles(
        component,
        plane,
        [(center_x, center_y, length - radius * 2.0, width)],
        depth,
        f"{name} side band",
    )

    corner_centers = [
        (center_x - length / 2.0 + radius, center_y - width / 2.0 + radius),
        (center_x + length / 2.0 - radius, center_y - width / 2.0 + radius),
        (center_x + length / 2.0 - radius, center_y + width / 2.0 - radius),
        (center_x - length / 2.0 + radius, center_y + width / 2.0 - radius),
    ]
    cut_cylinders(component, plane, corner_centers, radius * 2.0, depth, f"{name} corner radius")


def add_long_edge_magnet_ears(component, plane, half_length, half_width, thickness):
    ear_width = PARAMS_MM["magnet_ear_width"]
    hole_offset = PARAMS_MM["magnet_ear_hole_offset_from_shell"]
    ear_y_offset = half_width - ear_width / 2.0
    base_overlap = 1.0

    hole_centers = []
    for y in (-ear_y_offset, ear_y_offset):
        for side in (-1.0, 1.0):
            neck_length = hole_offset + base_overlap
            neck_center_x = side * (half_length - base_overlap / 2.0 + neck_length / 2.0)
            end_center_x = side * (half_length + hole_offset)
            add_rect_body(
                component,
                plane,
                neck_length,
                ear_width,
                thickness,
                "base magnet ear neck",
                neck_center_x,
                y,
                adsk.fusion.FeatureOperations.JoinFeatureOperation,
            )
            add_cylinders(
                component,
                plane,
                [(end_center_x, y)],
                ear_width,
                thickness,
                adsk.fusion.FeatureOperations.JoinFeatureOperation,
                "base magnet ear rounded end",
            )
            hole_centers.append((end_center_x, y))

    cut_cylinders(
        component,
        plane,
        hole_centers,
        PARAMS_MM["magnet_hole_diameter"],
        thickness,
        "base magnet mounting hole",
    )


def add_shell_cover(
    component,
    plane,
    inner_length,
    inner_width,
    height,
    wall,
    name_prefix="shell cover",
):
    outer_length = inner_length + wall * 2.0
    outer_width = inner_width + wall * 2.0

    shell_body = add_rect_body(
        component,
        plane,
        outer_length,
        outer_width,
        height,
        name_prefix,
    )
    cut_rectangles(
        component,
        plane,
        [(0.0, 0.0, inner_length, inner_width)],
        height - wall,
        f"{name_prefix} inner cavity",
        [shell_body],
    )
    return shell_body


def cut_end_face_holes(component, side, face_x, centers_yz, diameter, depth, name):
    plane = offset_plane(component, component.yZConstructionPlane, face_x, f"{name} plane")
    sketch = component.sketches.add(plane)
    sketch.name = f"{name} sketch"
    for y, z in centers_yz:
        center = sketch.modelToSketchSpace(adsk.core.Point3D.create(mm(face_x), mm(y), mm(z)))
        sketch.sketchCurves.sketchCircles.addByCenterRadius(center, mm(diameter / 2.0))

    profiles = adsk.core.ObjectCollection.create()
    for profile in sketch.profiles:
        profiles.add(profile)
    ext_input = component.features.extrudeFeatures.createInput(
        profiles, adsk.fusion.FeatureOperations.CutFeatureOperation
    )
    ext_input.setDistanceExtent(False, adsk.core.ValueInput.createByReal(mm(-side * depth)))
    component.features.extrudeFeatures.add(ext_input)


def y_wall_plane(component, y, name):
    normal_y = component.xZConstructionPlane.geometry.normal.y
    return offset_plane(component, component.xZConstructionPlane, y / normal_y, name)


def extrude_y_wall_profile(component, plane, center_x, center_z, length, height, depth, name,
                           operation, diameter=None, participant_bodies=None):
    sketch = component.sketches.add(plane)
    sketch.name = f"{name} sketch"
    y = plane.geometry.origin.y / MM_TO_CM
    center = sketch.modelToSketchSpace(adsk.core.Point3D.create(mm(center_x), mm(y), mm(center_z)))
    if diameter is None:
        lower_left = sketch.modelToSketchSpace(
            adsk.core.Point3D.create(mm(center_x - length / 2.0), mm(y), mm(center_z - height / 2.0))
        )
        upper_right = sketch.modelToSketchSpace(
            adsk.core.Point3D.create(mm(center_x + length / 2.0), mm(y), mm(center_z + height / 2.0))
        )
        sketch.sketchCurves.sketchLines.addTwoPointRectangle(lower_left, upper_right)
    else:
        sketch.sketchCurves.sketchCircles.addByCenterRadius(center, mm(diameter / 2.0))
    ext_input = component.features.extrudeFeatures.createInput(sketch.profiles.item(0), operation)
    inward_direction = -1.0 if plane.geometry.normal.y > 0 else 1.0
    ext_input.setDistanceExtent(False, adsk.core.ValueInput.createByReal(mm(depth * inward_direction)))
    if participant_bodies:
        ext_input.participantBodies = participant_bodies
    ext = component.features.extrudeFeatures.add(ext_input)
    if ext.bodies.count and operation == adsk.fusion.FeatureOperations.NewBodyFeatureOperation:
        ext.bodies.item(0).name = name
    return ext


def add_led_outer_recess(component, outer_wall_plane, shell_body):
    extrude_y_wall_profile(
        component,
        outer_wall_plane,
        PARAMS_MM["led_frame_center_x"],
        PARAMS_MM["led_frame_center_z"],
        PARAMS_MM["led_recess_length"],
        PARAMS_MM["led_recess_height"],
        PARAMS_MM["led_recess_depth"],
        "LED exterior locating recess",
        adsk.fusion.FeatureOperations.CutFeatureOperation,
        participant_bodies=[shell_body],
    )


def add_mpu6050_frame(component, plane):
    inner_l = PARAMS_MM["mpu6050_inner_length"]
    inner_w = PARAMS_MM["mpu6050_inner_width"]
    wall = PARAMS_MM["mpu6050_frame_wall"]
    height = PARAMS_MM["mpu6050_frame_height"]
    center_x = PARAMS_MM["mpu6050_frame_center_x"]
    center_y = PARAMS_MM["mpu6050_frame_center_y"]

    outer_l = inner_l + wall * 2.0
    rail_y = inner_w / 2.0 + wall / 2.0
    rails = [
        (center_x, center_y - rail_y, outer_l, wall, "MPU6050 frame short-side rail"),
        (center_x, center_y + rail_y, outer_l, wall, "MPU6050 frame short-side rail"),
    ]

    for rail_center_x, rail_center_y, rail_l, rail_w, name in rails:
        add_rect_body(
            component,
            plane,
            rail_l,
            rail_w,
            height,
            name,
            rail_center_x,
            rail_center_y,
            adsk.fusion.FeatureOperations.JoinFeatureOperation,
        )


def add_cylinders(component, plane, centers_mm, diameter_mm, height_mm, operation, name_prefix):
    sketch = component.sketches.add(plane)
    sketch.name = f"{name_prefix} sketch"
    circles = sketch.sketchCurves.sketchCircles
    for x, y in centers_mm:
        circles.addByCenterRadius(adsk.core.Point3D.create(mm(x), mm(y), 0), mm(diameter_mm / 2.0))

    profiles = adsk.core.ObjectCollection.create()
    for profile in sketch.profiles:
        profiles.add(profile)

    extrudes = component.features.extrudeFeatures
    ext_input = extrudes.createInput(profiles, operation)
    ext_input.setDistanceExtent(False, adsk.core.ValueInput.createByReal(mm(height_mm)))
    ext = extrudes.add(ext_input)

    for index in range(ext.bodies.count):
        ext.bodies.item(index).name = f"{name_prefix} {index + 1}"

    return ext


def add_tapered_cylinders(component, plane, centers_mm, bottom_diameter_mm,
                          top_diameter_mm, height_mm, name_prefix):
    sketch = component.sketches.add(plane)
    sketch.name = f"{name_prefix} sketch"
    for x, y in centers_mm:
        sketch.sketchCurves.sketchCircles.addByCenterRadius(
            adsk.core.Point3D.create(mm(x), mm(y), 0), mm(bottom_diameter_mm / 2.0)
        )

    profiles = adsk.core.ObjectCollection.create()
    for profile in sketch.profiles:
        profiles.add(profile)

    ext_input = component.features.extrudeFeatures.createInput(
        profiles, adsk.fusion.FeatureOperations.JoinFeatureOperation
    )
    ext_input.setDistanceExtent(False, adsk.core.ValueInput.createByReal(mm(height_mm)))
    inward_angle = -math.atan((bottom_diameter_mm - top_diameter_mm) / (2.0 * height_mm))
    ext_input.taperAngle = adsk.core.ValueInput.createByReal(inward_angle)
    return component.features.extrudeFeatures.add(ext_input)


def cut_cylinders(component, plane, centers_mm, diameter_mm, depth_mm, name):
    add_cylinders(
        component,
        plane,
        centers_mm,
        diameter_mm,
        depth_mm,
        adsk.fusion.FeatureOperations.CutFeatureOperation,
        name,
    )


def delete_collection_items(collection):
    for index in range(collection.count - 1, -1, -1):
        try:
            collection.item(index).deleteMe()
        except Exception:
            pass


def delete_previous_generated_bodies(component):
    # This script owns the current mockup design. Clear generated timeline
    # features first; deleting only bodies leaves stale parametric operations.
    delete_collection_items(component.features.moveFeatures)
    delete_collection_items(component.features.extrudeFeatures)
    delete_collection_items(component.sketches)
    delete_collection_items(component.constructionPlanes)

    for index in range(component.bRepBodies.count - 1, -1, -1):
        try:
            component.bRepBodies.item(index).deleteMe()
        except Exception:
            pass


def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        design = adsk.fusion.Design.cast(app.activeProduct)

        if not design:
            doc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
            design = adsk.fusion.Design.cast(doc.products.itemByProductType("DesignProductType"))

        design.designType = adsk.fusion.DesignTypes.ParametricDesignType
        component = design.rootComponent
        delete_previous_generated_bodies(component)

        length = PARAMS_MM["plate_length"]
        width = PARAMS_MM["plate_width"]
        shell_wall = PARAMS_MM["shell_wall_thickness"]
        shell_side_clearance = PARAMS_MM["shell_side_clearance"]
        shell_inner_length = length + 2.0 * shell_side_clearance
        shell_inner_width = width + 2.0 * shell_side_clearance
        base_length = shell_inner_length + 2.0 * shell_wall
        base_width = shell_inner_width + 2.0 * shell_wall
        thickness = PARAMS_MM["plate_thickness"]
        base_thickness = PARAMS_MM["base_plate_thickness"]
        base_leg_height = PARAMS_MM["base_leg_height"]
        standoff_height = PARAMS_MM["standoff_height"]
        arduino_height = PARAMS_MM["arduino_placeholder_height"]
        bms_height = PARAMS_MM["bms_placeholder_height"]
        board_offset_x = PARAMS_MM["board_offset_x"]
        board_offset_y = PARAMS_MM["board_offset_y"]

        half_length = length / 2.0
        half_width = width / 2.0
        base_half_length = base_length / 2.0
        base_half_width = base_width / 2.0
        corner_offset = PARAMS_MM["corner_hole_offset"]

        base_plane = component.xYConstructionPlane
        base_bottom_plane = offset_plane(
            component,
            base_plane,
            -base_leg_height - base_thickness,
            "Bottom of base plate",
        )
        base_top_plane = offset_plane(
            component,
            base_plane,
            -base_leg_height,
            "Top of base plate",
        )

        add_rect_body(
            component,
            base_bottom_plane,
            base_length,
            base_width,
            base_thickness,
            "base plate 117.2x67.2x4 mm",
        )

        add_long_edge_magnet_ears(
            component,
            base_bottom_plane,
            base_half_length,
            base_half_width,
            base_thickness,
        )

        plate_body = add_rect_body(
            component,
            base_plane,
            length,
            width,
            thickness,
            "110x60x3 mm mounting plate",
        )

        plate_corner_holes = [
            (-half_length + corner_offset, -half_width + corner_offset),
            (half_length - corner_offset, -half_width + corner_offset),
            (half_length - corner_offset, half_width - corner_offset),
            (-half_length + corner_offset, half_width - corner_offset),
        ]

        for index, (leg_x, leg_y) in enumerate(plate_corner_holes):
            add_rect_body(
                component,
                base_top_plane,
                PARAMS_MM["base_leg_size"],
                PARAMS_MM["base_leg_size"],
                base_leg_height - 0.1,
                f"base support leg square {index + 1}",
                leg_x,
                leg_y,
                adsk.fusion.FeatureOperations.JoinFeatureOperation,
            )
        cut_cylinders(
            component,
            base_plane,
            plate_corner_holes,
            PARAMS_MM["insert_pocket_diameter"],
            -PARAMS_MM["insert_pocket_depth"],
            "base leg heat-set insert pocket",
        )
        cut_cylinders(
            component,
            base_plane,
            plate_corner_holes,
            PARAMS_MM["corner_hole_diameter"],
            thickness,
            "plate corner hole",
        )

        # Lightweight cutouts only on the free end, away from board standoffs
        # and corner mounting holes.
        cut_rounded_rectangle(
            component,
            base_plane,
            -38.0,
            0.0,
            25.0,
            38.0,
            4.0,
            thickness,
            "mounting plate lightening cutout",
        )

        cut_cylinders(
            component,
            base_plane,
            [(half_length, PARAMS_MM["cable_notch_center_y"])],
            PARAMS_MM["cable_notch_width"],
            thickness,
            "mounting plate semicircle cable notch",
        )

        add_mpu6050_frame(component, base_top_plane)

        # Arduino UNO R4 WiFi outline/hole positions from the reference drawing.
        arduino_l = PARAMS_MM["arduino_length"]
        arduino_w = PARAMS_MM["arduino_width"]
        arduino_holes_from_lower_left = [
            (13.97, 2.54),
            (15.24, arduino_w - 6.40),
            (arduino_l - 2.54, 7.62),
            (arduino_l - 2.54, arduino_w - 17.78),
        ]
        arduino_centers = [
            (x - arduino_l / 2.0 + board_offset_x, y - arduino_w / 2.0 + board_offset_y)
            for x, y in arduino_holes_from_lower_left
        ]

        # BMS/power board positions estimated from the reference image:
        # board 65.15 x 56.21 mm, lower hole spacing 58.00 mm, vertical spacing 49.20 mm.
        bms_centers = [
            (-58.00 / 2.0 + board_offset_x, -49.20 / 2.0 + board_offset_y),
            (58.00 / 2.0 + board_offset_x, -49.20 / 2.0 + board_offset_y),
            (58.00 / 2.0 + board_offset_x, 49.20 / 2.0 + board_offset_y),
            (-58.00 / 2.0 + board_offset_x, 49.20 / 2.0 + board_offset_y),
        ]

        cut_rounded_rectangle(
            component,
            base_plane,
            17.0,
            0.0,
            32.0,
            32.0,
            4.0,
            thickness,
            "mounting plate central lightening cutout",
        )

        top_standoff_start = offset_plane(
            component, base_plane, thickness - 0.2, "Arduino standoff join plane"
        )
        top_standoff_top = offset_plane(
            component,
            base_plane,
            thickness + standoff_height,
            "Top of Arduino standoffs",
        )
        bottom_standoff_bottom = offset_plane(
            component,
            base_plane,
            -standoff_height,
            "Bottom of BMS standoffs",
        )
        bottom_board_bottom = offset_plane(
            component,
            base_plane,
            -standoff_height - bms_height,
            "Bottom of BMS placeholder board",
        )

        add_tapered_cylinders(
            component,
            top_standoff_start,
            arduino_centers,
            PARAMS_MM["standoff_diameter"],
            PARAMS_MM["arduino_standoff_top_diameter"],
            standoff_height + 0.2,
            "Arduino standoff",
        )
        cut_cylinders(
            component,
            top_standoff_top,
            arduino_centers,
            PARAMS_MM["insert_pocket_diameter"],
            -PARAMS_MM["insert_pocket_depth"],
            "Arduino heat-set insert pocket",
        )

        add_cylinders(
            component,
            bottom_standoff_bottom,
            bms_centers,
            PARAMS_MM["standoff_diameter"],
            standoff_height + 0.2,
            adsk.fusion.FeatureOperations.JoinFeatureOperation,
            "BMS standoff",
        )
        cut_cylinders(
            component,
            bottom_standoff_bottom,
            bms_centers,
            PARAMS_MM["insert_pocket_diameter"],
            PARAMS_MM["insert_pocket_depth"],
            "BMS heat-set insert pocket",
        )

        arduino_body = add_rect_body(
            component,
            top_standoff_top,
            arduino_l,
            arduino_w,
            arduino_height,
            "Arduino placeholder board 68.58x53.34x40 mm",
            board_offset_x,
            board_offset_y,
        )
        bms_body = add_rect_body(
            component,
            bottom_board_bottom,
            PARAMS_MM["bms_length"],
            PARAMS_MM["bms_width"],
            bms_height,
            "BMS placeholder board 65.15x56.21x20 mm",
            board_offset_x,
            board_offset_y,
        )

        cut_cylinders(
            component,
            top_standoff_top,
            arduino_centers,
            PARAMS_MM["board_mount_hole_diameter"],
            arduino_height,
            "Arduino placeholder mounting hole",
        )
        cut_cylinders(
            component,
            bottom_board_bottom,
            bms_centers,
            PARAMS_MM["board_mount_hole_diameter"],
            bms_height,
            "BMS placeholder mounting hole",
        )

        plate_body.name = "110x60x3 mm mounting plate with standoffs"
        arduino_body.name = "Arduino placeholder board"
        bms_body.name = "BMS placeholder board"

        content_top = thickness + standoff_height + arduino_height
        shell_start = -base_leg_height
        shell_height = content_top - shell_start + PARAMS_MM["shell_top_clearance"] + shell_wall
        shell_body = add_shell_cover(
            component,
            base_top_plane,
            shell_inner_length,
            shell_inner_width,
            shell_height,
            shell_wall,
        )

        side_hole_z = -PARAMS_MM["shell_screw_height_below_plate"]
        leg_outer_x = half_length
        shell_outer_x = shell_inner_length / 2.0 + shell_wall
        for side in (-1.0, 1.0):
            centers_yz = [(y, side_hole_z) for x, y in plate_corner_holes if x * side > 0]
            cut_end_face_holes(
                component,
                side,
                side * shell_outer_x,
                centers_yz,
                PARAMS_MM["shell_screw_clearance_diameter"],
                shell_wall,
                "shell cover side screw clearance",
            )
            cut_end_face_holes(
                component,
                side,
                side * leg_outer_x,
                centers_yz,
                PARAMS_MM["insert_pocket_diameter"],
                PARAMS_MM["insert_pocket_depth"],
                "base leg side heat-set insert pocket",
            )

        shell_outer_y = shell_inner_width / 2.0 + shell_wall
        outer_side_plane = y_wall_plane(component, shell_outer_y, "Accessory openings on positive Y wall")
        bms_near_edge_x = board_offset_x + PARAMS_MM["bms_length"] / 2.0
        switch_x_near = bms_near_edge_x - PARAMS_MM["switch_slot_from_bms_edge_near"]
        switch_x_far = bms_near_edge_x - PARAMS_MM["switch_slot_from_bms_edge_far"]
        switch_center_z = -standoff_height - bms_height / 2.0
        extrude_y_wall_profile(
            component, outer_side_plane,
            (switch_x_near + switch_x_far) / 2.0,
            switch_center_z,
            switch_x_near - switch_x_far,
            PARAMS_MM["switch_slot_height"],
            shell_wall,
            "BMS switch access slot",
            adsk.fusion.FeatureOperations.CutFeatureOperation,
            participant_bodies=[shell_body],
        )

        arduino_base_z = thickness + standoff_height
        extrude_y_wall_profile(
            component, outer_side_plane,
            PARAMS_MM["cable_exit_x"],
            arduino_base_z + PARAMS_MM["cable_exit_above_arduino_base"],
            0.0, 0.0, shell_wall,
            "Arduino cable exit hole",
            adsk.fusion.FeatureOperations.CutFeatureOperation,
            diameter=PARAMS_MM["cable_exit_diameter"],
            participant_bodies=[shell_body],
        )
        add_led_outer_recess(component, outer_side_plane, shell_body)

        antenna_y = PARAMS_MM["arduino_width"] / 2.0 - PARAMS_MM["antenna_offset_from_arduino_side"]
        cut_end_face_holes(
            component, -1.0, -shell_outer_x,
            [(antenna_y, arduino_base_z + PARAMS_MM["antenna_above_arduino_base"])],
            PARAMS_MM["antenna_hole_diameter"],
            shell_wall,
            "antenna hole on cutout end",
        )

        app.activeViewport.fit()
        document = app.activeDocument
        if document.isSaved and not document.save("Simplify shell-to-base joint and add LED recess"):
            raise RuntimeError("Fusion could not save the updated enclosure design")
        ui.messageBox(
            "Created mounting plate assembly mockup:\n"
            "- 110 x 60 x 3 mm mounting plate\n"
            "- 117.2 x 67.2 x 4 mm base plate with 29.9 mm support legs\n"
            "- shell starts at the base top, with no lower ear-clearance slots\n"
            "- shell inner size 111.2 x 61.2 mm (0.6 mm clearance per side)\n"
            "- two 3.0 mm screw holes on each end wall, aligned to the legs\n"
            "- four horizontal 3.8 x 3.5 mm heat-set insert pockets in the legs\n"
            "- 40 mm Arduino clearance block above\n"
            "- 20 mm BMS/power clearance block below\n"
            "- boards shifted 13 mm toward one end\n"
            "- rounded lightening cutout on the free end\n"
            "- additional 32 x 32 mm rounded cutout between the boards\n"
            "- Arduino standoffs taper from 8.5 to 7.0 mm\n"
            "- shifted semicircle cable notch between Arduino-side standoffs\n"
            "- MPU6050 two-short-edge locating rails on the base plate\n"
            "- four length-direction capsule magnet ears with 4.6 mm holes\n"
            "- BMS switch slot, 10 mm cable exit, and 34.8 x 26.8 x 0.8 mm exterior LED recess\n"
            "- 10 mm antenna hole on the cutout end\n"
            "- preliminary 3.8 mm heat-set insert pockets"
        )

    except Exception:
        if ui:
            ui.messageBox("Failed:\n{}".format(traceback.format_exc()))
