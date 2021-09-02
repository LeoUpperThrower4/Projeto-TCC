from os import error
import serial
import numpy as np
import pyvista as pv

serialCom = serial.Serial('COM4', 9600)
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
        print(currentLine)

serialCom.close()

points = np.array(points)
point_cloud = pv.PolyData(points)
point_cloud.plot()

point_cloud.save("out/point_cloud.vtk")

volume = point_cloud.delaunay_3d()
shell = volume.extract_geometry()
shell.plot()

shell.save('out/mesh.stl')
