from ctypes import *
import numpy as np
import os
import argparse
import configparser
from matplotlib import pyplot as plt
import window_level_gui as wlg
def manage_parse():
    """
    Setup the argparse library
    """
    parser = argparse.ArgumentParser(description="Main Control Script of UMRAM LimeSDR Receiver")
    parser.add_argument("--config", type=str, default=os.getcwd()+"/settings.ini", help="Config File Directory")
    parser.add_argument("--slice", type=int, default=1, help="Total Image Slice Count (NOT IMPLEMENTED")
    parser.add_argument("--db_name", type=str, default=None, help="Specift name of database. data wont " +
                        "be stored if no name was specified")
    return parser

def read_config(settings_dir: str):
    """
    Parse the .ini file
    """
    config = configparser.ConfigParser()
    config.read(settings_dir)
    rec_params = dict(config.items('Receiver Parameters'))
    mri_params = dict(config.items('MRI Parameters'))
    file_params =  dict(config.items('File Names'))
    file_dict = {"limecontroldll": file_params["limecontroldll"],
                "limecontroldll2": file_params["limecontroldll2"],
                "output_image_name": file_params["output_image_name"],
                "output_image_type": file_params["output_image_type"]
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
    return file_dict, rec_dict, mri_dict

def analyse_data(data, t_i, TR1, TE1, DW1, Ny1):
    """
    Python version of the matlab code written by Ergin Atalar
    """
    t0 = t_i
    fre = 500
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
    samples_ptr_ptr = pointer(pointer(c_int16()))
    sample_size = dll.rec(samples_ptr_ptr)
    print(f"Received {sample_size} samples")
    samples_ptr = cast(samples_ptr_ptr, POINTER(POINTER(c_int16))).contents
    samples = np.ctypeslib.as_array(samples_ptr, shape=(1,sample_size*2))
    samples_I = samples[0, ::2]
    samples_Q = samples[0, 1::2]
    data = samples_I + 1j*samples_Q
    return data

def main(args):
    file_dict, rec_dict, mri_dict = read_config(args.config)
    try:
        lime_dll = cdll.LoadLibrary(file_dict["limecontroldll"])
    except FileNotFoundError:
        print("The Lime Control DLL could not be found")
        return -1
    setparams_ret = lime_dll.set_params(rec_dict["rec_length"], rec_dict["sample_rate"], rec_dict["freq_offset"],
                            rec_dict["rx_channel"], rec_dict["norm_gain"], rec_dict["LPF_BW"])
    if setparams_ret != 1:
        print("Cant Set Params", setparams_ret)
        return -1

    ignite_ret = lime_dll.ignite_lime()
    if ignite_ret != 1:
        print("Cant Ignite Lime:", ignite_ret)
        return -1
    
    input("Receive for {time:} seconds on enter:".format(time=rec_dict["rec_length"].value) )
    data = receive(lime_dll)

    first_rf = np.argmax(np.abs(data) > 400)
    image = analyse_data(data, first_rf, mri_dict["tr1"],
                        mri_dict["te1"], mri_dict["dw1"], mri_dict["ny1"] )
    return np.flip(image, axis=0)

if __name__ == "__main__":
    args = manage_parse().parse_args()
    image = main(args)
    print("Plotting")
    np.savetxt("image.txt", np.abs(image))
    wlg.draw_gui(image)
    #img, axs = plt.subplots(1,3)
    #axs[0].imshow(np.abs(image), cmap="gray", vmax=200)
    #axs[1].imshow(np.abs(image), cmap="gray", vmax=300)
    #axs[2].imshow(np.abs(image), cmap="gray", vmax=400)
    #img.suptitle("Scanned Image")
    #plt.show()
    print("end")
