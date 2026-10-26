#!/usr/bin/env python3
"""Render source Qt Designer forms in a virtual display (no radio hardware needed).

Custom QSSTV widgets are represented by visible placeholders because their
runtime painting and behaviour cannot be reproduced by loading .ui alone.
"""
import os
import sys
from pathlib import Path
from PyQt5 import QtCore, QtWidgets, uic
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
    # PyQt5 on Ubuntu 22.04 does not expose customWidgets in loadUi.
    # Compile the .ui into a temporary module and supply placeholder classes
    # through an import hook for promoted widget headers.
    import importlib
    import tempfile
    import types
    from PyQt5.uic import compileUi
    from io import StringIO
    from unittest.mock import patch
    generated = StringIO()
    with source.open('r', encoding='utf-8') as ui_file:
        compileUi(ui_file, generated)
    code = generated.getvalue()
    # Generated modules may import custom widgets from their configured headers.
    # Register local temporary stand-ins, without importing C++ QSSTV modules.
    modules = {}
    for entry in tree.findall('.//customwidgets/customwidget'):
        header = entry.findtext('header')
        name = entry.findtext('class')
        if header and name:
            module_name = header.replace('/', '.').removesuffix('.h').removesuffix('.py')
            module = modules.setdefault(module_name, types.ModuleType(module_name))
            setattr(module, name, custom[name])
    with patch.dict(sys.modules, modules):
        namespace = {'__name__': '_qsstv_ui_preview'}
        exec(compile(code, str(source), 'exec'), namespace)
        form = next(value for key, value in namespace.items() if key.startswith('Ui_') and isinstance(value, type))
        widget_class = getattr(QtWidgets, tree.find('./widget').get('class', 'QWidget'), QtWidgets.QWidget)
        widget = widget_class()
        form().setupUi(widget)
        return widget

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
