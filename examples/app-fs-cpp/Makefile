
include /lib/antaris/include/Makefile.inc 

# fs Application specific build flow:
fs_app: file_system.cc fs_app.cc
	g++ -o fs_app fs_app.cc file_system.cc $(INCLUDES) $(LIBS)
clean:
	rm -f fs_app
