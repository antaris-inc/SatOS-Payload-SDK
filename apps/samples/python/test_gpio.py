import antaris_api_gpio as api_gpio

#sample test program
val = api_gpio. api_pa_pc_read_gpio(int(sys.argv[1]))
if val != g_GPIO_ERROR:
    print("initial Gpio vaue of  ", int(sys.argv[1])," is", val)
else:
    print("Error in pin no")


val = api_gpio.api_pa_pc_write_gpio(int(sys.argv[2]), int(sys.argv[3]))
if val != g_GPIO_ERROR:
    print("Written successfully")
else:
    print("error in pin no")


val = api_gpio.api_pa_pc_read_gpio(int(sys.argv[1]))
if val != g_GPIO_ERROR:
    print(" Final Gpio vaue of  ", int(sys.argv[1]), " is = ", val)
else:
    print("Error in pin no")
