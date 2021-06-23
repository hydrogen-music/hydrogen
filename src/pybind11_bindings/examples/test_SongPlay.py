from hydra.H2Core import *
import time
logger = Logger.bootstrap(
    Logger.Debug | Logger.Error | Logger.Warning | Logger.Info
)
Object.bootstrap(logger, True)
Filesystem.bootstrap(logger, "/usr/local/share/hydrogen/data/")
prefs = Preferences.create_instance()
hydrogen = Hydrogen.get_instance( False, True )
dk = Drumkit.load_file('/home/rebelcat/.hydrogen/data/drumkits/RoRBateria/drumkit.xml')
song = Song.load('/home/rebelcat/envelope_test.h2song')
hydrogen.setSong(song)
hydrogen.sequencer_play()
time.sleep(60)
hydrogen.sequencer_stop()
print(song)
