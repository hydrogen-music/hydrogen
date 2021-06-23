from PySide2.QtCore import QObject
from PySide2.QtWidgets import QFrame, QApplication
import h2gui
interactive = True 
app = QApplication.instance()
if not app:
    app = QApplication()
    interactive = False

fr = QFrame()

widget = h2gui.MainSampleWaveDisplay(fr)

class Spy(QObject):

    def eventFilter(self, obj, event):
        print("e", obj, event)
        return QObject.eventFilter(self, obj, event)
    
    def sliderEdited(self, mode):
        print("ES", mode)

    def doneEditingSlider(self, mode):
        print("DS", mode)

spy = Spy()

widget.sliderEdited.connect(spy.sliderEdited)
widget.doneEditingSlider.connect(spy.doneEditingSlider)
# fr.installEventFilter(spy)

fr.show()

if not interactive:
    app.exec_()


