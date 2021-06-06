from tkinter import * 
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, 
NavigationToolbar2Tk)
import numpy as np




def draw_gui(image_data):
    center = 200
    half_width = 200
    vmax = center + half_width
    vmin = center - half_width
    def slider1_handle(val):
        center = int(val)
        vmax = center + half_width
        vmin = center - half_width
        axs.imshow(image_data, cmap="gray", vmax=vmax, vmin=vmin)
        canvas.draw()
        
    def slider2_handle(val):
        half_width = int(val) / 2
        vmax = center + half_width
        vmin = center - half_width
        axs.imshow(image_data, cmap="gray", vmax=vmax, vmin=vmin)
        canvas.draw()
    # the main Tkinter window
    window = Tk()
    top_frame = Frame(window)
    bottom_frame = Frame(window)
    top_frame.pack(side=TOP)
    bottom_frame.pack(side=BOTTOM, fill=BOTH, expand=True)

    slider1 = Scale(top_frame, from_=0, to_= 1000, label="center", length=200,
                    orient= HORIZONTAL, command=slider1_handle)
    slider1.set(center)
    slider1.pack(side=LEFT)
    slider2 =  Scale(top_frame, from_=0, to_= 1000, label = "width", length=200,
                    orient= HORIZONTAL, command=slider2_handle)
    slider2.pack(side=RIGHT)
    slider1.set(half_width*2)

    fig = Figure(figsize = (8, 8) )
    axs = fig.add_subplot(111)
    axs.imshow(image_data, cmap="gray", vmax=vmax, vmin=vmin)


    canvas = FigureCanvasTkAgg(fig,
                                master = bottom_frame)  
    canvas.draw()
    canvas.get_tk_widget().pack()

    toolbar = NavigationToolbar2Tk(canvas,
                                    bottom_frame)
    toolbar.update()
    canvas.get_tk_widget().pack()
    
    window.mainloop()