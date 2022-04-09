from PIL import Image

for i in range(96):
	print(i)
	img = Image.open("out/%03d.pgm" % i)
	img.save("fin/%03d.png" % i)