from os import error
import serial
import numpy as np
import pyvista as pv

serialCom = serial.Serial('COM3', 9600)
serialCom.timeout = 1

points = []
while True:
    currentLine = serialCom.readline().decode('ascii').strip().replace('\r\n', '')
    if (not currentLine):
        continue
    if (currentLine == 'done'):
        break
    try:
        x, y, z = currentLine.split(',')
        point = np.array([float(x), float(y), float(z)])
        points.append(point)
    except:
        print('erro')


serialCom.close()

points = np.array(points)
point_cloud = pv.PolyData(points)
# mesh = pv.lines_from_points(points)
# pv.save_meshio("mesh.obj", mesh)
point_cloud.plot()
