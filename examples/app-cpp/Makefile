
include /lib/antaris/include/Makefile.inc 

# Payload Application specific build flow:
payload_app: payload_app.cc
	g++ -o payload_app payload_app.cc $(INCLUDES) $(LIBS)
clean:
	rm -f payload_app
