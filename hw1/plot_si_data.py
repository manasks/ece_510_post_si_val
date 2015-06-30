import re
import time
import matplotlib.pyplot as pyplot

def parsefile(inFile=r"silicon_data.txt",debug=0):
    with open(inFile,'r') as file_contents:
        for line in file_contents:
            if debug==1:
                print(line)
            if line[0] is not "#":
                id,voltage,frequency=re.findall(r'\d\.?\d*',line)
                if debug == 1:
                    print("id: ",id)
                    print("voltage: ",voltage)
                    print("frequency: ",frequency)
                pyplot.scatter(voltage,frequency)
                pyplot.draw()
                #time.sleep(0.1)
        pyplot.ylabel("frequency (MHz)")
        pyplot.xlabel("voltage (V)")
        pyplot.show()
		
if __name__ == '__main__':
    parsefile()
