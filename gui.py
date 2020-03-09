


baudrate = 9600 # for the discrete data transfer rate (has to match with the Teensy script)
#baudrate = 256000

serial_timeout = .01 # the timeout (in seconds) for port reading



# Define the communication protocol with the Teensy (needs to match the corresponding variables in Teensy)
MESSAGE_PLAY_STIMULUS = 44
MESSAGE_RECORD_TAPS   = 55
MESSAGE_STOP          = 66



import sys

if (sys.version_info > (3, 0)): # Python 3
    from tkinter import *
    from tkinter import messagebox
    from tkinter.scrolledtext import ScrolledText
    from tkinter import filedialog
else: # probably Python 2
    from Tkinter import *
    import tkMessageBox as messagebox
    import tkFileDialog as filedialog
    from ScrolledText import ScrolledText

import glob
import time
import struct
import os


def error_message(msg):
    print(msg)
    messagebox.showinfo("Error", msg)
    return



try:
    import serial
except:
    msg = "An error occurred while importing the pySerial module.\n\nGo to https://pypi.python.org/pypi/pyserial to install the module,\nor refer to our manual for help.\n\nThe program will now quit."
    error_message(msg)
    sys.exit(-1)



def openserial():
    # Open serial communications with the Teensy
    # This assumes that the Teensy is connected (otherwise the device will not be created).
    global config

    port = config["commport"].get()
    
    try:
        comm = serial.Serial(port, baudrate, timeout=0.01)
    except:
        error_message("Cannot open USB port %s. Is the device connected?\n"%port)
        config["capturing"]=False
        return False

    config["comm"]=comm
    config["capturing"]=True
    output("Now listening to serial port %s\n"%port)
    
    update_enabled()
    



def browse_serial():
    # Browse for serial port
    comm = config["commport"].get()
    if not os.path.exists(comm):
        comm = guess_serial()

    file_path_string = filedialog.askopenfilename(initialdir="/dev/",
                                                  title="Select serial communication port",
                                                  filetypes=(("tty","tty*"),("all files","*.*")),
                                                  initialfile=comm)
    if file_path_string:
        config["commport"].set(file_path_string)
    


    
def guess_serial():
    # Try to automatically find the serial port
    acm = '/dev/ttyACM0'
    if os.path.exists(acm):
        return acm

    candidates = glob.glob('/dev/tty.usb*')
    if len(candidates)>0:
        return candidates[0]

    return acm # the default
    

    
    
def on_closing():
    global keep_going
    keep_going = False
    


def output(msg):
    # Function that shows output on the screen and in the terminal
    global config

    fmt = time.strftime('[%Y-%m-%d %H:%M:%S] ')
    msg = fmt + msg
    config["report"].insert(END,msg+"\n")
    config["report"].see(END) #scroll down to the end
    print(msg)
    

    
def listen():
    # Listen for incoming data on the COMM port
    global config

    if config["capturing"]:

        ln = config["comm"].readline()
        if ln!=None and len(ln)>0:
            # Probably an incoming tap
            msg= ln.decode('ascii').strip()
            output(msg)

            # If the message contains a trial completion signal
            if msg.find('Trial completed at')>-1:
                config["running"]=False
                update_enabled()
            
            # Output to file if we have a output filename
            if "out.filename" in config:
                with open(config["out.filename"],'a') as f:
                    f.write(msg+"\n")



            
def check_and_convert_int(key,datadict):
    """ Check that a particular key in the datadict is really associated with an int, and if so, cast it to that datatype."""
    if key not in datadict:
        error_message("Internal error -- %s variable is not set")
        return

    val = datadict[key].strip()
    
    if not val.isdigit():
        error_message("Error, you entered '%s' for %s but that has to be an integer (whole) number"%(val,key))
        return

    return int(val)
                      




def update_enabled():
    """ Update the state of buttons depending on our state."""
    config["play.button"]   .configure(state=NORMAL if config["capturing"] else DISABLED)
    #config["record.button"] .configure(state=NORMAL if config["capturing"] else DISABLED)
    config["abort.button"].configure(state=NORMAL if config["capturing"] and config["running"] else DISABLED)


def send_play_config():
    """ 
    Communicate what we want to play to the Teensy 
    """

    # First, let's collect and verify the data

    trialinfo = {}
    trialinfo["stimulus"]=config["stimulus"].get().strip().upper()

    if trialinfo["stimulus"]=="":
        trialinfo["stimulus"]="---" # this indicates that we do not play anything
        
    if len(trialinfo["stimulus"])!=3:
        error_message("Your stimulus has to be empty or 3 characters long.")
        return False

    trialinfo["duration"]=config["duration"].get().strip()

    if not trialinfo["duration"].isdigit():
        error_message("Duration has to be an integer number.")
        return False

    trialinfo["duration"]=int(trialinfo["duration"])

    print(trialinfo)
    
    # Okay, so now we need to talk to Teensy to tell him to start this trial

    # First, tell Teensy to stop whatever it is it is doing at the moment (go to non-active mode)
    config["comm"].write(struct.pack('!B',MESSAGE_STOP))

    config["comm"].write(struct.pack('!B',MESSAGE_PLAY_STIMULUS))

    # Now we tell Teensy that we are going to send some config information
    config["comm"].write(bytes(trialinfo["stimulus"],'ascii')) #struct.pack('s',trialinfo["stimulus"]));

    # Now we tell Teensy that we are going to send some config information
    config["comm"].write(struct.pack('i',trialinfo["duration"]))
    
    start_recording()
    
    time.sleep(1) # Just wait a moment to allow Teensy to process (not sure if this is actually necessary)

    return True
    



