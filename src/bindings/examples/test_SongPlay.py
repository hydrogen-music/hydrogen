import h2core
from h2core import Logger, Object, Filesystem, Preferences, Hydrogen, Song
import time
print(h2core)
# log_levels = Logger.log_levels
LogLevel = Logger.log_levels
logger = Logger.bootstrap(LogLevel.Debug | LogLevel.Info | LogLevel.Warning | LogLevel.Error)
Object.bootstrap(logger, True)
Filesystem.bootstrap(logger, "/usr/local/share/hydrogen/data/")
Preferences.create_instance()
prefs = Preferences.instance
Hydrogen.create_instance()
hydrogen = Hydrogen.instance
# dk = Drumkit.load_file(
#      "/home/rebelcat/.hydrogen/data/drumkits/RoRBateria/drumkit.xml", False
# )
song = Song.load("/home/rebelcat/envelope_test.h2song")
hydrogen.song = song
hydrogen.sequencer_play()
time.sleep(3)
hydrogen.sequencer_stop()
# hydrogen.setSong(None)
print(song)
del song
del hydrogen
del prefs
del logger
print("alive:", Object.alive_object_count)

