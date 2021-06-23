from PySide2.QtCore import QObject
from PySide2.QtWidgets import QFrame, QApplication
import h2gui
interactive = True 
app = QApplication.instance()
if not app:
    app = QApplication()
    interactive = False

fr = QFrame()

widget = h2gui.DetailWaveDisplay(fr)

class Spy(QObject):

    def eventFilter(self, obj, event):
        print("e", obj, event.type())
        return QObject.eventFilter(self, obj, event)
    
    def envelopeEdited(self, mode):
        print("EE", mode)

    def doneEnvelopeEditing(self, mode):
        print("DE", mode)

spy = Spy()

# widget.envelopeEdited.connect(spy.envelopeEdited)
# widget.doneEditingEnvelope.connect(spy.doneEnvelopeEditing)
fr.installEventFilter(spy)

fr.show()

if not interactive:
    app.exec_()


