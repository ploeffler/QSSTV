#!/usr/bin/env python3
"""Render source Qt Designer forms in a virtual display (no radio hardware needed).

Custom QSSTV widgets are represented by visible placeholders because their
runtime painting and behaviour cannot be reproduced by loading .ui alone.
"""
import os
import sys
from pathlib import Path
from PyQt5 import QtCore, QtWidgets, QtUiTools

ROOT = Path(__file__).resolve().parents[1]
FORMS = {
    "main-window": "src/mainwindow.ui",
    "receive": "src/mainwidgets/rxwidget.ui",
    "transmit": "src/mainwidgets/txwidget.ui",
    "gallery": "src/mainwidgets/gallerywidget.ui",
    "configuration": "src/config/configdialog.ui",
}

class Loader(QtUiTools.QUiLoader):
    def createWidget(self, className, parent=None, name=""):
        try:
            widget = super().createWidget(className, parent, name)
        except RuntimeError:
            widget = None
        if widget is None:
            widget = QtWidgets.QFrame(parent)
            widget.setFrameShape(QtWidgets.QFrame.StyledPanel)
            layout = QtWidgets.QVBoxLayout(widget)
            label = QtWidgets.QLabel(className + " (runtime widget)", widget)
            label.setAlignment(QtCore.Qt.AlignCenter)
            layout.addWidget(label)
            widget.setMinimumSize(160, 100)
        widget.setObjectName(name)
        return widget

def main():
    output = Path(sys.argv[1] if len(sys.argv) > 1 else "screenshots")
    output.mkdir(parents=True, exist_ok=True)
    app = QtWidgets.QApplication(sys.argv)
    loader = Loader()
    for label, path in FORMS.items():
        source = ROOT / path
        qfile = QtCore.QFile(str(source))
        if not qfile.open(QtCore.QIODevice.ReadOnly):
            raise RuntimeError("Cannot open " + str(source))
        widget = loader.load(qfile)
        qfile.close()
        if widget is None:
            raise RuntimeError("Cannot load " + path + ": " + loader.errorString())
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
