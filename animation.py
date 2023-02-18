import csv
import matplotlib.pyplot as plt
from celluloid import Camera

fig = plt.figure()
camera = Camera(fig)
with open("output2.csv", 'r') as file:
    csvreader = csv.reader(file, delimiter=';')
    for row in csvreader:
        row.pop(0)
        dots = list(zip(row[::2], row[1::2]))
        print(dots)
        x = list()
        y = list()
        for num in dots:
            x.append(float(num[0]))
            y.append(float(num[1]))
        print(x)
        print(y)
        plt.plot(x, y, 'ro')
        plt.axis([-360, 100, -50, 50])  # вот здесь правим системы координат: максим и миним x y
        camera.snap()
        print()
animation = camera.animate(repeat=True)
plt.show()
# в пайчарм оно не запустится просто так, нужно file -> settings -> tools -> python scientific убрать галочку
# там где show plots что-то там
