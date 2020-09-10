Things not yet implemented:
- re-think name H2CORE_HAVE_LV2, what about conflict with the meaning "h2 as host for lv2 plugins"
- note off in lv2 midi driver
- midi out
- custom UI
- do not save preferences, if lv2 mode is active
- seperate player / sampler UIs? 
- handle OSC ports correctly if multiple instances are present
	- choose "random" OSC port everytime when lv2 is activated?
	   - probably makes no sense, since you can't control the address for the single
	     instance usecase
- state management
	- save last drumkit
