from PySide2.QtCore import QObject
from PySide2.QtWidgets import QFrame, QApplication
from h2core import H2Core as h2core
Logger = h2core.Logger
logger = Logger.bootstrap(
    Logger.Debug | Logger.Error | Logger.Warning | Logger.Info
)
h2core.Object.bootstrap(logger, True)
h2core.Filesystem.bootstrap(logger, "/usr/local/share/hydrogen/data/")
prefs = h2core.Preferences.create_instance()
hydrogen = h2core.Hydrogen.create_instance()
song = h2core.Song("foo", "bar", 120, 1)
print(song)
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
fr.installEventFilter(spy)

fr.show()

if not interactive:
    app.exec_()


