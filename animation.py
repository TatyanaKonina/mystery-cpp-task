import csv
import matplotlib.pyplot as plt
from celluloid import Camera


def main():
    fig = plt.figure()
    camera = Camera(fig)
    with open("output945points.csv", 'r') as file:
        csvreader = csv.reader(file, delimiter=';')
        for row in csvreader:
            row.pop(0)
            dots = list(zip(row[::2], row[1::2]))
            x = list()
            y = list()
            for num in dots:
                x.append(float(num[0]))
                y.append(float(num[1]))
            plt.plot(x, y, 'bo', markersize=1)
            # plt.axis([-1000, 1000, -500, 500])
            plt.axis([min(x), max(x), min(y), max(y)])
            camera.snap()
    animation = camera.animate(repeat=True)
    plt.show()
    # в пайчарм оно не запустится просто так, нужно file -> settings -> tools -> python scientific убрать галочку
    # там где show plots что-то там


if __name__ == "__main__":
    main()
