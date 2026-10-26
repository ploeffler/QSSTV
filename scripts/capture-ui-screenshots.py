#!/usr/bin/env python3
"""Render source Qt Designer forms in a virtual display (no radio hardware needed).

Custom QSSTV widgets are represented by visible placeholders because their
runtime painting and behaviour cannot be reproduced by loading .ui alone.
"""
import os
import sys
from pathlib import Path
from PyQt5 import QtCore, QtWidgets, uic
from PyQt5.uic.Loader import DynamicUILoader
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
FORMS = {
    "main-window": "src/mainwindow.ui",
    "receive": "src/mainwidgets/rxwidget.ui",
    "transmit": "src/mainwidgets/txwidget.ui",
    "gallery": "src/mainwidgets/gallerywidget.ui",
    "configuration": "src/config/configdialog.ui",
}

def load_form(source):
    tree = ET.parse(source)
    custom = {}
    for entry in tree.findall(".//customwidgets/customwidget"):
        name = entry.findtext("class")
        extends = entry.findtext("extends", default="QWidget")
        base = getattr(QtWidgets, extends, QtWidgets.QWidget)
        # Placeholder for QSSTV runtime widgets, preserving the Qt layout.
        custom[name] = type(name, (base,), {})
    loader = DynamicUILoader(customWidgets=custom)
    return loader.loadUi(str(source))

def main():
    output = Path(sys.argv[1] if len(sys.argv) > 1 else "screenshots")
    output.mkdir(parents=True, exist_ok=True)
    app = QtWidgets.QApplication(sys.argv)
    for label, path in FORMS.items():
        source = ROOT / path
        widget = load_form(source)
        widget.resize(max(widget.width(), 1100), max(widget.height(), 760))
        widget.show()
        app.processEvents()
        target = output / (label + ".png")
        if not widget.grab().save(str(target)):
            raise RuntimeError("Failed to save " + str(target))
        print(target)
        widget.close()

if __name__ == "__main__":
    main()
