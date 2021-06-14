from ctypes import *
import numpy as np
import os
import argparse
import configparser
from matplotlib import pyplot as plt
from sys import exit
import window_level_gui as wlg
def manage_parse():
	"""
	Setup the argparse library for command line arguments
	"""
	parser = argparse.ArgumentParser(description="Main Control Script of UMRAM LimeSDR Receiver")
	parser.add_argument("--config", type=str, default=os.getcwd()+"/settings.ini", help="Config File Directory")
	return parser

def read_config(settings_dir: str):
	"""
	Parse the .ini file to retrieve the parameters

	:param settings_dir: name and full path of the settings file
	:return: (dict, dict, dict) Dictionaries of the 3 entries in the settings file
	"""
	config = configparser.ConfigParser()
	config.read(settings_dir)
	rec_params = dict(config.items('Receiver Parameters'))
	mri_params = dict(config.items('MRI Parameters'))
	general_params =  dict(config.items('General Parameters'))
	general_dict = {"limecontroldll": general_params["limecontroldll"],
					"show_gui": int(general_params["show_gui"])
				}
	rec_dict = {"sample_rate": c_double(float(rec_params["sampling_rate"])),
				"rec_length" : c_double(float(rec_params["receive_length"])),
				"freq_offset": c_double(float(rec_params["frequency_offset"])),
				"rx_channel" : c_int(int(rec_params["rx_channel"])),
				"norm_gain" :  c_double(float(rec_params["normalized_gain"])),
				"LPF_BW" : c_double(float(rec_params["lpf_bw"]))
				}
	mri_dict = {"fre": int(mri_params["fre"]),
				"tr1": float(mri_params["tr1"]),
				"te1": int(mri_params["te1"]),
				"dw1": int(mri_params["dw1"]),
				"nx" : int(mri_params["nx"]),
				"ny1": int(mri_params["ny1"])
				}
	return general_dict, rec_dict, mri_dict

def analyse_data(data: np.ndarray, t_i, TR1, TE1, DW1, Ny1, fre):
	"""
	Python version of the matlab code written by Ergin Atalar
	:param data: the complex numpy data array
	:param t_i: The first index of the first RF pulse
	:param TR1, TE1, DW1, Ny1: The standard MRI parameters for reconstruction
	:return: the resulting 256xNy1 image matrix
	"""
	t0 = t_i
	TR = TR1*fre
	TE = TE1*fre
	DW = DW1
	Nx = 256
	Ny = Ny1
	readout = int(DW*Nx*fre/1000)
	k_space = np.zeros((readout, Ny), dtype=complex)
	for n in range(0,Ny):
		datastart = int(t0+n*TR+TE-readout/2)
		k_space[:,n] = data[datastart:datastart+readout]
	image = np.fft.fft2(k_space) / 1e3
	image = np.fft.fftshift(image, axes=0)

	cropstart = int(readout/2-Nx/2)
	image = image[cropstart-150:cropstart-150+Ny,:].T
	return image

def receive(dll):
	"""
	Calls the "rec" function of the LimeControlDLL to start data acquisition
	:param dll: A reference to the LimeControlDLL
	:return: (np.ndarray) the complex data array
	"""
	samples_ptr_ptr = pointer(pointer(c_int16()))
	sample_size = dll.rec(samples_ptr_ptr)
	print(f"Received {sample_size} samples")
	samples_ptr = cast(samples_ptr_ptr, POINTER(POINTER(c_int16))).contents
	samples = np.ctypeslib.as_array(samples_ptr, shape=(1,sample_size*2))
	samples_I = samples[0, ::2]
	samples_Q = samples[0, 1::2]
	data = samples_I + 1j*samples_Q
	return data

def main(general_dict, rec_dict, mri_dict):
	"""
	Run the entire sequence and return the image matrix
	"""
	try:
		lime_dll = cdll.LoadLibrary(general_dict["limecontroldll"])
	except OSError:
		print("The Lime Control DLL could not be found at:", general_dict["limecontroldll"])
		return -1
	setparams_ret = lime_dll.set_params(rec_dict["rec_length"], rec_dict["sample_rate"], rec_dict["freq_offset"],
							rec_dict["rx_channel"], rec_dict["norm_gain"], rec_dict["LPF_BW"])
	if setparams_ret != 1:
		print("Cant Set Params", setparams_ret)
		return -1

	ignite_ret = lime_dll.ignite_lime() # initialize the settings on LimeSDR
	if ignite_ret != 1:
		print("Cant Ignite Lime:", ignite_ret)
		return -1
	
	input("Receive for {time:} seconds on enter:".format(time=rec_dict["rec_length"].value) )
	data = receive(lime_dll)

	first_rf = np.argmax(np.abs(data) > 400)
	image = analyse_data(data, first_rf, mri_dict["tr1"],
						mri_dict["te1"], mri_dict["dw1"], mri_dict["ny1"], mri_dict["fre"]  )
	return np.flip(image, axis=0)

if __name__ == "__main__":
	args = manage_parse().parse_args()
	general_dict, rec_dict, mri_dict = read_config(args.config)

	image = main(general_dict, rec_dict, mri_dict)
	if image == -1:
		exit("Closing the lime_control script")
	print("Plotting")
	np.savetxt("image.txt", np.abs(image))
	if general_dict["show_gui"]:
		wlg.draw_gui(np.abs(image))
	else: # Plot the same image with different windowing levels
		img, axs = plt.subplots(1,3)
		axs[0].imshow(np.abs(image), cmap="gray", vmax=200)
		axs[1].imshow(np.abs(image), cmap="gray", vmax=300)
		axs[2].imshow(np.abs(image), cmap="gray", vmax=400)
		img.suptitle("Scanned Image")
		plt.show()
	print("end")
