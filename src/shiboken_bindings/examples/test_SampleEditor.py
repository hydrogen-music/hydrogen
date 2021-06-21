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

editor = h2gui.SampleEditor(fr, 0, 0, "/home/rebelcat/.hydrogen/data/drumkits/RoRBateria/ls_X.flac") 

class Spy(QObject):

    def eventFilter(self, obj, event):
        print("e", obj, event)
        return QObject.eventFilter(self, obj, event)
    
    def sliderEdited(self, mode):
        # print("ES", mode)
        pass

    def doneEditingSlider(self, mode):
        # print("DS", mode)
        pass

    def envelopeEdited(self, mode):
        # print("EE", mode)
        pass

    def doneEditingEnvelope(self, mode):
        pass

    def debug_combo(self, *args, **kwargs):
        print("DBG:", args, kwargs)


    
spy = Spy()

# widget.sliderEdited.connect(spy.sliderEdited)
# widget.doneEditingSlider.connect(spy.doneEditingSlider)
editor.m_pMainSampleWaveDisplay.sliderEdited.connect(spy.sliderEdited)
editor.m_pMainSampleWaveDisplay.doneEditingSlider.connect(spy.doneEditingSlider)
editor.m_pTargetSampleView.envelopeEdited.connect(spy.envelopeEdited)
editor.m_pTargetSampleView.doneEditingEnvelope.connect(spy.doneEditingEnvelope)
editor.EditTypeComboBox.currentIndexChanged.connect(spy.debug_combo)
#fr.installEventFilter(spy)
editor.show()
fr.show()

if not interactive:
    app.exec_()


