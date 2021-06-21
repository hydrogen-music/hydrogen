from h2core import H2Core as h2core
logger = h2core.Logger.bootstrap(31)
h2core.Object.bootstrap(logger, True)
h2core.Filesystem.bootstrap(logger, "/usr/local/share/hydrogen/data/")
prefs = h2core.Preferences.create_instance()
hydrogen = h2core.Hydrogen.create_instance()
song = h2core.Song("foo", "bar", 120, 1)
print(song)


