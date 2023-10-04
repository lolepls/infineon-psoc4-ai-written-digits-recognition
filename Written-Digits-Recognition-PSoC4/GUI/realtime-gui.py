import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.animation as animation
import matplotlib.pyplot as plt
import serial
import numpy as np
import re
import configparser

cfparser = configparser.SafeConfigParser()
cfparser.read('serial.conf')

serial_params = {
    'port': cfparser.get('SERIAL', 'port')
}

x_realtime_data = []
y_realtime_data = []
nn_input_image = np.zeros(shape=(28,28))
nn_output_data = []
predicted_value = []

serial_data_buffer = ""
processing_completed = False

def get_xy_coordinates():
    global serial_data_buffer
    
    pattern = r'\((\d+),(\d+)\)'
    matches = re.findall(pattern, serial_data_buffer)
    
    if(len(matches)==0):
        return [],[]
    
    xraw = []
    yraw = []
    unique = []
    
    for match in matches:
        if match not in unique:
            unique.append(match)
    
    for match in unique:
        xraw.append(int(match[0]))
        yraw.append(int(match[1]))
    
    for i, x in enumerate(xraw):
        xraw[i] = 111 - x
    return xraw, yraw

def read_serial_data(serial_port):
    global processing_completed, serial_data_buffer
    
    data = serial_port.read(serial_port.in_waiting).decode()

    serial_data_buffer = serial_data_buffer + data
    
    if("completed" in serial_data_buffer):
        processing_completed = True
        
    
        
def update_output_figures(nn_output_label, predicted_value_label):
    global serial_data_buffer, nn_input_image, nn_output_data, predicted_value
    out_data = serial_data_buffer.split("***")[1]
    out_data = out_data.replace("completed", "")
    
    out_data = out_data.split("*")
    
    image_data = [eval(i) for i in out_data[0].split(",")]
    nn_output_data = [eval(i) for i in out_data[1].split(",")]
    predicted_value = [eval(i) for i in out_data[2].split(",")]
    
    image_data = np.array(image_data)
    nn_input_image = image_data.reshape(28, 28)
    nn_input_image = np.flip(nn_input_image, axis=0)
    
    nn_output_label.config(text="Neural Network Output: " + str(nn_output_data))
    
    prediction = str(predicted_value[0])
    
    if(prediction == '11'):
        prediction = 'Unknown!'
        
    predicted_value_label.config(text="Predicted value: " + prediction)
     

def run_gui():
    
    global nn_output_data, predicted_value
    
    
    ## Matplotlib figures creation
    fig = plt.figure()

    real_time_input = fig.add_subplot(1, 2, 1)
    preprocessed_image = fig.add_subplot(1, 2, 2)
    
    # Setting the background color of the plot
    # using set_facecolor() method
    real_time_input.set_facecolor("black")
    real_time_input.set_xlim(left=0, right=111)
    real_time_input.set_ylim(bottom=0, top=111)
    real_time_input.set_box_aspect(1)
    real_time_input.set_title("Raw CAPSENSE Output")
    
    preprocessed_image.set_facecolor("black")
    preprocessed_image.set_xlim(left=0, right=27)
    preprocessed_image.set_ylim(bottom=0, top=27)
    preprocessed_image.set_box_aspect(1)
    preprocessed_image.set_title("PSoC4 Preprocessed Data")
    
    rti_plot = real_time_input.scatter(x_realtime_data, y_realtime_data, marker="s", lw=2, color="white")
    preprocessed_image.imshow(nn_input_image, cmap="gray")
    
    # Create the main tkinter window
    root = tk.Tk()
    root.title("Written Digits Recognition PSOC4")

    # Embed the Matplotlib figure in the tkinter window
    canvas = FigureCanvasTkAgg(fig, master=root)
    canvas_widget = canvas.get_tk_widget()
    canvas_widget.pack()
    nn_output_label = tk.Label(root, text="Neural Network Outputs will be shown here.", font=('Arial', 14))
    nn_output_label.pack()

    predicted_value_label = tk.Label(root, text="Predicted value will be shown here.", font=('Arial bold', 15))
    predicted_value_label.pack()
    
    # Establish UART communication (replace 'COM1' with the appropriate port name)
    try:
        serial_port = serial.Serial(serial_params['port'], baudrate=115200, timeout=0)
        print("Connection opened!")
    except serial.SerialException:
        print("Error: Unable to open the serial port.")
        return
    
    def run(*args):
        
        global x_realtime_data, y_realtime_data, processing_completed, serial_data_buffer
        # This function gets called each time the window must be updated

        read_serial_data(serial_port)
        
        if(processing_completed):
            #update the preprocessed image. Reset x_realtime and y_realtime
            x_realtime_data = []
            y_realtime_data = []
            
            update_output_figures(nn_output_label, predicted_value_label)
            
            # Reset buffers
            serial_data_buffer = ""
            processing_completed = False
            prep_plot = preprocessed_image.imshow(nn_input_image, cmap="gray")
            return prep_plot,
            
        else:
            x_realtime_data, y_realtime_data = get_xy_coordinates()
            
        rti_plot.set_offsets(np.c_[y_realtime_data, x_realtime_data])
        return rti_plot,
    
    
    ani = animation.FuncAnimation(fig, run, interval=1, blit=True)

    # Start the animation
    ani._start()

    # Run the tkinter main loop
    tk.mainloop()
    
run_gui()