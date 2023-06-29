OPENSCAD = /Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD
SLICER = /Applications/Original\ Prusa\ Drivers/PrusaSlicer.app/Contents/MacOS/PrusaSlicer

%.stl: case.scad
	$(OPENSCAD) -D'rendering="$(basename $@)"' $< -o $@

case.gcode: top.stl bottom.stl button.stl
	$(SLICER) --center 100,100 --slice --merge $^ --export-gcode --output $@

view: case.gcode
	$(SLICER) --gcodeviewer $^

all: case.gcode

clean:
	rm -f *.stl *.gcode
