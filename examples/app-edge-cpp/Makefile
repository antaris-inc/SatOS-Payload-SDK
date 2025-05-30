
include /lib/antaris/include/Makefile.inc 

# Payload Application specific build flow:
payload_app: payload_app.cu
	nvcc -o payload_app payload_app.cu $(INCLUDES) $(LIBS)
clean:
	rm -f payload_app