def start_recording():
    # Create the output file
    subjectid = config["subj"].get().strip()
    outdir = os.path.join('data',subjectid)
    if not os.path.exists(outdir): 
        os.makedirs(outdir)
    outf = os.path.join(outdir,"%s_%s.txt"%(subjectid,time.strftime("%Y%m%d_%H%M%S")))
    config["out.filename"]=outf
    output("")
    output("Output to %s"%config["out.filename"])





    
def send_play():
    """ This is when people click the "Go" button """
    config["running"]=False
    update_enabled()
    if send_play_config(): # this sends the configuration for the current trial
        config["running"]=True
    update_enabled()

    


    
def abort():
    config["comm"].write(struct.pack('!B',MESSAGE_STOP))
    config["running"]=False
    update_enabled()
    
    


def build_gui():
    
    #
    # 
    # Build the GUI
    #
    #

    # Set up the main interface scree
    w,h= 800,700
    
    master = Tk()
    master.title("Rhythm Error")
    master.geometry('%dx%d+%d+%d' % (w, h, 500, 200))

    buttonframe = Frame(master) #,padding="3 3 12 12")

    buttonframe.pack(padx=10,pady=10)

    row = 0
    openb = Button(buttonframe,text="open",        command=openserial)
    openb.grid(column=2, row=row, sticky=W)
    browseb = Button(buttonframe,text="browse",    command=browse_serial)
    browseb.grid(column=3, row=row, sticky=W)
    commport = StringVar()
    commport.set(guess_serial()) # default comm port
    Label(buttonframe, text="comm port").grid(column=0,row=row,sticky=W)
    ttydev  = Entry(buttonframe,textvariable=commport).grid(column=1,row=row,sticky=W)
    config["commport"]=commport

    row += 1 
    subj = StringVar()
    subj.set("subject") 
    Label(buttonframe, text="subject ID").grid(column=0,row=row,sticky=W)
    subjentry = Entry(buttonframe,textvariable=subj).grid(column=1,row=row,sticky=W)
    config["subj"]=subj


    row += 1
    Label(buttonframe, text="PLAY STIMULUS").grid(column=0,row=row,sticky=W)

    row += 1
    config["stimulus"] = StringVar()
    Label(buttonframe, text="Stimulus file").grid(column=0,row=row,sticky=W)
    stimentry = Entry(buttonframe,textvariable=config["stimulus"]).grid(column=1,row=row,sticky=W)
    #config["stimentry"]=stimentry
    #config["stimentry"].grid(column=0,row=row,sticky=W,padx=5,pady=5)


    row += 1
    Label(buttonframe, text="RECORD TAPS").grid(column=0,row=row,sticky=W)

    row += 1
    config["duration"] = StringVar()
    Label(buttonframe, text="Duration (seconds)").grid(column=0,row=row,sticky=W)
    durentry = Entry(buttonframe,textvariable=config["duration"]).grid(column=1,row=row,sticky=W)
    config["duration"].set("10")

   
    row += 1
    #Button(buttonframe,text="configure",     command=launch) .grid(column=2, row=row, sticky=W, padx=5,pady=20)
    config["play.button"]=Button(buttonframe,
                                 text="go",
                                 command=send_play,
                                 background="green",
                                 activebackground="lightgreen")
    config["play.button"].grid(column=3, row=row, sticky=W, padx=5,pady=20)


    #row += 1
    #Button(buttonframe,text="configure",     command=launch) .grid(column=2, row=row, sticky=W, padx=5,pady=20)
    #config["record.button"]=Button(buttonframe,
    #                               text="record",
    #                               command=send_record,
    #                               background="blue",
    #                               activebackground="lightblue")
    #config["record.button"].grid(column=3, row=row, sticky=W, padx=5,pady=20)

    
    row += 1
    
    config["abort.button"]=Button(buttonframe,
                                  text="abort",
                                  command=abort,
                                  background="red",
                                  activebackground="darkred")
    config["abort.button"].grid(column=4, row=row, sticky=W, padx=5,pady=20)
    
    
    row+=1
    report = ScrolledText(master)
    report.pack(padx=10,pady=10,fill=BOTH,expand=True)
    config["report"]=report


    # Draw the background against which everything else is going to happen
    master.protocol("WM_DELETE_WINDOW", on_closing)

    config["master"]=master


    update_enabled()



            
            
global config
config = {}
config["capturing"]=False
config["running"]=False





build_gui()


keep_going = True

while keep_going:

    listen()

    config["master"].update_idletasks()
    config["master"].update()
    
    time.sleep(0.01) # frame rate of our GUI update

    
